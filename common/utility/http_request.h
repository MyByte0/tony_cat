#ifndef COMMON_HTTP_REQUEST_H_
#define COMMON_HTTP_REQUEST_H_

#include "common/core_define.h"

#include <string>
#include <tuple>
#include <vector>

TONY_CAT_SPACE_BEGIN

namespace Http {

struct Header {
    std::string name;
    std::string value;
};

struct Request {
    std::string method;
    std::string uri;
    int http_version_major;
    int http_version_minor;
    std::vector<Header> headers;
    std::string content;

    void Reset()
    {
        method.clear();
        uri.clear();
        headers.clear();
    }
};

struct Reply {
    /// The status of the reply.
    enum StatusType {
        ok = 200,
        created = 201,
        accepted = 202,
        no_content = 204,
        multiple_choices = 300,
        moved_permanently = 301,
        moved_temporarily = 302,
        not_modified = 304,
        bad_request = 400,
        unauthorized = 401,
        forbidden = 403,
        not_found = 404,
        internal_server_error = 500,
        not_implemented = 501,
        bad_gateway = 502,
        service_unavailable = 503
    } status;

    std::vector<Header> headers;

    std::string content;

    static Reply BadReply(StatusType status);

    static std::string ToErrorBody(StatusType status);
    static std::string ToReplyString(StatusType status);
};

class RequestParser {

    enum class PaserState {
        method_start,
        method,
        uri,
        http_version_h,
        http_version_t_1,
        http_version_t_2,
        http_version_p,
        http_version_slash,
        http_version_major_start,
        http_version_major,
        http_version_minor_start,
        http_version_minor,
        expecting_newline_1,
        header_line_start,
        header_lws,
        header_name,
        space_before_header_value,
        header_value,
        expecting_newline_2,
        expecting_newline_3
    } m_state;
    Request m_req;

public:
    enum ResultType { good,
        bad,
        indeterminate };

    std::tuple<ResultType, std::size_t> Parse(
        const char* data, std::size_t length);

    Request& GetResult()
    {
        return m_req;
    }
    Request&& FetchResult()
    {
        return std::move(m_req);
    }
    void Reset()
    {
        m_state = PaserState::method_start;
        m_req.Reset();
    }
};

}

TONY_CAT_SPACE_END

#endif // COMMON_HTTP_REQUEST_H_
