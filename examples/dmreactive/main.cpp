#include "dmreactive.h"

int main(int argc, char* argv[]) {
    Idmreactive* module = dmreactiveGetModule();
    if (module) {
        module->Release();
    }
    return 0;
}