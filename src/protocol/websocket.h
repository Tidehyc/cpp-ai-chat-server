#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace chat {

// WebSocket 操作码
enum class Opcode : uint8_t {
    CONTINUATION = 0x0,
    TEXT = 0x1,
    BINARY = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA
};

// WebSocket 帧
struct WebSocketFrame {
    Opcode opcode;
    bool fin;
    bool mask;
    uint64_t payload_len;
    std::vector<uint8_t> masking_key;
    std::vector<uint8_t> payload;
};

// WebSocket 解析结果
enum class WSResult {
    SUCCESS,
    INCOMPLETE,
    ERROR_FRAME,
    ERROR_CLOSE
};

class WebSocketParser {
public:
    // 解析 WebSocket 帧
    static WSResult parse(const std::vector<uint8_t>& data, WebSocketFrame& frame);
    
    // 编码 WebSocket 帧
    static std::vector<uint8_t> encode(const std::string& message, Opcode opcode = Opcode::TEXT);
    
    // 解码 WebSocket 帧
    static std::string decode(const WebSocketFrame& frame);
    
    // 生成 WebSocket 握手响应
    static std::string create_handshake_response(const std::string& key, const std::string& protocol = "");

    // 检查是否是 WebSocket 握手请求
    static bool is_handshake_request(const std::string& request);
    
    // 提取 WebSocket key
    static std::string extract_websocket_key(const std::string& request);
    
    // 创建关闭帧
    static std::vector<uint8_t> create_close_frame(uint16_t code = 1000);
    
    // 创建 Ping 帧
    static std::vector<uint8_t> create_ping_frame();
    
    // 创建 Pong 帧
    static std::vector<uint8_t> create_pong_frame();
};

} // namespace chat
