cmake_minimum_required(VERSION 3.3)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

project(GreenTiger)


# Download automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
                  "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

if(WIN32)
  find_program(CONAN conan.bat)
endif()

conan_check()

conan_add_remote(
  NAME manu343726 
  URL https://api.bintray.com/conan/manu343726/conan-packages)


set(DEPENDENCIES 
      boost/1.69.0@conan/stable
      range-v3/0.9.0@ericniebler/stable
      Catch2/2.9.2@catchorg/stable
      gsl_microsoft/2.0.0@bincrafters/stable
      type_safe/0.3@Manu343726/testing
)

conan_cmake_run(REQUIRES ${DEPENDENCIES}
                CONAN_COMMAND ${CONAN}
                GENERATORS cmake_paths
                BUILD missing)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)


find_package(Boost REQUIRED COMPONENTS system filesystem)

find_package(Catch2 CONFIG REQUIRED)

find_package(range-v3 CONFIG REQUIRED)

target_compile_options(range-v3 INTERFACE $<$<CXX_COMPILER_ID:GNU>:-Wno-deprecated-declarations>)

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
add_subdirectory(Chapter09)
add_subdirectory(Chapter10)
