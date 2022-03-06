#ifndef COMMON_NET_NET_ACCEPTER_H_
#define COMMON_NET_NET_ACCEPTER_H_

#include "common/core_define.h"
#include "common/net/net_session.h"

#include <cstdint>

#include <asio.hpp>

TONY_CAT_SPACE_BEGIN

class Acceptor {
public:
    Acceptor();
    ~Acceptor();

public:
    void Reset(asio::io_context& ioContext, const std::string& Ip, uint32_t port, const Session::FunSessionRead& funSessionRead);
    void OnAccept(Session& session, const std::function<void(std::error_code ec)>& func);
    void OnAccept(Session& session, std::function<void(std::error_code ec)>&& func);
    const Session::FunSessionRead& GetFunSessionRead();

private:
    asio::ip::tcp::acceptor* m_acceptor = nullptr;
    Session::FunSessionRead m_funSessionRead = nullptr;
};

TONY_CAT_SPACE_END

#endif // COMMON_NET_NET_ACCEPTER_H_
