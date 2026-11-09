cmake_policy(SET CMP0007 NEW)

foreach(required_variable IN ITEMS API_EXECUTABLE DIGITS REFERENCE_VECTOR)
    if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} must be provided")
    endif()
endforeach()

if(NOT DIGITS MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "DIGITS must be a positive integer")
endif()

if(NOT EXISTS "${API_EXECUTABLE}")
    message(FATAL_ERROR "API executable does not exist: ${API_EXECUTABLE}")
endif()

if(NOT EXISTS "${REFERENCE_VECTOR}")
    message(FATAL_ERROR "PI reference vector does not exist: ${REFERENCE_VECTOR}")
endif()

execute_process(
    COMMAND "${API_EXECUTABLE}" --digits "${DIGITS}"
    RESULT_VARIABLE api_result
    OUTPUT_VARIABLE actual
    ERROR_VARIABLE api_error
)

if(NOT api_result EQUAL 0)
    message(FATAL_ERROR
        "PI calculation for ${DIGITS} fractional digits failed with exit code ${api_result}:\n${api_error}")
endif()

string(REGEX REPLACE "[\r\n]+$" "" actual "${actual}")
if(NOT actual MATCHES "^3\\.[0-9]+$")
    message(FATAL_ERROR
        "PI calculation for ${DIGITS} fractional digits produced malformed output: ${actual}")
endif()

file(STRINGS "${REFERENCE_VECTOR}" reference_lines)
list(LENGTH reference_lines reference_line_count)
if(reference_line_count LESS 4)
    message(FATAL_ERROR "PI reference vector is missing its digit data")
endif()

list(SUBLIST reference_lines 3 -1 reference_digit_lines)
string(JOIN "" reference_digits ${reference_digit_lines})
string(REGEX REPLACE "[^0-9]" "" reference_digits "${reference_digits}")

math(EXPR expected_digit_count "${DIGITS} + 1")
string(LENGTH "${reference_digits}" reference_digit_count)
if(reference_digit_count LESS expected_digit_count)
    message(FATAL_ERROR
        "PI reference vector has ${reference_digit_count} digits but ${expected_digit_count} are required")
endif()

string(SUBSTRING "${reference_digits}" 0 "${expected_digit_count}" expected_digits)
if(reference_digit_count GREATER expected_digit_count)
    string(SUBSTRING "${reference_digits}" "${expected_digit_count}" 1 rounding_digit)
    if(rounding_digit MATCHES "[56789]")
        math(EXPR rounding_index "${expected_digit_count} - 1")
        while(rounding_index GREATER_EQUAL 0)
            string(SUBSTRING "${expected_digits}" "${rounding_index}" 1 digit)
            if(digit STREQUAL "9")
                set(replacement "0")
            else()
                math(EXPR replacement "${digit} + 1")
            endif()

            string(SUBSTRING "${expected_digits}" 0 "${rounding_index}" prefix)
            math(EXPR suffix_start "${rounding_index} + 1")
            string(SUBSTRING "${expected_digits}" "${suffix_start}" -1 suffix)
            set(expected_digits "${prefix}${replacement}${suffix}")

            if(NOT digit STREQUAL "9")
                break()
            endif()

            math(EXPR rounding_index "${rounding_index} - 1")
        endwhile()
    endif()
endif()

string(SUBSTRING "${expected_digits}" 0 1 integer_part)
if(NOT integer_part STREQUAL "3")
    message(FATAL_ERROR "PI reference vector does not begin with 3")
endif()

string(SUBSTRING "${expected_digits}" 1 "${DIGITS}" fractional_part)
set(expected "${integer_part}.${fractional_part}")
string(LENGTH "${actual}" actual_length)
string(LENGTH "${expected}" expected_length)

if(NOT actual_length EQUAL expected_length)
    message(FATAL_ERROR
        "PI calculation for ${DIGITS} fractional digits returned ${actual_length} characters; expected ${expected_length}")
endif()

if(NOT actual STREQUAL expected)
    message(FATAL_ERROR "PI calculation for ${DIGITS} fractional digits differs from the known test vector")
endif()
