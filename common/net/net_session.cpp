#include "net_session.h"

#include "common/log/log_module.h"

#include <utility>

TONY_CAT_SPACE_BEGIN

Session::Session(Loop& loop, session_id_t session_id)
    : m_loopIO(loop)
    , m_socket(loop.GetIoContext())
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

void Session::SetSessionProtoContext(void* pProtoContext,
    const FunSessionProtoContextClose& funSessionProtoContextClose)
{
    assert(m_funSessionProtoContextClose == nullptr);
    if (nullptr != m_funSessionProtoContextClose) {
        m_funSessionProtoContextClose(m_pSessionContext);
    }
    m_pSessionContext = pProtoContext;
    m_funSessionProtoContextClose = funSessionProtoContextClose;
}

Session::session_id_t Session::GetSessionId()
{
    return m_nSessionId;
}

asio::ip::tcp::socket& Session::GetSocket()
{
    return m_socket;
}

void* Session::GetSessionContext()
{
    return m_pSessionContext;
}

bool Session::WriteAppend(const std::string& data)
{
    return m_buffWrite.Write(data.c_str(), data.length());
}

bool Session::WriteAppend(const char* data, size_t length)
{
    return m_buffWrite.Write((char*)data, length);
}

bool Session::WriteAppend(const char* dataHead, size_t lenHead, const char* data, size_t length)
{
    if (lenHead + length > m_buffWrite.MaxWritableSize()) [[unlikely]] {
        return false;
    }

    m_buffWrite.Write((char*)dataHead, lenHead);
    m_buffWrite.Write((char*)data, length);

    return true;
}

Loop& Session::GetLoop()
{
    return m_loopIO;
}

TONY_CAT_SPACE_END
