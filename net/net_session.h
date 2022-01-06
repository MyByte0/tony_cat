#ifndef NET_NET_SESSION_H_
#define NET_NET_SESSION_H_

#include <bit>
#include <cstdint>
#include <memory>
#include <vector>

#include "asio.hpp"

#include "common/core_define.h"

SER_NAME_SPACE_BEGIN

struct SessionBuffer{
private:
    struct BufferContext {
        std::vector<char> data_;
        size_t readPos;
        size_t writPos;
    };
    BufferContext context_;

public:
    enum
    {
        k_default_buffer_size = 32 * 4096,
        k_max_buffer_size = 1024 * 4096,
    };

    SessionBuffer() {
        context_.data_.resize(k_default_buffer_size, 0);
        context_.readPos = 0;
        context_.writPos = 0;
    }
    ~SessionBuffer() = default;

    bool empty() {
      return context_.readPos == context_.writPos;
    }

    bool full() {
      return context_.writPos - context_.readPos == context_.data_.size();
    }

    const char* GetReadData() {
        return &context_.data_[context_.readPos];
    }

    size_t GetReadableSize() {
        return context_.writPos - context_.readPos;
    }

    void RemoveData(size_t len) {
        if (len > GetReadableSize()) [[unlikely]] {
            len = GetReadableSize();
        }

        context_.readPos += len;
        if (context_.readPos == context_.writPos) {
            context_.readPos = context_.writPos = 0;
        }
    }

    bool FillData(size_t len) {
        if (len > MaxWritableSize()) [[unlikely]] {
            return false;
        }

        if (len > CurrentWritableSize()) {
            size_t beforeSize = context_.data_.size();
            size_t curSize = beforeSize;

            ReorganizeData();
            while (len > curSize - context_.writPos) {
                curSize = std::min(curSize << 2
                    , size_t(k_max_buffer_size));
            }

            context_.data_.resize(curSize);
        }

        context_.writPos += len;
        return true;
    }

    char* GetWriteData() {
        return &context_.data_[context_.writPos];
    }

    bool Write(const char* data, size_t len) {
        if (false == FillData(len)) {
            return false;
        }

        size_t writePos = context_.writPos - len;
        std::copy(data, data + len, context_.data_.begin() + writePos);
        return true;
    }

    size_t CurrentWritableSize() {
        return context_.data_.size() - context_.writPos;
    }

    size_t MaxWritableSize() {
        return k_max_buffer_size - GetReadableSize();
    }

private:
    void ReorganizeData() {
        if (context_.readPos > 0) {
            std::memmove(context_.data_.data(), GetReadData(), GetReadableSize());
            context_.writPos -= context_.readPos;
            context_.readPos = 0;
        }
    }
};

class Session
    : public std::enable_shared_from_this<Session>
{
public:
    typedef int64_t session_id_t;
    friend class NetModule;

    typedef std::function<void(session_id_t)> FunSessionClose;
    typedef std::function<void(SessionBuffer&)> FunSessionRead;

public:
    Session(asio::io_context& io_context, session_id_t session_id)
     : m_io_context(io_context),
       m_socket(io_context),
       m_sessionId(session_id)
    {
    }

    void AcceptInitialize() {
      asio::socket_base::linger lingerOpt(false, 0);
      m_socket.set_option(lingerOpt);
    }

    void OnSessionClose(const FunSessionClose& funOnSessionClose) {
      m_funSessionClose = funOnSessionClose;
    }
    void OnSessionClose(FunSessionClose&& funOnSessionClose) {
        m_funSessionClose = std::move(funOnSessionClose);
    }

    void OnSessionRead(const FunSessionRead& funOnSessionRead) {
        m_funSessionRead = funOnSessionRead;
    }
    void OnSessionRead(FunSessionRead&& funOnSessionRead) {
        m_funSessionRead = std::move(funOnSessionRead);
    }

    session_id_t GetSessionId() {
        return m_sessionId;
    }

    asio::ip::tcp::socket& GetSocket() { 
        return m_socket;
    }

    void HandleRead() {
        if (m_funSessionRead) [[likely]] {
            m_funSessionRead(m_ReadBuff);
        }
    }

    bool WriteData(const char* data, size_t length) { 
        m_WriteBuff.Write((char*)data, length);

        DoWrite();
        return true;
    }

    bool WriteData(const char* dataHead, size_t lenHead, const char* data, size_t length) {
        if (lenHead + length > m_WriteBuff.MaxWritableSize()) [[unlikely]] {
            return false;
        }

        m_WriteBuff.Write((char*)dataHead, lenHead);
        m_WriteBuff.Write((char*)data, length);

        DoWrite();
        return true;
    }

    asio::io_context& GetSocketIocontext() {
      return m_io_context;
    }

private:
    void DoConnect(asio::ip::tcp::endpoint& address) {
      auto self(shared_from_this());
      GetSocket().async_connect(address,
          [self](std::error_code ec) {
            if (!ec) {
              self->DoRead();
            }else{
              self->DoClose();
            }
          });
    }

    void DoRead() {
        auto self(shared_from_this());
        GetSocket().async_read_some(
          asio::buffer( m_ReadBuff.GetWriteData(),
                        m_ReadBuff.CurrentWritableSize()),
            [this, self](std::error_code ec, std::size_t length)
            {
                if (!ec) {
                    m_ReadBuff.FillData(length);
                    HandleRead();
                    
                    self->DoRead();
                }
                else {
                    self->DoClose();
                }
            });
    }

    void DoWrite() {
        auto self(shared_from_this());
        GetSocket().async_write_some(
            asio::buffer(m_WriteBuff.GetReadData(), m_WriteBuff.GetReadableSize()),
            [this, self](std::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    m_WriteBuff.RemoveData(length);
                    if (m_WriteBuff.GetReadableSize() > 0) {
                        self->DoWrite();
                    }
                }
                else {
                    self->DoClose();
                }
            });
    }

    void AsyncClose() {
      auto self(shared_from_this());
      GetSocketIocontext().post(
          [this, self]() {
        self->DoClose();
      });
    }

private:
    void DoClose() {
        if (GetSocket().is_open()) {
          GetSocket().shutdown(asio::socket_base::shutdown_both);
          GetSocket().close();
        }

        if (nullptr != m_funSessionClose) {
          m_funSessionClose(m_sessionId);
        }
    }

    asio::io_context& m_io_context;
    asio::ip::tcp::socket m_socket;
    enum {
        standard_read_buff_length = 512 * 1024,
        max_read_buff_length = 4096*1024 
    };
    enum {
        standard_write_buff_length = 512 * 1024,
        max_write_buff_length = 4096 * 1024
    };
    SessionBuffer m_ReadBuff;
    SessionBuffer m_WriteBuff;

    session_id_t m_sessionId;
    FunSessionClose m_funSessionClose;
    FunSessionRead  m_funSessionRead;
};

typedef std::shared_ptr<Session> SessionPtr;


class Acceptor
{
public:
    Acceptor() = default;
    ~Acceptor() {
        delete m_acceptor;
    };

public:
    void Reset(asio::io_context& ioContext, const std::string& Ip, uint32_t port) {
        if (m_acceptor != nullptr) {
            delete m_acceptor;
            m_acceptor = nullptr;
        }

        m_acceptor = new asio::ip::tcp::acceptor(
            ioContext,
            asio::ip::tcp::endpoint(asio::ip::make_address(Ip), port));
    }

    void OnAccept(Session& session, const std::function<void(std::error_code ec)>& func) {
        m_acceptor->async_accept(session.GetSocket(), func);
    }
    void OnAccept(Session& session, std::function<void(std::error_code ec)>&& func) {
        m_acceptor->async_accept(session.GetSocket(), std::move(func));
    }

private:
    asio::ip::tcp::acceptor* m_acceptor = nullptr;
};

SER_NAME_SPACE_END

#endif // NET_NET_SESSION_H_
