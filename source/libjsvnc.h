#ifndef LIBJSVNC_H
#define LIBJSVNC_H
#ifdef JSVNC_STATIC
#include <stdint.h>


#define VNC_STR_SIZE 256


class vnc_param
{
public:
    vnc_param();
    
    enum capture_area
    {
        desktop,                //Capture the whole Desktop
        window,                 //Capture the window who's handle is vnc_param.hwnd
        window_match_title      //Capture the first window who's title starts with vnc_param.wndTitlePrefix
    };
    ///data
    uint32_t port;              //HTTP serve port, default 8080
//<FFMPEG
    uint32_t outputWidth;       //Output size. default:0 (means the same as window size)
    uint32_t outputHeight;      //
    uint32_t bitRate;           //bitrate in kilobit/s 0 means estimated by output size, default: 5120
    uint32_t frameRate;         //target framerate. default: 60
//>
//<libfame
    uint32_t videoQuality;       //from 1 to 100, the greater the better.
//>
    capture_area captureArea;  
    void*   userData;
    void*   hwnd;               //Used when captureArea == window
    char    wndTitlePrefix[VNC_STR_SIZE];//Used when captureArea == window_match_title
    char    token[VNC_STR_SIZE];    //Safe token. Clients must use this token to remote
                                    //control this machine.
    //return true to accept, false to deny.
    bool (*OnPeerConnected)( const char* ipaddr, void *lwsocket, void* userData);
    void (*OnPeerDisconnected)( const char* ipaddr, void *lwsocket, void* userData);
    void (*OnServerCreated)(void* userData);
    void (*OnServerClosed)(void* userData);
    void (*ProfilingCallback)( int fps, double grab_time, double encoding_time, void* userData); //Can be null
};




#define JSVNC_OK 0
#define JSVNC_ERR 1
#define JSVNC_ERR_INVALID_PARAM 2
#define JSVNC_ERR_CREATE_SERVER_FAILED 3
#define JSVNC_ERR_SERVER_ALREAD_STARTED 4
#define JSVNC_ERR_SERVER_IS_NOT_RUNNING 5

int jsvnc_start_server(const vnc_param& vncParam);
int jsvnc_stop_server();
#endif
#endif