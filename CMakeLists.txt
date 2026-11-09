cmake_minimum_required(VERSION 3.20)

project(pionpi VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    apfloat
    GIT_REPOSITORY https://github.com/n7jti/apfloat.git
    GIT_TAG 3d85e83c87e9ff357e3eb9c8da702060e9de08d7
)
FetchContent_MakeAvailable(apfloat)

add_executable(pionpi src/main.cpp)
target_compile_features(pionpi PRIVATE cxx_std_17)
target_link_libraries(pionpi PRIVATE apfloat::apfloat)

include(CTest)
if (BUILD_TESTING)
    set(api_pi_reference_vector "${CMAKE_CURRENT_SOURCE_DIR}/docs/one-million.txt")
    set(api_pi_verifier "${CMAKE_CURRENT_SOURCE_DIR}/cmake/verify-pi.cmake")

    function(add_pi_precision_test digits)
        add_test(
            NAME "pionpi_chudnovsky_pi_${digits}"
            COMMAND "${CMAKE_COMMAND}"
                "-DAPI_EXECUTABLE=$<TARGET_FILE:pionpi>"
                "-DDIGITS=${digits}"
                "-DREFERENCE_VECTOR=${api_pi_reference_vector}"
                -P "${api_pi_verifier}"
        )
    endfunction()

    foreach(digits IN ITEMS 10 100 1000 10000 100000 1000000)
        add_pi_precision_test("${digits}")
    endforeach()

    set_tests_properties(pionpi_chudnovsky_pi_1000000 PROPERTIES LABELS extended)
endif()
