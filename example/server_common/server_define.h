#ifndef SERVERCOMMON_SERVER_DEFINE_H_
#define SERVERCOMMON_SERVER_DEFINE_H_

#include "common/core_define.h"

TONY_CAT_SPACE_BEGIN

enum ServerType {
    eTypeGateServer = 1,
    eTypeLogicServer = 2,
    eTypeDBServer = 3,
};

template <ServerType eType>
struct GetServerTypeStructHelper {
};

#define DEFINE_SERVER_TYPE_STRING(_SERVER_TYPE, _TYPE_STRING)                       \
    inline const char* GetServerTypeHelper(GetServerTypeStructHelper<_SERVER_TYPE>) \
    {                                                                               \
        const char* pServerName = _TYPE_STRING;                                     \
        return pServerName;                                                         \
    }

DEFINE_SERVER_TYPE_STRING(eTypeGateServer, "GateServer")
DEFINE_SERVER_TYPE_STRING(eTypeLogicServer, "LogicServer")
DEFINE_SERVER_TYPE_STRING(eTypeDBServer, "DBServer")

template <ServerType eType>
const char* GetServerType()
{
    return GetServerTypeHelper(GetServerTypeStructHelper<eType>());
}

TONY_CAT_SPACE_END

#endif // SERVERCOMMON_SERVER_DEFINE_H_