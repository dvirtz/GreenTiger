if(NOT DEFINED BUILD_TYPE)
    message(FATAL_ERROR "Please provide a build type with -DBUILD_TYPE=<build type>")
endif()

set(CMAKE_EXECUTE_PROCESS_COMMAND_ECHO STDOUT)

function(execute EXECUTABLE)
    execute_process(COMMAND ${EXECUTABLE} ${ARGN} RESULT_VARIABLE retcode)
    if(NOT "${retcode}" STREQUAL "0")
      message(FATAL_ERROR "Fatal error when running ${EXECUTABLE}")
    endif()
endfunction()

get_filename_component(SOURCE_FOLDER ${CMAKE_SCRIPT_MODE_FILE} DIRECTORY)
get_filename_component(SOURCE_FOLDER ${SOURCE_FOLDER} DIRECTORY)

if(NOT DEFINED BUILD_FOLDER)
    set(BUILD_FOLDER build)
endif()

execute(${CMAKE_COMMAND} -S ${SOURCE_FOLDER} -B ${BUILD_FOLDER} -G Ninja -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DRUN_TESTS_POSTBUILD=ON -DCMAKE_CXX_STANDARD=17)
execute(${CMAKE_COMMAND} --build ${BUILD_FOLDER})