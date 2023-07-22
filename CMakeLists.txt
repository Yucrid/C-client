cmake_minimum_required(VERSION 3.25)
project(sql)

set(CMAKE_CXX_STANDARD 17)
include_directories(path/to/directory_containing_client.pb.h)
include_directories(/home/user/CLionProjects/sql)

add_executable(sql main.cpp client.pb.cc)

# 添加 brpc 头文件搜索路径
include_directories(/usr/local/include)

# 添加 brpc 库文件搜索路径
link_directories(/usr/local/lib)

# 添加 protobuf 和 protobuf-lite 库文件搜索路径
link_directories(/usr/lib/x86_64-linux-gnu)

# 添加 glog 库文件搜索路径
link_directories(/usr/local/lib)
set(MYSQL_CONNECTOR_DIR "/usr/lib/x86_64-linux-gnu")  # 这里根据实际情况设置路径

# 链接库到 sql 目标
target_link_libraries(sql PRIVATE brpc protobuf protobuf-lite glog ssl crypto gflags leveldb ${MYSQL_CONNECTOR_DIR}/libmysqlclient.so)


