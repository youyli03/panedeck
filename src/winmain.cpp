#include <windows.h>

int main(int argc, char *argv[]);

/// GUI subsystem entry point. Qt's entry point library is not used so the
/// binary can be linked directly with MinGW in the local build.
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int show)
{
    (void)instance;
    (void)previousInstance;
    (void)commandLine;
    (void)show;
    return main(__argc, __argv);
}
