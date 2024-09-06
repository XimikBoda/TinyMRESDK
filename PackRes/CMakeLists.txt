# Add source to this project's executable.
add_executable (PackRes  "main.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET PackRes PROPERTY CXX_STANDARD 20)
endif()

