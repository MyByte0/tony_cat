#ifndef SERVERCOMMON_SERVER_DEFINE_H_
#define SERVERCOMMON_SERVER_DEFINE_H_

#include "common/core_define.h"

TONY_CAT_SPACE_BEGIN

enum ServerType {
    eTypeGateServer = 1,
    eTypeLogicServer = 2,
    eTypeDBServer = 3,
};

TONY_CAT_SPACE_END

#endif // SERVERCOMMON_SERVER_DEFINE_H_