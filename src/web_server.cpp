#include "lru_cache.h"
#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <vector>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

std::unordered_map<uint64_t, std::string> g_string_registry;

uint64_t to_uint64(const std::string& str) {
    try {
        uint64_t val = std::stoull(str);
        g_string_registry[val] = str;
        return val;
    } catch (...) {
        uint64_t hash_val = std::hash<std::string>{}(str);
        g_string_registry[hash_val] = str;
        return hash_val;
    }
}

std::string from_uint64(uint64_t val) {
    auto it = g_string_registry.find(val);
    if (it != g_string_registry.end()) {
        return it->second;
    }
    return std::to_string(val);
}

std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string get_query_param(const std::string& path, const std::string& param) {
    std::size_t pos = path.find(param + "=");
    if (pos == std::string::npos) return "";
    pos += param.length() + 1;
    std::size_t end = path.find("&", pos);
    if (end == std::string::npos) end = path.length();
    return path.substr(pos, end - pos);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "socket creation failed.\n";
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt failed.\n";
        return 1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "bind failed with error: " << WSAGetLastError() << "\n";
        return 1;
    }
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        std::cerr << "listen failed.\n";
        return 1;
    }

    std::cout << "Starting Raw WinSock LRU Cache Server at http://localhost:8080\n";

    auto cache = std::make_unique<lru_cache>(10);
    auto formatter = [](uint64_t val) { return from_uint64(val); };

    while (true) {
        SOCKET client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) continue;

        char buffer[4096] = {0};
        int bytes_received = recv(client_socket, buffer, 4095, 0);
        if (bytes_received <= 0) {
            closesocket(client_socket);
            continue;
        }
        std::string request(buffer, bytes_received);
        std::cout << "Received request:\n" << request.substr(0, 100) << "...\n";

        std::string response = "HTTP/1.1 200 OK\r\nConnection: close\r\n";
        std::string body = "";

        if (request.find("GET / ") == 0 || request.find("GET /?") == 0) {
            body = read_file("public/index.html");
            if (body.empty()) body = "<h1>public/index.html not found</h1>";
            response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
            response += "Content-Type: text/html\r\n\r\n" + body;
        } 
        else if (request.find("GET /index.css ") == 0 || request.find("GET /index.css?") == 0) {
            body = read_file("public/index.css");
            if (body.empty()) body = "/* public/index.css not found */";
            response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
            response += "Content-Type: text/css\r\n\r\n" + body;
        }
        else if (request.find("GET /app.js ") == 0 || request.find("GET /app.js?") == 0) {
            body = read_file("public/app.js");
            if (body.empty()) body = "/* public/app.js not found */";
            response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
            response += "Content-Type: application/javascript\r\n\r\n" + body;
        } 
        else if (request.find("GET /api/state") == 0) {
            body = cache->get_json_state(formatter);
            response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
            response += "Content-Type: application/json\r\n\r\n" + body;
        }
        else if (request.find("POST /api/put") == 0) {
            std::size_t path_end = request.find(" HTTP");
            std::string path = request.substr(5, path_end - 5);
            std::string key = get_query_param(path, "key");
            std::string val = get_query_param(path, "value");
            if (!key.empty() && !val.empty()) {
                cache->put(to_uint64(key), to_uint64(val));
            }
            body = cache->get_json_state(formatter);
            response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
            response += "Content-Type: application/json\r\n\r\n" + body;
        }
        else if (request.find("POST /api/get") == 0) {
            std::size_t path_end = request.find(" HTTP");
            std::string path = request.substr(5, path_end - 5);
            std::string key = get_query_param(path, "key");
            if (!key.empty()) cache->get(to_uint64(key));
            body = cache->get_json_state(formatter);
            response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
            response += "Content-Type: application/json\r\n\r\n" + body;
        }
        else if (request.find("POST /api/remove") == 0) {
            std::size_t path_end = request.find(" HTTP");
            std::string path = request.substr(5, path_end - 5);
            std::string key = get_query_param(path, "key");
            if (!key.empty()) cache->remove(to_uint64(key));
            body = cache->get_json_state(formatter);
            response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
            response += "Content-Type: application/json\r\n\r\n" + body;
        }
        else if (request.find("POST /api/reset") == 0) {
            std::size_t path_end = request.find(" HTTP");
            std::string path = request.substr(5, path_end - 5);
            std::string cap_str = get_query_param(path, "capacity");
            int cap = 10;
            try { cap = std::stoi(cap_str); } catch (...) {}
            if (cap <= 0) cap = 10;
            cache = std::make_unique<lru_cache>(cap);
            body = cache->get_json_state(formatter);
            response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
            response += "Content-Type: application/json\r\n\r\n" + body;
        }
        else if (request.find("POST /api/batch") == 0) {
            std::size_t path_end = request.find(" HTTP");
            std::string path = request.substr(5, path_end - 5);
            std::string ops_str = get_query_param(path, "ops");
            
            std::string json_array = "[";
            std::stringstream ss(ops_str);
            std::string token;
            bool first = true;
            
            while (std::getline(ss, token, '|')) {
                if (token.empty()) continue;
                char op = token[0];
                std::size_t c1 = token.find(',');
                if (c1 != std::string::npos) {
                    std::size_t c2 = token.find(',', c1 + 1);
                    std::string key = token.substr(c1 + 1, (c2 == std::string::npos ? std::string::npos : c2 - c1 - 1));
                    if (op == 'P' && c2 != std::string::npos) {
                        std::string val = token.substr(c2 + 1);
                        cache->put(to_uint64(key), to_uint64(val));
                    } else if (op == 'G') {
                        cache->get(to_uint64(key));
                    } else if (op == 'R') {
                        cache->remove(to_uint64(key));
                    }
                }
                if (!first) json_array += ",";
                json_array += cache->get_json_state(formatter);
                first = false;
            }
            json_array += "]";
            body = json_array;
            response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
            response += "Content-Type: application/json\r\n\r\n" + body;
        }
        else {
            response = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\nNot Found";
        }

        int total_sent = 0;
        int to_send = response.length();
        while (total_sent < to_send) {
            int sent = send(client_socket, response.c_str() + total_sent, to_send - total_sent, 0);
            if (sent <= 0) break;
            total_sent += sent;
        }

        shutdown(client_socket, SD_SEND);
        closesocket(client_socket);
    }

    WSACleanup();
    return 0;
}
