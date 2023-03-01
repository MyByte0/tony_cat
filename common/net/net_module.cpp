#include "net_module.h"

#include "common/log/log_module.h"
#include "common/module_manager.h"

TONY_CAT_SPACE_BEGIN

THREAD_LOCAL_VAR std::unordered_map<Session::session_id_t, SessionPtr> NetModule::t_mapSessionInNetLoop 
    = std::unordered_map<Session::session_id_t, SessionPtr>();

NetModule::NetModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
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
    SetNetThreadNum(static_cast<uint32_t>(netThreadNum));
    m_poolSessionContext.Start(netThreadNum);
}

void NetModule::AfterStop()
{
    for (auto& elemMapSession : m_mapSession) {
        DoSessionAsyncClose(elemMapSession.second);
    }

    m_poolSessionContext.Broadcast([]() {
        t_mapSessionInNetLoop.clear();
    });

    m_poolSessionContext.Stop();

    for (auto pAcceptor : m_vecAcceptors) {
        delete pAcceptor;
    }
    m_vecAcceptors.clear();
    m_loopAccpet.Stop();
}

Session::session_id_t NetModule::CreateSessionId()
{
    auto session_id = ++m_nextSessionId;
    return session_id;
}

SessionPtr NetModule::GetSessionInMainLoop(Session::session_id_t sessionId)
{
    assert(m_pModuleManager->OnMainLoop());
    if (auto itMapSession = m_mapSession.find(sessionId); itMapSession != m_mapSession.end()) {
        return itMapSession->second;
    }

    return nullptr;
}

SessionPtr NetModule::GetSessionInNetLoop(Session::session_id_t sessionId)
{
    if (auto itMapSession = t_mapSessionInNetLoop.find(sessionId); itMapSession != t_mapSessionInNetLoop.end())
    {
        return itMapSession->second;
    }

    return nullptr;
}

void NetModule::RemoveInSessionMap(Session::session_id_t sessionId)
{
    m_pModuleManager->GetMainLoop().Exec(
        [this, sessionId]() {
            LOG_TRACE("close net connect, session_id:{}", sessionId);
            m_mapSession.erase(sessionId);

            auto& context = m_poolSessionContext.GetIoContext(sessionId);
            context.post([sessionId]() { t_mapSessionInNetLoop.erase(sessionId); });
        });
}

bool NetModule::OnReadSession(Acceptor* pAcceptor, Session::session_id_t sessionId, SessionBuffer& buff)
{
    auto pSession = GetSessionInNetLoop(sessionId);
    if (nullptr == pSession) {
        LOG_ERROR("can not find session info, sessionId:{}", sessionId);
        return true;
    }
    if (nullptr == pAcceptor->GetFunSessionRead()
        || ((pAcceptor->GetFunSessionRead())(sessionId, buff)) == false) {
        LOG_INFO("read data disconnect, sessionId:{}", sessionId);
        DoSessionAsyncClose(pSession);
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
    pSession->SetSessionReadCb(std::bind(&NetModule::OnReadSession, this, pAcceptor, std::placeholders::_1, std::placeholders::_2));
    pAcceptor->OnAccept(*pSession,
        [this, pAcceptor, pSession](std::error_code ec) {
            if (!ec) {
                pSession->AcceptInitialize();
                pSession->GetSocketIocontext().post([this, pSession] {
                    t_mapSessionInNetLoop.emplace(pSession->GetSessionId(), pSession);

                    m_pModuleManager->GetMainLoop().Exec([this, pSession]() {
                        uint32_t port = pSession->GetSocket().remote_endpoint().port();
                        LOG_INFO("accpet net connect, session_id:{} address:{}:{}",
                            pSession->GetSessionId(),
                            pSession->GetSocket().remote_endpoint().address().to_string(),
                            port);
                        m_mapSession.emplace(pSession->GetSessionId(), pSession);
                        DoSessionRead(pSession);
                    });
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

    auto& loop = Loop::GetCurrentThreadLoop();
    auto pSession = std::make_shared<Session>(context, session_id);
    pSession->SetSessionReadCb(funOnSessionRead);
    // ensure call CloseCb in current loop
    pSession->SetSessionCloseCb([this, funOnSessionClose, &loop](auto nSessionId) {
        loop.Exec([this, funOnSessionClose, nSessionId]() {
            if (nullptr != funOnSessionClose) {
                funOnSessionClose(nSessionId);
            }
        });
    });

    pSession->GetSocketIocontext().post([this, pSession, &loop, endpoints, funOnSessionConnect(std::move(funOnSessionConnect))] () mutable{
        t_mapSessionInNetLoop.emplace(pSession->GetSessionId(), pSession);

        m_pModuleManager->GetMainLoop().Exec([this, pSession, &loop, endpoints, funOnSessionConnect(std::move(funOnSessionConnect))] () mutable {
            m_mapSession.emplace(pSession->GetSessionId(), pSession);
            // ensure call ConnectCb in current loop
            if (nullptr != funOnSessionConnect) {
                DoSessionConnect(pSession, endpoints, [this, funOnSessionConnect, &loop](auto nSessionId, auto bSuccess) mutable {
                    loop.Exec([this, funOnSessionConnect, nSessionId, bSuccess]() {
                        funOnSessionConnect(nSessionId, bSuccess);
                        if (!bSuccess) {
                            this->RemoveInSessionMap(nSessionId);
                        }
                    });
                });
            } else {
                DoSessionConnect(pSession, endpoints, nullptr);
            }
        });
    });

    return session_id;
}

void NetModule::CloseInMainLoop(Session::session_id_t session_id)
{
    if (auto itMapSession = m_mapSession.find(session_id); itMapSession != m_mapSession.end()) {
        DoSessionAsyncClose(itMapSession->second);
    }
    return;
}

void NetModule::HandleSessionRead(SessionPtr pSession)
{
    if (pSession->m_funSessionRead) [[likely]] {
        if (false == pSession->m_funSessionRead(pSession->GetSessionId(), pSession->m_buffRead)) {
            LOG_ERROR("read packet error on seesion: {}", pSession->GetSessionId());
            DoSessionAsyncClose(pSession);
        }
    }
}

void NetModule::DoSessionConnect(SessionPtr pSession, asio::ip::tcp::endpoint& address, const Session::FunSessionConnect& funFunSessionConnect /* = nullptr*/)
{
    pSession->GetSocket().async_connect(address,
        [this, pSession, funFunSessionConnect](std::error_code ec) {
            if (!ec) {
                if (nullptr != funFunSessionConnect) {
                    funFunSessionConnect(pSession->m_nSessionId, true);
                }
                DoSessionRead(pSession);
            } else {
                if (nullptr != funFunSessionConnect) {
                    funFunSessionConnect(pSession->m_nSessionId, false);
                }
            }
        });
}

void NetModule::DoSessionRead(SessionPtr pSession)
{
    pSession->GetSocket().async_read_some(
        asio::buffer(pSession->m_buffRead.GetWriteData(),
            pSession->m_buffRead.CurrentWritableSize()),
        [this, pSession](std::error_code ec, std::size_t length) {
            if (!ec) {
                pSession->m_buffRead.FillData(length);
                HandleSessionRead(pSession);

                DoSessionRead(pSession);
            } else {
                DoSessionAsyncClose(pSession);
            }
        });
}

bool NetModule::DoSessionWrite(SessionPtr pSession)
{
    pSession->GetSocket().async_write_some(
        asio::buffer(pSession->m_buffWrite.GetReadData(), pSession->m_buffWrite.GetReadableSize()),
        [this, pSession](std::error_code ec, std::size_t length) {
            if (!ec) {
                m_pModuleManager->GetMainLoop().Exec([this, pSession, length]() { 
                    pSession->m_buffWrite.RemoveData(length);
                    if (pSession->m_buffWrite.GetReadableSize() > 0) {
                    DoSessionWrite(pSession);
                } });
            } else {
                DoSessionAsyncClose(pSession);
            }
        });

    return true;
}

void NetModule::DoSessionAsyncClose(SessionPtr pSession)
{
    pSession->GetSocketIocontext().post(
        [this, pSession]() {
            DoSessionCloseInNetLoop(pSession);
        });
}

void NetModule::DoSessionCloseInNetLoop(SessionPtr pSession)
{
    if (pSession->GetSocket().is_open()) {
        try {
            pSession->GetSocket().shutdown(asio::socket_base::shutdown_both);
        } catch (...) {
            LOG_ERROR("shutdown fail on sessionId:{}", pSession->GetSessionId());
        }
        
        try {
            pSession->GetSocket().close();
        } catch (...) {
            LOG_ERROR("close fail on sessionId:{}", pSession->GetSessionId());
        }
    }

    LOG_TRACE("close session on sessionId:{}", pSession->GetSessionId());
    if (nullptr != pSession->m_funSessionProtoContextClose) {
        pSession->m_funSessionProtoContextClose(pSession->m_pProtoContext);
        pSession->m_funSessionProtoContextClose = nullptr;
    }
    if (nullptr != pSession->m_funSessionClose) {
        pSession->m_funSessionClose(pSession->m_nSessionId);
        pSession->m_funSessionClose = nullptr;
    }

    RemoveInSessionMap(pSession->m_nSessionId);
}

TONY_CAT_SPACE_END
