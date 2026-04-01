#include "httpparse.h"
#include <string.h>
#include <strings.h>
#include <algorithm>
#include "../broken_include/tool.h"
tool a;
std::unordered_map<std::string, http::HttpRequest::HTTP_HEADER> http::HttpRequestParse::header_map =
    {
        {"host", http::HttpRequest::HTTP_HEADER::Host},
        {"user-agent", http::HttpRequest::HTTP_HEADER::User_Agent},
        {"connection", http::HttpRequest::HTTP_HEADER::Connection},
        {"accept-encoding", http::HttpRequest::HTTP_HEADER::Accept_Encoding},
        {"accept-language", http::HttpRequest::HTTP_HEADER::Accept_Language},
        {"accept", http::HttpRequest::HTTP_HEADER::Accept},
        {"cache-control", http::HttpRequest::HTTP_HEADER::Cache_Control},
        {"upgrade-insecure-requests", http::HttpRequest::HTTP_HEADER::Upgrade_Insecure_Requests}};

// 我感觉内置类型传值虽然拷贝..

// 解析一行，这样防止tcp粘包
//  从buffer[check_inde,read_index)
http::HttpRequestParse::LINE_STATUS http::HttpRequestParse::parse_line(char *buffer, int &checked_index, int &read_index)
{
    while (checked_index < read_index)
    {
        char temp = buffer[checked_index];
        if (temp == CR)
        {
            // CR 已经读到，但 LF 还没到，等下一次 recv
            if (checked_index + 1 >= read_index)
            {
                return http::HttpRequestParse::LINE_MORE;
            }
            if (buffer[checked_index + 1] == LF)
            {
                buffer[checked_index] = '\0';
                buffer[checked_index + 1] = '\0';
                checked_index += 2;
                return http::HttpRequestParse::LINE_OK;
            }
            return http::HttpRequestParse::LINE_BAD;
        }
        checked_index++;
    }
    return http::HttpRequestParse::LINE_MORE;
}

http::HttpRequestParse::HTTP_CODE http::HttpRequestParse::parse_requestline(char *buffer, http::HttpRequestParse::PARSE_STATUS&checkstate, http::HttpRequest &request)
{
    // no......不想做字符串解析...

    // 所有字符串函数都是在原有基础上拷贝一份再用
    char *url = strpbrk(buffer, " \t"); // 这个是字符串寻找函数
    if (!url)
    {
        return http::HttpRequestParse::BAD_REQUEST;
    }
    *url = '\0';
    url++;
    char *method = buffer; // xxxx\0 xxxx

    if (strcasecmp(method, "GET") == 0)
    {
        request._Method = http::HttpRequest::GET;
    }
    else if (strcasecmp(method, "POST") == 0)
    {
        request._Method = http::HttpRequest::POST;
    }
    else if (strcasecmp(method, "PUT") == 0)
    {
        request._Method = http::HttpRequest::PUT;
    }
    else if (strcasecmp(method, "DELETE") == 0)
    {
        request._Method = http::HttpRequest::DELETE;
    }
    else
        return http::HttpRequestParse::BAD_REQUEST;

    // 更改URL指针的位置
    url += strspn(url, " \t"); // 跳过空格

    char *ver = strpbrk(url, " \t");
    if (!ver)
    {
        return http::HttpRequestParse::BAD_REQUEST;
    }

    *ver = '\0';
    ver++;
    ver += strspn(ver, " \t");
    if (strcasecmp(ver, "HTTP/1.1") == 0)
    {
        request._Version = http::HttpRequest::HTTP_11;
    }
    else if (strcasecmp(ver, "HTTP/1.0") == 0)
    {
        request._Version = http::HttpRequest::HTTP_10;
    }
    else
        return http::HttpRequestParse::BAD_REQUEST;

    // 检查Url是否合法的同时，把所有的请求都变成/这种格式。
    if (strncasecmp(url, "http://", 7) == 0)
    {
        url += 7;
        url = strchr(url, '/');
    }
    if (!url || url[0] != '/')
        return http::HttpRequestParse::BAD_REQUEST;
    request._Url = static_cast<std::string>(url);
    checkstate = http::HttpRequestParse::PARSE_HEADER;
    return http::HttpRequestParse::NO_REQUEST; // 因为header和body还没解析完。
}

http::HttpRequestParse::HTTP_CODE http::HttpRequestParse::parse_headers(char *buffer, http::HttpRequestParse::PARSE_STATUS &checkstate, http::HttpRequest &request)
{
    if (buffer[0] == '\0')
    {
        checkstate = http::HttpRequestParse::PARSE_BODY;
        return http::HttpRequestParse::GET_REQUEST;
    }
    else
    {
        std::string tmp = buffer;
        // 用:去切割
        size_t pos = tmp.find(':');
        if (pos == std::string::npos)
            return http::HttpRequestParse::NO_REQUEST;

        std::string key = a.trim(tmp.substr(0, pos));
        std::string value = a.trim(tmp.substr(pos + 1));

        a.to_lower(key);

        auto it = header_map.find(key);

        if (it != header_map.end())
        {
            request._Header[it->second] = value;
        }
    }

    return http::HttpRequestParse::NO_REQUEST;
}
http::HttpRequestParse::HTTP_CODE http::HttpRequestParse::parse_body(char *buffer, http::HttpRequestParse::PARSE_STATUS &checkstate, http::HttpRequest &request)
{
    request._body = buffer;
    checkstate = http::HttpRequestParse::PARSE_BODY;
    return http::HttpRequestParse::GET_REQUEST;
}

// HTTP请求的入口
http::HttpRequestParse::HTTP_CODE http::HttpRequestParse::parse_content(char *buffer,
                                                       int &check_index, int &read_index,
                                                       http::HttpRequestParse::PARSE_STATUS &parse_state,
                                                       int &start_line,
                                                       http::HttpRequest &request)
{

    http::HttpRequestParse::LINE_STATUS LINE = http::HttpRequestParse::LINE_BAD;
    http::HttpRequestParse::HTTP_CODE retcode = http::HttpRequestParse::NO_REQUEST;
    while ((LINE = parse_line(buffer, check_index, read_index)) == http::HttpRequestParse::LINE_OK)
    {
        char *temp = buffer + start_line; // 这一行在buffer中的起始位置
        start_line = check_index;         // 下一行起始位置
        switch (parse_state)
        {
        case http::HttpRequestParse::PARSE_REQUESTLINE:
            retcode = parse_requestline(temp, parse_state, request);
            if (retcode == http::HttpRequestParse::BAD_REQUEST)
                return http::HttpRequestParse::BAD_REQUEST;
            break;
        case http::HttpRequestParse::PARSE_HEADER:
            retcode = parse_headers(temp, parse_state, request);
            if (retcode == http::HttpRequestParse::BAD_REQUEST)
                return http::HttpRequestParse::BAD_REQUEST;
            if (retcode == http::HttpRequestParse::GET_REQUEST)
                return http::HttpRequestParse::GET_REQUEST;
            break;
        case http::HttpRequestParse::PARSE_BODY:
            retcode = parse_body(temp, parse_state, request);
            if (retcode == http::HttpRequestParse::BAD_REQUEST)
                return http::HttpRequestParse::BAD_REQUEST;
            if (retcode == http::HttpRequestParse::GET_REQUEST)
                return http::HttpRequestParse::GET_REQUEST;
            break;
        default:
            break;
        }
    }
    if (LINE == http::HttpRequestParse::LINE_MORE)
    {
        return http::HttpRequestParse::NO_REQUEST;
    }
    else
    {
        return http::HttpRequestParse::BAD_REQUEST;
    }
}
