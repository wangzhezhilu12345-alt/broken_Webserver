# 编译器
CXX = g++

# 编译选项
CXXFLAGS = -std=c++17 -Wall -g -I../broken_include -I../http -I../io -I../logic -I../server

# 源文件（你当前项目）
SRCS = ../logic/main.cpp ../io/Socket.cpp ../logic/Threadpool.cpp tooh.cpp \
       ../io/connection.cpp ../logic/logic.cpp ../logic/page_service.cpp ../io/epoll.cpp \
       ../server/server.cpp ../http/http_handler.cpp ../http/httpprase.cpp ../http/httpreponse.cpp

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
