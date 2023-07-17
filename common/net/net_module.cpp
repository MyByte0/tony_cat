#include "net_module.h"

#include <utility>

#include "common/log/log_module.h"
#include "common/module_manager.h"

TONY_CAT_SPACE_BEGIN

THREAD_LOCAL_VAR std::unordered_map<Session::session_id_t, SessionPtr>
    NetModule::t_mapSessionInNetLoop =
        std::unordered_map<Session::session_id_t, SessionPtr>();

NetModule::NetModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager) {}

NetModule::~NetModule() {}

void NetModule::BeforeInit() {}

void NetModule::OnInit() {
    if (m_defaultListenLoop != nullptr) {
        m_defaultListenLoop->Start();
    }
}

void NetModule::OnStop() {
    for (auto& elemMapSession : m_mapSession) {
        DoSessionAsyncClose(elemMapSession.second);
    }

    for (auto pAccepter : m_vecAcceptors) {
        pAccepter->GetWorkLoop()->Broadcast(
            []() { t_mapSessionInNetLoop.clear(); });
    }

    m_vecAcceptors.clear();
    if (m_defaultListenLoop) {
        m_defaultListenLoop->Stop();
    }
}

Session::session_id_t NetModule::CreateSessionId() {
    auto session_id = ++m_nextSessionId;
    return session_id;
}

SessionPtr NetModule::GetSessionInMainLoop(Session::session_id_t sessionId) {
    assert(m_pModuleManager->OnMainLoop());
    if (auto itMapSession = m_mapSession.find(sessionId);
        itMapSession != m_mapSession.end()) {
        return itMapSession->second;
    }

    return nullptr;
}

SessionPtr NetModule::GetSessionInNetLoop(Session::session_id_t sessionId) {
    if (auto itMapSession = t_mapSessionInNetLoop.find(sessionId);
        itMapSession != t_mapSessionInNetLoop.end()) {
        assert(&Loop::GetCurrentThreadLoop() ==
               &itMapSession->second->GetLoop());
        return itMapSession->second;
    }

    return nullptr;
}

void NetModule::RemoveMapSession(Session::session_id_t sessionId) {
    m_pModuleManager->GetMainLoop().Exec([this, sessionId]() {
        LOG_TRACE("close net connect, session_id:{}", sessionId);
        auto pSession = GetSessionInMainLoop(sessionId);
        if (pSession != nullptr) {
            m_mapSession.erase(sessionId);
            auto& loop = pSession->GetLoop();
            loop.Exec(
                [sessionId]() { t_mapSessionInNetLoop.erase(sessionId); });
        }
    });
}

bool NetModule::OnReadSession(AcceptorPtr pAcceptor,
                              Session::session_id_t sessionId,
                              SessionBuffer& buff) {
    auto pSession = GetSessionInNetLoop(sessionId);
    if (nullptr == pSession) {
        LOG_ERROR("can not find session info, sessionId:{}", sessionId);
        return true;
    }
    if (nullptr == pAcceptor->GetFunSessionRead() ||
        ((pAcceptor->GetFunSessionRead())(sessionId, buff)) == false) {
        LOG_INFO("read data disconnect, sessionId:{}", sessionId);
        DoSessionAsyncClose(pSession);
    }

    pSession->GetLoop().Cancel(pSession->m_timerSessionAlive);
    pSession->m_timerSessionAlive = nullptr;
    pSession->m_timerSessionAlive = pSession->GetLoop().ExecAfter(
        kDefaultTimeoutMillseconds,
        [this, sessionId]() { this->DoTimerSessionReadTimeout(sessionId); });
    return true;
}

void NetModule::Listen(AcceptorPtr pAcceptor) {
    m_vecAcceptors.emplace_back(pAcceptor);
    pAcceptor->GetListenLoop()->Exec(
        [this, pAcceptor]() { this->Accept(pAcceptor); });
}

void NetModule::Accept(AcceptorPtr pAcceptor) {
    auto session_id = CreateSessionId();
    auto session_loop = pAcceptor->GetWorkLoop()->GetLoop(session_id);
    assert(session_loop != nullptr);

    auto pSession = std::make_shared<Session>(*session_loop, session_id);
    pSession->m_timerSessionAlive = session_loop->ExecAfter(
        kDefaultTimeoutMillseconds,
        [this, session_id]() { this->DoTimerSessionReadTimeout(session_id); });
    pSession->SetSessionReadCb(std::bind(&NetModule::OnReadSession, this,
                                         pAcceptor, std::placeholders::_1,
                                         std::placeholders::_2));
    pAcceptor->OnAccept(*pSession, [this, pAcceptor,
                                    pSession](std::error_code ec) {
        if (!ec) {
            pSession->AcceptInitialize();
            pSession->GetLoop().Exec([this, pSession] {
                t_mapSessionInNetLoop.emplace(pSession->GetSessionId(),
                                              pSession);

                m_pModuleManager->GetMainLoop().Exec([this, pSession]() {
                    uint32_t port =
                        pSession->GetSocket().remote_endpoint().port();
                    LOG_INFO("accpet net connect, session_id:{} address:{}:{}",
                             pSession->GetSessionId(),
                             pSession->GetSocket()
                                 .remote_endpoint()
                                 .address()
                                 .to_string(),
                             port);
                    m_mapSession.emplace(pSession->GetSessionId(), pSession);
                    DoSessionRead(pSession);
                });
            });
        }

        Accept(pAcceptor);
    });
}

void NetModule::Connect(
    const std::string& strAddressIP, uint16_t addressPort,
    const Session::FunSessionRead& funOnSessionRead,
    const Session::FunSessionConnect& funOnSessionConnect /* = nullptr*/,
    const Session::FunSessionClose& funOnSessionClose /* = nullptr*/) {
    auto endpoints = asio::ip::tcp::endpoint(
        asio::ip::make_address(strAddressIP), addressPort);
    auto& loopRun = Loop::GetCurrentThreadLoop();

    auto pSession = std::make_shared<Session>(loopRun, Session::session_id_t(0));
    pSession->SetSessionReadCb(funOnSessionRead);
    // ensure call CloseCb in current loop
    pSession->SetSessionCloseCb(
        [this, funOnSessionClose, &loopRun](auto nSessionId) {
            loopRun.Exec([this, funOnSessionClose, nSessionId]() {
                if (nullptr != funOnSessionClose) {
                    funOnSessionClose(nSessionId);
                }

                auto pSession = GetSessionInMainLoop(nSessionId);
                if (pSession != nullptr) {
                    pSession->GetLoop().Exec([nSessionId] () {
                        t_mapSessionInNetLoop.erase(nSessionId);
                    });
                    // maybe need erase after t_mapSessionInNetLoop
                    m_mapSession.erase(nSessionId);
                }
            });
        });

    m_pModuleManager->GetMainLoop().Exec(
        [this, pSession, &loopRun, endpoints,
         funOnSessionConnect(std::move(funOnSessionConnect))] () mutable {
            // ensure call ConnectCb in current loop
            DoSessionConnect(
                pSession, endpoints,
                [this, pSession, endpoints, funOnSessionConnect, &loopRun]
                (auto nSessionId, auto bSuccess) mutable {
                    if (bSuccess) {
                        pSession->SetSessionId(CreateSessionId());
                        pSession->GetLoop().Exec(
                            [this, pSession, &loopRun, endpoints, bSuccess, nSessionId, funOnSessionConnect(std::move(funOnSessionConnect))]
                            () mutable {
                                t_mapSessionInNetLoop.emplace(pSession->GetSessionId(), pSession);
                                m_pModuleManager->GetMainLoop().Exec([this, funOnSessionConnect(std::move(funOnSessionConnect)), pSession, bSuccess]() {
                                    m_mapSession.emplace(pSession->GetSessionId(), pSession);

                                    if (nullptr != funOnSessionConnect) {
                                        funOnSessionConnect(pSession->GetSessionId(), bSuccess);
                                    }
                                });
                            });
                    } else {
                        DoSessionConnect(pSession, endpoints, nullptr);
                    }
            });
        });
}

void NetModule::CloseInMainLoop(Session::session_id_t session_id) {
    if (auto itMapSession = m_mapSession.find(session_id);
        itMapSession != m_mapSession.end()) {
        DoSessionAsyncClose(itMapSession->second);
    }
    return;
}

void NetModule::HandleSessionRead(SessionPtr pSession) {
    if (pSession->m_funSessionRead) [[likely]] {
        if (false == pSession->m_funSessionRead(pSession->GetSessionId(),
                                                pSession->m_buffRead)) {
            LOG_ERROR("read packet error on seesion: {}",
                      pSession->GetSessionId());
            DoSessionAsyncClose(pSession);
        }
    }
}

void NetModule::DoSessionConnect(
    SessionPtr pSession, asio::ip::tcp::endpoint& address,
    const Session::FunSessionConnect& funFunSessionConnect /* = nullptr*/) {
    pSession->GetSocket().async_connect(
        address, [this, pSession, funFunSessionConnect](std::error_code ec) {
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

void NetModule::DoSessionRead(SessionPtr pSession) {
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

bool NetModule::DoSessionWrite(SessionPtr pSession) {
    pSession->GetSocket().async_write_some(
        asio::buffer(pSession->m_buffWrite.GetReadData(),
                     pSession->m_buffWrite.GetReadableSize()),
        [this, pSession](std::error_code ec, std::size_t length) {
            if (!ec) {
                m_pModuleManager->GetMainLoop().Exec(
                    [this, pSession, length]() {
                        pSession->m_buffWrite.RemoveData(length);
                        if (pSession->m_buffWrite.GetReadableSize() > 0) {
                            DoSessionWrite(pSession);
                        }
                    });
            } else {
                DoSessionAsyncClose(pSession);
            }
        });

    return true;
}

void NetModule::DoSessionAsyncClose(SessionPtr pSession) {
    pSession->GetLoop().Exec(
        [this, pSession]() { DoSessionCloseInNetLoop(pSession); });
}

void NetModule::DoSessionCloseInNetLoop(SessionPtr pSession) {
    if (pSession->GetSocket().is_open()) {
        try {
            pSession->GetSocket().shutdown(asio::socket_base::shutdown_both);
        } catch (...) {
            LOG_ERROR("shutdown fail on sessionId:{}",
                      pSession->GetSessionId());
        }

        try {
            pSession->GetSocket().close();
        } catch (...) {
            LOG_ERROR("close fail on sessionId:{}", pSession->GetSessionId());
        }
    }

    LOG_TRACE("close session on sessionId:{}", pSession->GetSessionId());
    if (nullptr != pSession->m_funSessionProtoContextClose) {
        pSession->m_funSessionProtoContextClose(pSession->m_pSessionContext);
        pSession->m_funSessionProtoContextClose = nullptr;
    }
    if (nullptr != pSession->m_funSessionClose) {
        pSession->m_funSessionClose(pSession->m_nSessionId);
        pSession->m_funSessionClose = nullptr;
    }

    RemoveMapSession(pSession->m_nSessionId);
}

void NetModule::DoTimerSessionReadTimeout(Session::session_id_t sessionId) {
    LOG_INFO("close timeout session on sessionId:{}", sessionId);

    auto pSession = GetSessionInNetLoop(sessionId);
    if (!pSession) {
        LOG_WARN("not find session: {}", sessionId);
        return;
    }
    DoSessionAsyncClose(pSession);
}

TONY_CAT_SPACE_END
