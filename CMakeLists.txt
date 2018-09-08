cmake_minimum_required(VERSION 3.3)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(HunterGate)

HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.22.19.tar.gz"
    SHA1 "54749ffc945f78362aa680a7abbafcaa40ff9069"
)

project(GreenTiger)

set(BOOST_COMPONENTS system filesystem)
hunter_add_package(Boost COMPONENTS ${BOOST_COMPONENTS})
find_package(Boost CONFIG REQUIRED ${BOOST_COMPONENTS})

hunter_add_package(Catch)
find_package(Catch2 CONFIG REQUIRED)

# hunter_add_package(type_safe)
# find_package(type_safe CONFIG REQUIRED)

include(CompilerWarnings)

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
  enable_testing()

  option(POST_BUILD_TESTS "Run tests on each target's post build step" TRUE)

  function(parse_unit_tests target)
    include(CatchDiscoverTests)
    cmake_parse_arguments(
      "TEST"
      ""
      ""
      "ARCHS"
      ${ARGN}
    )
    # abort on first failure
    if(TEST_ARCHS)
      foreach(ARCH ${TEST_ARCHS})
        catch_discover_tests(${target} 
          EXTRA_ARGS -a -w NoAssertions --arch ${ARCH}
          PROPERTIES LABELS ${target}
        )
      endforeach()
    else()
      catch_discover_tests(${target} 
        EXTRA_ARGS -a -w NoAssertions 
        PROPERTIES LABELS ${target}
      )
    endif()

    if (POST_BUILD_TESTS)
      add_custom_command(TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> -L ${target} --output-on-failure -j8 ${TEST_UNPARSED_ARGUMENTS}
        COMMENT "Running ${target} unit tests")
    endif()
  endfunction()
else()
  function(parse_unit_tests)
  endfunction()
endif()

add_subdirectory(include)
add_subdirectory(Chapter01)
add_subdirectory(Chapter02)
add_subdirectory(Chapter03)
add_subdirectory(Chapter04)
add_subdirectory(Chapter05)
add_subdirectory(Chapter06)
add_subdirectory(Chapter07)
add_subdirectory(Chapter08)
