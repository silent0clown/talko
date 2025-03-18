#!/bin/bash

# 默认构建类型为 Release
BUILD_TYPE="Release"
BUILD_TESTS=false

# 解析命令行参数
for arg in "$@"; do
    case $arg in
        -test)
            BUILD_TESTS=true
            ;;
        -Debug)
            BUILD_TYPE="Debug"
            ;;
        -Release)
            BUILD_TYPE="Release"
            ;;
        *)
            echo "未知参数: $arg"
            exit 1
            ;;
    esac
done

# 创建构建目录
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
else
    # 清空构建目录
    rm -rf "$BUILD_DIR"/*
    rm -rf "$BUILD_DIR"/.* 2>/dev/null
fi

# 进入构建目录
cd "$BUILD_DIR" || exit 1

# 运行 CMake 配置
if [ "$BUILD_TESTS" = true ]; then
    echo "构建类型: $BUILD_TYPE (包含测试)"
    cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_TESTS=ON
else
    echo "构建类型: $BUILD_TYPE"
    cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_TESTS=OFF
fi

# 编译项目
make -j$(nproc)

# 执行安装（如果有安装规则）
make install

# 返回项目根目录
cd ..

# 清空构建目录
rm -rf "$BUILD_DIR"/*
rm -rf "$BUILD_DIR"/.* 2>/dev/null
rmdir -p "$BUILD_DIR"

echo "构建完成！"