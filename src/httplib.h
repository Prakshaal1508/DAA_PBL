#pragma once
/*
 * httplib.h  –  Minimal single-header HTTP/1.1 server
 *
 * Interface is compatible with yhirose/cpp-httplib so you can swap in
 * the full library later without changing server.cpp at all.
 *
 * Supports:
 *   GET, POST, OPTIONS   (enough for our REST API)
 *   CORS headers
 *   Query params via req.get_param_value()
 *   Body parsing
 */

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <algorithm>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

namespace httplib {

// ─── Request ────────────────────────────────────────────────────
struct Request {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> params;   // query + body params
    std::map<std::string, std::string> headers;

    // Get a query/body param by name (returns "" if missing)
    std::string get_param_value(const std::string& key) const {
        auto it = params.find(key);
        return it != params.end() ? it->second : "";
    }
};

// ─── Response ───────────────────────────────────────────────────
struct Response {
    int         status  = 200;
    std::string body;
    std::string content_type = "application/json";
    std::map<std::string, std::string> headers;

    void set_content(const std::string& b, const std::string& ct) {
        body         = b;
        content_type = ct;
    }
};

// ─── Handler type ───────────────────────────────────────────────
using Handler = std::function<void(const Request&, Response&)>;

// ─── Helpers ────────────────────────────────────────────────────
static std::string urlDecode(const std::string& s) {
    std::string r;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+') { r += ' '; }
        else if (s[i] == '%' && i+2 < s.size()) {
            r += (char)std::stoi(s.substr(i+1,2), nullptr, 16);
            i += 2;
        } else r += s[i];
    }
    return r;
}

static std::map<std::string,std::string> parseKV(const std::string& qs) {
    std::map<std::string,std::string> m;
    std::stringstream ss(qs);
    std::string tok;
    while (std::getline(ss, tok, '&')) {
        auto eq = tok.find('=');
        if (eq == std::string::npos) continue;
        m[urlDecode(tok.substr(0,eq))] = urlDecode(tok.substr(eq+1));
    }
    return m;
}

// ─── Server ─────────────────────────────────────────────────────
class Server {
public:
    // Register GET handler
    Server& Get(const std::string& path, Handler h) {
        gets_[path] = h; return *this;
    }
    // Register POST handler
    Server& Post(const std::string& path, Handler h) {
        posts_[path] = h; return *this;
    }

    // Start listening (blocks forever)
    bool listen(const char* host, int port) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) { perror("socket"); return false; }

        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port        = htons(port);

        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind"); return false;
        }
        ::listen(fd, 20);

        std::cout << "✅  Server listening on http://" << host << ":" << port << "\n";
        std::cout << "    Open  public/index.html  in your browser\n";
        std::cout << "    Ctrl+C to stop\n\n";

        while (true) {
            int client = accept(fd, nullptr, nullptr);
            if (client < 0) continue;
            handleClient(client);
            close(client);
        }
        close(fd);
        return true;
    }

private:
    std::map<std::string, Handler> gets_;
    std::map<std::string, Handler> posts_;

    void handleClient(int client) {
        char buf[16384] = {};
        int n = read(client, buf, sizeof(buf)-1);
        if (n <= 0) return;

        std::string raw(buf, n);

        // ── Parse request line ──────────────────────────────────
        std::stringstream rs(raw);
        Request req;
        std::string fullPath, ver;
        rs >> req.method >> fullPath >> ver;

        // ── Split path and query string ─────────────────────────
        std::string queryStr;
        auto qp = fullPath.find('?');
        if (qp == std::string::npos) {
            req.path = fullPath;
        } else {
            req.path  = fullPath.substr(0, qp);
            queryStr  = fullPath.substr(qp+1);
        }

        // ── Parse query params ──────────────────────────────────
        req.params = parseKV(queryStr);

        // ── Parse body ──────────────────────────────────────────
        auto bp = raw.find("\r\n\r\n");
        if (bp != std::string::npos) {
            req.body = raw.substr(bp+4);
            // merge body params into params map
            auto bodyParams = parseKV(req.body);
            for (auto& [k,v] : bodyParams)
                req.params[k] = v;
        }

        // ── CORS pre-flight ─────────────────────────────────────
        if (req.method == "OPTIONS") {
            std::string resp =
                "HTTP/1.1 204 No Content\r\n"
                "Access-Control-Allow-Origin: *\r\eeen"
                "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                "Access-Control-Allow-Headers: Content-Type\r\n"
                "Connection: close\r\n\r\n";
            write(client, resp.c_str(), resp.size());
            return;
        }

        // ── Find handler ────────────────────────────────────────
        Response res;
        bool found = false;

        if (req.method == "GET" && gets_.count(req.path)) {
            gets_[req.path](req, res);
            found = true;
        } else if (req.method == "POST" && posts_.count(req.path)) {
            posts_[req.path](req, res);
            found = true;
        }

        if (!found) {
            res.status = 404;
            res.set_content("{\"error\":\"Not found\"}", "application/json");
        }

        // ── Build HTTP response ─────────────────────────────────
        std::string httpResp =
            "HTTP/1.1 " + std::to_string(res.status) + " OK\r\n"
            "Content-Type: "   + res.content_type + "; charset=utf-8\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Connection: close\r\n"
            "Content-Length: " + std::to_string(res.body.size()) + "\r\n\r\n"
            + res.body;

        write(client, httpResp.c_str(), httpResp.size());
    }
};

} // namespace httplib
