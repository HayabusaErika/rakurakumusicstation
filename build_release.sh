#!/bin/bash

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Rakuraku Music Station Release Build  ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}\n"

# 1. 检查依赖
echo -e "${YELLOW}[1/6] 检查编译环境...${NC}"

if ! command -v g++ &> /dev/null; then
    echo -e "${RED}✗ g++ 未安装${NC}"
    echo "执行: sudo apt install build-essential"
    exit 1
fi
echo -e "${GREEN}✓ g++ 已安装${NC}"

if ! command -v ffmpeg &> /dev/null; then
    echo -e "${RED}✗ ffmpeg 未安装${NC}"
    echo "执行: sudo apt install ffmpeg"
    exit 1
fi
echo -e "${GREEN}✓ ffmpeg 已安装${NC}"

# 检查必要的库
if ! pkg-config --exists openssl 2>/dev/null; then
    echo -e "${RED}✗ OpenSSL 开发库未安装${NC}"
    echo "执行: sudo apt install libssl-dev"
    exit 1
fi
echo -e "${GREEN}✓ OpenSSL 已安装${NC}"

# 2. 清理旧构建
echo -e "\n${YELLOW}[2/6] 清理旧构建文件...${NC}"
rm -rf build release radioserver radioserver_release
echo -e "${GREEN}✓ 清理完成${NC}"

# 3. 创建编译目录
echo -e "\n${YELLOW}[3/6] 创建编译目录...${NC}"
mkdir -p release
cd release
echo -e "${GREEN}✓ 目录已创建${NC}"

# 4. 编译
echo -e "\n${YELLOW}[4/6] 编译源代码（Release 模式）...${NC}"
echo "编译命令:"
echo "g++ ../radioserver.cpp -o radioserver_release \\"
echo "    -std=c++17 \\"
echo "    -lpthread -lssl -lcrypto \\"
echo "    -O3 -march=native -flto \\"
echo "    -I.. -w"
echo ""

g++ ../radioserver.cpp -o radioserver_release \
    -std=c++17 \
    -lpthread -lssl -lcrypto \
    -O3 -march=native -flto \
    -I.. -w

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ 编译失败${NC}"
    exit 1
fi
echo -e "${GREEN}✓ 编译成功${NC}"

# 5. 检查文件
echo -e "\n${YELLOW}[5/6] 验证编译产物...${NC}"
if [ -f "radioserver_release" ]; then
    SIZE=$(ls -lh radioserver_release | awk '{print $5}')
    echo -e "${GREEN}✓ 可执行文件: radioserver_release (${SIZE})${NC}"
    
    # 显示文件信息
    file radioserver_release
    
    # 显示依赖的动态库
    echo -e "\n依赖的动态库:"
    ldd radioserver_release | grep -E "libc|libpthread|libssl"
else
    echo -e "${RED}✗ 编译失败，未生成可执行文件${NC}"
    exit 1
fi

# 6. 创建媒体目录
echo -e "\n${YELLOW}[6/6] 设置运行环境...${NC}"
mkdir -p ../media
echo -e "${GREEN}✓ 媒体目录已创建: ../media${NC}"

# 完成
echo -e "\n${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${GREEN}✓ Release 构建完成！${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}\n"

echo -e "${YELLOW}运行服务器:${NC}"
echo "  cd release"
echo "  ./radioserver_release"
echo ""
echo -e "${YELLOW}Web 界面:${NC}"
echo "  http://localhost:2240"
echo ""
echo -e "${YELLOW}音频流:${NC}"
echo "  http://localhost:2241"
echo ""
