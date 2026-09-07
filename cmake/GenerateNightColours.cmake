function(generate_night_colours output_file data_file puzzles_dir)
    file(STRINGS "${data_file}" night_colour_lines REGEX "^[a-z0-9]")

    set(generated "#pragma once\n\n")
    string(APPEND generated "#include <cstdint>\n\n")
    string(APPEND generated "struct NightColourOverride {\n")
    string(APPEND generated "    const char *game;\n")
    string(APPEND generated "    int index;\n")
    string(APPEND generated "    std::uint32_t rgb;\n")
    string(APPEND generated "};\n\n")
    string(APPEND generated "inline constexpr NightColourOverride nightColourOverrides[] = {\n")

    foreach(line IN LISTS night_colour_lines)
        if(NOT line MATCHES "^([a-z0-9]+)[ \t]+([a-z0-9_]+)[ \t]+([0-9A-Fa-f]+)$")
            message(FATAL_ERROR "Invalid night colour entry: ${line}")
        endif()
        set(game_name "${CMAKE_MATCH_1}")
        set(role_name "${CMAKE_MATCH_2}")
        set(rgb "${CMAKE_MATCH_3}")

        set(source_file "${puzzles_dir}/${game_name}.c")
        file(READ "${source_file}" source_text)
        string(REGEX MATCH
            "enum[ \t\r\n]*\\{[^}]*COL_[A-Z_]*BACKGROUND[^}]*NCOLOURS[^}]*\\}"
            colour_enum "${source_text}")
        if(NOT colour_enum)
            message(FATAL_ERROR "Could not find colour enum in ${source_file}")
        endif()

        string(REGEX REPLACE "/\\*([^*]|\\*+[^*/])*\\*/" " " colour_enum "${colour_enum}")
        string(REGEX REPLACE "//[^\n]*" " " colour_enum "${colour_enum}")
        string(REGEX REPLACE "^[^{]*\\{" "" colour_enum "${colour_enum}")
        string(REGEX REPLACE "\\}.*$" "" colour_enum "${colour_enum}")
        string(REPLACE "," ";" enum_members "${colour_enum}")

        set(colour_names)
        foreach(member IN LISTS enum_members)
            string(STRIP "${member}" member)
            if(member MATCHES "^(COL_[A-Z0-9_]+)")
                set(colour_name "${CMAKE_MATCH_1}")
                if(NOT colour_names OR NOT member MATCHES "=")
                    list(APPEND colour_names "${colour_name}")
                endif()
            endif()
        endforeach()

        # Signpost reserves four banks of 16 colours using assigned enum
        # values. Expand those banks just as the Android generator does.
        if(game_name STREQUAL "signpost")
            list(POP_BACK colour_names)
            foreach(prefix IN ITEMS B M D X)
                foreach(number RANGE 0 15)
                    list(APPEND colour_names "COL_${prefix}${number}")
                endforeach()
            endforeach()
        endif()

        string(TOUPPER "${role_name}" role_upper)
        list(FIND colour_names "COL_${role_upper}" colour_index)
        if(colour_index EQUAL -1)
            message(FATAL_ERROR
                "Night colour role ${game_name}/${role_name} is absent from ${source_file}")
        endif()

        string(APPEND generated
            "    {\"${game_name}\", ${colour_index}, 0x${rgb}U},\n")
    endforeach()

    string(APPEND generated "};\n")
    set(temporary_file "${output_file}.tmp")
    file(WRITE "${temporary_file}" "${generated}")
    configure_file("${temporary_file}" "${output_file}" COPYONLY)
    file(REMOVE "${temporary_file}")
endfunction()
