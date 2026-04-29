CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Isrc

SRCS = src/server.cpp src/graph.cpp src/dfs.cpp src/auth.cpp src/campus.cpp
TARGET = campus_server

all: $(TARGET)
	@echo "✅  Build done. Run:  make run"

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

run: $(TARGET)
	@echo "🚀  http://localhost:8080  |  Open public/index.html"
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
