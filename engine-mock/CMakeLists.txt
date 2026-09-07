cmake_minimum_required(VERSION 3.20)
project(visor_engine_mock CXX)

# C++23を強制
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FlatBuffersのインクルードパス（自動生成コード用）
include_directories(${CMAKE_SOURCE_DIR}/../generated)

add_executable(engine_mock src/main.cpp)