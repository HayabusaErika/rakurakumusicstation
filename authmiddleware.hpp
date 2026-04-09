// =============================================================================
// Web服务器管理器（更新版，整合认证系统）
// =============================================================================
class WebServer {
public:
    struct Config {
        bool allow_guest_skip = false;  // 是否允许游客切歌
        std::string admin_password = "";
        std::string station_name = "我的音乐电台";
        std::string subtitle = "极简流媒体服务器";
        std::string primary_color = "#764ba2";
        std::string secondary_color = "#667eea";
        std::string bg_color = "#f4f4f9";
        
        static Config load_from_settings() {
            Config config;
            std::ifstream conf_file("settings.json");
            if (conf_file.is_open()) {
                std::stringstream ss;
                ss << conf_file.rdbuf();
                auto j = crow::json::load(ss.str());
                if (j) {
                    if (j.has("allow_guest_skip")) 
                        config.allow_guest_skip = j["allow_guest_skip"].b();
                    if (j.has("admin_password")) 
                        config.admin_password = j["admin_password"].s();
                    if (j.has("station_name")) 
                        config.station_name = j["station_name"].s();
                    if (j.has("subtitle")) 
                        config.subtitle = j["subtitle"].s();
                    if (j.has("primary_color")) 
                        config.primary_color = j["primary_color"].s();
                    if (j.has("secondary_color")) 
                        config.secondary_color = j["secondary_color"].s();
                    if (j.has("bg_color")) 
                        config.bg_color = j["bg_color"].s();
                }
            }
            
            // 如果未设置管理员密码，使用默认密码
            if (config.admin_password.empty()) {
                config.admin_password = "admin123";
                std::cout << "[Web] 警告：使用默认管理员密码: admin123" << std::endl;
                std::cout << "[Web] 请在 settings.json 中设置 admin_password" << std::endl;
            }
            
            return config;
        }
    };

    WebServer(std::vector<std::string>* playlist, std::atomic<size_t>* current_track,
          StreamServer* stream_server, AudioPlayer* audio_player, std::mutex* playlist_mutex)
    : config_(Config::load_from_settings()),
      playlist_(playlist), current_track_(current_track),
      stream_server_(stream_server), audio_player_(audio_player),
      playlist_mutex_(playlist_mutex), running_(false) {
        // 初始化认证中间件
        auth_middleware_ = std::make_unique<AuthMiddleware>(config_.admin_password);
        
        // 设置Crow应用使用认证中间件
        app_.template use(middleware_);
        
        std::cout << "[Web] 加载配置文件成功" << std::endl;
        std::cout << "[Web] 管理员密码已配置" << std::endl;
        std::cout << "[Web] 允许游客切歌: " << (config_.allow_guest_skip ? "是" : "否") << std::endl;
    }
    
    ~WebServer() {
        stop();
    }
    
    bool start() {
        if (running_) return false;
        
        setup_routes();
        
        running_ = true;
        
        thread_ = std::thread([this]() {
            try {
                std::cout << "[Web] 服务器启动在端口 " << Config::WEB_PORT << std::endl;
                app_.port(Config::WEB_PORT).multithreaded().run();
            } catch (const std::exception& e) {
                std::cerr << "[Web] 错误: " << e.what() << std::endl;
            }
            running_ = false;
        });
        
        return true;
    }
    
    void stop() {
        if (!running_.exchange(false)) return;
        
        app_.stop();
        if (thread_.joinable()) thread_.join();
        
        std::cout << "[Web] 服务器已停止" << std::endl;
    }

private:
    // 辅助函数：替换字符串中所有的指定内容
    static void replace_all(std::string& str, const std::string& from, const std::string& to) {
        if(from.empty()) return;
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }
    
    // 读取HTML模板并替换变量
    std::string render_template(const std::string& filename, 
                       const std::map<std::string, std::string>& context = {},
                       bool is_admin = false) {
        std::string template_path;
        
        // 优先在当前目录查找模板
        template_path = filename;
        std::ifstream html_file(template_path);
        
        if (!html_file.is_open()) {
            // 如果在当前目录找不到，尝试在templates目录查找
            template_path = "templates/" + filename;
            html_file = std::ifstream(template_path);
        }
        
        if (!html_file.is_open()) {
            throw std::runtime_error("无法打开模板文件: " + filename + " 或 templates/" + filename);
        }
        
        std::stringstream ss;
        ss << html_file.rdbuf();
        std::string html_content = ss.str();
        
        // 基本配置替换
        replace_all(html_content, "{{STATION_NAME}}", config_.station_name);
        replace_all(html_content, "{{SUBTITLE}}", config_.subtitle);
        replace_all(html_content, "{{PRIMARY_COLOR}}", config_.primary_color);
        replace_all(html_content, "{{SECONDARY_COLOR}}", config_.secondary_color);
        replace_all(html_content, "{{BG_COLOR}}", config_.bg_color);
        
        // 权限相关的替换
        replace_all(html_content, "{{IS_ADMIN}}", is_admin ? "true" : "false");
        
        // 用户定义的上下文变量
        for (const auto& [key, value] : context) {
            replace_all(html_content, "{{" + key + "}}", value);
        }
        
        return html_content;
    }
    
    void setup_routes() {
        // 认证中间件上下文访问器
        auto get_auth_context = [this](const crow::request& req) -> AuthMiddleware::context& {
            return app_.get_context<AuthMiddleware>(req);
        };
        
        // 主页 - 根据是否登录显示不同界面
        CROW_ROUTE(app_, "/")([this](const crow::request& req) {
            auto& ctx = get_auth_context(req);
            
            try {
                if (ctx.is_admin) {
                    // 管理员显示管理面板
                    auto admin_context = std::map<std::string, std::string>{
                        {"CLIENT_COUNT", std::to_string(stream_server_->client_count())}
                    };
                    
                    // 获取播放列表信息
                    std::lock_guard<std::mutex> lock(*playlist_mutex_);
                    admin_context["TRACK_COUNT"] = std::to_string(playlist_->size());
                    admin_context["CURRENT_TRACK"] = std::to_string(current_track_->load() + 1);
                    
                    return crow::response(render_template("admin_panel.html", admin_context, true));
                } else {
                    // 普通用户显示收听界面
                    return crow::response(render_template("index.html", {}, false));
                }
            } catch (const std::exception& e) {
                // 如果模板不存在，返回错误
                return crow::response(500, std::string("模板错误: ") + e.what());
            }
        });
        
        // 管理员登录页面
        CROW_ROUTE(app_, "/admin")([this](const crow::request& req) {
            auto& ctx = get_auth_context(req);
            
            if (ctx.is_admin) {
                // 如果已经登录，重定向到管理面板
                crow::response res(302);
                res.set_header("Location", "/");
                return res;
            }
            
            try {
                return crow::response(render_template("admin_login.html", {}, false));
            } catch (const std::exception& e) {
                // 如果模板不存在，返回简单登录页面
                std::string simple_login = R"(
<!DOCTYPE html>
<html>
<head><title>管理员登录</title><style>body{font-family:Arial;text-align:center;padding:50px}</style></head>
<body>
    <h1>管理员登录</h1>
    <div style="max-width:300px;margin:20px auto;">
        <input type="password" id="password" placeholder="密码" style="padding:10px;width:100%;margin:10px 0;">
        <button onclick="login()" style="padding:10px 20px;background:#764ba2;color:white;border:none;cursor:pointer;width:100%;">登录</button>
    </div>
    <script>
    async function login() {
        const password = document.getElementById('password').value;
        const response = await fetch('/admin/login', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({password: password})
        });
        if (response.ok) window.location.href = '/';
        else alert('密码错误');
    }
    </script>
</body>
</html>
                )";
                replace_all(simple_login, "{{STATION_NAME}}", config_.station_name);
                return crow::response(simple_login);
            }
        });
        
        // 登录API
        CROW_ROUTE(app_, "/admin/login").methods("POST"_method)([this](const crow::request& req) {
            try {
                auto j = crow::json::load(req.body);
                if (!j || !j.has("password")) {
                    return crow::response(400, "缺少密码参数");
                }
                
                std::string password = j["password"].s();
                if (auth_middleware_->verify_password(password)) {
                    auto session = auth_middleware_->create_admin_session();
                    crow::response res(200);
                    res.set_header("Set-Cookie", 
                        "session_id=" + session->session_id + 
                        "; Path=/; HttpOnly; Max-Age=86400; SameSite=Lax");
                    res.write("登录成功");
                    return res;
                }
                return crow::response(401, "密码错误");
            } catch (const std::exception& e) {
                return crow::response(400, std::string("请求格式错误: ") + e.what());
            }
        });
        
        // 登出API
        CROW_ROUTE(app_, "/admin/logout").methods("POST"_method)([this](const crow::request& req) {
            // 从Cookie中获取session_id
            std::string session_id = auth_middleware_->get_session_id_from_cookies(req.get_header_value("Cookie"));
            auth_middleware_->destroy_session(session_id);
            
            crow::response res(200);
            res.set_header("Set-Cookie", 
                "session_id=; Path=/; HttpOnly; Max-Age=0; SameSite=Lax");
            res.write("登出成功");
            return res;
        });
        
        // 公开API：播放列表信息
        CROW_ROUTE(app_, "/api/playlist")([this]() {
            std::lock_guard<std::mutex> lock(*playlist_mutex_);
            crow::json::wvalue result;
            result["playlist"] = *playlist_;
            result["current"] = (int)current_track_->load();
            return crow::response(result);
        });
        
        // 公开API：统计信息
        CROW_ROUTE(app_, "/api/stats")([this]() {
            crow::json::wvalue result;
            result["clients"] = (int)stream_server_->client_count();
            return crow::response(result);
        });
        
        // 需要权限的API：上传文件（仅管理员）
        CROW_ROUTE(app_, "/upload").methods("POST"_method)([this](const crow::request& req) {
            auto& ctx = get_auth_context(req);
            if (!ctx.is_admin) return crow::response(403, "需要管理员权限");
            
            // 检查上传大小
            size_t content_length = req.content_length;
            if (content_length > Config::MAX_UPLOAD_SIZE) {
                return crow::response(413, "文件太大，最大50MB");
            }
            
            return handle_upload(req);
        });
        
        // 需要权限的API：下一首（如果允许游客切歌，则游客也可以使用）
        CROW_ROUTE(app_, "/api/next").methods("POST"_method)([this](const crow::request& req) {
            auto& ctx = get_auth_context(req);
            if (!ctx.is_admin && !config_.allow_guest_skip) {
                return crow::response(403, "需要登录才能执行此操作");
            }
            
            // 权限检查通过，执行切歌
            std::lock_guard<std::mutex> lock(*playlist_mutex_);
            if (playlist_->empty()) return crow::response{400, "播放列表为空"};
            
            size_t new_index = (current_track_->load() + 1) % playlist_->size();
            current_track_->store(new_index);
            audio_player_->load_file((*playlist_)[new_index]);
            
            return crow::response{200, "跳到下一首"};
        });
        
        // 需要权限的API：上一首（如果允许游客切歌，则游客也可以使用）
        CROW_ROUTE(app_, "/api/prev").methods("POST"_method)([this](const crow::request& req) {
            auto& ctx = get_auth_context(req);
            if (!ctx.is_admin && !config_.allow_guest_skip) {
                return crow::response(403, "需要登录才能执行此操作");
            }
            
            // 权限检查通过，执行切歌
            std::lock_guard<std::mutex> lock(*playlist_mutex_);
            if (playlist_->empty()) return crow::response{400, "播放列表为空"};
            
            size_t size = playlist_->size();
            size_t new_index = (current_track_->load() + size - 1) % size;
            current_track_->store(new_index);
            audio_player_->load_file((*playlist_)[new_index]);
            
            return crow::response{200, "跳到上一首"};
        });
        
        // 需要权限的API：播放指定歌曲（如果允许游客切歌，则游客也可以使用）
        CROW_ROUTE(app_, "/api/play/<int>").methods("POST"_method)([this](int idx, const crow::request& req) {
            auto& ctx = get_auth_context(req);
            if (!ctx.is_admin && !config_.allow_guest_skip) {
                return crow::response(403, "需要登录才能执行此操作");
            }
            
            // 权限检查通过，执行切歌
            std::lock_guard<std::mutex> lock(*playlist_mutex_);
            if (playlist_->empty()) return crow::response{400, "播放列表为空"};
            
            size_t index = static_cast<size_t>(idx);
            if (index >= playlist_->size()) return crow::response{400, "索引超出范围"};
            
            current_track_->store(index);
            audio_player_->load_file((*playlist_)[index]);
            
            return crow::response{200, "播放歌曲: " + std::to_string(index)};
        });
        
        // 需要权限的API：删除歌曲（仅管理员）
        CROW_ROUTE(app_, "/api/delete/<int>").methods("POST"_method)([this](int idx, const crow::request& req) {
            auto& ctx = get_auth_context(req);
            if (!ctx.is_admin) return crow::response(403, "需要管理员权限");
            
            std::lock_guard<std::mutex> lock(*playlist_mutex_);
            if (playlist_->empty()) return crow::response{400, "播放列表为空"};
            
            size_t index = static_cast<size_t>(idx);
            if (index >= playlist_->size()) return crow::response{400, "索引超出范围"};
            
            playlist_->erase(playlist_->begin() + index);
            
            // 调整当前播放索引
            size_t current = current_track_->load();
            if (current >= playlist_->size()) {
                current_track_->store(0);
                if (!playlist_->empty()) {
                    audio_player_->load_file((*playlist_)[0]);
                }
            }
            
            return crow::response{200, "删除成功"};
        });
    }
    
    // 文件上传处理函数（从原始代码中保留）
    crow::response handle_upload(const crow::request& req) {
        auto boundary_info = req.headers.find("Content-Type");
        if (boundary_info == req.headers.end()) return crow::response(400, "缺少Content-Type");
        
        std::string content_type = boundary_info->second;
        auto boundary_pos = content_type.find("boundary=");
        if (boundary_pos == std::string::npos) return crow::response(400, "无效的Content-Type");
        std::string boundary = content_type.substr(boundary_pos + 9);
        
        std::istringstream body_stream(req.body);
        std::string line;
        size_t total_read = 0;
        
        while (std::getline(body_stream, line) && total_read < req.content_length) {
            total_read += line.length() + 1;
            if (line.find("Content-Disposition: form-data;") != std::string::npos &&
                line.find("name=\"file\"") != std::string::npos) {
                // 跳过两行空白行
                std::getline(body_stream, line); total_read += line.length() + 1; // 跳过空行
                std::getline(body_stream, line); total_read += line.length() + 1;
                
                // 读取文件名
                auto filename_start = line.find("filename=\"");
                if (filename_start == std::string::npos) continue;
                filename_start += 10;
                auto filename_end = line.find("\"", filename_start);
                std::string filename = line.substr(filename_start, filename_end - filename_start);
                
                // 读取文件数据
                std::vector<char> file_data;
                while (std::getline(body_stream, line) && line != "--" + boundary + "--") {
                    total_read += line.length() + 1;
                    if (line.find("--" + boundary) == 0) break;
                    
                    line += "\n"; // 恢复被getline移除的换行符
                    file_data.insert(file_data.end(), line.begin(), line.end());
                }
                
                // 移除末尾多余的换行符
                while (!file_data.empty() && (file_data.back() == '\n' || file_data.back() == '\r')) {
                    file_data.pop_back();
                }
                
                // 保存文件
                if (file_data.empty()) {
                    return crow::response(400, "上传的文件为空");
                }
                
                // 检查扩展名
                bool supported = false;
                for (const auto& ext : Config::SUPPORTED_FORMATS) {
                    if (filename.size() >= ext.size() && 
                        filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0) {
                        supported = true;
                        break;
                    }
                }
                
                if (!supported) {
                    std::string supported_formats;
                    for (const auto& ext : Config::SUPPORTED_FORMATS) supported_formats += ext + " ";
                    return crow::response(400, "不支持的文件格式，支持: " + supported_formats);
                }
                
                std::ofstream out_file(filename, std::ios::binary);
                if (!out_file) return crow::response(500, "无法创建文件");
                out_file.write(file_data.data(), file_data.size());
                out_file.close();
                
                // 添加到播放列表
                std::lock_guard<std::mutex> lock(*playlist_mutex_);
                playlist_->push_back(filename);
                
                // 如果是第一首歌，开始播放
                if (playlist_->size() == 1) {
                    current_track_->store(0);
                    audio_player_->load_file(filename);
                }
                
                return crow::response(200, "上传成功: " + filename);
            }
        }
        
        return crow::response(400, "未找到文件数据");
    }

private:
    Config config_;
    std::unique_ptr<AuthMiddleware> auth_middleware_;
    crow::App<AuthMiddleware> app_;
    std::thread thread_;
    std::atomic<bool> running_;
    
    // 其他成员变量保持不变
    std::vector<std::string>* playlist_;
    std::atomic<size_t>* current_track_;
    StreamServer* stream_server_;
    AudioPlayer* audio_player_;
    std::mutex* playlist_mutex_;
};
