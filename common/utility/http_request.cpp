#include "http_request.h"

TONY_CAT_SPACE_BEGIN

namespace Http {

const std::string s_err_ok = "HTTP/1.1 200 OK\r\n";
const std::string s_err_created = "HTTP/1.1 201 Created\r\n";
const std::string s_err_accepted = "HTTP/1.1 202 Accepted\r\n";
const std::string s_err_no_content = "HTTP/1.1 204 No Content\r\n";
const std::string s_err_multiple_choices = "HTTP/1.1 300 Multiple Choices\r\n";
const std::string s_err_moved_permanently = "HTTP/1.1 301 Moved Permanently\r\n";
const std::string s_err_moved_temporarily = "HTTP/1.1 302 Moved Temporarily\r\n";
const std::string s_err_not_modified = "HTTP/1.1 304 Not Modified\r\n";
const std::string s_err_bad_request = "HTTP/1.1 400 Bad Request\r\n";
const std::string s_err_unauthorized = "HTTP/1.1 401 Unauthorized\r\n";
const std::string s_err_forbidden = "HTTP/1.1 403 Forbidden\r\n";
const std::string s_err_not_found = "HTTP/1.1 404 Not Found\r\n";
const std::string s_err_internal_server_error = "HTTP/1.1 500 Internal Server Error\r\n";
const std::string s_err_not_implemented = "HTTP/1.1 501 Not Implemented\r\n";
const std::string s_err_bad_gateway = "HTTP/1.1 502 Bad Gateway\r\n";
const std::string s_err_service_unavailable = "HTTP/1.1 503 Service Unavailable\r\n";

std::string Reply::ToErrorBody(Reply::StatusType status)
{
    switch (status) {
    case Reply::ok:
        return s_err_ok;
    case Reply::created:
        return s_err_created;
    case Reply::accepted:
        return s_err_accepted;
    case Reply::no_content:
        return s_err_no_content;
    case Reply::multiple_choices:
        return s_err_multiple_choices;
    case Reply::moved_permanently:
        return s_err_moved_permanently;
    case Reply::moved_temporarily:
        return s_err_moved_temporarily;
    case Reply::not_modified:
        return s_err_not_modified;
    case Reply::bad_request:
        return s_err_bad_request;
    case Reply::unauthorized:
        return s_err_unauthorized;
    case Reply::forbidden:
        return s_err_forbidden;
    case Reply::not_found:
        return s_err_not_found;
    case Reply::internal_server_error:
        return s_err_internal_server_error;
    case Reply::not_implemented:
        return s_err_not_implemented;
    case Reply::bad_gateway:
        return s_err_bad_gateway;
    case Reply::service_unavailable:
        return s_err_service_unavailable;
    default:
        return s_err_internal_server_error;
    }
}

const std::string s_ok = "";
const std::string s_created = "<html>"
                              "<head><title>Created</title></head>"
                              "<body><h1>201 Created</h1></body>"
                              "</html>";
const std::string s_accepted = "<html>"
                               "<head><title>Accepted</title></head>"
                               "<body><h1>202 Accepted</h1></body>"
                               "</html>";
const std::string s_no_content = "<html>"
                                 "<head><title>No Content</title></head>"
                                 "<body><h1>204 Content</h1></body>"
                                 "</html>";
const std::string s_multiple_choices = "<html>"
                                       "<head><title>Multiple Choices</title></head>"
                                       "<body><h1>300 Multiple Choices</h1></body>"
                                       "</html>";
const std::string s_moved_permanently = "<html>"
                                        "<head><title>Moved Permanently</title></head>"
                                        "<body><h1>301 Moved Permanently</h1></body>"
                                        "</html>";
const std::string s_moved_temporarily = "<html>"
                                        "<head><title>Moved Temporarily</title></head>"
                                        "<body><h1>302 Moved Temporarily</h1></body>"
                                        "</html>";
const std::string s_not_modified = "<html>"
                                   "<head><title>Not Modified</title></head>"
                                   "<body><h1>304 Not Modified</h1></body>"
                                   "</html>";
const std::string s_bad_request = "<html>"
                                  "<head><title>Bad Request</title></head>"
                                  "<body><h1>400 Bad Request</h1></body>"
                                  "</html>";
const std::string s_unauthorized = "<html>"
                                   "<head><title>Unauthorized</title></head>"
                                   "<body><h1>401 Unauthorized</h1></body>"
                                   "</html>";
const std::string s_forbidden = "<html>"
                                "<head><title>Forbidden</title></head>"
                                "<body><h1>403 Forbidden</h1></body>"
                                "</html>";
const std::string s_not_found = "<html>"
                                "<head><title>Not Found</title></head>"
                                "<body><h1>404 Not Found</h1></body>"
                                "</html>";
const std::string s_internal_server_error = "<html>"
                                            "<head><title>Internal Server Error</title></head>"
                                            "<body><h1>500 Internal Server Error</h1></body>"
                                            "</html>";
const std::string s_not_implemented = "<html>"
                                      "<head><title>Not Implemented</title></head>"
                                      "<body><h1>501 Not Implemented</h1></body>"
                                      "</html>";
const std::string s_bad_gateway = "<html>"
                                  "<head><title>Bad Gateway</title></head>"
                                  "<body><h1>502 Bad Gateway</h1></body>"
                                  "</html>";
const std::string s_service_unavailable = "<html>"
                                          "<head><title>Service Unavailable</title></head>"
                                          "<body><h1>503 Service Unavailable</h1></body>"
                                          "</html>";

std::string Reply::ToReplyString(Reply::StatusType status)
{
    switch (status) {
    case Reply::ok:
        return s_ok;
    case Reply::created:
        return s_created;
    case Reply::accepted:
        return s_accepted;
    case Reply::no_content:
        return s_no_content;
    case Reply::multiple_choices:
        return s_multiple_choices;
    case Reply::moved_permanently:
        return s_moved_permanently;
    case Reply::moved_temporarily:
        return s_moved_temporarily;
    case Reply::not_modified:
        return s_not_modified;
    case Reply::bad_request:
        return s_bad_request;
    case Reply::unauthorized:
        return s_unauthorized;
    case Reply::forbidden:
        return s_forbidden;
    case Reply::not_found:
        return s_not_found;
    case Reply::internal_server_error:
        return s_internal_server_error;
    case Reply::not_implemented:
        return s_not_implemented;
    case Reply::bad_gateway:
        return s_bad_gateway;
    case Reply::service_unavailable:
        return s_service_unavailable;
    default:
        return s_internal_server_error;
    }
}

Reply Reply::BadReply(Reply::StatusType status)
{
    Reply rep;
    rep.status = status;
    rep.content = ToReplyString(status);
    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = std::to_string(rep.content.size());
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = "text/html";
    return rep;
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

std::tuple<RequestParser::ResultType, std::size_t> RequestParser::Parse(
    const char* data, std::size_t length)
{
    const char* current = data;
    const char* end = data + length;
    for (; current != end; ++current) {
        RequestParser::ResultType result;
        switch (m_state) {
        case PaserState::method_start:
            if (!IsChar(*current) || IsCtl(*current) || IsTspecial(*current)) {
                result = bad;
                continue;
            } else {
                m_state = PaserState::method;
                m_req.method.push_back(*current);
                result = indeterminate;
                continue;
            }
        case PaserState::method:
            if (*current == ' ') {
                m_state = PaserState::uri;
                result = indeterminate;
                continue;
            } else if (!IsChar(*current) || IsCtl(*current) || IsTspecial(*current)) {
                result = bad;
                continue;
            } else {
                m_req.method.push_back(*current);
                result = indeterminate;
                continue;
            }
        case PaserState::uri:
            if (*current == ' ') {
                m_state = PaserState::http_version_h;
                result = indeterminate;
                continue;
            } else if (IsCtl(*current)) {
                result = bad;
                continue;
            } else {
                m_req.uri.push_back(*current);
                result = indeterminate;
                continue;
            }
        case PaserState::http_version_h:
            if (*current == 'H') {
                m_state = PaserState::http_version_t_1;
                result = indeterminate;
                continue;
            } else {
                result = bad;
                continue;
            }
        case PaserState::http_version_t_1:
            if (*current == 'T') {
                m_state = PaserState::http_version_t_2;
                result = indeterminate;
                continue;
            } else {
                result = bad;
                continue;
            }
        case PaserState::http_version_t_2:
            if (*current == 'T') {
                m_state = PaserState::http_version_p;
                result = indeterminate;
                continue;
            } else {
                result = bad;
                continue;
            }
        case PaserState::http_version_p:
            if (*current == 'P') {
                m_state = PaserState::http_version_slash;
                result = indeterminate;
                continue;
            } else {
                result = bad;
                continue;
            }
        case PaserState::http_version_slash:
            if (*current == '/') {
                m_req.http_version_major = 0;
                m_req.http_version_minor = 0;
                m_state = PaserState::http_version_major_start;
                result = indeterminate;
                continue;
            } else {
                result = bad;
                continue;
            }
        case PaserState::http_version_major_start:
            if (IsDigit(*current)) {
                m_req.http_version_major = m_req.http_version_major * 10 + *current - '0';
                m_state = PaserState::http_version_major;
                result = indeterminate;
                continue;
            } else {
                result = bad;
                continue;
            }
        case PaserState::http_version_major:
            if (*current == '.') {
                m_state = PaserState::http_version_minor_start;
                result = indeterminate;
                continue;
            } else if (IsDigit(*current)) {
                m_req.http_version_major = m_req.http_version_major * 10 + *current - '0';
                result = indeterminate;
                continue;
            } else {
                result = bad;
                continue;
            }
        case PaserState::http_version_minor_start:
            if (IsDigit(*current)) {
                m_req.http_version_minor = m_req.http_version_minor * 10 + *current - '0';
                m_state = PaserState::http_version_minor;
                result = indeterminate;
                continue;
            } else {
                result = bad;
                continue;
            }
        case PaserState::http_version_minor:
            if (*current == '\r') {
                m_state = PaserState::expecting_newline_1;
                result = indeterminate;
                continue;
            } else if (IsDigit(*current)) {
                m_req.http_version_minor = m_req.http_version_minor * 10 + *current - '0';
                result = indeterminate;
                continue;
            } else {
                result = bad;
                continue;
            }
        case PaserState::expecting_newline_1:
            if (*current == '\n') {
                m_state = PaserState::header_line_start;
                result = indeterminate;
                continue;
            } else {
                result = bad;
                continue;
            }
        case PaserState::header_line_start:
            if (*current == '\r') {
                m_state = PaserState::expecting_newline_3;
                result = indeterminate;
                continue;
            } else if (!m_req.headers.empty() && (*current == ' ' || *current == '\t')) {
                m_state = PaserState::header_lws;
                result = indeterminate;
                continue;
            } else if (!IsChar(*current) || IsCtl(*current) || IsTspecial(*current)) {
                result = bad;
                continue;
            } else {
                m_req.headers.push_back(Header());
                m_req.headers.back().name.push_back(*current);
                m_state = PaserState::header_name;
                result = indeterminate;
                continue;
            }
        case PaserState::header_lws:
            if (*current == '\r') {
                m_state = PaserState::expecting_newline_2;
                result = indeterminate;
                continue;
            } else if (*current == ' ' || *current == '\t') {
                result = indeterminate;
                continue;
            } else if (IsCtl(*current)) {
                result = bad;
                continue;
            } else {
                m_state = PaserState::header_value;
                m_req.headers.back().value.push_back(*current);
                result = indeterminate;
                continue;
            }
        case PaserState::header_name:
            if (*current == ':') {
                m_state = PaserState::space_before_header_value;
                result = indeterminate;
                continue;
            } else if (!IsChar(*current) || IsCtl(*current) || IsTspecial(*current)) {
                result = bad;
                continue;
            } else {
                m_req.headers.back().name.push_back(*current);
                result = indeterminate;
                continue;
            }
        case PaserState::space_before_header_value:
            if (*current == ' ') {
                m_state = PaserState::header_value;
                result = indeterminate;
                continue;
            } else {
                result = bad;
                continue;
            }
        case PaserState::header_value:
            if (*current == '\r') {
                m_state = PaserState::expecting_newline_2;
                result = indeterminate;
                continue;
            } else if (IsCtl(*current)) {
                result = bad;
                continue;
            } else {
                m_req.headers.back().value.push_back(*current);
                result = indeterminate;
                continue;
            }
        case PaserState::expecting_newline_2:
            if (*current == '\n') {
                m_state = PaserState::header_line_start;
                result = indeterminate;
                continue;
            } else {
                result = bad;
                continue;
            }
        case PaserState::expecting_newline_3:
            result = (*current == '\n') ? good : bad;
            continue;
        default:
            result = bad;
            continue;
        }

        if (result == good || result == bad)
            return std::make_tuple(result, current + 1 - data);
    }
    return std::make_tuple(indeterminate, current - data);
}

}

TONY_CAT_SPACE_END
