cmake_minimum_required(VERSION 3.8)

project(SpongeBob C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# include(../cmake/common.cmake)

find_package(Threads REQUIRED)
list(APPEND CMAKE_PREFIX_PATH "/opt/grpc")

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
  find_program(_PROTOBUF_PROTOC protoc)
else()
  set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
  find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

# Proto file
get_filename_component(sf_proto "./protos/spongebob.proto" ABSOLUTE)
get_filename_component(sf_proto_path "${sf_proto}" PATH)

# Generated sources
set(sf_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/spongebob.pb.cc")
set(sf_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/spongebob.pb.h")
set(sf_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/spongebob.grpc.pb.cc")
set(sf_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/spongebob.grpc.pb.h")
add_custom_command(
      OUTPUT "${sf_proto_srcs}" "${sf_proto_hdrs}" "${sf_grpc_srcs}" "${sf_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${sf_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${sf_proto}"
      DEPENDS "${sf_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# sf_grpc_proto
add_library(sf_grpc_proto
  ${sf_grpc_srcs}
  ${sf_grpc_hdrs}
  ${sf_proto_srcs}
  ${sf_proto_hdrs})
target_link_libraries(sf_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

link_libraries("ibverbs")
link_libraries("fuse")
# Compiler options
set (CMAKE_C_FLAGS "-Wall -Wno-deprecated-declarations -Wsign-compare")
set (CMAKE_CXX_FLAGS "-g ${CMAKE_C_FLAGS} -O0 -DTHREAD -D_FILE_OFFSET_BITS=64")
option (SANITIZE "Turn on sanitization" OFF)
if (SANITIZE)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

# Find 3rd party libs
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
find_package(Crypto REQUIRED)
include_directories(${CRYPTO_INCLUDE_DIR})
#message("-------: ${CRYPTO_INCLUDE_DIR}")
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Incs and Libs
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories("/usr/local/ofed/include")
link_libraries(${CRYPTO_LIBRARIES} ${CMAKE_DL_LIBS})
#message("---: ${CRYPTO_LIBRARIES}")

# Source file define
set(FS_SRC ${PROJECT_SOURCE_DIR}/src/fs)
set(NET_SRC ${PROJECT_SOURCE_DIR}/src/net)
set(TOOLS_SRC ${PROJECT_SOURCE_DIR}/src/tools)
set(SERVER_SRC ${PROJECT_SOURCE_DIR}/src/server)


# Build server
file(GLOB_RECURSE SERVER_SRC ${FS_SRC}/*.cpp ${FS_SRC}/*.c ${NET_SRC}/*.cpp ${TOOLS_SRC}/*.cpp )
add_executable(dmfs ${PROJECT_SOURCE_DIR}/src/server/dmfs.cpp ${SERVER_SRC})
target_link_libraries(dmfs
  Threads::Threads
  sf_grpc_proto
  absl::flags
  absl::flags_parse
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)

# Build server
add_executable(metaserver ${PROJECT_SOURCE_DIR}/src/server/metaserver.cpp ${SERVER_SRC} ${PROJECT_SOURCE_DIR}/src/metadata/MetaServer.cpp)
target_link_libraries(metaserver
  Threads::Threads
  sf_grpc_proto
  absl::flags
  absl::flags_parse
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# greeter_client.cc
# file(GLOB GRPC_TESTS_SOURCES ${PROJECT_SOURCE_DIR}/src/test/*.cc)
# foreach(GRPC_TESTSRC
#   ${GRPC_TESTS_SOURCES})
#   get_filename_component(GRPC_TESTSRC_NAME ${GRPC_TESTSRC} NAME_WE)
#   add_executable(${GRPC_TESTSRC_NAME} ${GRPC_TESTSRC})
#   target_link_libraries(${GRPC_TESTSRC_NAME}
#     sf_grpc_proto
#     absl::flags
#     absl::flags_parse
#     ${_REFLECTION}
#     ${_GRPC_GRPCPP}
#     ${_PROTOBUF_LIBPROTOBUF})
# endforeach()

# Build client library
set(CLIENT_BASE ${PROJECT_SOURCE_DIR}/client)
set(CNET_SRC ${PROJECT_SOURCE_DIR}/src/net)
set(CLIB_SRC ${PROJECT_SOURCE_DIR}/src/client)
file(GLOB_RECURSE NRFS_LIB_SRC ${CNET_SRC}/*.cpp ${CLIB_SRC}/*.cpp ${TOOLS_SRC}/*.cpp ${FS_SRC}/*.cpp)
# add_library(nrfsc SHARED ${NRFS_LIB_SRC})
add_library(nrfsc ${NRFS_LIB_SRC}) # Disable shared for now
target_link_libraries(nrfsc
  Threads::Threads
  sf_grpc_proto
  absl::flags
  absl::flags_parse
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

link_libraries(nrfsc)

file(GLOB TEST_SOURCES ${PROJECT_SOURCE_DIR}/src/test/*.cpp)
message("${TEST_SOURCES}")
foreach (TSTSRC ${TEST_SOURCES})
    get_filename_component(TSTNAME ${TSTSRC} NAME_WE)
    add_executable(${TSTNAME} ${TSTSRC})
endforeach()


# add_subdirectory(src/testfuse)