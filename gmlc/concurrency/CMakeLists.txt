#
# Copyright © 2017-2019, Battelle Memorial Institute; Lawrence Livermore National
# Security, LLC; Alliance for Sustainable Energy, LLC All rights reserved. See LICENSE
# file and DISCLAIMER for more details.
#

set(
  concurrency_headers
  TriggerVariable.hpp
)

set(
  concurrency_sources
  TriggerVariable.cpp
  empty.cpp
)


add_library(concurrency STATIC ${concurrency_sources} ${concurrency_headers})

target_link_libraries(concurrency concurrency_base)
target_include_directories(concurrency INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

