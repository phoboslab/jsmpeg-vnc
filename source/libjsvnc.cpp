#include "libjsvnc.h"
#include <memory.h>

vnc_param::vnc_param()
    :port(8080),
    outputWidth(0),
    outputHeight(0),
    bitRate(5120),
    frameRate(60),
    captureArea(desktop),
    hwnd(nullptr)
{
    memset(wndTitlePrefix, 0, VNC_STR_SIZE);
    memset(token, 0, VNC_STR_SIZE);
};