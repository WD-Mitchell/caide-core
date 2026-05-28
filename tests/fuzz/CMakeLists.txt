# Fuzz harnesses (libFuzzer).
#
# Requires Clang with -fsanitize=fuzzer (and ideally ,address,undefined).
# Enable with -DCAIDE_BUILD_FUZZ=ON.
#
# Apple Clang on macOS does NOT ship libFuzzer runtime. For local smoke
# verification on macOS, use -DCAIDE_FUZZ_STANDALONE=ON which builds a tiny
# self-driving main() that invokes LLVMFuzzerTestOneInput with synthetic
# inputs (no sanitizer instrumentation, no coverage). For real fuzzing
# use mainline LLVM clang on Linux (or Homebrew LLVM on macOS).

option(CAIDE_FUZZ_STANDALONE
    "Build fuzz harnesses with a stub main() instead of libFuzzer (for macOS smoke)"
    OFF)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR
        "CAIDE_BUILD_FUZZ=ON requires Clang (current: ${CMAKE_CXX_COMPILER_ID}). "
        "Re-configure with `-DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang`.")
endif()

set(CAIDE_FUZZ_FLAGS "-fsanitize=fuzzer-no-link,address,undefined" CACHE STRING
    "Sanitizer flags applied to fuzz harnesses (compile)")
set(CAIDE_FUZZ_LINK_FLAGS "-fsanitize=fuzzer,address,undefined" CACHE STRING
    "Sanitizer flags applied to fuzz harnesses (link)")

function(caide_add_fuzz name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE caide_core)
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
    if(CAIDE_FUZZ_STANDALONE)
        target_compile_definitions(${name} PRIVATE CAIDE_FUZZ_STANDALONE=1)
        target_compile_options(${name} PRIVATE -g -O1)
    else()
        target_compile_options(${name} PRIVATE -g -O1 ${CAIDE_FUZZ_FLAGS})
        target_link_options(${name} PRIVATE ${CAIDE_FUZZ_LINK_FLAGS})
    endif()
endfunction()

caide_add_fuzz(fuzz_command_executor fuzz_command_executor.cpp)
caide_add_fuzz(fuzz_history fuzz_history.cpp)

# 60-second smoke target. With libFuzzer: -max_total_time=60 + runs cap.
# With CAIDE_FUZZ_STANDALONE: the standalone driver runs its built-in corpus
# (single-shot, completes in milliseconds — the 60s budget is unused).
set(_caide_fuzz_smoke_corpus ${CMAKE_CURRENT_BINARY_DIR}/corpus)
file(MAKE_DIRECTORY ${_caide_fuzz_smoke_corpus})

if(CAIDE_FUZZ_STANDALONE)
    add_custom_target(fuzz-smoke
        COMMAND ${CMAKE_COMMAND} -E echo "[fuzz-smoke] standalone driver: fuzz_command_executor"
        COMMAND $<TARGET_FILE:fuzz_command_executor>
        COMMAND ${CMAKE_COMMAND} -E echo "[fuzz-smoke] standalone driver: fuzz_history"
        COMMAND $<TARGET_FILE:fuzz_history>
        DEPENDS fuzz_command_executor fuzz_history
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running standalone fuzz smoke (synthetic inputs)"
        USES_TERMINAL)
else()
    add_custom_target(fuzz-smoke
        COMMAND ${CMAKE_COMMAND} -E echo "[fuzz-smoke] libFuzzer: fuzz_command_executor for 60s..."
        COMMAND $<TARGET_FILE:fuzz_command_executor>
            -max_total_time=60 -runs=500000 -print_final_stats=1
            ${_caide_fuzz_smoke_corpus}
        COMMAND ${CMAKE_COMMAND} -E echo "[fuzz-smoke] libFuzzer: fuzz_history for 60s..."
        COMMAND $<TARGET_FILE:fuzz_history>
            -max_total_time=60 -runs=500000 -print_final_stats=1
            ${_caide_fuzz_smoke_corpus}
        DEPENDS fuzz_command_executor fuzz_history
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running 60-second libFuzzer smoke on caide_core harnesses"
        USES_TERMINAL)
endif()
