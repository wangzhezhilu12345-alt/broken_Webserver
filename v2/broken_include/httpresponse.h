#pragma once    
#include "../broken_include/httpparse.h"
#include <unordered_map>

namespace http
{

    struct MimeType
    {
        MimeType(const std::string &str) : type(str) {};
        MimeType(const char *str) : type(str) {};

        std::string type;
    };//这个是告诉前端要以怎么样的方式解析信息。
    class Httpresponse
    {
    public:
        void appenBuffer(char *buffer) const;
        enum HttpStatusCode
        {
            Unknow,
            OK = 200,
            Frobidden403 = 403,
            NOTFOUND = 404
        };
        Httpresponse()
            : Statuscode(Unknow),
              _Version(http::HttpRequest::HTTP_11),
              _Contentlen(0),
              _body(nullptr),
              _isClose(false),
              mime("text/html")
        {}

    public:
        HttpStatusCode Statuscode;
        http::HttpRequest::HTTP_VERSION _Version;
        std::string statusmsg;//信息状态码.OK
        bool _isClose;
        char *_body; // 内容主体
        int _Contentlen;
        MimeType mime = "text/html"; 
        std::unordered_map<std::string,std::string> _header;//这个是处理什么的
        std::string _filepath;
    };
    extern std::unordered_map < std::string,MimeType> mime_map;
}