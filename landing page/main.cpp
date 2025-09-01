// Includes and setup
#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <random>
#include <chrono>
#include "db.h"

#pragma comment(lib, "ws2_32.lib")
using namespace std;

// Session storage
map<string, int> sessions;

// Generate session token
string generateSessionToken() {
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_int_distribution<> dis(0, 15);
    
    string token;
    for (int i = 0; i < 32; ++i) {
        token += "0123456789abcdef"[dis(gen)];
    }
    return token;
}

// Get user ID from session
int getSessionUserId(const string& request) {
    size_t pos = request.find("Session-Token: ");
    if (pos == string::npos) return -1;
    
    pos += 15;
    size_t end = request.find("\r\n", pos);
    if (end == string::npos) return -1;
    
    string token = request.substr(pos, end - pos);
    auto it = sessions.find(token);
    return (it != sessions.end()) ? it->second : -1;
}

// Read a file
string readFile(const string& path) {
    ifstream file(path, ios::in | ios::binary);
    if (!file) return "<h1>File Not Found</h1>";
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Receive full HTTP request
string recvFullRequest(SOCKET client) {
    string data;
    char buf[65536];  // Increased to 64KB buffer
    int n;

    // Step 1: Read headers
    while ((n = recv(client, buf, sizeof(buf), 0)) > 0) {
        data.append(buf, n);
        if (data.find("\r\n\r\n") != string::npos) break; // headers end
    }

    size_t header_end = data.find("\r\n\r\n");
    string headers = (header_end != string::npos) ? data.substr(0, header_end) : data;

    // Step 2: Extract Content-Length
    size_t content_length = 0;
    size_t cl_pos = headers.find("Content-Length:");
    if (cl_pos != string::npos) {
        cl_pos += 15; // skip "Content-Length:"
        while (cl_pos < headers.size() && headers[cl_pos] == ' ') cl_pos++;
        size_t line_end = headers.find("\r\n", cl_pos);
        content_length = stoi(headers.substr(cl_pos, line_end - cl_pos));
    }

    // Step 3: Read body until Content-Length
    string body = (header_end != string::npos) ? data.substr(header_end + 4) : "";
    while (body.size() < content_length) {
        n = recv(client, buf, sizeof(buf), 0);
        if (n <= 0) break;
        body.append(buf, n);
    }

    return headers + "\r\n\r\n" + body;
}

// Send HTTP response
void sendResponse(SOCKET client, const string& content, const string& status = "200 OK", const string& contentType = "text/html") {
    string response = "HTTP/1.1 " + status + "\r\n"
                      "Content-Type: " + contentType + "\r\n"
                      "Content-Length: " + to_string(content.length()) + "\r\n"
                      "Connection: close\r\n\r\n" + content;
    send(client, response.c_str(), response.size(), 0);
}

// Extract URL-encoded form data
string extract(const string& key, const string& body) {
    size_t pos = body.find(key + "=");
    if (pos == string::npos) return "";
    pos += key.length() + 1;
    size_t end = body.find("&", pos);
    if (end == string::npos) end = body.length();
    
    string value = body.substr(pos, end - pos);

    // URL decode
    string decoded;
    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '%' && i + 2 < value.length()) {
            int hex = stoi(value.substr(i + 1, 2), nullptr, 16);
            decoded += static_cast<char>(hex);
            i += 2;
        } else if (value[i] == '+') {
            decoded += ' ';
        } else {
            decoded += value[i];
        }
    }
    return decoded;
}

// Escape HTML
string escapeHtml(const string& input) {
    string output;
    for (char c : input) {
        switch (c) {
            case '<': output += "&lt;"; break;
            case '>': output += "&gt;"; break;
            case '&': output += "&amp;"; break;
            case '"': output += "&quot;"; break;
            case '\'': output += "&#x27;"; break;
            default: output += c;
        }
    }
    return output;
}

// Validate input
bool validateInput(const string& input, size_t maxLength = 1000) {
    return !input.empty() && input.length() <= maxLength;
}

// Escape JSON
string escapeJson(const string& input) {
    string output;
    for (char c : input) {
        switch (c) {
            case '"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += c;
        }
    }
    return output;
}

// Build JSON for entries
string buildEntriesJson(const vector<DiaryEntry>& entries) {
    string json = "[";
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        json += "{";
        json += "\"id\":" + to_string(e.id) + ",";
        json += "\"title\":\"" + escapeJson(e.title) + "\",";
        json += "\"content\":\"" + escapeJson(e.content) + "\",";
        json += "\"entry_date\":\"" + escapeJson(e.entry_date) + "\",";
        json += "\"created_at\":\"" + escapeJson(e.created_at) + "\"";
        json += "}";
        if (i != entries.size() - 1) json += ",";
    }
    json += "]";
    return json;
}

// Main server setup
int main() {
    createTables();

    WSADATA wsa;
    SOCKET server, client;
    struct sockaddr_in server_addr{}, client_addr{};
    int client_len = sizeof(client_addr);

    WSAStartup(MAKEWORD(2, 2), &wsa);
    server = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bind(server, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server, 3);

    cout << "Server running at http://localhost:8080\n";

    while ((client = accept(server, (struct sockaddr*)&client_addr, &client_len)) != INVALID_SOCKET) {
        
        string req = recvFullRequest(client); // ✅ full request read

        // Handle requests
        if (req.find("GET / ") != string::npos || req.find("GET /index.html") != string::npos) {
            sendResponse(client, readFile("public/index.html"));

        } else if (req.find("GET /style.css") != string::npos) {
            sendResponse(client, readFile("public/style.css"), "200 OK", "text/css");

        } else if (req.find("GET /script.js") != string::npos) {
            sendResponse(client, readFile("public/script.js"), "200 OK", "application/javascript");

        } else if (req.find("GET /Diary.png") != string::npos) {
            sendResponse(client, readFile("public/Diary.png"), "200 OK", "image/png");

        } else if (req.find("GET /diary_background.jpg") != string::npos) {
            sendResponse(client, readFile("public/diary_background.jpg"), "200 OK", "image/jpeg");

        } else if (req.find("POST /login") != string::npos) {
            size_t pos = req.find("\r\n\r\n");
            string body = req.substr(pos + 4);
            string uname = extract("username", body);
            string pwd = extract("password", body);

            if (!validateInput(uname, 50) || !validateInput(pwd, 100)) {
                sendResponse(client, "INVALID_INPUT");
                closesocket(client);
                continue;
            }

            int uid = loginUser(uname, pwd);
            if (uid > 0) {
                string token = generateSessionToken();
                sessions[token] = uid;
                string response = "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/plain\r\n"
                                "Session-Token: " + token + "\r\n"
                                "Content-Length: 13\r\n"
                                "Connection: close\r\n\r\nLOGIN_SUCCESS";
                send(client, response.c_str(), response.size(), 0);
            } else {
                sendResponse(client, "LOGIN_FAILED");
            }

        } else if (req.find("POST /register") != string::npos) {
            size_t pos = req.find("\r\n\r\n");
            string body = req.substr(pos + 4);
            string uname = extract("username", body);
            string pwd = extract("password", body);

            if (!validateInput(uname, 50) || !validateInput(pwd, 100)) {
                sendResponse(client, "INVALID_INPUT");
                closesocket(client);
                continue;
            }

            bool registered = registerUser(uname, pwd);
            if (registered) {
                sendResponse(client, "REGISTER_SUCCESS");
            } else {
                sendResponse(client, "REGISTER_FAILED");
            }

        } else if (req.find("GET /logout") != string::npos) {
            string token;
            size_t pos = req.find("Session-Token: ");
            if (pos != string::npos) {
                pos += 15;
                size_t end = req.find("\r\n", pos);
                token = req.substr(pos, end - pos);
                sessions.erase(token);
            }
            sendResponse(client, "Logged out");

        } else if (req.find("POST /entry/create") != string::npos) {
            int user_id = getSessionUserId(req);
            if (user_id <= 0) {
                sendResponse(client, "Unauthorized", "401 Unauthorized");
                closesocket(client);
                continue;
            }

            size_t pos = req.find("\r\n\r\n");
            string body = req.substr(pos + 4);
            string title = extract("title", body);
            string content = extract("content", body);
            string entry_date = extract("entry_date", body);

            if (!validateInput(title, 200) || !validateInput(content, 100000)) {
                sendResponse(client, "Invalid input", "400 Bad Request");
                closesocket(client);
                continue;
            }

            bool success = insertEntry(user_id, title, content, entry_date);
            if (success) {
                sendResponse(client, "Entry created");
            } else {
                sendResponse(client, "Failed to create entry", "500 Internal Server Error");
            }

        } else if (req.find("GET /entry/view") != string::npos) {
            int user_id = getSessionUserId(req);
            if (user_id <= 0) {
                sendResponse(client, "Unauthorized", "401 Unauthorized");
                closesocket(client);
                continue;
            }

            vector<DiaryEntry> entries = fetchEntries(user_id);
            string json = buildEntriesJson(entries);
            sendResponse(client, json, "200 OK", "application/json");

        } else if (req.find("POST /entry/edit") != string::npos) {
            int user_id = getSessionUserId(req);
            if (user_id <= 0) {
                sendResponse(client, "Unauthorized", "401 Unauthorized");
                closesocket(client);
                continue;
            }
            size_t pos = req.find( "\r\n\r\n" ); 
        string body = req.substr(pos + 4);
        int id = stoi(extract("id", body));
        string title = extract("title", body);
        string content = extract("content", body);
        string entry_date = extract("entry_date", body);
        if (updateEntry(id, title, content, entry_date)) {
            sendResponse(client, "Entry updated");
        } else {
            sendResponse(client, "Failed to update entry", "500 Internal Server Error");
        }

    } else if (req.find("GET /entry/delete?") != string::npos) {
        int user_id = getSessionUserId(req);
        if (user_id <= 0) {
            sendResponse(client, "Unauthorized", "401 Unauthorized");
            closesocket(client);
            continue;
        }

        size_t pos = req.find("id=");
        if (pos != string::npos) {
            pos += 3; // Skip "id="
            size_t end = req.find(" ", pos); // Find end of ID (space before HTTP/1.1)
            if (end == string::npos) end = req.find("&", pos);
            if (end == string::npos) end = req.find("\r", pos);
            
            string idStr = req.substr(pos, end - pos);
            int id = stoi(idStr);
            
            if (deleteEntry(id, user_id)) {
                sendResponse(client, "Entry deleted");
            } else {
                sendResponse(client, "Failed to delete entry", "500 Internal Server Error");
            }
        } else {
            sendResponse(client, "Missing entry ID", "400 Bad Request");
        }

    } else if (req.find("GET /entry/search?") != string::npos) {
        int user_id = getSessionUserId(req);
        if (user_id <= 0) {
            sendResponse(client, "Unauthorized", "401 Unauthorized");
            closesocket(client);
            continue;
        }

        size_t pos = req.find("q=");
        if (pos != string::npos) {
            string keyword = req.substr(pos + 2);
            if (validateInput(keyword, 100)) {
                vector<DiaryEntry> results = searchEntries(user_id, keyword);
                string json = buildEntriesJson(results);
                sendResponse(client, json, "200 OK", "application/json");
            } else {
                sendResponse(client, "Invalid search query", "400 Bad Request");
            }
        }

    } else if (req.find("GET /entry/export") != string::npos) {
        int user_id = getSessionUserId(req);
        if (user_id <= 0) {
            sendResponse(client, "Unauthorized", "401 Unauthorized");
            closesocket(client);
            continue;
        }

        vector<DiaryEntry> entries = fetchEntries(user_id);
        string json = buildEntriesJson(entries);
        sendResponse(client, json, "200 OK", "application/json");

    } else {
        sendResponse(client, "<h1>404 Not Found</h1>", "404 Not Found");
    }

    closesocket(client);
}

// Close server
closesocket(server);
WSACleanup();
return 0;
}

