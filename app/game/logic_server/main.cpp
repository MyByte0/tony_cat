#include "logic_server_app.h"

#include <cstdint>

int main(int argc, char* argv[])
{
    int32_t nServerIndex = 1;
    if (argc > 1) {
        nServerIndex = std::atoi(argv[1]);
    }

    tony_cat::LogicServerApp serverApp;
    serverApp.Start(nServerIndex);
}
