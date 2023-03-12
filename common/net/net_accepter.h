#ifndef COMMON_NET_NET_ACCEPTER_H_
#define COMMON_NET_NET_ACCEPTER_H_

#include "common/core_define.h"
#include "common/loop/loop.h"
#include "common/loop/loop_pool.h"
#include "common/net/net_session.h"

#include <cstdint>
#include <memory>

#include <asio.hpp>

TONY_CAT_SPACE_BEGIN


class Acceptor {
public:
    Acceptor();
    ~Acceptor();

public:
    void Init(LoopPoolPtr workLoops, LoopPtr listenLoop, const std::string& Ip, uint32_t port, const Session::FunSessionRead& funSessionRead);
    void OnAccept(Session& session, const std::function<void(std::error_code ec)>& func);
    void OnAccept(Session& session, std::function<void(std::error_code ec)>&& func);
    const Session::FunSessionRead& GetFunSessionRead() {
        return m_funSessionRead;
    }
    LoopPoolPtr GetWorkLoop() {
        return m_workLoop;
    }
    LoopPtr GetListenLoop() {
        return m_listenLoop;
    }

private:
    asio::ip::tcp::acceptor* m_acceptor = nullptr;
    Session::FunSessionRead m_funSessionRead = nullptr;
    LoopPoolPtr m_workLoop = nullptr;
    LoopPtr m_listenLoop = nullptr;
};

typedef std::shared_ptr<Acceptor> AcceptorPtr;

TONY_CAT_SPACE_END

#endif // COMMON_NET_NET_ACCEPTER_H_
