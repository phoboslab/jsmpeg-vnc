#ifdef JSVNC_STATIC
#include "app.h"

#include "logt.h"

#include "libjsvnc.h"
#include <memory.h>
#include <process.h>
#include <Windows.h>


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

vnc_param g_vncParam;
app_t*  g_theApp = nullptr;
bool g_exitThread = false;
bool g_isServerRunning = false;
HANDLE g_workthreadHandle = NULL;


//private functions
int _jsvnc_check_param(const vnc_param& vncParam)
{
    return JSVNC_OK;
}

unsigned int _stdcall _jsvnc_server_thread(void* param)
{
    app_run(g_theApp, g_vncParam.frameRate);
    _endthreadex(0);
    return 0;
}
//defined in jsmpeg-vnc.c
HWND window_with_prefix(char *title_prefix);
//public functions
int jsvnc_start_server(const vnc_param& vncParam)
{
    int rtv = JSVNC_ERR;
    if (g_isServerRunning)
    {
        return JSVNC_ERR_SERVER_ALREAD_STARTED;
    }

    if (JSVNC_OK == _jsvnc_check_param(vncParam))
    {
        g_vncParam = vncParam;
        HWND window = NULL;
        switch (g_vncParam.captureArea)
        {
        case vnc_param::desktop: 
            window = GetDesktopWindow();
            break;

        case vnc_param::window_match_title:
            window = window_with_prefix(g_vncParam.wndTitlePrefix);
            break;

        case vnc_param::window:
            window = (HWND)g_vncParam.hwnd;
            break;
        default:
            TGLOGFILE((L"invalid window capture area %d", g_vncParam.captureArea),(L""));
            break;
        }

        if (window != NULL)
        {

            g_theApp = app_create(window,
                g_vncParam.port,
                g_vncParam.bitRate,
                g_vncParam.outputWidth,
                g_vncParam.outputHeight,
                g_vncParam.videoQuality);

            if (g_theApp)
            {
                g_exitThread = false;
                g_workthreadHandle = (HANDLE)_beginthreadex(NULL, 0, _jsvnc_server_thread, NULL, 0, NULL);
                g_isServerRunning = true;
                
                rtv = JSVNC_OK;
            }
            else
                rtv = JSVNC_ERR_CREATE_SERVER_FAILED;
        }
        else
            TGLOGFILE((L"window is null"),(L""));
    }
    else
        rtv = JSVNC_ERR_INVALID_PARAM;
    return rtv;
}

int jsvnc_stop_server()
{
    
    if (!g_isServerRunning)
    {
        return JSVNC_ERR_SERVER_IS_NOT_RUNNING;
    }
    int rtv = JSVNC_ERR;
    g_exitThread = true;

    if(WaitForSingleObject(g_workthreadHandle, 30000) == WAIT_OBJECT_0)
    {
        rtv = JSVNC_OK;
    }

    CloseHandle(g_workthreadHandle);
    g_workthreadHandle = NULL;
    g_isServerRunning = false;
    app_destroy(g_theApp);
    g_theApp = NULL;
    return rtv;
};
#endif