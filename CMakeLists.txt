cmake_minimum_required(VERSION 2.8.9)
project(webserver)
 
#Bring the headers, such as Student.h into the project
include_directories(inc src/libwebsockets)
 
#Can manually add the sources using the set command as follows:
#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "src/libwebsockets/*.c" "src/main.cpp" "src/server/stagehand.c")
add_executable(webserver ${SOURCES})
target_link_libraries(webserver -lz)
