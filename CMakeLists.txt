cmake_minimum_required(VERSION 3.22)
project(tmnf-x_100)

set(CMAKE_CXX_STANDARD 20)

add_executable(Setup main.cpp)
add_library(Requests STATIC requests.cpp requests.h)

find_package(Boost 1.85.0 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

message(Boost_LIBRARIES="${Boost_LIBRARIES}")

target_link_libraries(Requests
        ${Boost_LIBRARIES}
        OpenSSL::Crypto
        OpenSSL::SSL
        CURL::libcurl
)

target_include_directories(Requests
        PUBLIC
        ${Boost_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
)

target_link_libraries(Setup Requests)