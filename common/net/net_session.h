#ifndef COMMON_NET_NET_SESSION_H_
#define COMMON_NET_NET_SESSION_H_

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
    typedef std::function<void(void* protoContext)> FunSessionProtoContextClose;

public:
    Session(asio::io_context& io_context, session_id_t session_id);
    ~Session();

    void AcceptInitialize();

    void SetSessionCloseCb(const FunSessionClose& funOnSessionClose);
    void SetSessionCloseCb(FunSessionClose&& funOnSessionClose);

    void SetSessionReadCb(const FunSessionRead& funOnSessionRead);
    void SetSessionReadCb(FunSessionRead&& funOnSessionRead);

    void SetSessionProtoContext(void* pProtoContext,
        const FunSessionProtoContextClose& funSessionProtoContextClose);

    session_id_t GetSessionId();
    asio::ip::tcp::socket& GetSocket();
    void HandleRead();

    bool WriteAppend(const std::string& data);
    bool WriteAppend(const char* data, size_t length);
    bool WriteData(const char* data, size_t length);
    bool WriteData(const char* dataHead, size_t lenHead, const char* data, size_t length);
    void DoWrite();

    asio::io_context& GetSocketIocontext();
    void* GetProtoContext();

private:
    void DoConnect(asio::ip::tcp::endpoint& address, const FunSessionConnect& funFunSessionConnect = nullptr);
    void DoRead();
    void AsyncClose();

private:
    void DoClose();

    asio::io_context& m_io_context;
    asio::ip::tcp::socket m_socket;

    SessionBuffer m_buffRead;
    SessionBuffer m_buffWrite;

    session_id_t m_nSessionId;
    FunSessionClose m_funSessionClose;
    FunSessionRead m_funSessionRead;

    void* m_pProtoContext = nullptr;
    FunSessionProtoContextClose m_funSessionProtoContextClose;
};

typedef std::shared_ptr<Session> SessionPtr;

TONY_CAT_SPACE_END

#endif // COMMON_NET_NET_SESSION_H_
