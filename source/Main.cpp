#include "Game/DoomMain.h"
#if WIN32
#include <Windows.h>

int WINAPI wWinMain(
    [[maybe_unused]] HINSTANCE hInstance,
    [[maybe_unused]] HINSTANCE hPrevInstance,
    [[maybe_unused]] LPWSTR lpCmdLine,
    [[maybe_unused]] int nCmdShow
) {
#else 
int main(int argc, char* argv[]) noexcept {
#endif
#if APPLE
    @autoreleasepool {
#endif
    D_DoomMain();
    return 0;
}

