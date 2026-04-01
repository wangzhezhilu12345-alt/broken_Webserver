#include "page_service.h"
#include <sys/stat.h>

namespace
{
    serverlogic::PageResult BuildErrorPageResult(http::Httpresponse::HttpStatusCode status_code,
                                                 const std::string &status_message,
                                                 const std::string &filename,
                                                 const char *filebase)
    {
        serverlogic::PageResult result;
        result._status_code = status_code;
        result._status_message = status_message;
        result._filepath = std::string(filebase) + "/" + filename;
        result._mime = http::MimeType("text/html");
        return result;
    }
}

serverlogic::PageResult serverlogic::PageService::Resolve(const http::HttpRequest &request, const char *filebase) const
{
    PageResult result;
    const std::string base_path(filebase);

    std::string filepath = request._Url;
    if (filepath == "/")
    {
        filepath = "/index.html";
    }

    std::string::size_type pos = filepath.rfind('?');
    if (pos != std::string::npos)
    {
        filepath.erase(pos);
    }

    if (filepath == "/403.html")
    {
        return BuildErrorPageResult(http::Httpresponse::Frobidden403, "Forbidden", "403.html", filebase);
    }
    if (filepath == "/404.html")
    {
        return BuildErrorPageResult(http::Httpresponse::NOTFOUND, "Not Found", "404.html", filebase);
    }
    if (filepath == "/500.html")
    {
        return BuildErrorPageResult(http::Httpresponse::InternalServerError500, "Internal Server Error", "500.html", filebase);
    }
    if (filepath == "/504.html")
    {
        return BuildErrorPageResult(http::Httpresponse::GatewayTimeout504, "Gateway Timeout", "504.html", filebase);
    }

    std::string mime_key;
    if (filepath.rfind('.') != std::string::npos)
    {
        mime_key = filepath.substr(filepath.rfind('.'));
    }

    std::unordered_map<std::string, http::MimeType>::iterator mime_it = http::mime_map.find(mime_key);
    if (mime_it != http::mime_map.end())
    {
        result._mime = mime_it->second;
    }

    std::string real_file = base_path + filepath;
    struct stat file_stat;
    if (stat(real_file.c_str(), &file_stat) < 0)
    {
        return BuildErrorPageResult(http::Httpresponse::NOTFOUND, "Not Found", "404.html", filebase);
    }

    if (!S_ISREG(file_stat.st_mode))
    {
        return BuildErrorPageResult(http::Httpresponse::Frobidden403, "Forbidden", "403.html", filebase);
    }

    result._status_code = http::Httpresponse::OK;
    result._status_message = "OK";
    result._filepath = real_file;
    return result;
}
