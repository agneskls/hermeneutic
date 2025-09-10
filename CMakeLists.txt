cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE ../external/vcpkg/scripts/buildsystems/vcpkg.cmake)

project(my_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(OpenSSL REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(Poco REQUIRED COMPONENTS Foundation Net JSON NetSSL)
find_package(GTest CONFIG REQUIRED)

# Proto file for aggregator service
add_library(aggregator_protos
  src/protos/aggregator.grpc.pb.cc
  src/protos/aggregator.pb.cc
  src/protos/aggregator.grpc.pb.h
  src/protos/aggregator.pb.h
)
target_link_libraries(aggregator_protos
  absl::check
  gRPC::grpc++
  gRPC::grpc++_reflection
  protobuf::libprotobuf 
)

set (grpc_app_libs
  gRPC::grpc++
  gRPC::grpc++_reflection
  protobuf::libprotobuf
  absl::check
  absl::flags
  absl::flags_parse
  absl::log
  absl::log_initialize
)


add_executable(server 
  src/server/server.cc
  )
target_link_libraries(server 
  ${grpc_app_libs} 
  aggregator_protos 

  Poco::NetSSL
  Poco::JSON
  Poco::Net  
)


add_executable(client1
  src/client/client1.cc
)
target_link_libraries(client1 
  ${grpc_app_libs} 
  aggregator_protos
)

add_executable(client2
  src/client/client2.cc
)
target_link_libraries(client2
  ${grpc_app_libs} 
  aggregator_protos
)

add_executable(client3
  src/client/client3.cc
)
target_link_libraries(client3
  ${grpc_app_libs} 
  aggregator_protos
)

add_executable(test_orderbook src/tests/test_orderbook.cc)
target_link_libraries(test_orderbook PRIVATE 
  GTest::gtest 
  GTest::gtest_main
  aggregator_protos
  )
# Enable testing
enable_testing()
add_test(NAME orderbook_unit_test COMMAND test_orderbook)