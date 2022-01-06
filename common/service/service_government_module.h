#ifndef SERVICE_GOVERNMENT_MODULE_H_
#define SERVICE_GOVERNMENT_MODULE_H_

#include "common/core_define.h"
#include "common/module_base.h"
#include "common/loop.h"
#include "common/loop_coroutine.h"
#include "common/loop_pool.h"
#include "net/net_session.h"
#include "protocol/server_base.pb.h"
#include "protocol/server_common.pb.h"

#include <cstdint>
#include <coroutine>
#include <functional>
#include <thread>
#include <unordered_map>

SER_NAME_SPACE_BEGIN

class NetPbModule;

class ServiceGovernmentModule : public ModuleBase {
public:
    ServiceGovernmentModule(ModuleManager* pModuleManager);
    virtual ~ServiceGovernmentModule();

    virtual void BeforeInit();

public:
    void OnHandleSSHeartbeat(Session::session_id_t sessionId, ServerHead& head, SSHeartbeat& heartbeat);

private:
    NetPbModule* m_pNetPbModule = nullptr;
};

SER_NAME_SPACE_END

#endif  // SERVICE_GOVERNMENT_MODULE_H_
