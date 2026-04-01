#pragma once
#include "../io/connection.h"
#include "../logic/page_service.h"
#include "httpparse.h"
#include "httpresponse.h"
#include <memory>
#include <string>

namespace http
{
    class HttpHandler
    {
    public:
        HttpRequestParse::HTTP_CODE ParseRequest(const std::shared_ptr<servercore::Connection> &connection) const;
        void BuildResponse(const HttpRequest &request,
                           const serverlogic::PageResult &page,
                           Httpresponse &response) const;
        bool BuildResponseData(const Httpresponse &response, std::string &output) const;
        std::string BuildSimpleResponseData(const std::string &status_line, const std::string &body) const;
    };
}
