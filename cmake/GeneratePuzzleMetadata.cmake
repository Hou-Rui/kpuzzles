function(escape_cpp_string output_variable input)
    string(REPLACE "\\" "\\\\" escaped "${input}")
    string(REPLACE "\"" "\\\"" escaped "${escaped}")
    string(REPLACE "\r" "\\r" escaped "${escaped}")
    string(REPLACE "\n" "\\n" escaped "${escaped}")
    set(${output_variable} "${escaped}" PARENT_SCOPE)
endfunction()

function(generate_puzzle_metadata output_file upstream_cmake_file)
    set(selected_games ${ARGN})
    if(NOT selected_games)
        message(FATAL_ERROR "No games were supplied for puzzle metadata generation")
    endif()

    file(READ "${upstream_cmake_file}" upstream_cmake)
    string(REGEX MATCHALL
        "puzzle\\([A-Za-z0-9_]+[^)]*\\)"
        puzzle_declarations "${upstream_cmake}")

    set(generated "#pragma once\n\n")
    string(APPEND generated "struct PuzzleMetadata {\n")
    string(APPEND generated "    const char *id;\n")
    string(APPEND generated "    const char *displayName;\n")
    string(APPEND generated "    const char *description;\n")
    string(APPEND generated "    const char *objective;\n")
    string(APPEND generated "};\n\n")
    string(APPEND generated "inline constexpr PuzzleMetadata puzzleMetadata[] = {\n")

    set(found_games)
    foreach(declaration IN LISTS puzzle_declarations)
        if(NOT declaration MATCHES "puzzle\\(([A-Za-z0-9_]+)")
            continue()
        endif()
        set(game_name "${CMAKE_MATCH_1}")

        list(FIND selected_games "${game_name}" selected_index)
        if(selected_index EQUAL -1)
            continue()
        endif()

        list(FIND found_games "${game_name}" duplicate_index)
        if(NOT duplicate_index EQUAL -1)
            message(FATAL_ERROR
                "Duplicate upstream metadata for selected game ${game_name}")
        endif()

        foreach(field IN ITEMS DISPLAYNAME DESCRIPTION OBJECTIVE)
            if(NOT declaration MATCHES "${field}[ \t\r\n]+\"([^\"]+)\"")
                message(FATAL_ERROR
                    "Missing ${field} in upstream metadata for ${game_name}")
            endif()

            set(field_value "${CMAKE_MATCH_1}")
            # Match CMake's quoted-argument line continuation behaviour.
            string(REPLACE "\\\r\n" "" field_value "${field_value}")
            string(REPLACE "\\\n" "" field_value "${field_value}")
            escape_cpp_string(field_value "${field_value}")
            set(${field} "${field_value}")
        endforeach()

        string(APPEND generated
            "    {\"${game_name}\", \"${DISPLAYNAME}\", \"${DESCRIPTION}\", \"${OBJECTIVE}\"},\n")
        list(APPEND found_games "${game_name}")
    endforeach()

    foreach(game_name IN LISTS selected_games)
        list(FIND found_games "${game_name}" found_index)
        if(found_index EQUAL -1)
            message(FATAL_ERROR
                "No upstream puzzle metadata found for selected game ${game_name}")
        endif()
    endforeach()

    string(APPEND generated "};\n")
    set(temporary_file "${output_file}.tmp")
    file(WRITE "${temporary_file}" "${generated}")
    configure_file("${temporary_file}" "${output_file}" COPYONLY)
    file(REMOVE "${temporary_file}")
endfunction()
