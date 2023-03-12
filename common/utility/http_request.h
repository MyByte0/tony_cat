#ifndef COMMON_HTTP_REQUEST_H_
#define COMMON_HTTP_REQUEST_H_

#include "common/core_define.h"

#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

TONY_CAT_SPACE_BEGIN

namespace Http {

struct Request {
    enum class Method {
        kInvalid,
        kGet,
        kPost,
        kHead,
        kPut,
        kDelete
    };
    enum class Version {
        kUnknown,
        kHttp10,
        kHttp11,
        kHttp20
    };

    Method method = Method::kInvalid;
    Version version = Version::kUnknown;
    std::string path;
    std::string query;
    std::unordered_map<std::string, std::string> headers;
    std::string content;

    void Reset()
    {
        method = Method::kInvalid;
        version = Version::kUnknown;
        path.clear();
        query.clear();
        headers.clear();
        content.clear();
    }

    void LoadPathAndQuery(std::string_view word)
    {
        size_t pos_query = word.find("?");
        if (pos_query != std::string_view::npos) {
            path = word.substr(0, pos_query);
            query = word.substr(pos_query + 1);
        }
        else {
            path = word;
        }
    }

    static Version StringToVersion(std::string_view data)
    {
        if (data == "HTTP/1.1") {
            return Version::kHttp11;
        }
        else if (data == "HTTP/1.0") {
            return Version::kHttp10;
        }
        else if (data == "HTTP/2.0") {
            return Version::kHttp20;
        }
        return Version::kUnknown;
    }

    static Method StringToMethod(std::string_view data)
    {
        if (data == "GET") {
            return Method::kGet;
        }
        else if (data == "POST") {
            return Method::kPost;
        }
        else if (data == "HEAD") {
            return Method::kHead;
        }
        else if (data == "PUT") {
            return Method::kPut;
        }
        else if (data == "DELETE") {
            return Method::kDelete;
        }
        return Method::kInvalid;
    }
};

struct Reply {
    /// The status of the reply.
    enum class HttpStatusCode {
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
    };

    HttpStatusCode statusCode;
    std::string statusMessage;
    std::unordered_map<std::string, std::string> headers;
    std::string content;
    bool closeConnection = true;

    static Reply BadReply(HttpStatusCode status);
    static std::string ToErrorHead(HttpStatusCode status);
};

struct RequestParser {
private:
    enum PaserState {
        method_uri_version,
        http_header,
        http_context,
        end
    };

    enum : int32_t {
        k_http_max_size = 1 * 1024 * 1024,   // default 1MB max
    };
    PaserState m_state = PaserState::method_uri_version;
    Request m_req;
    size_t m_contentLength = 0;
    size_t m_readLength = 0;
    std::string m_readingLine;

    size_t ParseBody(std::string_view data);
    bool ParseLine(std::string_view data);
    bool OnReadBody();
    bool OnDone();

public:
    enum class ResultType {
        good,
        bad,
        indeterminate
    };

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
        m_state = PaserState::method_uri_version;
        m_req.Reset();
        m_contentLength = 0;
        m_readLength = 0;
        m_readingLine.clear();
    }
};

}

TONY_CAT_SPACE_END

#endif // COMMON_HTTP_REQUEST_H_
