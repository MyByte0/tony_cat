#ifndef COMMON_NET_NET_MODULE_H_
#define COMMON_NET_NET_MODULE_H_

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/core_define.h"
#include "common/loop/loop.h"
#include "common/loop/loop_pool.h"
#include "common/module_base.h"
#include "common/net/net_accepter.h"
#include "common/net/net_session.h"
#include "common/var_define.h"

TONY_CAT_SPACE_BEGIN

class NetPbModule;

class NetModule : public ModuleBase {
 public:
    explicit NetModule(ModuleManager* pModuleManager);
    virtual ~NetModule();

    void BeforeInit() override;
    void OnInit() override;
    void OnStop() override;

 public:
    typedef std::function<bool(Session::session_id_t, SessionBuffer&)>
        FunNetRead;

    enum int32_t {
        kDefaultTimeoutMillseconds = 3 * 60 * 1000,
    };

    LoopPtr GetDefaultListenLoop() {
        if (m_defaultListenLoop == nullptr) {
            m_defaultListenLoop = std::make_shared<Loop>();
            m_defaultListenLoop->Start();
        }
        return m_defaultListenLoop;
    }

    void Listen(AcceptorPtr pAcceptor);
    void Connect(
        const std::string& strAddress, uint16_t addressPort,
        const Session::FunSessionRead& funOnSessionRead,
        const Session::FunSessionConnect& funOnSessionConnect = nullptr,
        const Session::FunSessionClose& funOnSessionClose = nullptr);
    void CloseInMainLoop(Session::session_id_t session_id);

    // for write session in main loop
    SessionPtr GetSessionInMainLoop(Session::session_id_t sessionId);
    // for read session in net loop
    SessionPtr GetSessionInNetLoop(Session::session_id_t sessionId);

    bool SessionSend(SessionPtr pSession) { return DoSessionWrite(pSession); }

 private:
    void Accept(AcceptorPtr pAcceptor);
    Session::session_id_t CreateSessionId();
    void RemoveMapSession(Session::session_id_t sessionId);
    bool OnReadSession(AcceptorPtr pAcceptor, Session::session_id_t sessionId,
                       SessionBuffer& buff);

 private:
    void HandleSessionRead(SessionPtr pSession);
    void DoSessionConnect(
        SessionPtr pSession, asio::ip::tcp::endpoint& address,
        const Session::FunSessionConnect& funFunSessionConnect = nullptr);
    void DoSessionRead(SessionPtr pSession);
    bool DoSessionWrite(SessionPtr pSession);
    void DoSessionAsyncClose(SessionPtr pSession);
    void DoSessionCloseInNetLoop(SessionPtr pSession);

    void DoTimerSessionReadTimeout(Session::session_id_t sessionId);

 private:
    // sessions for main loop manager
    std::unordered_map<Session::session_id_t, SessionPtr> m_mapSession;
    static THREAD_LOCAL_VAR
        std::unordered_map<Session::session_id_t, SessionPtr>
            t_mapSessionInNetLoop;

    LoopPtr m_defaultListenLoop = nullptr;
    std::vector<AcceptorPtr> m_vecAcceptors;

    std::atomic<Session::session_id_t> m_nextSessionId = 0;
};

TONY_CAT_SPACE_END

#endif  // COMMON_NET_NET_MODULE_H_
