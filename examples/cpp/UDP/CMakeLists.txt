cmake_minimum_required(VERSION 3.10)
project(MyCrossCompiledProject)

# 启用CMake的CROSSCOMPILING模式
set(CMAKE_CROSSCOMPILING TRUE)

# 指定C和C++编译器
set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "aarch64-linux-gnu-g++")

# 设置目标平台的根文件系统路径
set(CMAKE_SYSROOT "/path/to/your/sysroot")

# 如果有工具链文件，指定其路径
# set(CMAKE_TOOLCHAIN_FILE "/path/to/your/toolchain.cmake")

# 其他设置，例如C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加你的源文件
add_executable(UDPtest main.cpp)

# 如果需要链接特定库，可以使用target_link_libraries
# target_link_libraries(my_executable some_library)