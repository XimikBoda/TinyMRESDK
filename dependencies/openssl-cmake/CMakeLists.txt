message(STATUS "Fetching openssl-cmake...")

option(WITH_APPS "" OFF)

FetchContent_MakeAvailable(openssl-cmake)