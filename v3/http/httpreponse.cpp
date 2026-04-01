#include "httpresponse.h"
#include <string.h>
using namespace http;

std::unordered_map<std::string, http::MimeType> http::mime_map = {{".html", "text/html"},
                                                                  {".xml", "text/xml"},
                                                                  {".xhtml", "application/xhtml+xml"},
                                                                  {".txt", "text/plain"},
                                                                  {".rtf", "application/rtf"},
                                                                  {".pdf", "application/pdf"},
                                                                  {".word", "application/msword"},
                                                                  {".png", "image/png"},
                                                                  {".gif", "image/gif"},
                                                                  {".jpg", "image/jpeg"},
                                                                  {".jpeg", "image/jpeg"},
                                                                  {".au", "audio/basic"},
                                                                  {".mpeg", "video/mpeg"},
                                                                  {".mpg", "video/mpeg"},
                                                                  {".avi", "video/x-msvideo"},
                                                                  {".gz", "application/x-gzip"},
                                                                  {".tar", "application/x-tar"},
                                                                  {".css", "text/css"},
                                                                  {"", "text/plain"},
                                                                  {"default", "text/plain"}};

// 注意这里是个输出型参数！！！
//这里也是一种序列化
void http::Httpresponse::appenBuffer(char *buffer) const
{
    std::string res;

    if (_Version == HttpRequest::HTTP_11)
    {
        res += "HTTP/1.1 ";
    }
    else
    {
        res += "HTTP/1.0 ";
    }

    res += std::to_string(Statuscode);
    res += " ";
    res += statusmsg;
    res += "\r\n";

    
    res += "Content-Type: ";
    res += mime.type;
    res += "\r\n";


    res += "Content-Length: ";
    res += std::to_string(strlen(_body));
    res += "\r\n";

    if (_isClose)
    {
        res += "Connection: close\r\n";
    }
    else
    {
        res += "Connection: keep-alive\r\n";
    }

    //header结束
    res += "\r\n";

    //body
    res += _body;

    
    strcpy(buffer, res.c_str());
}
