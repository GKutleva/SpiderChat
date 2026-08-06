cmake_minimum_required(VERSION 3.15)
project(spiderchat-server VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# If using vcpkg, pass the toolchain file when configuring CMake:
#   -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
# Optionally set -DVCPKG_TARGET_TRIPLET (e.g., x64-osx or arm64-osx)

find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

# Generate protobuf sources
set(PROTO_FILES ${CMAKE_SOURCE_DIR}/data/chat_message.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
add_library(proto_files ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(proto_files PRIVATE protobuf::libprotobuf)
# Ensure generated headers are visible to consumers
target_include_directories(proto_files PUBLIC ${CMAKE_BINARY_DIR})

add_executable(spiderchat-server
    src/main.cpp
    src/ws/server.cpp
    src/ws/session.cpp
    src/chat/chat_room.cpp
)

target_link_libraries(spiderchat-server PRIVATE
    fmt::fmt
    Boost::system
    Boost::thread
    nlohmann_json::nlohmann_json
    protobuf::libprotobuf
    proto_files
)
# Include source and build dirs so headers (including generated .pb.h and src headers) can be found
target_include_directories(spiderchat-server PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR})

install(TARGETS spiderchat-server DESTINATION bin)
