include(FetchContent)

FetchContent_Declare(
  openssl-cmake
  GIT_REPOSITORY https://github.com/janbar/openssl-cmake
  GIT_TAG b0ac69581958cd658364147da9057da89a01c394
)
add_subdirectory(openssl-cmake)