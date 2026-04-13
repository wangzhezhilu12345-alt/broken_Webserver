#pragma once

#include <iostream>
#include <map>
#include <string>
#include <unordered_map>

#define CR '\r'
#define LF '\n'

namespace http
{
    class HttpRequest
    {
    public:
        enum HTTP_METHOND
        {
            GET = 0,
            POST,
            PUT,
            DELETE,
            METHOND_NOT_SUPPORT
        };

        enum HTTP_VERSION
        {
            HTTP_10,
            HTTP_11,
            VERSION_NOT_SUPPORT
        };

        enum HTTP_HEADER
        {
            Host = 0,
            User_Agent,
            Connection,
            Content_Type,
            Content_Length,
            Accept_Encoding,
            Accept_Language,
            Accept,
            Cache_Control,
            Upgrade_Insecure_Requests
        };

        HttpRequest(HTTP_METHOND method = METHOND_NOT_SUPPORT,
                    std::string url = "",
                    HTTP_VERSION version = VERSION_NOT_SUPPORT)
            : _Method(method),
              _Url(url),
              _Version(version),
              _body(nullptr),
              _ContentLength(0)
        {
        }

        struct ENUMHASH
        {
            template <typename T>
            std::size_t operator()(T t) const
            {
                return static_cast<std::size_t>(t);
            }
        };

    public:
        HTTP_METHOND _Method;
        std::string _Url;
        HTTP_VERSION _Version;
        char *_body;
        int _ContentLength;
        std::string _Body;
        std::unordered_map<HTTP_HEADER, std::string, ENUMHASH> _Header;
    };

    class HttpRequestParse
    {
    public:
        enum LINE_STATUS
        {
            LINE_OK,
            LINE_BAD,
            LINE_MORE
        };

        enum PARSE_STATUS
        {
            PARSE_REQUESTLINE,
            PARSE_HEADER,
            PARSE_BODY
        };

        enum HTTP_CODE
        {
            NO_REQUEST,
            GET_REQUEST,
            BAD_REQUEST,
            FORBIDDEN_REQUEST,
            INTERNAL_ERROR,
            CLOSED_CONNECTION
        };

        static std::unordered_map<std::string, http::HttpRequest::HTTP_HEADER> header_map;
        static LINE_STATUS parse_line(char *buffer, int &checked_index, int &read_index);
        static HTTP_CODE parse_requestline(char *buffer, PARSE_STATUS &checkstate, http::HttpRequest &request);
        static HTTP_CODE parse_headers(char *buffer, PARSE_STATUS &checkstate, http::HttpRequest &request);
        static HTTP_CODE parse_body(char *buffer, int body_length, http::HttpRequest &request);
        static HTTP_CODE parse_content(char *buffer,
                                       int &check_index,
                                       int &read_index,
                                       PARSE_STATUS &parse_state,
                                       int &start_line,
                                       http::HttpRequest &request);
    };
}
