foreach(required IN ITEMS IEUM_EXECUTABLE IEUM_SOURCE EXPECTED_EXIT EXPECTED_TEXT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing required variable: ${required}")
    endif()
endforeach()

execute_process(
    COMMAND "${IEUM_EXECUTABLE}" "${IEUM_SOURCE}"
    RESULT_VARIABLE actual_exit
    OUTPUT_VARIABLE standard_output
    ERROR_VARIABLE standard_error
    ENCODING UTF-8
)

if(NOT "${actual_exit}" STREQUAL "${EXPECTED_EXIT}")
    message(FATAL_ERROR
        "Unexpected exit code for ${IEUM_SOURCE}: expected ${EXPECTED_EXIT}, got ${actual_exit}\n"
        "stdout:\n${standard_output}\n"
        "stderr:\n${standard_error}"
    )
endif()

if(NOT "${EXPECTED_TEXT}" STREQUAL "")
    set(combined_output "${standard_output}\n${standard_error}")
    string(FIND "${combined_output}" "${EXPECTED_TEXT}" expected_text_position)
    if(expected_text_position EQUAL -1)
        message(FATAL_ERROR
            "Expected output not found for ${IEUM_SOURCE}: ${EXPECTED_TEXT}\n"
            "stdout:\n${standard_output}\n"
            "stderr:\n${standard_error}"
        )
    endif()
endif()

message(STATUS
    "Verified ${IEUM_SOURCE}: exit ${actual_exit}, output contains '${EXPECTED_TEXT}'"
)
