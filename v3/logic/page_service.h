#pragma once
#include "../http/httpparse.h"
#include "../http/httpresponse.h"
#include <string>

namespace serverlogic
{
    struct PageResult
    {
        PageResult()
            : _status_code(http::Httpresponse::Unknow),
              _status_message("Unknown"),
              _filepath(),
              _mime("text/plain")
        {
        }

        http::Httpresponse::HttpStatusCode _status_code;
        std::string _status_message;
        std::string _filepath;
        http::MimeType _mime;
    };

    class PageService
    {
    public:
        PageResult Resolve(const http::HttpRequest &request, const char *filebase) const;
    };
}
