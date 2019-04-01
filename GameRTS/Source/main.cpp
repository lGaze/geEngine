#include <vld.h>

#if GE_PLATFORM == GE_PLATFORM_WIN32
# include <Win32/geWin32Windows.h>
#endif

#include "RTSApplication.h"

using namespace geEngineSDK;
using namespace RTSGame;

int main() {
  RTSApplication rtsApp;
  return rtsApp.run();
}
