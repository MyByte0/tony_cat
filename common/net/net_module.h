#ifndef COMMON_NET_NET_MODULE_H_
#define COMMON_NET_NET_MODULE_H_

#include "common/core_define.h"
#include "common/loop/loop.h"
#include "common/loop/loop_pool.h"
#include "common/module_base.h"
#include "common/net/net_accepter.h"
#include "common/net/net_session.h"
#include "common/var_define.h"

#include <atomic>
#include <unordered_map>

TONY_CAT_SPACE_BEGIN

class NetPbModule;

class NetModule : public ModuleBase {
public:
    explicit NetModule(ModuleManager* pModuleManager);
    virtual ~NetModule();

    virtual void BeforeInit() override;
    virtual void OnInit() override;
    virtual void AfterStop() override;

public:
    typedef std::function<bool(Session::session_id_t, SessionBuffer&)> FunNetRead;

    void Listen(const std::string& strAddress, uint16_t addressPort, const FunNetRead& funNetRead);
    Session::session_id_t Connect(const std::string& strAddress, uint16_t addressPort,
        const Session::FunSessionRead& funOnSessionRead,
        const Session::FunSessionConnect& funOnSessionConnect = nullptr,
        const Session::FunSessionClose& funOnSessionClose = nullptr);
    void CloseInMainLoop(Session::session_id_t session_id);

    // for write session in main loop
    SessionPtr GetSessionInMainLoop(Session::session_id_t sessionId);
    // for read session in net loop
    SessionPtr GetSessionInNetLoop(Session::session_id_t sessionId);

    bool SessionSend(SessionPtr pSession) {
        return DoSessionWrite(pSession);
    }

    DEFINE_MEMBER_UINT32_PUBLIC(NetThreadNum);

private:
    void Accept(Acceptor* pAcceptor);
    Session::session_id_t CreateSessionId();
    void RemoveInSessionMap(Session::session_id_t sessionId);
    bool OnReadSession(Acceptor* pAcceptor, Session::session_id_t sessionId, SessionBuffer& buff);

private:
    void HandleSessionRead(SessionPtr pSession);
    void DoSessionConnect(SessionPtr pSession, asio::ip::tcp::endpoint& address, const Session::FunSessionConnect& funFunSessionConnect = nullptr);
    void DoSessionRead(SessionPtr pSession);
    bool DoSessionWrite(SessionPtr pSession);
    void DoSessionAsyncClose(SessionPtr pSession);
    void DoSessionCloseInNetLoop(SessionPtr pSession);

private:
    std::unordered_map<Session::session_id_t, SessionPtr> m_mapSession;
    static THREAD_LOCAL_VAR std::unordered_map<Session::session_id_t, SessionPtr> t_mapSessionInNetLoop;

    Loop m_loopAccpet;
    std::vector<Acceptor*> m_vecAcceptors;

    std::atomic<Session::session_id_t> m_nextSessionId = 0;
    LoopPool m_poolSessionContext;
};

TONY_CAT_SPACE_END

#endif // COMMON_NET_NET_MODULE_H_
