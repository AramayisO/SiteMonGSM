cmake_minimum_required(VERSION 3.0)

project(SiteMonGSM)

include_directories(include)

add_executable(${PROJECT_NAME} src/main.c src/serial.c src/gsm.c src/util.c)
