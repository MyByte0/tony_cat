#include "net_session.h"

TONY_CAT_SPACE_BEGIN

Session::Session(asio::io_context& io_context, session_id_t session_id)
    : m_io_context(io_context)
    , m_socket(io_context)
    , m_sessionId(session_id)
{
}

Session::~Session() { }

void Session::AcceptInitialize()
{
    asio::socket_base::linger lingerOpt(false, 0);
    m_socket.set_option(lingerOpt);
}

void Session::SetSessionCloseCb(const FunSessionClose& funOnSessionClose)
{
    m_funSessionClose = funOnSessionClose;
}
void Session::SetSessionCloseCb(FunSessionClose&& funOnSessionClose)
{
    m_funSessionClose = std::move(funOnSessionClose);
}

void Session::SetSessionReadCb(const FunSessionRead& funOnSessionRead)
{
    m_funSessionRead = funOnSessionRead;
}
void Session::SetSessionReadCb(FunSessionRead&& funOnSessionRead)
{
    m_funSessionRead = std::move(funOnSessionRead);
}

Session::session_id_t Session::GetSessionId()
{
    return m_sessionId;
}

asio::ip::tcp::socket& Session::GetSocket()
{
    return m_socket;
}

void Session::HandleRead()
{
    if (m_funSessionRead) [[likely]] {
        m_funSessionRead(m_sessionId, m_ReadBuff);
    }
}

bool Session::WriteData(const char* data, size_t length)
{
    m_WriteBuff.Write((char*)data, length);

    DoWrite();
    return true;
}

bool Session::WriteData(const char* dataHead, size_t lenHead, const char* data, size_t length)
{
    if (lenHead + length > m_WriteBuff.MaxWritableSize()) [[unlikely]] {
        return false;
    }

    m_WriteBuff.Write((char*)dataHead, lenHead);
    m_WriteBuff.Write((char*)data, length);

    DoWrite();
    return true;
}

asio::io_context& Session::GetSocketIocontext()
{
    return m_io_context;
}

void Session::DoConnect(asio::ip::tcp::endpoint& address, const FunSessionConnect& funFunSessionConnect /* = nullptr*/)
{
    auto self(shared_from_this());
    GetSocket().async_connect(address,
        [self, funFunSessionConnect](std::error_code ec) {
            if (!ec) {
                if (nullptr != funFunSessionConnect) {
                    funFunSessionConnect(self->m_sessionId, true);
                }
                self->DoRead();
            } else {
                if (nullptr != funFunSessionConnect) {
                    funFunSessionConnect(self->m_sessionId, false);
                }
                self->DoClose();
            }
        });
}

void Session::DoRead()
{
    auto self(shared_from_this());
    GetSocket().async_read_some(
        asio::buffer(m_ReadBuff.GetWriteData(),
            m_ReadBuff.CurrentWritableSize()),
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                m_ReadBuff.FillData(length);
                HandleRead();

                self->DoRead();
            } else {
                self->DoClose();
            }
        });
}

void Session::DoWrite()
{
    auto self(shared_from_this());
    GetSocket().async_write_some(
        asio::buffer(m_WriteBuff.GetReadData(), m_WriteBuff.GetReadableSize()),
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                m_WriteBuff.RemoveData(length);
                if (m_WriteBuff.GetReadableSize() > 0) {
                    self->DoWrite();
                }
            } else {
                self->DoClose();
            }
        });
}

void Session::AsyncClose()
{
    auto self(shared_from_this());
    GetSocketIocontext().post(
        [this, self]() {
            self->DoClose();
        });
}

void Session::DoClose()
{
    if (GetSocket().is_open()) {
        GetSocket().shutdown(asio::socket_base::shutdown_both);
        GetSocket().close();
    }

    if (nullptr != m_funSessionClose) {
        m_funSessionClose(m_sessionId);
    }
}

TONY_CAT_SPACE_END
