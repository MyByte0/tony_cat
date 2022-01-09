#include "net_session.h"

TONY_CAT_SPACE_BEGIN

SessionBuffer::SessionBuffer(size_t maxBuffSize)
{
    nMaxBuffSize = maxBuffSize;
    size_t nDefaultBuffSize = nMaxBuffSize / k_init_buffer_size_div_number;
    bufContext.vecData.resize(nDefaultBuffSize, 0);
    bufContext.nReadPos = 0;
    bufContext.nWritePos = 0;
}

SessionBuffer::~SessionBuffer() {};

bool SessionBuffer::Empty()
{
    return bufContext.nReadPos == bufContext.nWritePos;
}

bool SessionBuffer::Full()
{
    return bufContext.nWritePos - bufContext.nReadPos == bufContext.vecData.size();
}

const char* SessionBuffer::GetReadData()
{
    return &bufContext.vecData[bufContext.nReadPos];
}

size_t SessionBuffer::GetReadableSize()
{
    return bufContext.nWritePos - bufContext.nReadPos;
}

void SessionBuffer::RemoveData(size_t len)
{
    if (len > GetReadableSize()) [[unlikely]] {
        len = GetReadableSize();
    }

    bufContext.nReadPos += len;
    if (bufContext.nReadPos == bufContext.nWritePos) {
        bufContext.nReadPos = bufContext.nWritePos = 0;
    }
}

bool SessionBuffer::FillData(size_t len)
{
    if (len > MaxWritableSize()) [[unlikely]] {
        return false;
    }

    if (len > CurrentWritableSize()) {
        size_t beforeSize = bufContext.vecData.size();
        size_t curSize = beforeSize;

        ReorganizeData();
        while (len > curSize - bufContext.nWritePos) {
            curSize = std::min(curSize << 2, nMaxBuffSize);
        }

        bufContext.vecData.resize(curSize);
    }

    bufContext.nWritePos += len;
    return true;
}

char* SessionBuffer::GetWriteData()
{
    return &bufContext.vecData[bufContext.nWritePos];
}

bool SessionBuffer::Write(const char* data, size_t len)
{
    if (false == FillData(len)) {
        return false;
    }

    size_t writePos = bufContext.nWritePos - len;
    std::copy(data, data + len, bufContext.vecData.begin() + writePos);
    return true;
}

size_t SessionBuffer::CurrentWritableSize()
{
    return bufContext.vecData.size() - bufContext.nWritePos;
}

size_t SessionBuffer::MaxWritableSize()
{
    return nMaxBuffSize - GetReadableSize();
}

void SessionBuffer::ReorganizeData()
{
    if (bufContext.nReadPos > 0) {
        std::memmove(bufContext.vecData.data(), GetReadData(), GetReadableSize());
        bufContext.nWritePos -= bufContext.nReadPos;
        bufContext.nReadPos = 0;
    }
}

bool SessionBuffer::Shrink()
{
    size_t nDefaultBuffSize = nMaxBuffSize / k_init_buffer_size_div_number;
    if (GetReadableSize() >= nDefaultBuffSize) {
        return false;
    }

    std::vector<char> vecBuff(nDefaultBuffSize);
    std::memmove(vecBuff.data(), GetReadData(), GetReadableSize());
    bufContext.nWritePos -= bufContext.nReadPos;
    bufContext.nReadPos = 0;
    bufContext.vecData.swap(vecBuff);
    return true;
}

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

Acceptor::Acceptor() { }
Acceptor::~Acceptor()
{
    delete m_acceptor;
};

void Acceptor::Reset(asio::io_context& ioContext, const std::string& Ip, uint32_t port, const Session::FunSessionRead& funSessionRead)
{
    if (m_acceptor != nullptr) {
        delete m_acceptor;
        m_acceptor = nullptr;
    }

    m_funSessionRead = funSessionRead;
    m_acceptor = new asio::ip::tcp::acceptor(
        ioContext,
        asio::ip::tcp::endpoint(asio::ip::make_address(Ip), port));
    //asio::ip::tcp::acceptor::send_buffer_size opt_send_buffer_size(102400);
    //asio::ip::tcp::acceptor::receive_buffer_size opt_recv_buffer_size(102400);
    //m_acceptor->set_option(opt_send_buffer_size);
    //m_acceptor->set_option(opt_recv_buffer_size);
}

void Acceptor::OnAccept(Session& session, const std::function<void(std::error_code ec)>& func)
{
    m_acceptor->async_accept(session.GetSocket(), func);
}
void Acceptor::OnAccept(Session& session, std::function<void(std::error_code ec)>&& func)
{
    m_acceptor->async_accept(session.GetSocket(), std::move(func));
}

const Session::FunSessionRead& Acceptor::GetFunSessionRead()
{
    return m_funSessionRead;
}

TONY_CAT_SPACE_END
