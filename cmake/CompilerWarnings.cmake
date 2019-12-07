
option(TREAT_WARNINGS_AS_ERRORS "Treat warnings as errors" TRUE)
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /experimental:external /external:W0 ")
    set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "/external:I ")
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        string(APPEND CMAKE_CXX_FLAGS " /W4")
    endif()
    string(APPEND CMAKE_CXX_FLAGS " /DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE /D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS /wd4702")
    if(TREAT_WARNINGS_AS_ERRORS)
      string(APPEND CMAKE_CXX_FLAGS " /WX")
    endif()
    string(APPEND CMAKE_CXX_FLAGS " /permissive-")
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -pedantic -Wno-unused-function -Wno-missing-field-initializers -Wno-missing-braces")
    string(APPEND CMAKE_C_FLAGS " -Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -Wno-unused-function")
    if(TREAT_WARNINGS_AS_ERRORS)
      string(APPEND CMAKE_CXX_FLAGS " -Werror")
    endif()
else()
    message(WARNING "Unable to set warning level: Unknown compiler ${CMAKE_CXX_COMPILER_ID}")
endif()
