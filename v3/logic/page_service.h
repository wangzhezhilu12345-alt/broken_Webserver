#pragma once

#include "../http/httpparse.h"
#include "../http/httpresponse.h"
#include "mysql_store.h"

#include <string>

namespace serverlogic
{
    struct PageResult
    {
        PageResult()
            : _status_code(http::Httpresponse::Unknow),
              _status_message("Unknown"),
              _filepath(),
              _body(),
              _mime("text/plain")
        {
        }

        http::Httpresponse::HttpStatusCode _status_code;
        std::string _status_message;
        std::string _filepath;
        std::string _body;
        http::MimeType _mime;
    };

    class PageService
    {
    public:
        PageService();

        bool Init();
        PageResult Resolve(const http::HttpRequest &request, const char *filebase) const;

    private:
        MysqlStore _mysql_store;
    };
}
