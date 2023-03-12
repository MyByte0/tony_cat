#include "http_request.h"

#include <cstdlib>
#include <ranges>

TONY_CAT_SPACE_BEGIN

namespace Http {

Reply Reply::BadReply(HttpStatusCode status)
{
    Reply ret;
    ret.statusCode = status;
    return ret;
}

std::string Reply::ToErrorHead(Reply::HttpStatusCode status)
{
    switch (status) {
    case Reply::HttpStatusCode::ok:
        return "HTTP/1.1 200 OK\r\n";
    case Reply::HttpStatusCode::created:
        return "HTTP/1.1 201 Created\r\n";
    case Reply::HttpStatusCode::accepted:
        return "HTTP/1.1 202 Accepted\r\n";
    case Reply::HttpStatusCode::no_content:
        return "HTTP/1.1 204 No Content\r\n";
    case Reply::HttpStatusCode::multiple_choices:
        return "HTTP/1.1 300 Multiple Choices\r\n";
    case Reply::HttpStatusCode::moved_permanently:
        return "HTTP/1.1 301 Moved Permanently\r\n";
    case Reply::HttpStatusCode::moved_temporarily:
        return "HTTP/1.1 302 Moved Temporarily\r\n";
    case Reply::HttpStatusCode::not_modified:
        return "HTTP/1.1 304 Not Modified\r\n";
    case Reply::HttpStatusCode::bad_request:
        return "HTTP/1.1 400 Bad Request\r\n";
    case Reply::HttpStatusCode::unauthorized:
        return "HTTP/1.1 401 Unauthorized\r\n";
    case Reply::HttpStatusCode::forbidden:
        return "HTTP/1.1 403 Forbidden\r\n";
    case Reply::HttpStatusCode::not_found:
        return "HTTP/1.1 404 Not Found\r\n";
    case Reply::HttpStatusCode::internal_server_error:
        return "HTTP/1.1 500 Internal Server Error\r\n";
    case Reply::HttpStatusCode::not_implemented:
        return "HTTP/1.1 501 Not Implemented\r\n";
    case Reply::HttpStatusCode::bad_gateway:
        return "HTTP/1.1 502 Bad Gateway\r\n";
    case Reply::HttpStatusCode::service_unavailable:
        return "HTTP/1.1 503 Service Unavailable\r\n";
    default:
        return "HTTP/1.1 500 Internal Server Error\r\n";
    }
}

static bool IsChar(int c)
{
    return c >= 0 && c <= 127;
}

static bool IsCtl(int c)
{
    return (c >= 0 && c <= 31) || (c == 127);
}

static bool IsTspecial(int c)
{
    switch (c) {
    case '(':
    case ')':
    case '<':
    case '>':
    case '@':
    case ',':
    case ';':
    case ':':
    case '\\':
    case '"':
    case '/':
    case '[':
    case ']':
    case '?':
    case '=':
    case '{':
    case '}':
    case ' ':
    case '\t':
        return true;
    default:
        return false;
    }
}

static bool IsDigit(int c)
{
    return c >= '0' && c <= '9';
}

// return still need reading data lenth
size_t RequestParser::ParseBody(std::string_view data)
{
    auto needReadCount = m_contentLength - m_req.content.length();
    if (data.length() >= needReadCount) {
        m_req.content.append(data.substr(0, needReadCount));
        m_state = PaserState::end;
        return 0;
    }
   
    m_req.content.append(data);
    return needReadCount - data.size();
}

bool RequestParser::ParseLine(std::string_view line)
{
    switch (m_state) {
        case PaserState::method_uri_version: {
            size_t i_word = 0;
            size_t pos = 0;
            // get method
            pos = line.find(" ");
            m_req.method = Request::StringToMethod(line.substr(0, pos));
            line = line.substr(pos+1);
            // get uri
            pos = line.find(" ");
            m_req.LoadPathAndQuery(line.substr(0, pos));
            line = line.substr(pos+1);
            // get version
            pos = line.find(" ");
            m_req.version = Request::StringToVersion(line.substr(0, pos));
            if (m_req.version == Request::Version::kUnknown) {
                return false;
            }
            m_state = PaserState::http_header;
            break;
        }
        case PaserState::http_header: {
            if (line.empty()) {
                if (auto it = m_req.headers.find("Content-Length"); it != m_req.headers.end()) {
                    m_contentLength = ::atoll(it->second.c_str());
                    m_state = PaserState::http_context;
                }
                else {
                    m_state = PaserState::end;
                }
                break;
            }

            // Split the line into key and value
            auto pos = line.find(": ");
            std::string_view key = line.substr(0, pos);
            std::string_view value = line.substr(pos + 2);
            // Add the key-value pair to the map
            m_req.headers.emplace(key, value);
            break;
        }
        case PaserState::http_context:
            return false;
        case PaserState::end:
            return false;
        default:
            return false;
        }
        return true;
}

bool RequestParser::OnReadBody()
{
    return m_state == PaserState::http_context;
}

bool RequestParser::OnDone()
{
    return m_state == PaserState::end;
}

std::tuple<RequestParser::ResultType, std::size_t> RequestParser::Parse(
    const char* data, std::size_t length)
{
    if (m_readLength >= k_http_max_size) {
        return std::make_tuple(ResultType::bad, length);
    }
    std::size_t readableLength = std::min(length, k_http_max_size-m_readLength);
    std::string_view request(data, readableLength);
    // Split the request into lines
    size_t pos = 0;
    while (!OnDone() && !OnReadBody() && (pos = request.find("\r\n")) != std::string_view::npos) {
        std::string_view line;
        if (!m_readingLine.empty()) {
            // only call on the first loop
            m_readingLine.append(data, pos);
            line = m_readingLine;
        } else {
            line = request.substr(0, pos);
        }
        
        if (ParseLine(line) == false) {
            return std::make_tuple(ResultType::bad, pos);
        }
        request = request.substr(pos + 2);
        if (!m_readingLine.empty()) {
            m_readingLine.clear();
        }
    }

    if (OnReadBody()) {
        auto next_start = ParseBody(request);
        // need read remain data
        if (next_start >= request.length()) {
            m_readLength += readableLength;
            return std::make_tuple(ResultType::indeterminate, readableLength);
        }
        request = request.substr(next_start);
    }

    if (OnDone()) {
        m_readLength += readableLength - request.length();
        return std::make_tuple(ResultType::good, readableLength - request.length());
    }

    m_readingLine.append(request);
    m_readLength += readableLength;
    return std::make_tuple(ResultType::indeterminate, readableLength);
}

}

TONY_CAT_SPACE_END
