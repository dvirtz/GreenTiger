set(HEADERS testsHelper.h.in overload_set.h variantMatch.h fusionAccumulate.h strong_typedef.h vectorApply.h type_traits.h warning_suppress.h)

add_custom_target(include SOURCES ${HEADERS})

add_library(includeHeaders INTERFACE)

set(TESTS_LOCATION ${CMAKE_SOURCE_DIR}/module_bundle/testcases)
configure_file(testsHelper.h.in ${CMAKE_CURRENT_BINARY_DIR}/testsHelper.h)

target_include_directories(includeHeaders INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})