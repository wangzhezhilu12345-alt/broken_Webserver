# 编译器
CXX = g++

# 编译选项
CXXFLAGS = -std=c++17 -Wall -g

# 源文件（你当前项目）
SRCS = main.cpp httpserver.cpp httpprase.cpp httpreponse.cpp Socket.cpp Threadpool.cpp tooh.cpp

# 生成目标
TARGET = server

# 默认规则
all: $(TARGET)

# 链接
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

# 清理
clean:
	rm -f $(TARGET)
