#include "net_accepter.h"

TONY_CAT_SPACE_BEGIN

Acceptor::Acceptor() { }
Acceptor::~Acceptor()
{
    if (nullptr != m_acceptor) {
        m_acceptor->cancel();
        delete m_acceptor;
    }
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
