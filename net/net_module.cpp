#include "net_module.h"

#include "net_pb_module.h"
#include "common/module_manager.h"
#include "log/log_module.h"

SER_NAME_SPACE_BEGIN

NetModule::NetModule(ModuleManager* pModuleManager) :
	ModuleBase(pModuleManager) {
	m_nextSessionId = 0;

}

NetModule::~NetModule() {
}

void NetModule::BeforeInit() {
    m_loopAccpet.Start();

    size_t netThreadNum = 4;
    m_poolSessionContext.Start(netThreadNum);
}

void NetModule::AfterStop() {
    for (auto& elemMapSession : m_mapSession)
    {
        elemMapSession.second->AsyncClose();
    }
    m_poolSessionContext.Stop();

    for (auto pAcceptor : m_vecAcceptors)
    {
        delete pAcceptor;
    }
    m_vecAcceptors.clear();
}

Session::session_id_t NetModule::CreateSessionId() {
    auto session_id = ++m_nextSessionId;
    return session_id;
}

SessionPtr NetModule::GetSessionById(Session::session_id_t sessionId) {
    auto itMapSession = m_mapSession.find(sessionId);
    if (itMapSession != m_mapSession.end()) {
        return itMapSession->second;
    }

    return nullptr;
}

void NetModule::OnCloseSession(Session::session_id_t sessionId) {
  m_pModuleManager->GetMainLoop().Exec(
      [this, sessionId]() { 
          LOG_INFO("close net connect, session_id:{}", sessionId);
          m_mapSession.erase(sessionId); 
      });
}

void NetModule::Listen(const std::string strAddress, uint16_t addressPort) {
    Acceptor* pAcceptor = new Acceptor();
    pAcceptor->Reset(
        m_loopAccpet.GetIoContext(),
        strAddress, addressPort);
    m_vecAcceptors.emplace_back(pAcceptor);

    m_loopAccpet.Exec([this, pAcceptor]() { this->Accept(pAcceptor); });
}

void NetModule::Accept(Acceptor* pAcceptor) {
    auto session_id  = CreateSessionId();
    auto& context = m_poolSessionContext.GetIoContext(session_id);

    auto pSession = std::make_shared<Session>(context, session_id);
    pSession->SetSessionCloseCb(
        std::bind(&NetModule::OnCloseSession, this, std::placeholders::_1));
    pSession->SetSessionReadCb([this, pSession](auto& buf) {
            if (nullptr == m_funNetRead || m_funNetRead(pSession->GetSessionId(), buf) == false) {
                pSession->AsyncClose();
            }
        });
    pAcceptor->OnAccept(*pSession,
        [this, pAcceptor, pSession](std::error_code ec)
        {
            if (!ec)
            {
                pSession->AcceptInitialize();
                m_pModuleManager->GetMainLoop().Exec([this, pSession]() {
                  uint32_t port =
                      pSession->GetSocket().remote_endpoint().port();
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

Session::session_id_t NetModule::Connect(const std::string strAddressIP, uint16_t addressPort) {
  auto endpoints = asio::ip::tcp::endpoint(asio::ip::make_address(strAddressIP),
                                           addressPort);
  auto session_id = CreateSessionId();
  auto& context = m_poolSessionContext.GetIoContext(session_id);

  auto pSession = std::make_shared<Session>(context, session_id);
  pSession->SetSessionCloseCb(
      std::bind(&NetModule::OnCloseSession, this, std::placeholders::_1));
  m_mapSession[pSession->GetSessionId()] = pSession;
  pSession->DoConnect(endpoints);
  return session_id;
}

void NetModule::Close(Session::session_id_t session_id) {
  auto itMapSession = m_mapSession.find(session_id);
  if (itMapSession == m_mapSession.end())
  {
    return;
  }

  itMapSession->second->AsyncClose();
}

SER_NAME_SPACE_END
