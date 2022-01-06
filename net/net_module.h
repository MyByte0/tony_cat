#ifndef NET_NET_MODULE_H_
#define NET_NET_MODULE_H_

#include "common/core_define.h"
#include "common/module_base.h"
#include "common/loop.h"
#include "common/loop_pool.h"

#include "net_session.h"

#include <thread>
#include <unordered_map>

SER_NAME_SPACE_BEGIN

class NetPbModule;

class NetModule :public ModuleBase {
public:
	NetModule(ModuleManager* pModuleManager);
	virtual ~NetModule();

	virtual void BeforeInit();

public:
    void Connect(const std::string strAddress, uint16_t addressPort);
	void Close(Session::session_id_t session_id);

	typedef std::function<bool(Session::session_id_t, SessionBuffer&)> FunNetRead;
	SessionPtr GetSessionById(Session::session_id_t sessionId);
	void SetFunSessionRead(const FunNetRead& funNetRead) {
		m_funNetRead = funNetRead;
	}

private:
	void Accept();
	Session::session_id_t CreateSessionId();
	void OnCloseSession(Session::session_id_t sessionId);

private:
    std::unordered_map<Session::session_id_t, SessionPtr> m_mapSession;

    Loop m_loopAccpet;
	Acceptor m_acceptor;

	Session::session_id_t m_nextSessionId;
    LoopPool m_poolSessionContext;

private:
	FunNetRead m_funNetRead = nullptr;

};

SER_NAME_SPACE_END

#endif  // NET_NET_MODULE_H_
