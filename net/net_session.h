#ifndef NET_NET_SESSION_H_
#define NET_NET_SESSION_H_

#include <bit>
#include <cstdint>
#include <memory>
#include <vector>

#include "asio.hpp"

#include "common/core_define.h"

TONY_CAT_SPACE_BEGIN

struct SessionBuffer {
private:
    struct BufferContext {
        std::vector<char> vecData;
        size_t nReadPos;
        size_t nWritePos;
    };
    size_t nMaxBuffSize;
    BufferContext bufContext;

public:
    enum {
        k_init_buffer_size_div_number = 16,
        k_default_max_buffer_size = 8 * 1024 * 4096, // 8MB
    };

    SessionBuffer(size_t maxBuffSize = k_default_max_buffer_size);
    ~SessionBuffer();

    bool Empty();
    bool Full();
    const char* GetReadData();

    size_t GetReadableSize();
    void RemoveData(size_t len);
    bool FillData(size_t len);
    char* GetWriteData();
    bool Write(const char* data, size_t len);
    size_t CurrentWritableSize();
    size_t MaxWritableSize();
    bool Shrink();

private:
    void ReorganizeData();
};

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

#endif // NET_NET_SESSION_H_
