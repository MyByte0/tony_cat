#ifndef COMMON_NET_NET_HTTP_MODULE_H_
#define COMMON_NET_NET_HTTP_MODULE_H_

#include "common/core_define.h"
#include "common/log/log_module.h"
#include "common/loop/loop_coroutine.h"
#include "common/module_base.h"
#include "common/module_manager.h"
#include "common/net/net_session.h"
#include "common/utility/spin_lock.h"
#include "common/utility/http_request.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

TONY_CAT_SPACE_BEGIN

class ModuleManager;
class NetModule;
class ServiceGovernmentModule;

class NetHttpModule : public ModuleBase {
public:
    explicit NetHttpModule(ModuleManager* pModuleManager);
    virtual ~NetHttpModule();

    virtual void BeforeInit() override;

    virtual void OnUpdate() override;

public:
    enum : int {
        kHeadLen = 12,
        kPacketHeadMaxLen = 16 * 1024,
        kPacketBodyMaxCache = 64 * 1024,
        kPacketTTLDefaultvalue = 30,
    };

    void Listen(const std::string& strAddress, uint16_t addressPort);
    Session::session_id_t Connect(const std::string& strAddress, uint16_t addressPort,
        const Session::FunSessionConnect& funOnSessionConnect = nullptr,
        const Session::FunSessionClose& funOnSessionClose = nullptr);
    bool ReadData(Session::session_id_t sessionId, SessionBuffer& buf);
    bool WriteData(Session::session_id_t sessionId, Http::Reply& respond);
    bool ReadPbPacket(Session::session_id_t sessionId, Http::Request& req);

    typedef std::function<
        void(Session::session_id_t sessionId, Http::Request& req)>
        FuncHttpHandleType;

    void RegisterPacketHandle(std::string strHttpPath, FuncHttpHandleType func);
    void SetDefaultPacketHandle(FuncHttpHandleType func);

public:
    struct AwaitableConnect {
        AwaitableConnect(const std::string& strIP, uint16_t nPort)
            : m_strIP(strIP)
            , m_nPort(nPort)
            , m_nSessionId(0) {};
        bool await_ready() const { return false; }
        auto await_resume() { return m_nSessionId; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            NetHttpModule::GetInstance()->Connect(m_strIP, m_nPort, [this, handle](auto nSessionId, auto bSuccess) {
                if (bSuccess) {
                    m_nSessionId = nSessionId;
                }
                handle.resume();
            });
        }

    private:
        const std::string& m_strIP;
        uint16_t m_nPort;
        Session::session_id_t m_nSessionId;
    };

private:
    static NetHttpModule* GetInstance();

protected:
    std::unordered_map<std::string, FuncHttpHandleType> m_mapPackethandle;
    FuncHttpHandleType m_funDefaultPacketHandle;

    SpinLock m_lockMsgFunction;
    std::vector<std::function<void()>> m_vecMsgFunction;

protected:
    NetModule* m_pNetModule = nullptr;
    ServiceGovernmentModule* m_pServiceGovernmentModule = nullptr;

private:
    static NetHttpModule* m_pNetHttpModule;
};

TONY_CAT_SPACE_END

#endif // COMMON_NET_NET_HTTP_MODULE_H_
