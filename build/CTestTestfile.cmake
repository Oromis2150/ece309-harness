# CMake generated Testfile for 
# Source directory: /home/oromis/ece309/ece309-harness
# Build directory: /home/oromis/ece309/ece309-harness/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(p2_regression "/home/oromis/ece309/ece309-harness/build/test_p2")
set_tests_properties(p2_regression PROPERTIES  WORKING_DIRECTORY "/home/oromis/ece309/ece309-harness/build" _BACKTRACE_TRIPLES "/home/oromis/ece309/ece309-harness/CMakeLists.txt;82;add_test;/home/oromis/ece309/ece309-harness/CMakeLists.txt;0;")
add_test(p3 "/home/oromis/ece309/ece309-harness/build/test_p3")
set_tests_properties(p3 PROPERTIES  WORKING_DIRECTORY "/home/oromis/ece309/ece309-harness/build" _BACKTRACE_TRIPLES "/home/oromis/ece309/ece309-harness/CMakeLists.txt;86;add_test;/home/oromis/ece309/ece309-harness/CMakeLists.txt;0;")
add_test(tool_loop "/home/oromis/ece309/ece309-harness/build/test_loop")
set_tests_properties(tool_loop PROPERTIES  WORKING_DIRECTORY "/home/oromis/ece309/ece309-harness/build" _BACKTRACE_TRIPLES "/home/oromis/ece309/ece309-harness/CMakeLists.txt;90;add_test;/home/oromis/ece309/ece309-harness/CMakeLists.txt;0;")
add_test(e2e "bash" "/home/oromis/ece309/ece309-harness/tests/e2e/run_e2e.sh" "/home/oromis/ece309/ece309-harness/build/miniharness")
set_tests_properties(e2e PROPERTIES  WORKING_DIRECTORY "/home/oromis/ece309/ece309-harness" _BACKTRACE_TRIPLES "/home/oromis/ece309/ece309-harness/CMakeLists.txt;99;add_test;/home/oromis/ece309/ece309-harness/CMakeLists.txt;0;")
