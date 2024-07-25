cmake_minimum_required (VERSION 3.10)
project(vdms-utils)
include_directories(include/comm include/chrono include/stats include/timers)
add_library(vdms-utils SHARED src/timers/TimerMap.cc src/comm/ConnClient.cc src/comm/Connection.cc src/comm/Exception.cc src/comm/ConnServer.cc src/stats/SystemStats.cc)
