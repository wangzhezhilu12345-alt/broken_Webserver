#include "http_handler.h"

#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <vector>

http::HttpRequestParse::HTTP_CODE http::HttpHandler::ParseRequest(
    const std::shared_ptr<servercore::Connection> &connection) const
{
    MutexlockGuard lock(connection->_mutex);
    return http::HttpRequestParse::parse_content(connection->_buffer.data(),
                                                 connection->_check_index,
                                                 connection->_read_index,
                                                 connection->_parse_state,
                                                 connection->_start_line,
                                                 connection->_request);
}

void http::HttpHandler::BuildResponse(const HttpRequest &request,
                                      const serverlogic::PageResult &page,
                                      Httpresponse &response) const
{
    response._Version = request._Version == HttpRequest::HTTP_10 ? HttpRequest::HTTP_10 : HttpRequest::HTTP_11;
    response._header["Broken"] = "Broken Server";
    response.Statuscode = page._status_code;
    response.statusmsg = page._status_message;
    response.mime = page._mime;
    response._filepath = page._filepath;
    response._body_text = page._body;
}

bool http::HttpHandler::BuildResponseData(const Httpresponse &response, std::string &output) const
{
    std::string header_data;
    header_data += "HTTP/1.1 ";
    header_data += std::to_string(static_cast<int>(response.Statuscode));
    header_data += " ";
    header_data += response.statusmsg;
    header_data += "\r\n";
    header_data += "Connection: close\r\n";
    header_data += "Content-Type: " + response.mime.type + "\r\n";

    if (!response._body_text.empty())
    {
        header_data += "Content-Length: " + std::to_string(response._body_text.size()) + "\r\n";
        header_data += "\r\n";
        output = header_data + response._body_text;
        return true;
    }

    struct stat file_stat;
    if (stat(response._filepath.c_str(), &file_stat) < 0)
    {
        output = BuildSimpleResponseData("404 Not Found", "404 Not Found");
        return true;
    }

    int filefd = ::open(response._filepath.c_str(), O_RDONLY);
    if (filefd < 0)
    {
        output = BuildSimpleResponseData("500 Internal Server Error", "error");
        return true;
    }

    header_data += "Content-Length: " + std::to_string(file_stat.st_size) + "\r\n";
    header_data += "\r\n";

    std::vector<char> body(static_cast<size_t>(file_stat.st_size));
    size_t offset = 0;
    while (offset < body.size())
    {
        ssize_t n = ::read(filefd, &body[offset], body.size() - offset);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            close(filefd);
            output = BuildSimpleResponseData("500 Internal Server Error", "error");
            return true;
        }
        if (n == 0)
        {
            break;
        }
        offset += static_cast<size_t>(n);
    }

    close(filefd);
    if (offset != body.size())
    {
        output = BuildSimpleResponseData("500 Internal Server Error", "error");
        return true;
    }

    output = header_data;
    output.append(body.begin(), body.end());
    return true;
}

std::string http::HttpHandler::BuildSimpleResponseData(const std::string &status_line, const std::string &body) const
{
    std::string response;
    response += "HTTP/1.1 ";
    response += status_line;
    response += "\r\n";
    response += "Connection: close\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "\r\n";
    response += body;
    return response;
}
