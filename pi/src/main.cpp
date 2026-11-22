// Placeholder entry point (build skeleton). 
#include <cstdio>

int main() {
#if VIGIL_SIM
    const char* flavour = "sim";
#else
    const char* flavour = "target";
#endif
    std::printf("vigil %s (%s)\n", VIGIL_VERSION, flavour);
    return 0;
}
