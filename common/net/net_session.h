#ifndef NET_NET_SESSION_H_
#define NET_NET_SESSION_H_

#include "common/core_define.h"
#include "common/net/net_buffer.h"

#include <cstdint>
#include <memory>

#include <asio.hpp>

TONY_CAT_SPACE_BEGIN

class Session
    : public std::enable_shared_from_this<Session> {
public:
    typedef int64_t session_id_t;
    friend class NetModule;

    typedef std::function<void(session_id_t)> FunSessionClose;
    typedef std::function<void(session_id_t, bool)> FunSessionConnect;
    typedef std::function<bool(session_id_t, SessionBuffer&)> FunSessionRead;

public:
    Session(asio::io_context& io_context, session_id_t session_id);
    ~Session();

    void AcceptInitialize();

    void SetSessionCloseCb(const FunSessionClose& funOnSessionClose);
    void SetSessionCloseCb(FunSessionClose&& funOnSessionClose);

    void SetSessionReadCb(const FunSessionRead& funOnSessionRead);
    void SetSessionReadCb(FunSessionRead&& funOnSessionRead);

    session_id_t GetSessionId();
    asio::ip::tcp::socket& GetSocket();
    void HandleRead();

    bool WriteData(const char* data, size_t length);
    bool WriteData(const char* dataHead, size_t lenHead, const char* data, size_t length);

    asio::io_context& GetSocketIocontext();

private:
    void DoConnect(asio::ip::tcp::endpoint& address, const FunSessionConnect& funFunSessionConnect = nullptr);
    void DoRead();
    void DoWrite();
    void AsyncClose();

private:
    void DoClose();

    asio::io_context& m_io_context;
    asio::ip::tcp::socket m_socket;

    SessionBuffer m_ReadBuff;
    SessionBuffer m_WriteBuff;

    session_id_t m_sessionId;
    FunSessionClose m_funSessionClose;
    FunSessionRead m_funSessionRead;
};

typedef std::shared_ptr<Session> SessionPtr;

TONY_CAT_SPACE_END

#endif // NET_NET_SESSION_H_
