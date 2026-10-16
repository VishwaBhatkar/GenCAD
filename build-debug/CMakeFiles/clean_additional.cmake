# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  [[CMakeFiles\gencad_autogen.dir\AutogenUsed.txt]]
  [[CMakeFiles\gencad_autogen.dir\ParseCache.txt]]
  [[CMakeFiles\gencad_tests_autogen.dir\AutogenUsed.txt]]
  [[CMakeFiles\gencad_tests_autogen.dir\ParseCache.txt]]
  "gencad_autogen"
  "gencad_tests_autogen"
  )
endif()
