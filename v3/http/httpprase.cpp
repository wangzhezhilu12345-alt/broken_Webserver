#include "httpparse.h"

#include <cstdlib>
#include <string.h>
#include <strings.h>

#include "../broken_include/tool.h"

tool a;

std::unordered_map<std::string, http::HttpRequest::HTTP_HEADER> http::HttpRequestParse::header_map = {
    {"host", http::HttpRequest::HTTP_HEADER::Host},
    {"user-agent", http::HttpRequest::HTTP_HEADER::User_Agent},
    {"connection", http::HttpRequest::HTTP_HEADER::Connection},
    {"content-type", http::HttpRequest::HTTP_HEADER::Content_Type},
    {"content-length", http::HttpRequest::HTTP_HEADER::Content_Length},
    {"accept-encoding", http::HttpRequest::HTTP_HEADER::Accept_Encoding},
    {"accept-language", http::HttpRequest::HTTP_HEADER::Accept_Language},
    {"accept", http::HttpRequest::HTTP_HEADER::Accept},
    {"cache-control", http::HttpRequest::HTTP_HEADER::Cache_Control},
    {"upgrade-insecure-requests", http::HttpRequest::HTTP_HEADER::Upgrade_Insecure_Requests}};

http::HttpRequestParse::LINE_STATUS http::HttpRequestParse::parse_line(char *buffer, int &checked_index, int &read_index)
{
    while (checked_index < read_index)
    {
        char temp = buffer[checked_index];
        if (temp == CR)
        {
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
        ++checked_index;
    }
    return http::HttpRequestParse::LINE_MORE;
}

http::HttpRequestParse::HTTP_CODE http::HttpRequestParse::parse_requestline(
    char *buffer,
    http::HttpRequestParse::PARSE_STATUS &checkstate,
    http::HttpRequest &request)
{
    char *url = strpbrk(buffer, " \t");
    if (!url)
    {
        return http::HttpRequestParse::BAD_REQUEST;
    }
    *url = '\0';
    ++url;

    char *method = buffer;
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
    {
        return http::HttpRequestParse::BAD_REQUEST;
    }

    url += strspn(url, " \t");
    char *ver = strpbrk(url, " \t");
    if (!ver)
    {
        return http::HttpRequestParse::BAD_REQUEST;
    }

    *ver = '\0';
    ++ver;
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
    {
        return http::HttpRequestParse::BAD_REQUEST;
    }

    if (strncasecmp(url, "http://", 7) == 0)
    {
        url += 7;
        url = strchr(url, '/');
    }
    if (!url || url[0] != '/')
    {
        return http::HttpRequestParse::BAD_REQUEST;
    }

    request._Url = std::string(url);
    request._Body.clear();
    request._ContentLength = 0;
    request._body = nullptr;
    request._Header.clear();
    checkstate = http::HttpRequestParse::PARSE_HEADER;
    return http::HttpRequestParse::NO_REQUEST;
}

http::HttpRequestParse::HTTP_CODE http::HttpRequestParse::parse_headers(
    char *buffer,
    http::HttpRequestParse::PARSE_STATUS &checkstate,
    http::HttpRequest &request)
{
    if (buffer[0] == '\0')
    {
        if (request._Method == http::HttpRequest::POST && request._ContentLength > 0)
        {
            checkstate = http::HttpRequestParse::PARSE_BODY;
            return http::HttpRequestParse::NO_REQUEST;
        }
        return http::HttpRequestParse::GET_REQUEST;
    }

    std::string tmp = buffer;
    size_t pos = tmp.find(':');
    if (pos == std::string::npos)
    {
        return http::HttpRequestParse::NO_REQUEST;
    }

    std::string key = a.trim(tmp.substr(0, pos));
    std::string value = a.trim(tmp.substr(pos + 1));
    a.to_lower(key);

    std::unordered_map<std::string, http::HttpRequest::HTTP_HEADER>::iterator it = header_map.find(key);
    if (it != header_map.end())
    {
        request._Header[it->second] = value;
        if (it->second == http::HttpRequest::Content_Length)
        {
            request._ContentLength = std::atoi(value.c_str());
            if (request._ContentLength < 0)
            {
                return http::HttpRequestParse::BAD_REQUEST;
            }
        }
    }

    return http::HttpRequestParse::NO_REQUEST;
}

http::HttpRequestParse::HTTP_CODE http::HttpRequestParse::parse_body(
    char *buffer,
    int body_length,
    http::HttpRequest &request)
{
    if (body_length < request._ContentLength)
    {
        return http::HttpRequestParse::NO_REQUEST;
    }

    request._body = buffer;
    request._Body.assign(buffer, static_cast<size_t>(request._ContentLength));
    return http::HttpRequestParse::GET_REQUEST;
}

http::HttpRequestParse::HTTP_CODE http::HttpRequestParse::parse_content(
    char *buffer,
    int &check_index,
    int &read_index,
    http::HttpRequestParse::PARSE_STATUS &parse_state,
    int &start_line,
    http::HttpRequest &request)
{
    http::HttpRequestParse::LINE_STATUS line = http::HttpRequestParse::LINE_BAD;
    http::HttpRequestParse::HTTP_CODE retcode = http::HttpRequestParse::NO_REQUEST;

    while ((line = parse_line(buffer, check_index, read_index)) == http::HttpRequestParse::LINE_OK)
    {
        char *temp = buffer + start_line;
        start_line = check_index;

        switch (parse_state)
        {
        case http::HttpRequestParse::PARSE_REQUESTLINE:
            retcode = parse_requestline(temp, parse_state, request);
            if (retcode == http::HttpRequestParse::BAD_REQUEST)
            {
                return http::HttpRequestParse::BAD_REQUEST;
            }
            break;
        case http::HttpRequestParse::PARSE_HEADER:
            retcode = parse_headers(temp, parse_state, request);
            if (retcode == http::HttpRequestParse::BAD_REQUEST)
            {
                return http::HttpRequestParse::BAD_REQUEST;
            }
            if (retcode == http::HttpRequestParse::GET_REQUEST)
            {
                return http::HttpRequestParse::GET_REQUEST;
            }
            break;
        case http::HttpRequestParse::PARSE_BODY:
            retcode = parse_body(buffer + start_line, read_index - start_line, request);
            if (retcode == http::HttpRequestParse::GET_REQUEST)
            {
                return http::HttpRequestParse::GET_REQUEST;
            }
            return retcode;
        default:
            return http::HttpRequestParse::BAD_REQUEST;
        }
    }

    if (parse_state == http::HttpRequestParse::PARSE_BODY)
    {
        return parse_body(buffer + start_line, read_index - start_line, request);
    }

    if (line == http::HttpRequestParse::LINE_MORE)
    {
        return http::HttpRequestParse::NO_REQUEST;
    }
    return http::HttpRequestParse::BAD_REQUEST;
}
