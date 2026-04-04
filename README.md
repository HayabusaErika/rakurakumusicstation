# Rakuraku Music Station 🎵

一个高性能、轻量级的 C++ 流媒体广播服务器，基于 Apache License 2.0 协议开源。专为音乐发烧友和广播爱好者设计，将您的音乐库转换为专业的在线电台。

## ✨ 核心特性

### 🚀 **高性能架构**
*   **双引擎设计**：epoll 驱动的高并发流媒体引擎 + Crow 框架的现代化 Web 管理界面
*   **极低资源占用**：单实例支持数千并发连接，CPU 占用率极低
*   **智能缓存**：环形缓冲区平衡解码与传输速率，避免卡顿

### 🔧 **智能环境适配**
*   **跨平台支持**：自动适配 Arch Linux、Debian/Ubuntu（含 WSL）
*   **自动修复**：自动检测并配置中文语言环境，彻底解决特殊字符文件名导致的 FFmpeg 崩溃问题
*   **一键部署**：全自动构建脚本，无需手动安装依赖

### 🎛️ **专业功能**
*   **实时流媒体**：支持 MP3、FLAC、WAV、M4A 等主流音频格式
*   **远程管理**：Web 界面实时监控、文件上传、播放控制
*   **自动转码**：智能音频格式转换，确保客户端兼容性

## 🛠️ 技术栈

| 组件 | 技术 | 用途 |
|------|------|------|
| **核心引擎** | C++17, epoll, pthreads | 高并发流媒体分发 |
| **Web 框架** | Crow C++ 微框架 | RESTful API 和 Web 界面 |
| **音频处理** | FFmpeg, libmp3lame | 音频解码和转码 |
| **网络通信** | libcrypto, TCP/IP | 安全连接和流传输 |
| **构建系统** | CMake, bash | 跨平台编译和部署 |

## 🚀 快速开始

### 第 1 步：获取代码
```bash
git clone https://github.com/yourusername/rakurakumusicstation.git
cd rakurakumusicstation
```

### 第 2 步：一键构建
```bash
# 赋予执行权限
chmod +x build.sh

# 全自动构建（适配 Arch/Ubuntu）
./build.sh
```

**构建脚本自动完成以下操作：**
- ✅ 检测并安装系统依赖（g++、CMake、OpenSSL、Boost、FFmpeg）
- ✅ 配置中文语言环境（解决特殊字符问题）
- ✅ 启用最高级别优化（-O3、-flto、-march=native）
- ✅ 编译并打包到 `dist/` 目录

### 第 3 步：部署音乐
```bash
cd dist

# 将音乐文件复制到 media 目录（支持所有 FFmpeg 格式）
cp ~/Music/*.mp3 ./media/
# 或使用软链接
ln -s ~/Music ./media
```

### 第 4 步：启动服务器
```bash
# 前台运行（调试模式）
./radioserver

# 或后台运行
nohup ./radioserver > server.log 2>&1 &
```

## 🌐 访问服务

| 服务 | 地址 | 用途 |
|------|------|------|
| **Web 管理界面** | http://localhost:2240 | 文件管理、播放控制、系统监控 |
| **音频流** | http://localhost:2241/stream | 流媒体播放（VLC、浏览器等） |

## 📱 客户端连接

### 桌面客户端
```bash
# VLC
vlc http://localhost:2241/stream

# MPV
mpv http://localhost:2241/stream

# 浏览器
# 直接打开 http://localhost:2241/stream
```

### 移动设备
- iOS：在 Safari 中打开流媒体链接
- Android：使用 VLC for Android
- 通用：支持所有标准 HTTP 流媒体客户端

## ⚙️ 配置说明

### 配置文件 `settings.json`
```json
{
  "stream_port": 2241,
  "web_port": 2240,
  "media_directory": "./media",
  "buffer_size": 1048576,
  "max_clients": 1000,
  "audio_format": "mp3",
  "bitrate": "128k"
}
```

### 环境变量
```bash
# 强制 UTF-8 环境（解决中文文件名问题）
export LC_ALL=zh_CN.UTF-8
export LANG=zh_CN.UTF-8

# 自定义媒体目录
export RAKURAKU_MEDIA_DIR=/path/to/music
```

## 🔧 高级管理

### 进程管理
```bash
# 查看服务器状态
ps aux | grep radioserver

# 查看连接数
sudo lsof -i :2241 | grep ESTABLISHED

# 监控资源使用
top -p $(pgrep radioserver)
```

### 日志分析
```bash
# 实时查看日志
tail -f dist/server.log

# 查看错误信息
grep -i "error\|warn\|fail" dist/server.log

# 监控流媒体状态
grep -i "playing\|buffer\|client" dist/server.log
```

## 🐛 故障排除

### 常见问题

#### 1. **端口占用错误**
```bash
# 检查端口占用
sudo lsof -i :2241

# 释放端口
sudo kill $(sudo lsof -t -i:2241)
sudo kill $(sudo lsof -t -i:2240)
```

#### 2. **多个服务器实例**
```bash
# 停止所有实例
sudo pkill radioserver
sudo pkill ffmpeg

# 确认清理
ps aux | grep -E "(radio|ffmpeg)" | grep -v grep
```

#### 3. **音频无法播放**
```bash
# 测试本地文件
ffplay ./media/your_song.mp3

# 检查文件权限
ls -la ./media/

# 测试流媒体
curl -v http://localhost:2241/stream
```

#### 4. **中文文件名乱码**
```bash
# 设置正确语言环境
export LC_ALL=zh_CN.UTF-8
export LANG=zh_CN.UTF-8

# 重启服务器
./radioserver
```

### 调试模式
```bash
# 启用详细日志
./radioserver --verbose 2>&1 | tee debug.log

# 测试特定功能
curl -I http://localhost:2241/stream
mpv --no-video --msg-level=all=info http://localhost:2241/stream
```

## 🏗️ 项目结构
```
rakurakumusicstation/
├── src/                    # C++ 源代码
│   ├── StreamServer.cpp   # 流媒体服务器核心
│   ├── AudioPlayer.cpp    # 音频播放器
│   ├── RingBuffer.cpp     # 环形缓冲区
│   └── WebServer.cpp      # Web 管理界面
├── include/               # 头文件
├── media/                 # 音乐文件目录
├── dist/                  # 编译输出目录
│   ├── radioserver        # 主程序
│   ├── server.log         # 运行日志
│   └── media/             # 媒体文件（自动同步）
├── build.sh              # 自动构建脚本
├── settings.json         # 配置文件
└── README.md             # 本文档
```

## 📡 网络架构
```
客户端 (浏览器/VLC) → HTTP 请求 → 流媒体服务器 (2241端口)
                     ↳ Web 管理 → Web 服务器 (2240端口)
                     
音频文件 → FFmpeg 解码 → 环形缓冲区 → epoll 分发 → 所有客户端
```

## 🔒 安全建议

1. **防火墙配置**
   ```bash
   # 只允许特定 IP 访问
   sudo ufw allow from 192.168.1.0/24 to any port 2240,2241
   ```

2. **生产环境部署**
   ```bash
   # 使用 systemd 管理
   sudo cp systemd/radioserver.service /etc/systemd/system/
   sudo systemctl enable radioserver
   sudo systemctl start radioserver
   ```

3. **反向代理（推荐）**
   ```nginx
   # Nginx 配置示例
   location /stream {
       proxy_pass http://localhost:2241;
       proxy_buffering off;
   }
   
   location / {
       proxy_pass http://localhost:2240;
   }
   ```

## 📊 性能监控

### 基础指标
```bash
# 连接数统计
netstat -an | grep :2241 | wc -l

# 带宽使用
iftop -i eth0 -f "port 2241"

# CPU/内存占用
htop -p $(pgrep radioserver)
```

### Prometheus 监控
```yaml
# 暴露 metrics 端点（需实现）
- targets: ['localhost:2240']
  metrics_path: '/metrics'
```

## 🔄 维护和更新

### 日常维护
```bash
# 定期清理日志
find dist/ -name "*.log" -mtime +7 -delete

# 更新音乐库
rsync -av --delete ~/Music/ dist/media/
```

### 版本升级
```bash
# 拉取最新代码
git pull origin main

# 重新构建
./build.sh

# 平滑重启
sudo systemctl restart radioserver
```

## 📜 许可证

本项目采用 Apache License 2.0 协议开源。详见 [LICENSE](LICENSE) 文件。

```
Copyright 2026 Rakuraku Music Station Contributors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

## 🙏 鸣谢

特别感谢 **Cynun** 提供的技术支持和宝贵建议，让这个项目变得更好。

---

**🎵 让音乐流动起来！**

遇到问题？请查阅 [故障排除](#故障排除) 部分或提交 Issue。