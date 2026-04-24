#!/bin/bash

echo "🎵 Rakuraku Music Station 停止脚本"
echo "========================================"

# 首先尝试使用PID文件
if [ -f .server.pid ]; then
    PID=$(cat .server.pid)
    if ps -p $PID > /dev/null 2>&1; then
        echo "🔴 正在停止服务器 (PID: $PID)..."

        # 先发送SIGTERM，优雅关闭
        kill $PID

        # 等待进程退出，最多等待10秒
        for i in {1..10}; do
            if ! ps -p $PID > /dev/null 2>&1; then
                break
            fi
            echo "⏳ 等待进程中... ($i/10)"
            sleep 1
        done

        if ! ps -p $PID > /dev/null 2>&1; then
            rm .server.pid
            echo "✅ 服务器已正常停止 (PID: $PID)"
        else
            echo "⚠️ 进程仍在运行，强制终止..."
            kill -9 $PID
            sleep 1
            rm .server.pid
            echo "✅ 服务器已强制停止 (PID: $PID)"
        fi
    else
        rm .server.pid
        echo "⚠️  PID 文件存在但进程 $PID 已终止，已清理PID文件"
    fi
else
    echo "ℹ️  未找到 .server.pid 文件"
fi

# 二次清理：确保没有残留的radioserver进程
RADIO_PROCESSES=$(pgrep radioserver 2>/dev/null)
if [ -n "$RADIO_PROCESSES" ]; then
    echo "🔄 发现残留的 radioserver 进程，正在清理..."
    echo "📋 进程ID: $RADIO_PROCESSES"

    # 先尝试优雅终止
    pkill radioserver 2>/dev/null
    sleep 2

    # 再次检查并强制终止任何残留进程
    REMAINING=$(pgrep radioserver 2>/dev/null)
    if [ -n "$REMAINING" ]; then
        echo "⚠️  仍有进程残留，强制清理中..."
        kill -9 $REMAINING 2>/dev/null
        sleep 1
    fi

    echo "✅ 已清理所有 radioserver 进程"
else
    echo "ℹ️  没有发现残留的 radioserver 进程"
fi

echo "========================================"
echo "🛑 停止脚本执行完成"
