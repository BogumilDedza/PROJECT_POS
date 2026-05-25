cmake_minimum_required(VERSION 3.16)
project(image_processor CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OpenCV_DIR "C:/Users/DELL/Downloads/opencv/build")

find_package(OpenCV REQUIRED)

add_library(inih STATIC ini.c)

configure_file(config.ini ${CMAKE_BINARY_DIR}/config.ini COPYONLY)

add_executable(image_processor main.cpp)

target_link_libraries(image_processor
    PRIVATE
    inih
    ${OpenCV_LIBS}
)