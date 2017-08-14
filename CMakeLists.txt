cmake_minimum_required(VERSION 3.3)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(HunterGate)

HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.19.35.tar.gz"
    SHA1 "bacf59b6856e37964c8eb298a57e0f6ef37e56d6"
)

project(GreenTiger)

set(CMAKE_CXX_STANDARD 14)

set(BOOST_COMPONENTS system filesystem)
hunter_add_package(Boost COMPONENTS ${BOOST_COMPONENTS})
find_package(Boost CONFIG REQUIRED ${BOOST_COMPONENTS})

hunter_add_package(Catch)
find_package(Catch CONFIG REQUIRED)

enable_testing()

function(parse_unit_tests target)
    include(ParseAndAddCatchTests)
    # abort on first failure
    ParseAndAddCatchTests(${target} "-a")

    include(ProcessorCount)
    ProcessorCount(CORES)
    set(CTEST_OPTIONS -C $<CONFIG> -L ${target})
    add_custom_command(TARGET ${target}
	    POST_BUILD
		COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> -L ${target} --output-on-failure
	    COMMENT "Running ${target} unit tests")
endfunction()

add_subdirectory(include)
add_subdirectory(Chapter01)
add_subdirectory(Chapter02)