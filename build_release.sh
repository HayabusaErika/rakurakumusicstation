#!/bin/bash

# =============================================================================
# Rakuraku Music Station - 构建脚本 v4.0
# =============================================================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' 

echo -e "${BLUE}====================================================${NC}"
echo -e "${BLUE}   Rakuraku Music Station - FFmpeg       ${NC}"
echo -e "${BLUE}====================================================${NC}\n"

# 1. 基础环境检查
echo -e "${YELLOW}[1/6] 检查基础编译环境...${NC}"
sudo apt-get update -y
sudo apt-get install -y build-essential libssl-dev locales strip wget curl xz-utils

# 2. FFmpeg
echo -e "\n${YELLOW}[2/6] 配置全量版 FFmpeg...${NC}"

# 函数：检查 FFmpeg 是否具备 MP3 和 AAC 编码能力
check_ffmpeg_full() {
    if command -v ffmpeg &> /dev/null; then
        local has_mp3=$(ffmpeg -encoders | grep libmp3lame || true)
        local has_aac=$(ffmpeg -encoders | grep libfdk_aac || ffmpeg -encoders | grep aac || true)
        if [[ -n "$has_mp3" && -n "$has_aac" ]]; then
            return 0 # 你通关！
        fi
    fi
    return 1 # 不过呐……
}

if check_ffmpeg_full; then
    echo -e "${GREEN}✓ 当前系统 FFmpeg 已具备编码能力${NC}"
else
    echo -e "${PURPLE}当前 FFmpeg 可能是精简版，正在尝试升级为标准版...${NC}"
    
    # 尝试 1: 安装扩展包
    sudo apt-get install -y ffmpeg libavcodec-extra || true
    
    # 尝试 2: 如果还是不行，下载官方静态 Full Build (John Van Sickle)
    if ! check_ffmpeg_full; then
        echo -e "${YELLOW}警告: 系统仓库版本不足。正在下载官方包 ...${NC}"
        ARCH=$(uname -m)
        if [ "$ARCH" == "x86_64" ]; then
            FFMPEG_URL="https://johnvansickle.com/ffmpeg/releases/ffmpeg-release-amd64-static.tar.xz"
        else
            FFMPEG_URL="https://johnvansickle.com/ffmpeg/releases/ffmpeg-release-arm64-static.tar.xz"
        fi
        
        wget -N $FFMPEG_URL
        mkdir -p ffmpeg_full
        tar -xJf ffmpeg-release-*-static.tar.xz -C ffmpeg_full --strip-components=1
        
        # 移动到 /usr/local/bin 或当前目录备用
        sudo cp ffmpeg_full/ffmpeg /usr/local/bin/ffmpeg
        sudo cp ffmpeg_full/ffprobe /usr/local/bin/ffprobe
        chmod +x /usr/local/bin/ffmpeg
        echo -e "${GREEN}✓ FFmpeg 已部署至 /usr/local/bin${NC}"
    fi
fi

# 3. 中文环境补全 (解决文件名解析 revents=16 问题)
echo -e "\n${YELLOW}[3/6] 配置中文路径支持...${NC}"
sudo locale-gen zh_CN.UTF-8 || true
export LANG=zh_CN.UTF-8

# 4. 编译
echo -e "\n${YELLOW}[4/6] 编译服务器核心...${NC}"
RELEASE_DIR="dist"
rm -rf $RELEASE_DIR && mkdir -p $RELEASE_DIR/media

g++ radioserver.cpp -o $RELEASE_DIR/radioserver \
    -std=c++17 -O3 -flto -lpthread -lssl -lcrypto -I. -w

if [ -f "$RELEASE_DIR/radioserver" ]; then
    strip $RELEASE_DIR/radioserver
    echo -e "${GREEN}✓ 编译成功${NC}"
else
    echo -e "${RED}✗ 编译失败${NC}"
    exit 1
fi

# 5. 生成生产环境管理脚本
echo -e "\n${YELLOW}[5/6] 写入管理脚本...${NC}"

cat <<'EOF' > $RELEASE_DIR/start.sh
#!/bin/bash
# 强制开启 UTF-8 环境
export LANG=zh_CN.UTF-8
export LC_ALL=zh_CN.UTF-8

# 确保 FFmpeg 路径正确（优先使用刚安装的全量版）
export PATH=/usr/local/bin:$PATH

mkdir -p media
echo "Rakuraku Music Station 正在启动..."
echo "检测 FFmpeg 版本:"
ffmpeg -version | head -n 1

nohup ./radioserver > server.log 2>&1 &
echo $! > .server.pid
echo "服务已运行 (PID: $(cat .server.pid))，Web: http://localhost:2240"
EOF

chmod +x $RELEASE_DIR/start.sh

# 6. 发布总结
echo -e "\n${BLUE}====================================================${NC}"
echo -e "${GREEN}        构建成功！FFmpeg 环境已就绪          ${NC}"
echo -e "${BLUE}====================================================${NC}"
echo -e "${YELLOW}当前 FFmpeg 路径:${NC} $(which ffmpeg)"
echo -e "${YELLOW}支持的 MP3 编码器:${NC} $(ffmpeg -encoders | grep libmp3lame)"
echo -e "${BLUE}====================================================${NC}\n"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ 核心程序编译成功${NC}"
else
    echo -e "${RED}✗ 编译失败，请检查 C++ 语法错误${NC}"
    exit 1
fi

# 4. 二进制精简
echo -e "\n${YELLOW}[4/6] 二进制体积精简...${NC}"
BEFORE_SIZE=$(ls -lh $RELEASE_DIR/radioserver | awk '{print $5}')
strip $RELEASE_DIR/radioserver
AFTER_SIZE=$(ls -lh $RELEASE_DIR/radioserver | awk '{print $5}')
echo -e "${GREEN}✓ 体积压缩完成: $BEFORE_SIZE -> $AFTER_SIZE${NC}"

# 5. 生成辅助脚本
echo -e "\n${YELLOW}[5/6] 生成管理脚本...${NC}"

# 生成启动脚本
cat <<EOF > $RELEASE_DIR/start.sh
#!/bin/bash
# 自动创建媒体目录
mkdir -p media
# 检查ffmpeg
if ! command -v ffmpeg &> /dev/null; then
    echo "错误: 运行环境缺少 ffmpeg，无法解码音乐。"
    exit 1
fi
echo "电台启动中... Web界面: http://localhost:2240"
nohup ./radioserver > server.log 2>&1 &
echo \$! > .server.pid
echo "服务已进入后台运行，PID: \$(cat .server.pid)"
EOF

# 生成停止脚本
cat <<EOF > $RELEASE_DIR/stop.sh
#!/bin/bash
if [ -f .server.pid ]; then
    PID=\$(cat .server.pid)
    kill \$PID && rm .server.pid
    echo "服务已停止 (PID: \$PID)"
else
    pkill radioserver
    echo "已尝试停止所有 radioserver 进程"
fi
EOF

chmod +x $RELEASE_DIR/*.sh
echo -e "${GREEN}✓ 管理脚本 (start.sh/stop.sh) 已生成${NC}"

# 6. 发布总结
echo -e "\n${BLUE}====================================================${NC}"
echo -e "${GREEN}        构建成功! 产物已存至: ./$RELEASE_DIR         ${NC}"
echo -e "${BLUE}====================================================${NC}"
echo -e "${YELLOW}使用说明:${NC}"
echo -e "1. 将音乐文件放入 ${RELEASE_DIR}/media 文件夹"
echo -e "2. 执行 ${RELEASE_DIR}/start.sh 启动服务"
echo -e "3. 访问 Web 页面: http://$(hostname -I | awk '{print $1}'):2240"
echo -e "${BLUE}====================================================${NC}\n"
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
