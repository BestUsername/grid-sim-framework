# Coverage.cmake

# The defined functions must be called in the following order:
# 1. initialize_coverage() once
# 2. add_directory_to_coverage() zero to many times
# 3. finalize_coverage() once

# One way to make sure you only call finalize_coverage() once is to use it in the top level CMakeLists.txt file like this:
#   if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
#     finalize_coverage()
#   endif()

# only add coverage testing if the build type is Debug
if(CMAKE_BUILD_TYPE MATCHES Debug)

    if(NOT DEFINED COVERAGE_FIRST_CALL)
      # include ctest if it hasn't already been included
      if(NOT COMMAND ctest)
        include(CTest)
      endif()

      # include googletest if it hasn't already been included
      if(NOT TARGET GTest::GTest)
        include(GoogleTest)
      endif()

      # Set the C++ flags for code coverage
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fprofile-arcs -ftest-coverage")
      set(COVERAGE_FIRST_CALL TRUE)
    endif()

    # write the beginnning of the coverage script
    if(NOT COMMAND initialize_coverage)
      function(initialize_coverage)

        if(DEFINED COVERAGE_INITIALIZED)
          message(STATUS "Coverage has already been initialized. Skipping initialize_coverage().")
          return()
        endif()

        # message(STATUS "${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}")

        set(COVERAGE_INITIALIZED TRUE PARENT_SCOPE)

        # write the beginning of the coverage script
        file(WRITE ${CMAKE_BINARY_DIR}/coverage.sh
          "#!/bin/bash\n"
          # clear total coverage by setting a dummy coverage file. 
          # Requires removing the dummy.cpp file from the coverage data before running genhtml.
          "echo \"TN:\nSF:/dummy.cpp\nFN:1,main\nFNDA:1,main\nDA:1,1\nDA:2,1\nend_of_record\" > ${CMAKE_BINARY_DIR}/total_coverage.info\n"
          #initialize counters
          "TESTS=0\n"
          "ERRORS=0\n"
          # run tests
          "ctest -j$(nproc)\n"
        )

      endfunction()
    endif()

    # add a directory to the coverage script
    if(NOT COMMAND add_directory_to_coverage)
      function(add_directory_to_coverage TEST_DIRECTORY THRESHOLD_LINE_COVERAGE THRESHOLD_FUNCTION_COVERAGE)
        file(RELATIVE_PATH RELATIVE_DIR ${CMAKE_SOURCE_DIR} ${TEST_DIRECTORY})
        set(SOURCE_DIRECTORY ${CMAKE_SOURCE_DIR}/${RELATIVE_DIR})
        set(TEST_DIRECTORY ${CMAKE_BINARY_DIR}/${RELATIVE_DIR})

        message(STATUS "-= COVERAGE: adding directory ${TEST_DIRECTORY} with thresholds (${THRESHOLD_LINE_COVERAGE} ${THRESHOLD_FUNCTION_COVERAGE}) =-")

        if(NOT TEST_DIRECTORY OR NOT THRESHOLD_LINE_COVERAGE OR NOT THRESHOLD_FUNCTION_COVERAGE)
          message(FATAL_ERROR "add_directory_to_coverage() requires three arguments - something is wrong with the calling script")
          return()
        endif()

        if(NOT DEFINED COVERAGE_INITIALIZED)
          message(FATAL_ERROR "You cannot call add_directory_to_coverage() before initialize_coverage() - something is wrong with the config order")
        endif()

        if(DEFINED COVERAGE_FINALIZED)
          message(FATAL_ERROR "You cannot call add_directory_to_coverage() after finalize_coverage() - something is wrong with the config order")
        endif()

        file(APPEND ${CMAKE_BINARY_DIR}/coverage.sh 
          # clear out existing coverage data for this directory - the lcov command that overwrites can fail and then reports use old data
          "rm -f ${TEST_DIRECTORY}/coverage.info\n"
          # capture coverage info for the specified directory (overwrites the file if it already exists)
          "lcov -q --ignore-errors inconsistent --rc geninfo_unexecuted_blocks=1 --directory ${TEST_DIRECTORY} --capture --output-file ${TEST_DIRECTORY}/coverage.info 2>/dev/null\n"
          # keep only files that belong to the current source directory
          "lcov -q --ignore-errors inconsistent,unused --extract ${TEST_DIRECTORY}/coverage.info '${SOURCE_DIRECTORY}/*' --output-file ${TEST_DIRECTORY}/coverage.info 2>/dev/null\n"
          # remove system files and googletest files from the coverage data
          "lcov -q --ignore-errors inconsistent,unused --remove ${TEST_DIRECTORY}/coverage.info '/usr/*' --output-file ${TEST_DIRECTORY}/coverage.info 2>/dev/null\n"
          "lcov -q --ignore-errors inconsistent,unused --remove ${TEST_DIRECTORY}/coverage.info '*/googletest/*' --output-file ${TEST_DIRECTORY}/coverage.info 2>/dev/null\n"
          "lcov -q --ignore-errors inconsistent,unused --remove ${TEST_DIRECTORY}/coverage.info '*/tests/*' --output-file ${TEST_DIRECTORY}/coverage.info 2>/dev/null\n"
          # add the coverage data from this directory to the total coverage data
          "lcov -q --ignore-errors inconsistent,empty --add-tracefile ${CMAKE_BINARY_DIR}/total_coverage.info --add-tracefile ${TEST_DIRECTORY}/coverage.info --output-file ${CMAKE_BINARY_DIR}/total_coverage.info\n"
          # display an overview of the coverage data for this directory
          "lcov -q --list ${TEST_DIRECTORY}/coverage.info 2>/dev/null\n"
          
          # grab line coverage percentage from the lcov summary. (rounds down floats to nearest integer)
          "LINE_COVERAGE_PERCENT=$(lcov --summary ${TEST_DIRECTORY}/coverage.info 2>&1 | grep 'lines' | awk '{printf(\"%d\\n\", $2)}')\n"
          # add to error count if line coverage is less than the threshold set in THRESHOLD_LINE_COVERAGE.
          "TESTS=$((TESTS+1))\n"
          "if [ \"$LINE_COVERAGE_PERCENT\" -lt ${THRESHOLD_LINE_COVERAGE} ]; then\n"
          "  echo \"ERROR: Code line coverage is less than ${THRESHOLD_LINE_COVERAGE}%: $LINE_COVERAGE_PERCENT%\"\n"
          "  ERRORS=$((ERRORS+1))\n"
          "fi\n"

          # grab function coverage percentage from the lcov summary. (rounds down floats to nearest integer)
          "FN_COVERAGE_PERCENT=$(lcov --summary ${TEST_DIRECTORY}/coverage.info 2>&1 | grep 'functions' | awk '{printf(\"%d\\n\", $2)}')\n"
          # add to Error count if function coverage is less than the threshold set in THRESHOLD_FUNCTION_COVERAGE.
          "TESTS=$((TESTS+1))\n"
          "if [ \"$FN_COVERAGE_PERCENT\" -lt ${THRESHOLD_FUNCTION_COVERAGE} ]; then\n"
          "  echo \"ERROR: Code functions coverage is less than ${THRESHOLD_FUNCTION_COVERAGE}%: $FN_COVERAGE_PERCENT%\"\n"
          "  ERRORS=$((ERRORS+1))\n"
          "fi\n"
        )
      endfunction()
    endif()

    if(NOT COMMAND finalize_coverage)
      function(finalize_coverage)
        message(STATUS "-= COVERAGE: Finalizing =-")
        
        # error out if initialize_coverage() hasn't been called.
        if(NOT DEFINED COVERAGE_INITIALIZED)
          message(FATAL_ERROR "initialize_coverage() must be called before finalize_coverage().")
        endif()  
        
        # Currently doesn't error out if add_directory_to_coverage() hasn't been called.
        
        # gracefully exit if finalize_coverage() has already been called.
        if(DEFINED COVERAGE_FINALIZED)
          message(STATUS "Coverage has already been finalized. Skipping finalize_coverage().")
          return()
        endif()
        set(COVERAGE_FINALIZED TRUE)

        # write the end of the coverage script
        file(APPEND ${CMAKE_BINARY_DIR}/coverage.sh
          # Remove dummy.cpp coverage data (required by the clear total coverage step above)
          "lcov -q --remove ${CMAKE_BINARY_DIR}/total_coverage.info '*/dummy.cpp' --output-file ${CMAKE_BINARY_DIR}/total_coverage.info\n"
          # Generate HTML report from total_coverage.info
          "genhtml -q --ignore-errors unsupported ${CMAKE_BINARY_DIR}/total_coverage.info --output-directory ${CMAKE_BINARY_DIR}/coverage_report\n"
          # Exit with error if ERRORS while displaying how many tests failed out of the total
          "if [ \"$ERRORS\" -ge 1 ]; then\n"
          "  echo \"FAILURES: \"$ERRORS\" / \"$TESTS\"\"\n"
          "  exit 1\n"
          "fi\n"
        )

        # create the coverage target
        message(STATUS "Adding coverage target...")
        add_custom_target(coverage
          COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --parallel
          COMMAND chmod +x ${CMAKE_BINARY_DIR}/coverage.sh
          COMMAND ${CMAKE_BINARY_DIR}/coverage.sh
        )

        # convenience target to build all and then run coverage
        add_custom_target(all_and_coverage
          COMMAND ${CMAKE_MAKE_PROGRAM}
          COMMAND ${CMAKE_MAKE_PROGRAM} coverage
          COMMENT "Building all and coverage targets"
        )

      endfunction()
    endif()

endif() # end of if(CMAKE_BUILD_TYPE MATCHES Debug)
