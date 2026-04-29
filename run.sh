#!/bin/bash
# Campus Navigation System – one-click start
set -e
cd "$(dirname "${BASH_SOURCE[0]}")"
echo "🔨 Compiling..."
g++ -std=c++17 -O2 -Isrc \
    src/server.cpp src/graph.cpp src/dfs.cpp src/auth.cpp src/campus.cpp \
    -o campus_server
echo "✅ Done! Starting server..."
echo ""
echo "  Open:  public/index.html  in your browser"
echo "  Login: admin / admin123"
echo ""
./campus_server
