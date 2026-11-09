#include <apfloat/apfloat.hpp>

#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

constexpr std::size_t default_digits = 1'000'000;
constexpr std::size_t decimal_guard_digits = 96;
constexpr std::size_t binary_guard_bits = 256;

struct options {
    std::size_t digits = default_digits;
    std::string output_path;
};

std::size_t parse_digits(std::string_view text) {
    std::size_t digits = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), digits);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || digits == 0)
        throw std::invalid_argument("digit count must be a positive integer");
    return digits;
}

options parse_options(int argc, char* argv[]) {
    options result;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--digits") {
            if (++index == argc) throw std::invalid_argument("--digits requires a value");
            result.digits = parse_digits(argv[index]);
        } else if (argument == "--output") {
            if (++index == argc) throw std::invalid_argument("--output requires a path");
            result.output_path = argv[index];
        } else if (argument == "--help") {
            std::cout << "Usage: api [--digits COUNT] [--output FILE]\n";
            std::exit(EXIT_SUCCESS);
        } else {
            throw std::invalid_argument("unknown option: " + std::string(argument));
        }
    }
    return result;
}

apfloat::number constant(std::string_view value, std::size_t precision) {
    apfloat::number result;
    if (result.set_precision(precision) != apfloat::error::none ||
        apfloat::number::parse(value, result) != apfloat::error::none)
        throw std::runtime_error("could not initialize a calculation constant");
    return result;
}

std::size_t calculation_digits(std::size_t digits) {
    if (digits > (std::numeric_limits<std::size_t>::max)() - decimal_guard_digits)
        throw std::invalid_argument("digit count is too large");
    return digits + decimal_guard_digits;
}

std::size_t required_precision(std::size_t digits) {
    constexpr std::size_t numerator = 332193;
    constexpr std::size_t denominator = 100000;
    if (digits > ((std::numeric_limits<std::size_t>::max)() - binary_guard_bits) / numerator)
        throw std::invalid_argument("digit count is too large");
    return digits * numerator / denominator + binary_guard_bits;
}

apfloat::integer integer_from_decimal(std::string_view text) {
    apfloat::integer result;
    for (const char digit : text) {
        result.multiply(10);
        result.add(static_cast<std::uint32_t>(digit - '0'));
    }
    return result;
}

apfloat::number number_from_integer(const apfloat::integer& value, std::size_t precision) {
    return constant(value.to_string(), precision);
}

struct signed_integer {
    bool negative = false;
    apfloat::integer magnitude;

    signed_integer() = default;
    explicit signed_integer(apfloat::integer value, bool sign = false)
        : negative(sign), magnitude(std::move(value)) {
        if (magnitude.zero()) negative = false;
    }

    void multiply(const apfloat::integer& value) {
        magnitude.multiply(value);
        if (magnitude.zero()) negative = false;
    }
};

signed_integer add(const signed_integer& left, const signed_integer& right) {
    if (left.negative == right.negative) {
        signed_integer result(left.magnitude, left.negative);
        result.magnitude.add(right.magnitude);
        return result;
    }
    const int comparison = left.magnitude.compare(right.magnitude);
    if (comparison == 0) return signed_integer();
    if (comparison > 0) {
        signed_integer result(left.magnitude, left.negative);
        result.magnitude.subtract(right.magnitude);
        return result;
    }
    signed_integer result(right.magnitude, right.negative);
    result.magnitude.subtract(left.magnitude);
    return result;
}

struct split_result {
    apfloat::integer p;
    apfloat::integer q;
    signed_integer t;
};

split_result chudnovsky_split(std::uint32_t begin, std::uint32_t end,
                              const apfloat::integer& c3_over_24) {
    if (end - begin == 1) {
        if (begin == 0)
            return {apfloat::integer(1), apfloat::integer(1),
                    signed_integer(apfloat::integer(13'591'409))};

        apfloat::integer p(6 * begin - 5);
        p.multiply(2 * begin - 1);
        p.multiply(6 * begin - 1);

        apfloat::integer q(begin);
        q.multiply(begin);
        q.multiply(begin);
        q.multiply(c3_over_24);

        apfloat::integer coefficient(545'140'134);
        coefficient.multiply(begin);
        coefficient.add(13'591'409);
        apfloat::integer t = p;
        t.multiply(coefficient);
        return {std::move(p), std::move(q), signed_integer(std::move(t), (begin & 1u) != 0)};
    }

    const std::uint32_t middle = begin + (end - begin) / 2;
    split_result left = chudnovsky_split(begin, middle, c3_over_24);
    split_result right = chudnovsky_split(middle, end, c3_over_24);

    apfloat::integer t_left = left.t.magnitude;
    t_left.multiply(right.q);
    signed_integer combined(std::move(t_left), left.t.negative);
    signed_integer t_right = right.t;
    t_right.multiply(left.p);
    combined = add(combined, t_right);

    left.p.multiply(right.p);
    left.q.multiply(right.q);
    return {std::move(left.p), std::move(left.q), std::move(combined)};
}

apfloat::number compute_pi_chudnovsky(std::size_t digits) {
    const std::size_t precision = required_precision(calculation_digits(digits));
    const double decimal_digits_per_term = 14.181647462725477;
    const std::size_t terms = static_cast<std::size_t>(
        std::ceil(static_cast<double>(calculation_digits(digits)) / decimal_digits_per_term)) + 1;
    if (terms > (std::numeric_limits<std::uint32_t>::max)() / 6)
        throw std::invalid_argument("digit count is too large");

    const auto c3_over_24 = integer_from_decimal("10939058860032000");
    const split_result split = chudnovsky_split(0, static_cast<std::uint32_t>(terms), c3_over_24);
    if (split.t.negative || split.t.magnitude.zero())
        throw std::runtime_error("Chudnovsky series produced an invalid denominator");

    const auto nearest = apfloat::rounding::nearest;
    const auto q = number_from_integer(split.q, precision);
    const auto t = number_from_integer(split.t.magnitude, precision);
    const auto scale = apfloat::number::multiply(
        constant("426880", precision), apfloat::number::sqrt(constant("10005", precision), nearest), nearest);
    return apfloat::number::divide(apfloat::number::multiply(q, scale, nearest), t, nearest);
}

std::string format_pi(const apfloat::number& pi, std::size_t digits) {
    std::string value = pi.to_string();
    const std::size_t decimal = value.find('.');
    if (decimal == std::string::npos || decimal == 0 || decimal + digits >= value.size())
        throw std::runtime_error("calculation did not produce enough decimal digits");

    std::string result = value.substr(0, decimal + digits + 1);
    if (value[decimal + digits + 1] < '5') return result;

    for (std::size_t index = result.size(); index-- > 0;) {
        if (result[index] == '.') continue;
        if (result[index] != '9') {
            ++result[index];
            return result;
        }
        result[index] = '0';
    }
    throw std::runtime_error("could not round decimal output");
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const options options = parse_options(argc, argv);
        const std::string pi = format_pi(compute_pi_chudnovsky(options.digits), options.digits);
        if (options.output_path.empty()) {
            std::cout << pi << '\n';
        } else {
            std::ofstream output(options.output_path, std::ios::binary);
            if (!output) throw std::runtime_error("could not open output file: " + options.output_path);
            output << pi << '\n';
            if (!output) throw std::runtime_error("could not write output file: " + options.output_path);
        }
    } catch (const std::exception& error) {
        std::cerr << "api: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
