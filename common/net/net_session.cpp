#include "net_session.h"

#include "common/log/log_module.h"

TONY_CAT_SPACE_BEGIN

Session::Session(asio::io_context& io_context, session_id_t session_id)
    : m_io_context(io_context)
    , m_socket(io_context)
    , m_nSessionId(session_id)
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
    return m_nSessionId;
}

asio::ip::tcp::socket& Session::GetSocket()
{
    return m_socket;
}

void Session::HandleRead()
{
    if (m_funSessionRead) [[likely]] {
        m_funSessionRead(m_nSessionId, m_buffRead);
    }
}

bool Session::WriteData(const char* data, size_t length)
{
    m_buffWrite.Write((char*)data, length);

    DoWrite();
    return true;
}

bool Session::WriteData(const char* dataHead, size_t lenHead, const char* data, size_t length)
{
    if (lenHead + length > m_buffWrite.MaxWritableSize()) [[unlikely]] {
        return false;
    }

    m_buffWrite.Write((char*)dataHead, lenHead);
    m_buffWrite.Write((char*)data, length);

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
                    funFunSessionConnect(self->m_nSessionId, true);
                }
                self->DoRead();
            } else {
                if (nullptr != funFunSessionConnect) {
                    funFunSessionConnect(self->m_nSessionId, false);
                }
                self->DoClose();
            }
        });
}

void Session::DoRead()
{
    auto self(shared_from_this());
    GetSocket().async_read_some(
        asio::buffer(m_buffRead.GetWriteData(),
            m_buffRead.CurrentWritableSize()),
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                m_buffRead.FillData(length);
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
        asio::buffer(m_buffWrite.GetReadData(), m_buffWrite.GetReadableSize()),
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                m_buffWrite.RemoveData(length);
                if (m_buffWrite.GetReadableSize() > 0) {
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
        try {
            GetSocket().shutdown(asio::socket_base::shutdown_both);
        } catch (...) {
            LOG_ERROR("shutdown fail on sessionId:{}", m_nSessionId);
        }

        try {
            GetSocket().close();
        } catch (...) {
            LOG_ERROR("close fail on sessionId:{}", m_nSessionId);
        }
    }

    if (nullptr != m_funSessionClose) {
        m_funSessionClose(m_nSessionId);
    }
}

TONY_CAT_SPACE_END
