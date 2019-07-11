set(CMAKE_C_FLAGS "-fsanitize=thread,undefined -g -O1" CACHE STRING "Flags used by the compiler during all build types.")
set(CMAKE_CXX_FLAGS "-fsanitize=thread,undefined -g -O1" CACHE STRING "Flags used by the compiler during all build types.")
set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=thread,undefined" CACHE STRING "Flags used by the linker during all build types.")
set(CMAKE_MODULE_LINKER_FLAGS "-fsanitize=thread,undefined" CACHE STRING "Flags used by the linker during all build types.")
