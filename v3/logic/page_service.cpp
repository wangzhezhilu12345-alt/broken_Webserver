#include "page_service.h"

#include <cctype>
#include <sstream>
#include <sys/stat.h>
#include <unordered_map>

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

    serverlogic::PageResult BuildInlineHtmlResult(http::Httpresponse::HttpStatusCode status_code,
                                                  const std::string &status_message,
                                                  const std::string &body)
    {
        serverlogic::PageResult result;
        result._status_code = status_code;
        result._status_message = status_message;
        result._body = body;
        result._mime = http::MimeType("text/html");
        return result;
    }

    std::string HtmlEscape(const std::string &input)
    {
        std::string escaped;
        escaped.reserve(input.size());
        for (size_t i = 0; i < input.size(); ++i)
        {
            switch (input[i])
            {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '\'':
                escaped += "&#39;";
                break;
            default:
                escaped.push_back(input[i]);
                break;
            }
        }
        return escaped;
    }

    int HexValue(char ch)
    {
        if (ch >= '0' && ch <= '9')
        {
            return ch - '0';
        }
        if (ch >= 'a' && ch <= 'f')
        {
            return ch - 'a' + 10;
        }
        if (ch >= 'A' && ch <= 'F')
        {
            return ch - 'A' + 10;
        }
        return -1;
    }

    std::string UrlDecode(const std::string &input)
    {
        std::string decoded;
        decoded.reserve(input.size());
        for (size_t i = 0; i < input.size(); ++i)
        {
            if (input[i] == '+')
            {
                decoded.push_back(' ');
                continue;
            }
            if (input[i] == '%' && i + 2 < input.size())
            {
                int high = HexValue(input[i + 1]);
                int low = HexValue(input[i + 2]);
                if (high >= 0 && low >= 0)
                {
                    decoded.push_back(static_cast<char>((high << 4) | low));
                    i += 2;
                    continue;
                }
            }
            decoded.push_back(input[i]);
        }
        return decoded;
    }

    std::unordered_map<std::string, std::string> ParseFormBody(const std::string &body)
    {
        std::unordered_map<std::string, std::string> values;
        std::stringstream stream(body);
        std::string pair;
        while (std::getline(stream, pair, '&'))
        {
            const std::string::size_type pos = pair.find('=');
            const std::string key = UrlDecode(pair.substr(0, pos));
            const std::string value = pos == std::string::npos ? std::string() : UrlDecode(pair.substr(pos + 1));
            values[key] = value;
        }
        return values;
    }

    std::string BuildLoginResultPage(const std::string &headline,
                                     const std::string &message,
                                     const std::string &username,
                                     const std::string &tone)
    {
        const std::string safe_username = HtmlEscape(username);
        const std::string safe_message = HtmlEscape(message);

        std::string accent = "#2e7d32";
        std::string accent_soft = "rgba(46, 125, 50, 0.12)";
        if (tone == "error")
        {
            accent = "#b23a48";
            accent_soft = "rgba(178, 58, 72, 0.12)";
        }

        std::string html;
        html += "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">";
        html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
        html += "<title>Login Result</title><style>";
        html += "body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;";
        html += "background:linear-gradient(135deg,#f7efe6,#eadfd1);font-family:Segoe UI,sans-serif;color:#1f2937;padding:24px;}";
        html += ".card{width:min(520px,100%);background:#fffaf5;border-radius:24px;padding:32px;box-shadow:0 24px 60px rgba(0,0,0,.12);}";
        html += ".tag{display:inline-block;padding:8px 12px;border-radius:999px;background:" + accent_soft + ";color:" + accent + ";font-weight:700;font-size:12px;letter-spacing:.08em;}";
        html += "h1{margin:16px 0 12px;font-size:36px;}p{line-height:1.7;color:#4b5563;}strong{color:#111827;}a{display:inline-block;margin-top:18px;padding:12px 18px;border-radius:14px;background:";
        html += accent + ";color:#fff;text-decoration:none;font-weight:700;}</style></head><body><main class=\"card\">";
        html += "<span class=\"tag\">BROKEN SERVER LOGIN</span>";
        html += "<h1>" + HtmlEscape(headline) + "</h1>";
        html += "<p><strong>User:</strong> " + safe_username + "</p>";
        html += "<p>" + safe_message + "</p>";
        html += "<a href=\"/\">Back</a>";
        html += "</main></body></html>";
        return html;
    }
}

serverlogic::PageService::PageService()
    : _mysql_store()
{
}

bool serverlogic::PageService::Init()
{
    return _mysql_store.Init();
}

serverlogic::PageResult serverlogic::PageService::Resolve(const http::HttpRequest &request, const char *filebase) const
{
    const std::string base_path(filebase);
    std::string filepath = request._Url;
    if (filepath.empty())
    {
        filepath = "/";
    }

    std::string::size_type pos = filepath.rfind('?');
    if (pos != std::string::npos)
    {
        filepath.erase(pos);
    }

    if (request._Method != http::HttpRequest::GET && request._Method != http::HttpRequest::POST)
    {
        return BuildInlineHtmlResult(http::Httpresponse::MethodNotAllowed405,
                                     "Method Not Allowed",
                                     BuildLoginResultPage("Method not allowed",
                                                          "Only GET and POST are supported in this demo.",
                                                          "guest",
                                                          "error"));
    }

    if (filepath == "/login")
    {
        if (request._Method == http::HttpRequest::GET)
        {
            filepath = "/index.html";
        }
        else
        {
            std::unordered_map<http::HttpRequest::HTTP_HEADER, std::string, http::HttpRequest::ENUMHASH>::const_iterator header_it =
                request._Header.find(http::HttpRequest::Content_Type);
            if (header_it != request._Header.end() &&
                header_it->second.find("application/x-www-form-urlencoded") == std::string::npos)
            {
                return BuildInlineHtmlResult(http::Httpresponse::BadRequest400,
                                             "Bad Request",
                                             BuildLoginResultPage("Unsupported form",
                                                                  "The login form must use application/x-www-form-urlencoded.",
                                                                  "guest",
                                                                  "error"));
            }

            const std::unordered_map<std::string, std::string> form = ParseFormBody(request._Body);
            const std::string username = form.count("username") == 0 ? std::string() : form.find("username")->second;
            const std::string password = form.count("password") == 0 ? std::string() : form.find("password")->second;
            const serverlogic::LoginCheckResult login_result = _mysql_store.LoginOrCreate(username, password);

            if (login_result.status == serverlogic::LoginCheckResult::Success)
            {
                return BuildInlineHtmlResult(http::Httpresponse::OK,
                                             "OK",
                                             BuildLoginResultPage("Welcome back",
                                                                  login_result.message,
                                                                  username,
                                                                  "success"));
            }
            if (login_result.status == serverlogic::LoginCheckResult::Created)
            {
                return BuildInlineHtmlResult(http::Httpresponse::OK,
                                             "OK",
                                             BuildLoginResultPage("Account created",
                                                                  login_result.message,
                                                                  username,
                                                                  "success"));
            }
            if (login_result.status == serverlogic::LoginCheckResult::WrongPassword)
            {
                return BuildInlineHtmlResult(http::Httpresponse::Unauthorized401,
                                             "Unauthorized",
                                             BuildLoginResultPage("Login failed",
                                                                  login_result.message,
                                                                  username,
                                                                  "error"));
            }
            if (login_result.status == serverlogic::LoginCheckResult::InvalidInput)
            {
                return BuildInlineHtmlResult(http::Httpresponse::BadRequest400,
                                             "Bad Request",
                                             BuildLoginResultPage("Invalid input",
                                                                  login_result.message,
                                                                  username,
                                                                  "error"));
            }

            return BuildInlineHtmlResult(http::Httpresponse::InternalServerError500,
                                         "Internal Server Error",
                                         BuildLoginResultPage("Database error",
                                                              login_result.message.empty() ? "Please check MySQL connection settings." : login_result.message,
                                                              username,
                                                              "error"));
        }
    }

    if (request._Method == http::HttpRequest::POST)
    {
        return BuildInlineHtmlResult(http::Httpresponse::NOTFOUND,
                                     "Not Found",
                                     BuildLoginResultPage("Unknown route",
                                                          "This POST endpoint does not exist.",
                                                          "guest",
                                                          "error"));
    }

    if (filepath == "/")
    {
        filepath = "/index.html";
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

    serverlogic::PageResult result;
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
