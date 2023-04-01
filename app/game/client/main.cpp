#include "client_test_app.h"

int main(int argc, char* argv[]) {
    int32_t nServerIndex = 1;
    if (argc > 1) {
        nServerIndex = std::atoi(argv[1]);
    }

    tony_cat::ClientTestApp clientApp;
    clientApp.Start(nServerIndex);

    return 0;
}
