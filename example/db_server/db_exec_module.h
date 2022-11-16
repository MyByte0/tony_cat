#ifndef DB_SERVER_DB_EXEC_MODULE_H_
#define DB_SERVER_DB_EXEC_MODULE_H_

#include "common/core_define.h"
#include "common/module_base.h"
#include "common/module_manager.h"
#include "common/net/net_pb_module.h"
#include "common/net/net_session.h"
#include "protocol/db_data.pb.h"
#include "protocol/server_base.pb.h"
#include "protocol/server_db.pb.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

#ifdef USE_MYSQL
#include "common/database/mysql/mysql_module.h"
TONY_CAT_SPACE_BEGIN
class MysqlModule;
TONY_CAT_SPACE_END
typedef tony_cat::MysqlModule DBModule;

#else
#ifdef USE_ROCKSDB
#include "common/database/rocksdb/rocksdb_module.h"
TONY_CAT_SPACE_BEGIN
class RocksDBModule;
TONY_CAT_SPACE_END

typedef tony_cat::RocksDBModule DBModule;

#endif

#endif


TONY_CAT_SPACE_BEGIN

class NetModule;
class ServiceGovernmentModule;

class DBExecModule : public ModuleBase {
public:
    explicit DBExecModule(ModuleManager* pModuleManager);
    ~DBExecModule();
    virtual void BeforeInit() override;

public:
    typedef std::string USER_ID;

    struct GateUserInfo {
        USER_ID userId;
        Session::session_id_t userSessionId;
    };

    typedef std::shared_ptr<GateUserInfo> GateUserInfoPtr;

private:
    void OnHandleSSSaveDataReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSSaveDataReq& msgReq);
    void OnHandleSSQueryDataReq(Session::session_id_t sessionId, Pb::ServerHead& head, Pb::SSQueryDataReq& msgReq);

private:
    NetPbModule* m_pNetPbModule = nullptr;
    DBModule* m_pDBModule = nullptr;

};

TONY_CAT_SPACE_END

#endif // DB_SERVER_DB_EXEC_MODULE_H_
