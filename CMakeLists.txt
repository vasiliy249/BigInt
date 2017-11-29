cmake_minimum_required (VERSION 2.6)
project (BigInt)

set(sources
  stdafx.cpp
  main.cpp
)

set(headers
  stdafx.h
)

include (CMakePCH.txt)
use_precompiled_header(sources stdafx.h stdafx.cpp)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED)                                                                            
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

include_directories(${PROJECT_SOURCE_DIR})

add_executable (BigInt ${sources} ${headers})

target_link_libraries(BigInt ${Boost_LIBRARIES})