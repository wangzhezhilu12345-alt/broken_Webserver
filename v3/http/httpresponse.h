#pragma once

#include "httpparse.h"

#include <string>
#include <unordered_map>

namespace http
{
    struct MimeType
    {
        MimeType(const std::string &str) : type(str) {}
        MimeType(const char *str) : type(str) {}

        std::string type;
    };

    class Httpresponse
    {
    public:
        void appenBuffer(char *buffer) const;

        enum HttpStatusCode
        {
            Unknow,
            OK = 200,
            BadRequest400 = 400,
            Unauthorized401 = 401,
            Frobidden403 = 403,
            NOTFOUND = 404,
            MethodNotAllowed405 = 405,
            InternalServerError500 = 500,
            BadGateway502 = 502,
            ServiceUnavailable503 = 503,
            GatewayTimeout504 = 504
        };

        Httpresponse()
            : Statuscode(Unknow),
              _Version(http::HttpRequest::HTTP_11),
              _isClose(false),
              _body(nullptr),
              _Contentlen(0),
              mime("text/html")
        {
        }

    public:
        HttpStatusCode Statuscode;
        http::HttpRequest::HTTP_VERSION _Version;
        std::string statusmsg;
        bool _isClose;
        char *_body;
        int _Contentlen;
        MimeType mime;
        std::unordered_map<std::string, std::string> _header;
        std::string _filepath;
        std::string _body_text;
    };

    extern std::unordered_map<std::string, MimeType> mime_map;
}
