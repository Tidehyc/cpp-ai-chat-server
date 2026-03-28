#include "websocket.h"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <cstring>
#include <algorithm>
#include <iostream>

namespace chat {

// WebSocket GUID (用于握手)
static const std::string WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// 基础 64 编码表
static const char BASE64_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64_encode(const uint8_t* data, size_t len) {
    std::string ret;
    ret.reserve(((len + 2) / 3) * 4);
    
    for (size_t i = 0; i < len; i += 3) {
        uint32_t n = (static_cast<uint32_t>(data[i]) << 16);
        if (i + 1 < len) n |= (static_cast<uint32_t>(data[i + 1]) << 8);
        if (i + 2 < len) n |= static_cast<uint32_t>(data[i + 2]);
        
        ret.push_back(BASE64_CHARS[(n >> 18) & 0x3F]);
        ret.push_back(BASE64_CHARS[(n >> 12) & 0x3F]);
        ret.push_back(BASE64_CHARS[(n >> 6) & 0x3F]);
        ret.push_back(BASE64_CHARS[n & 0x3F]);
    }
    
    // 处理 padding
    if (len % 3 == 1) {
        ret[ret.size() - 2] = '=';
        ret[ret.size() - 1] = '=';
    } else if (len % 3 == 2) {
        ret[ret.size() - 1] = '=';
    }
    
    return ret;
}

bool WebSocketParser::is_handshake_request(const std::string& request) {
    return request.find("GET ") != std::string::npos &&
           request.find("Upgrade: websocket") != std::string::npos;
}

std::string WebSocketParser::extract_websocket_key(const std::string& request) {
    size_t pos = request.find("Sec-WebSocket-Key:");
    if (pos == std::string::npos) return "";
    
    pos += 18; // "Sec-WebSocket-Key:" length
    while (pos < request.size() && (request[pos] == ' ' || request[pos] == '\t')) pos++;
    
    size_t end = request.find('\r', pos);
    if (end == std::string::npos) end = request.find('\n', pos);
    
    return request.substr(pos, end - pos);
}

std::string WebSocketParser::create_handshake_response(const std::string& key) {
    // 计算 Sec-WebSocket-Accept
    std::string accept_input = key + WS_GUID;
    
    unsigned char hash[SHA_DIGEST_LENGTH];
    EVP_Digest(accept_input.c_str(), accept_input.length(), hash, nullptr, EVP_sha1(), nullptr);
    
    std::string accept = base64_encode(hash, SHA_DIGEST_LENGTH);
    
    std::string response =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + accept + "\r\n"
        "Sec-WebSocket-Protocol: chat\r\n"
        "\r\n";
    
    return response;
}

WSResult WebSocketParser::parse(const std::vector<uint8_t>& data, WebSocketFrame& frame) {
    if (data.size() < 2) {
        return WSResult::INCOMPLETE;
    }
    
    size_t pos = 0;
    
    // 字节 1: FIN + RSV + Opcode
    frame.fin = (data[0] & 0x80) != 0;
    frame.opcode = static_cast<Opcode>(data[0] & 0x0F);
    
    // 字节 2: MASK + Payload Length
    frame.mask = (data[1] & 0x80) != 0;
    uint64_t payload_len = data[1] & 0x7F;
    
    pos = 2;
    
    // 扩展 payload length
    if (payload_len == 126) {
        if (data.size() < 4) return WSResult::INCOMPLETE;
        payload_len = (static_cast<uint64_t>(data[2]) << 8) | data[3];
        pos = 4;
    } else if (payload_len == 127) {
        if (data.size() < 10) return WSResult::INCOMPLETE;
        payload_len = 0;
        for (int i = 0; i < 8; ++i) {
            payload_len = (payload_len << 8) | data[2 + i];
        }
        pos = 10;
    }
    
    frame.payload_len = payload_len;
    
    // 处理 masking key
    if (frame.mask) {
        if (data.size() < pos + 4) return WSResult::INCOMPLETE;
        frame.masking_key.assign(data.begin() + pos, data.begin() + pos + 4);
        pos += 4;
    }
    
    // 检查是否有足够的数据
    if (data.size() < pos + payload_len) {
        return WSResult::INCOMPLETE;
    }
    
    // 复制 payload
    frame.payload.assign(data.begin() + pos, data.begin() + pos + payload_len);
    
    // 如果有 mask，解 mask
    if (frame.mask) {
        for (size_t i = 0; i < frame.payload.size(); ++i) {
            frame.payload[i] ^= frame.masking_key[i % 4];
        }
    }
    
    // 检查关闭帧
    if (frame.opcode == Opcode::CLOSE) {
        return WSResult::ERROR_CLOSE;
    }
    
    return WSResult::SUCCESS;
}

std::vector<uint8_t> WebSocketParser::encode(const std::string& message, Opcode opcode) {
    std::vector<uint8_t> frame;
    
    const uint8_t* data = reinterpret_cast<const uint8_t*>(message.c_str());
    size_t len = message.size();
    
    // 字节 1: FIN + Opcode
    frame.push_back(0x80 | static_cast<uint8_t>(opcode));
    
    // 字节 2: Payload length (服务器发送不需要 mask)
    if (len < 126) {
        frame.push_back(static_cast<uint8_t>(len));
    } else if (len < 65536) {
        frame.push_back(126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
    }
    
    // Payload
    frame.insert(frame.end(), data, data + len);
    
    return frame;
}

std::string WebSocketParser::decode(const WebSocketFrame& frame) {
    if (frame.opcode != Opcode::TEXT && frame.opcode != Opcode::BINARY) {
        return "";
    }
    return std::string(frame.payload.begin(), frame.payload.end());
}

std::vector<uint8_t> WebSocketParser::create_close_frame(uint16_t code) {
    std::vector<uint8_t> frame;
    frame.push_back(0x80 | static_cast<uint8_t>(Opcode::CLOSE));
    frame.push_back(2); // 2 bytes for close code
    frame.push_back((code >> 8) & 0xFF);
    frame.push_back(code & 0xFF);
    return frame;
}

std::vector<uint8_t> WebSocketParser::create_ping_frame() {
    std::vector<uint8_t> frame;
    frame.push_back(0x80 | static_cast<uint8_t>(Opcode::PING));
    frame.push_back(0); // no payload
    return frame;
}

std::vector<uint8_t> WebSocketParser::create_pong_frame() {
    std::vector<uint8_t> frame;
    frame.push_back(0x80 | static_cast<uint8_t>(Opcode::PONG));
    frame.push_back(0); // no payload
    return frame;
}

} // namespace chat
