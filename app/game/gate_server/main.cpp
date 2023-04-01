#include <cstdint>

#include "gate_server_app.h"

int main(int argc, char* argv[]) {
    int32_t nServerIndex = 1;
    if (argc > 1) {
        nServerIndex = std::atoi(argv[1]);
    }

    tony_cat::GateServerApp serverApp;
    serverApp.Start(nServerIndex);
}
