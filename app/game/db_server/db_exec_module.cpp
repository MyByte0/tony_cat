#include "db_exec_module.h"

#include <cstdlib>
#include <utility>

#include "app/game/server_define.h"
#include "common/database/db_define.h"
#include "common/database/db_utils.h"
#include "common/database/mysql/mysql_module.h"
#include "common/database/rocksdb/rocksdb_module.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "common/utility/crc.h"

TONY_CAT_SPACE_BEGIN

DBExecModule::DBExecModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager) {}

DBExecModule::~DBExecModule() {}

void DBExecModule::BeforeInit() {
    m_pNetPbModule = FIND_MODULE(m_pModuleManager, NetPbModule);
    m_pDBModule = FIND_MODULE(m_pModuleManager, DBModule);

    m_pNetPbModule->RegisterHandle(this, &DBExecModule::OnHandleSSSaveDataReq);
    m_pNetPbModule->RegisterHandle(this, &DBExecModule::OnHandleSSQueryDataReq);
}

void DBExecModule::OnHandleSSSaveDataReq(
    Session::session_id_t sessionId,
    NetMemoryPool::PacketNode<Pb::ServerHead> head,
    NetMemoryPool::PacketNode<Pb::SSSaveDataReq> msgReq) {
    auto userId = head->user_id();

    auto& loop = Loop::GetCurrentThreadLoop();
    m_pDBModule->DBLoopExec(
        userId, [this, sessionId, head, userId, &loop, msgReq]() mutable {
            auto pMsgRsp = NetMemoryPool::PacketCreate<Pb::SSSaveDataRsp>();
            m_pDBModule->MessageUpdate(
                *msgReq->mutable_kv_data_update()->mutable_user_data());
            m_pDBModule->MessageDelete(
                *msgReq->mutable_kv_data_delete()->mutable_user_data());
            loop.Exec([this, sessionId, head, pMsgRsp]() mutable {
                m_pNetPbModule->SendPacket(sessionId, head, pMsgRsp);
            });
        });
}

void DBExecModule::OnHandleSSQueryDataReq(
    Session::session_id_t sessionId,
    NetMemoryPool::PacketNode<Pb::ServerHead> head,
    NetMemoryPool::PacketNode<Pb::SSQueryDataReq> msgReq) {
    auto userId = msgReq->user_id();
    auto& loop = Loop::GetCurrentThreadLoop();
    m_pDBModule->DBLoopExec(
        userId, [this, sessionId, head, userId, &loop]() mutable {
            auto pMsgRsp = NetMemoryPool::PacketCreate<Pb::SSQueryDataRsp>();
            auto pUserData = pMsgRsp->mutable_user_data();
            pUserData->set_user_id(userId);
            m_pDBModule->MessageLoad(*pUserData);
            loop.Exec([this, sessionId, head, pMsgRsp]() mutable {
                m_pNetPbModule->SendPacket(sessionId, head, pMsgRsp);
            });
            return;
        });

    return;
}

TONY_CAT_SPACE_END
