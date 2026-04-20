function(enum_option optvar description default)
    cmake_parse_arguments(PARSE_ARGV 0 arg
            "" "" "OPTIONS")
    set(${optvar} ${default} CACHE STRING ${description})
    set_property(CACHE ${optvar} PROPERTY STRINGS ${arg_OPTIONS})

    list(FIND arg_OPTIONS ${${optvar}} opt_index)
    if (opt_index EQUAL "-1")
        message(SEND_ERROR "${optvar} was set to '${${optvar}}' which is not one of [${arg_OPTIONS}]")
    endif ()
endfunction()