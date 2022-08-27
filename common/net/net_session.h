#ifndef COMMON_NET_NET_SESSION_H_
#define COMMON_NET_NET_SESSION_H_

#include <asio.hpp>
#include <cstdint>
#include <memory>
#include <string>

#include "common/core_define.h"
#include "common/loop/loop.h"
#include "common/net/net_buffer.h"

TONY_CAT_SPACE_BEGIN

class Session {
public:
    typedef int64_t session_id_t;
    friend class NetModule;

    typedef std::function<void(session_id_t)> FunSessionClose;
    typedef std::function<void(session_id_t, bool)> FunSessionConnect;
    typedef std::function<bool(session_id_t, SessionBuffer&)> FunSessionRead;
    typedef std::function<void(void* protoContext)> FunSessionProtoContextClose;

public:
    Session(Loop& io_context, session_id_t session_id);
    ~Session();

    void AcceptInitialize();

    void SetSessionCloseCb(const FunSessionClose& funOnSessionClose);
    void SetSessionCloseCb(FunSessionClose&& funOnSessionClose);

    void SetSessionReadCb(const FunSessionRead& funOnSessionRead);
    void SetSessionReadCb(FunSessionRead&& funOnSessionRead);

    void SetSessionProtoContext(
        void* pProtoContext,
        const FunSessionProtoContextClose& funSessionProtoContextClose);

    session_id_t GetSessionId();
    asio::ip::tcp::socket& GetSocket();

    char* GetWriteData(size_t len);
    bool MoveWritePos(size_t len);

    bool WriteAppend(const std::string& data);
    bool WriteAppend(const char* data, size_t length);
    bool WriteAppend(const char* dataHead, size_t lenHead, const char* data,
                     size_t length);

    Loop& GetLoop();
    void* GetSessionContext();

private:
    void SetSessionId(Session::session_id_t nSessionId);

private:
    Loop& m_loopIO;
    asio::ip::tcp::socket m_socket;

    SessionBuffer m_buffRead;
    SessionBuffer m_buffWrite;
    bool m_onWriting = false;

    session_id_t m_nSessionId;
    FunSessionClose m_funSessionClose;
    FunSessionRead m_funSessionRead;

    Loop::TimerHandle m_timerSessionAlive = nullptr;

    void* m_pSessionContext = nullptr;
    FunSessionProtoContextClose m_funSessionProtoContextClose;
};

typedef std::shared_ptr<Session> SessionPtr;

TONY_CAT_SPACE_END

#endif  // COMMON_NET_NET_SESSION_H_
