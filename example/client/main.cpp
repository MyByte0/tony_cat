#include "common/config/xml_config_module.h"
#include "common/log/log_module.h"
#include "common/loop.h"
#include "common/module_manager.h"
#include "common/net/net_module.h"
#include "common/net/net_pb_module.h"
#include "common/service/rpc_module.h"
#include "common/service/service_government_module.h"
#include "server_common/server_define.h"

#include <csignal>
#include <memory>

TONY_CAT_SPACE_BEGIN

TONY_CAT_SPACE_END

int main(int argc, char* argv[])
{
    int32_t nServerIndex = 1;
    if (argc > 1) {
        nServerIndex = std::atoi(argv[1]);
    }

    //tony_cat::GateServer serverApp;
    //serverApp.Start(nServerIndex);

    return 0;
}
