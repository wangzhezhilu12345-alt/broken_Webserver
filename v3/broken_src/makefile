CXX = g++

MYSQL_INCLUDE ?= /usr/include/mysql
MYSQL_LIBS ?= -lmysqlclient

CXXFLAGS = -std=c++17 -Wall -g -I../broken_include -I../http -I../io -I../logic -I../server -I$(MYSQL_INCLUDE)
LDFLAGS = $(MYSQL_LIBS)

SRCS = ../logic/main.cpp ../io/Socket.cpp ../logic/Threadpool.cpp tooh.cpp \
       ../io/connection.cpp ../logic/logic.cpp ../logic/page_service.cpp ../logic/mysql_store.cpp \
       ../io/epoll.cpp ../server/server.cpp ../http/http_handler.cpp ../http/httpprase.cpp ../http/httpreponse.cpp

TARGET = server

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
