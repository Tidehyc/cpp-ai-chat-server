#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <csignal>
#include <cstdio>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;

// 全局标志
static std::atomic<bool> g_running(true);

// 信号处理函数
void signal_handler(int signum) {
    // 关闭 stdin 来中断 getline
    std::fclose(stdin);
    g_running = false;
}

// 简单的 HTTP/WebSocket 客户端
class WSClient {
public:
    WSClient(boost::asio::io_context& io_context, const std::string& host, const std::string& port)
        : socket_(io_context), host_(host), port_(port) {}

    bool connect() {
        try {
            tcp::resolver resolver(socket_.get_executor());
            auto endpoints = resolver.resolve(host_, port_);
            boost::asio::connect(socket_, endpoints);
            std::cout << "✅ 已连接到 " << host_ << ":" << port_ << std::endl;
            return true;
        } catch (std::exception& e) {
            std::cerr << "❌ 连接失败：" << e.what() << std::endl;
            return false;
        }
    }

    bool send_handshake() {
        std::string key = "dGhlIHNhbXBsZSBub25jZQ==";
        std::string request =
            "GET / HTTP/1.1\r\n"
            "Host: " + host_ + ":" + port_ + "\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: " + key + "\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "Sec-WebSocket-Protocol: chat\r\n"
            "\r\n";

        boost::asio::write(socket_, boost::asio::buffer(request));

        // 读取响应
        char buffer[1024];
        size_t n = socket_.read_some(boost::asio::buffer(buffer));
        std::string response(buffer, n);

        if (response.find("101 Switching Protocols") != std::string::npos) {
            std::cout << "✅ WebSocket 握手成功" << std::endl;
            return true;
        } else {
            std::cerr << "❌ 握手失败" << std::endl;
            std::cerr << response << std::endl;
            return false;
        }
    }

    void send_text_message(const std::string& message) {
        std::vector<uint8_t> frame;
        
        // FIN + TEXT opcode
        frame.push_back(0x81);
        
        // 掩码 + 长度
        size_t len = message.size();
        if (len < 126) {
            frame.push_back(0x80 | len);  // mask = 1
        } else if (len < 65536) {
            frame.push_back(0x80 | 126);
            frame.push_back((len >> 8) & 0xFF);
            frame.push_back(len & 0xFF);
        } else {
            frame.push_back(0x80 | 127);
            for (int i = 7; i >= 0; --i) {
                frame.push_back((len >> (i * 8)) & 0xFF);
            }
        }

        // 生成随机掩码
        std::vector<uint8_t> mask_key = {0x12, 0x34, 0x56, 0x78};
        frame.insert(frame.end(), mask_key.begin(), mask_key.end());

        // 掩码后的数据
        for (size_t i = 0; i < len; ++i) {
            frame.push_back(message[i] ^ mask_key[i % 4]);
        }

        boost::asio::write(socket_, boost::asio::buffer(frame));
    }

    std::string read_message() {
        char buffer[4096];
        size_t n = socket_.read_some(boost::asio::buffer(buffer));
        
        // 解析 WebSocket 帧
        if (n < 2) return "";
        
        uint8_t* data = reinterpret_cast<uint8_t*>(buffer);
        bool masked = (data[1] & 0x80) != 0;
        size_t payload_len = data[1] & 0x7F;
        size_t header_len = 2;

        if (payload_len == 126) {
            payload_len = (data[2] << 8) | data[3];
            header_len = 4;
        } else if (payload_len == 127) {
            payload_len = 0;
            for (int i = 0; i < 8; ++i) {
                payload_len = (payload_len << 8) | data[2 + i];
            }
            header_len = 10;
        }

        std::string result;
        if (masked) {
            std::vector<uint8_t> mask_key(data + header_len, data + header_len + 4);
            header_len += 4;
            
            for (size_t i = 0; i < payload_len; ++i) {
                result += data[header_len + i] ^ mask_key[i % 4];
            }
        } else {
            result = std::string(reinterpret_cast<char*>(data) + header_len, payload_len);
        }
        
        // 尝试解析 JSON 并提取 content 字段（简化处理）
        if (!result.empty() && result[0] == '{') {
            size_t pos = result.find("\"content\"");
            if (pos != std::string::npos) {
                pos = result.find(':', pos);
                if (pos != std::string::npos) {
                    pos = result.find('"', pos + 1);
                    if (pos != std::string::npos) {
                        size_t end = result.find('"', pos + 1);
                        while (end != std::string::npos && result[end-1] == '\\') {
                            end = result.find('"', end + 1);
                        }
                        if (end != std::string::npos) {
                            std::string content = result.substr(pos + 1, end - pos - 1);
                            // 替换转义字符
                            size_t p = 0;
                            while ((p = content.find("\\n", p)) != std::string::npos) {
                                content.replace(p, 2, "\n");
                                p++;
                            }
                            return content;
                        }
                    }
                }
            }
        }
        
        return result;
    }

    void close() {
        std::vector<uint8_t> close_frame = {0x88, 0x82, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE8};
        boost::asio::write(socket_, boost::asio::buffer(close_frame));
        socket_.close();
        std::cout << "✅ 连接已关闭" << std::endl;
    }

private:
    tcp::socket socket_;
    std::string host_;
    std::string port_;
};

int main() {
    try {
        // 注册信号处理
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        boost::asio::io_context io_context;
        WSClient client(io_context, "localhost", "8080");

        std::cout << "========================================\n";
        std::cout << "  C++ AI Chat Server - 测试客户端\n";
        std::cout << "========================================\n\n";

        // 连接
        if (!client.connect()) {
            std::cerr << "\n无法连接到服务器，请确保服务器正在运行：\n";
            std::cerr << "  cd build && ./chat_server\n";
            return 1;
        }

        // WebSocket 握手
        if (!client.send_handshake()) {
            return 1;
        }

        std::cout << "\n提示：输入命令后按回车发送，输入 /quit 退出\n\n";

        // 读取消息的线程
        std::thread read_thread([&]() {
            while (g_running) {
                try {
                    std::string msg = client.read_message();
                    if (!msg.empty()) {
                        std::cout << "\n[收到] " << msg << "\n> ";
                        std::cout.flush();
                    }
                } catch (...) {
                    break;
                }
            }
        });

        // 主循环
        std::string line;
        while (g_running) {
            std::cout << "> ";
            std::getline(std::cin, line);
            
            // 检查退出标志或输入失败（stdin 被关闭）
            if (!g_running || std::cin.fail()) {
                break;
            }
            
            if (line == "quit" || line == "/quit" || line == "exit") {
                // 发送退出命令到服务器
                client.send_text_message("/quit");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                break;
            }
            
            if (!line.empty()) {
                // 直接发送纯文本
                client.send_text_message(line);
            }
        }
        
        std::cout << "\n[系统] 正在退出..." << std::endl;

        client.close();
        read_thread.join();

        std::cout << "\n✅ 再见！\n";

    } catch (std::exception& e) {
        std::cerr << "错误：" << e.what() << std::endl;
        return 1;
    }

    return 0;
}
