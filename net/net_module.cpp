#include "net_module.h"

#include "common/module_manager.h"
#include "log/log_module.h"
#include "net_pb_module.h"

TONY_CAT_SPACE_BEGIN

NetModule::NetModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
    m_nextSessionId = 0;
}

NetModule::~NetModule()
{
}

void NetModule::BeforeInit()
{
    m_loopAccpet.Start();
}

void NetModule::OnInit()
{
    size_t netThreadNum = GetNetThreadNum();
    m_poolSessionContext.Start(netThreadNum);
}

void NetModule::AfterStop()
{
    for (auto& elemMapSession : m_mapSession) {
        elemMapSession.second->AsyncClose();
    }
    m_poolSessionContext.Stop();

    for (auto pAcceptor : m_vecAcceptors) {
        delete pAcceptor;
    }
    m_vecAcceptors.clear();
}

Session::session_id_t NetModule::CreateSessionId()
{
    auto session_id = ++m_nextSessionId;
    return session_id;
}

SessionPtr NetModule::GetSessionById(Session::session_id_t sessionId)
{
    auto itMapSession = m_mapSession.find(sessionId);
    if (itMapSession != m_mapSession.end()) {
        return itMapSession->second;
    }

    return nullptr;
}

void NetModule::OnCloseSession(Session::session_id_t sessionId)
{
    m_pModuleManager->GetMainLoop().Exec(
        [this, sessionId]() {
            LOG_INFO("close net connect, session_id:{}", sessionId);
            m_mapSession.erase(sessionId);
        });
}

bool NetModule::OnReadSession(Acceptor* pAcceptor, Session::session_id_t sessionId, SessionBuffer& buff)
{
    auto pSession = GetSessionById(sessionId);
    if (nullptr == pSession) {
        LOG_ERROR("can not find session info, sessionId:{}", sessionId);
        return true;
    }
    if (nullptr == pAcceptor->GetFunSessionRead()
        || ((pAcceptor->GetFunSessionRead())(sessionId, buff)) == false) {
        LOG_INFO("read data disconnect, sessionId:{}", sessionId);
        pSession->AsyncClose();
    }
    return true;
}

void NetModule::Listen(const std::string& strAddress, uint16_t addressPort, const FunNetRead& funNetRead)
{
    Acceptor* pAcceptor = new Acceptor();
    pAcceptor->Reset(
        m_loopAccpet.GetIoContext(),
        strAddress, addressPort, funNetRead);
    m_vecAcceptors.emplace_back(pAcceptor);

    m_loopAccpet.Exec([this, pAcceptor]() { this->Accept(pAcceptor); });
}

void NetModule::Accept(Acceptor* pAcceptor)
{
    auto session_id = CreateSessionId();
    auto& context = m_poolSessionContext.GetIoContext(session_id);

    auto pSession = std::make_shared<Session>(context, session_id);
    pSession->SetSessionCloseCb(
        std::bind(&NetModule::OnCloseSession, this, std::placeholders::_1));
    pSession->SetSessionReadCb(std::bind(&NetModule::OnReadSession, this, pAcceptor, std::placeholders::_1, std::placeholders::_2));
    pAcceptor->OnAccept(*pSession,
        [this, pAcceptor, pSession](std::error_code ec) {
            if (!ec) {
                pSession->AcceptInitialize();
                m_pModuleManager->GetMainLoop().Exec([this, pSession]() {
                    uint32_t port = pSession->GetSocket().remote_endpoint().port();
                    LOG_INFO("accpet net connect, session_id:{} address:{}:{}",
                        pSession->GetSessionId(),
                        pSession->GetSocket().remote_endpoint().address().to_string(),
                        port);
                    m_mapSession[pSession->GetSessionId()] = pSession;
                    pSession->DoRead();
                });
            }

            Accept(pAcceptor);
        });
}

Session::session_id_t NetModule::Connect(const std::string& strAddressIP, uint16_t addressPort,
    const Session::FunSessionRead& funOnSessionRead,
    const Session::FunSessionConnect& funOnSessionConnect /* = nullptr*/,
    const Session::FunSessionClose& funOnSessionClose /* = nullptr*/)
{
    auto endpoints = asio::ip::tcp::endpoint(asio::ip::make_address(strAddressIP),
        addressPort);
    auto session_id = CreateSessionId();
    auto& context = m_poolSessionContext.GetIoContext(session_id);

    auto pSession = std::make_shared<Session>(context, session_id);
    pSession->SetSessionCloseCb([this, funOnSessionClose](auto nSessionId) {
        if (nullptr != funOnSessionClose) {
            funOnSessionClose(nSessionId);
        }
        this->OnCloseSession(nSessionId);
    });
    pSession->SetSessionReadCb(funOnSessionRead);
    m_mapSession[pSession->GetSessionId()] = pSession;
    pSession->DoConnect(endpoints, funOnSessionConnect);
    return session_id;
}

void NetModule::Close(Session::session_id_t session_id)
{
    auto itMapSession = m_mapSession.find(session_id);
    if (itMapSession == m_mapSession.end()) {
        return;
    }

    itMapSession->second->AsyncClose();
}

TONY_CAT_SPACE_END
