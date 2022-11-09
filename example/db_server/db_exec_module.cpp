#include "db_exec_module.h"
#include "common/database/db_define.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "common/database/db_utils.h"
#include "common/database/mysql/mysql_module.h"
#include "common/database/rocksdb/rocksdb_module.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "common/utility/crc.h"
#include "server_common/server_define.h"

#include  <cstdlib>

TONY_CAT_SPACE_BEGIN
#ifdef USE_MYSQL
inline MysqlModule* GetDBModule(void* pDBModule)
{
    return reinterpret_cast<MysqlModule*>(pDBModule);
}


#else
#ifdef USE_ROCKSDB
inline RocksDBModule* GetDBModule(void* pDBModule)
{
    return reinterpret_cast<RocksDBModule*>(pDBModule);
}
#endif


#endif

DBExecModule::DBExecModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

DBExecModule::~DBExecModule() { }

void DBExecModule::BeforeInit()
{
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);
    m_pDBModule = FIND_MODULE(m_pModuleManager, MysqlModule);

    // OnTest();
}

void DBExecModule::OnHandleSSSaveDataReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSSaveDataReq& msgReq)
{
    auto userId = head.user_id();

    auto& loop = Loop::GetCurrentThreadLoop();
    GetDBModule(m_pDBModule)->GetLoopPool().Exec(CRC32(userId),
        [this, sessionId, head, userId, &loop, msgReq = std::move(msgReq)]() mutable {
            auto pMsgRsp = std::make_shared<Pb::SSSaveDataRsp>();
            GetDBModule(m_pDBModule)->UpdateMessage(*msgReq.mutable_kv_data_update()->mutable_user_data());
            GetDBModule(m_pDBModule)->DeleteMessage(*msgReq.mutable_kv_data_delete()->mutable_user_data());
            loop.Exec([this, sessionId, head, pMsgRsp]() mutable { m_pNetPbModule->SendPacket(sessionId, head, *pMsgRsp); });
        });
}

void DBExecModule::OnHandleSSQueryDataReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSQueryDataReq& msgReq)
{
    auto userId = msgReq.user_id();

    auto& loop = Loop::GetCurrentThreadLoop();
    GetDBModule(m_pDBModule)->GetLoopPool().Exec(CRC32(userId),
        [this, sessionId, head, userId, &loop]() mutable {
            auto pMsgRsp = std::make_shared<Pb::SSQueryDataRsp>();
            auto pUserData = pMsgRsp->mutable_user_data();
            pUserData->set_user_id(userId);
            GetDBModule(m_pDBModule)->LoadMessage(*pUserData);
            loop.Exec([this, sessionId, head, pMsgRsp]() mutable { m_pNetPbModule->SendPacket(sessionId, head, *pMsgRsp); });
            return;
        });
    
    return;
}

TONY_CAT_SPACE_END
