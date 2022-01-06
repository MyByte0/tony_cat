#include "service_government_module.h"
#include "common/module_manager.h"
#include "common/utility/crc.h"
#include "log/log_module.h"
#include "net/net_pb_module.h"

SER_NAME_SPACE_BEGIN

ServiceGovernmentModule::ServiceGovernmentModule(ModuleManager* pModuleManager)
    :ModuleBase(pModuleManager) {

}

ServiceGovernmentModule::~ServiceGovernmentModule() {}

void ServiceGovernmentModule::BeforeInit() {
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);

    m_pNetPbModule->RegisterHandle(SSHeartbeat::ss_msgid, this, &ServiceGovernmentModule::OnHandleSSHeartbeat);
}

void ServiceGovernmentModule::OnHandleSSHeartbeat(Session::session_id_t sessionId, ServerHead& head, SSHeartbeat& heartbeat)
{
}

SER_NAME_SPACE_END
