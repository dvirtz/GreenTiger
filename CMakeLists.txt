cmake_minimum_required(VERSION 3.3)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(HunterGate)

HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.19.124.tar.gz"
    SHA1 "6f0daa06ced6eaf68d34f4dae4d1c02fffdd890b"
)

project(GreenTiger)

set(CMAKE_CXX_STANDARD 14)

set(BOOST_COMPONENTS system filesystem)
hunter_add_package(Boost COMPONENTS ${BOOST_COMPONENTS})
find_package(Boost CONFIG REQUIRED ${BOOST_COMPONENTS})

hunter_add_package(Catch)
find_package(Catch CONFIG REQUIRED)

enable_testing()

option(POST_BUILD_TESTS "Run tests on each target's post build step" TRUE)

function(parse_unit_tests target)
  include(ParseAndAddCatchTests)
  # abort on first failure
  ParseAndAddCatchTests(${target} "-a")

  if (POST_BUILD_TESTS)
    add_custom_command(TARGET ${target}
      POST_BUILD
      COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> -L ${target} --output-on-failure -j8
      COMMENT "Running ${target} unit tests")
  endif()
endfunction()

add_subdirectory(include)
add_subdirectory(Chapter01)
add_subdirectory(Chapter02)
add_subdirectory(Chapter03)
add_subdirectory(Chapter04)
add_subdirectory(Chapter05)
add_subdirectory(Chapter06)