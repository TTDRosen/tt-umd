function(FIND_AND_SET_CLANG17)
    find_program(CLANGPP_17 clang++-17)
    find_program(CLANG_17 clang-17)

    if(NOT CLANGPP_17 OR NOT CLANG_17)
        message(
            FATAL_ERROR
            "Clang-17 not found. Make sure you have clang-17 and clang++-17 installed and in your PATH"
        )
    endif()

    set(CMAKE_CXX_COMPILER "${CLANGPP_17}" PARENT_SCOPE)
    set(CMAKE_C_COMPILER "${CLANG_17}" PARENT_SCOPE)
endfunction()

function(CHECK_COMPILERS)
    message(STATUS "Checking compilers")

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} -stdlib=libc++"
            CACHE STRING
            "CXX FLAGS for clang"
        )
        if(
            CMAKE_CXX_COMPILER_VERSION
                VERSION_LESS
                "17.0.0"
            OR CMAKE_CXX_COMPILER_VERSION
                GREATER_EQUAL
                "18.0.0"
        )
            message(WARNING "Only Clang-17 is tested right now")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.0.0")
            message(WARNING "GCC-12 or higher is suggested")
        elseif(CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL "13.0.0")
            message(WARNING "Only GCC-12 is tested right now")
        endif()
    else()
        message(
            FATAL_ERROR
            "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID} ! Only Clang and GCC are supported"
        )
    endif()
endfunction()
