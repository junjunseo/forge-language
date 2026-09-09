foreach(required IN ITEMS IEUM_EXECUTABLE IEUM_SOURCE OUTPUT_TARGET EXPECTED_EXIT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing required variable: ${required}")
    endif()
endforeach()

execute_process(
    COMMAND "${IEUM_EXECUTABLE}" "${IEUM_SOURCE}" --emit-dot "${OUTPUT_TARGET}"
    RESULT_VARIABLE actual_exit
    OUTPUT_VARIABLE standard_output
    ERROR_VARIABLE standard_error
    ENCODING UTF-8
)

if(NOT "${actual_exit}" STREQUAL "${EXPECTED_EXIT}")
    message(FATAL_ERROR
        "Graph failure changed validation exit: expected ${EXPECTED_EXIT}, got ${actual_exit}\n"
        "stdout:\n${standard_output}\n"
        "stderr:\n${standard_error}"
    )
endif()

set(combined_output "${standard_output}\n${standard_error}")
string(FIND "${combined_output}" "graph_export=failed" warning_position)
if(warning_position EQUAL -1)
    message(FATAL_ERROR
        "Expected graph export warning was not printed\n"
        "stdout:\n${standard_output}\n"
        "stderr:\n${standard_error}"
    )
endif()

message(STATUS "Verified graph export failure preserves exit ${actual_exit}")
