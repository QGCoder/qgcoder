# Runs one interpreter test case and compares its canonical output with the
# recorded one. Driven by add_test() in the top-level CMakeLists.txt.
file(MAKE_DIRECTORY "${WORKDIR}")

execute_process(
    COMMAND "${TOOL}" "${NGC}" "${WORKDIR}/rs274ngc.var"
    OUTPUT_FILE "${WORKDIR}/actual"
    ERROR_VARIABLE stderr_text
    RESULT_VARIABLE status
)

if(NOT status EQUAL 0)
    message(FATAL_ERROR "interpreting ${NGC} failed (${status}):\n${stderr_text}")
endif()

# CMake's compare_files is exact; the canonical output is deterministic, so a
# difference is a real one rather than formatting noise.
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${WORKDIR}/actual" "${EXPECTED}"
    RESULT_VARIABLE differs
)
if(NOT differs EQUAL 0)
    execute_process(COMMAND diff -u "${EXPECTED}" "${WORKDIR}/actual"
                    OUTPUT_VARIABLE d ERROR_QUIET)
    message(FATAL_ERROR "canonical output changed for ${NGC}:\n${d}")
endif()
