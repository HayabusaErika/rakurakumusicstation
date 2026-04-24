#!/bin/bash

# 设置中文环境支持
if locale -a | grep -qi "zh_CN.utf8"; then
    export LANG=zh_CN.UTF-8
    export LC_ALL=zh_CN.UTF-8
fi

# 确保媒体目录存在
mkdir -p media

echo "🎵 Rakuraku Music Station 启动中..."
echo "========================================"
echo "Web 界面: http://localhost:2240"
echo "流媒体: http://localhost:2241"
echo "========================================"
echo "音乐文件请放置在 media/ 目录"
echo ""

# 后台运行服务器
nohup ./radioserver > server.log 2>&1 &
echo $! > .server.pid

echo "✅ 服务器已启动 (PID: $(cat .server.pid))"
echo "📄 查看日志: tail -f server.log"
echo "🛑 停止服务: ./stop.sh"
