foreach(required IN ITEMS
        IEUM_EXECUTABLE IEUM_SOURCE ACTUAL_DOT EXPECTED_DOT EXPECTED_EXIT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing required variable: ${required}")
    endif()
endforeach()

file(REMOVE "${ACTUAL_DOT}")
execute_process(
    COMMAND "${IEUM_EXECUTABLE}" "${IEUM_SOURCE}" --emit-dot "${ACTUAL_DOT}"
    RESULT_VARIABLE actual_exit
    OUTPUT_VARIABLE standard_output
    ERROR_VARIABLE standard_error
    ENCODING UTF-8
)

if(NOT "${actual_exit}" STREQUAL "${EXPECTED_EXIT}")
    message(FATAL_ERROR
        "Unexpected graph command exit: expected ${EXPECTED_EXIT}, got ${actual_exit}\n"
        "stdout:\n${standard_output}\n"
        "stderr:\n${standard_error}"
    )
endif()

if(NOT EXISTS "${ACTUAL_DOT}")
    message(FATAL_ERROR "Graph output was not created: ${ACTUAL_DOT}")
endif()

file(READ "${ACTUAL_DOT}" actual_graph)
file(READ "${EXPECTED_DOT}" expected_graph)
string(REPLACE "\r\n" "\n" actual_graph "${actual_graph}")
string(REPLACE "\r\n" "\n" expected_graph "${expected_graph}")

if(NOT "${actual_graph}" STREQUAL "${expected_graph}")
    message(FATAL_ERROR
        "Graph snapshot mismatch for ${IEUM_SOURCE}\n"
        "Expected:\n${expected_graph}\n"
        "Actual:\n${actual_graph}"
    )
endif()

message(STATUS "Verified deterministic graph snapshot for ${IEUM_SOURCE}")
