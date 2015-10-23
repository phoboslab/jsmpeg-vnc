// log.h 日志记录文件
//
//*****************************************************************************
//修改记录
//xiongyong 2008-09-22 [添加int _declspec(selectany) g_LocateHelper=0 不用手动添加全局变量了]
//log使用说明：
//1 *.h中定义
//#define TGLOG_MODULENAME _T("test.exe")//定义模块名
//#include <logT.h>
//*****************************************************************************
//*****************************************************************************
//修改记录
//xiongyong 2008-09-09 [debug下log文件写在当前执行目录下，release下还是在windows temp目录下]
//*****************************************************************************

//*****************************************************************************
//log使用说明：
//1 *.h中定义
//extern int g_aaaaaa;
//#define ALTERNATE_GLOBAL_VAR g_aaaaaa//定义一个使用的全局变量
//#define TGLOG_MODULENAME _T("test.exe")//定义模块名
//#include <logT.h>
//2 *.cpp中定义
//int g_aaaaaa = 0;
//*****************************************************************************

#pragma once
#include "stdio.h"
#include "tchar.h"
#include <Shlwapi.h>
#pragma comment(lib,"Shlwapi")
#ifndef _AFX
#    include <Windows.h>
#endif
#ifdef UNICODE
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)
#define __TFILE__ __WFILE__

#define __TFUNCSIG__ WIDEN(__FUNCTION__)
#else
#define __TFILE__ __FILE__
#define __TFUNCSIG__ __FUNCTION__
#endif

int _declspec(selectany) g_LocateHelper=0;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/**/  
/**/  #define DEFAULT_MODULENAME TEXT("Unknown")
/**/  #ifndef TGLOG_MODULENAME
/**/  #  define TGLOG_MODULENAME DEFAULT_MODULENAME
/**/  #endif
/**/ 
/**/  #if _MSC_VER < 1310
/**/  #  ifdef _DEBUG
/**/  #    define TGLOGFILE(x,y) do{DWORD dwErr=GetLastError();TgTrace tt(dwErr);tt.WriteTrace(TEXT("%s(%d): "),__TFILE__,__LINE__);tt.WriteTrace x ;tt.WriteTrace y; SYSTEMTIME st;GetLocalTime(&st);WriteLog(TEXT("%s(%d): %04d/%02d/%02d %02d:%02d:%02d %08x"),__TFILE__,__LINE__,st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,dwErr);WriteLog x ; WriteLog y; if(tt.AskStop(TEXT("software\\tglogfile\\") TGLOG_MODULENAME, __TFILE__, __LINE__)){__asm int 3};}while(0)
/**/  #  else
/**/  #    define TGLOGFILE(x,y) do{DWORD dwErr=GetLastError();SYSTEMTIME st;GetLocalTime(&st);WriteLog(TEXT("%s(%d): %04d/%02d/%02d %02d:%02d:%02d %08x"),__TFILE__,__LINE__,st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,dwErr);WriteLog y ;}while(0)
/**/  #  endif
/**/  #else
/**/  #  ifdef _DEBUG
/**/  #    define TGLOGFILE(x,y) do{DWORD dwErr=GetLastError();TgTrace tt(dwErr);tt.WriteTrace(TEXT("%s(%d): in %s"),__TFILE__,__LINE__,__TFUNCSIG__);tt.WriteTrace x ;tt.WriteTrace y; SYSTEMTIME st;GetLocalTime(&st);WriteLog(TEXT("%s(%d): in %s : %04d/%02d/%02d %02d:%02d:%02d %08x"),__TFILE__,__LINE__,__TFUNCSIG__,st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,dwErr);WriteLog x ; WriteLog y; if(tt.AskStop(TEXT("software\\tglogfile\\") TGLOG_MODULENAME, __TFILE__, __LINE__)){__asm int 3};}while(0)
/**/  #  else
/**/  #    define TGLOGFILE(x,y) do{DWORD dwErr=GetLastError();SYSTEMTIME st;GetLocalTime(&st);WriteLog(TEXT("%s(%d): %04d/%02d/%02d %02d:%02d:%02d %08x"),__TFILE__,__LINE__,st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,dwErr);WriteLog y ;}while(0)
/**/  #  endif
/**/  #endif
/**/  
/**/  struct TgTrace 
/**/  {
/**/      TCHAR msgbuf[1024];
/**/      TgTrace(DWORD dwErr)
/**/      {
/**/          TCHAR path[MAX_PATH]={0};
/**/          MEMORY_BASIC_INFORMATION mbi;
/**/          memset(&mbi,0,sizeof(mbi));
/**/#ifdef ALTERNATE_GLOBAL_VAR
/**/          if(VirtualQuery(& ALTERNATE_GLOBAL_VAR,  //如果没有_Module, 就随便取一个本模块内的全局变量或者函数就可以
/**/#else
/**/          if(VirtualQuery(&g_LocateHelper,  //如果没有_Module, 就随便取一个本模块内的全局变量或者函数就可以
/**/#endif
/**/                   &mbi,sizeof(mbi)))
/**/          {
/**/             GetModuleFileName((HMODULE)mbi.AllocationBase,path,sizeof(path)/(sizeof TCHAR));
/**/          }        
/**/	      SYSTEMTIME st;
/**/          GetLocalTime(&st);
/**/          _stprintf(msgbuf,TEXT("%.128s\n%02d:%02d:%02d Tid:%d Pid:%d Err:%p\n\n"),path,st.wHour,st.wMinute,st.wSecond,GetCurrentThreadId(),GetCurrentProcessId(),dwErr);
/**/      }
/**/      
/**/      bool WriteTrace(TCHAR*msg, ...)
/**/      {
/**/          va_list ptr;
/**/          TCHAR buf[512]={0};
/**/  
/**/          va_start(ptr, msg);
/**/          _vsntprintf(buf,sizeof(buf)/(sizeof TCHAR),msg,ptr);
/**/          va_end(ptr);
/**/  
/**/          size_t left=sizeof(msgbuf)/(sizeof TCHAR)-_tcslen(msgbuf);
/**/          if(left > _tcslen(buf)+3)
/**/          {
/**/              _tcsncat(msgbuf,buf,left);
/**/              _tcscat(msgbuf,TEXT("\n"));
/**/          }
/**/          //else ignore
/**/  
/**/          OutputDebugString(buf);
/**/          OutputDebugString(TEXT("\n"));
/**/  
/**/          return 0;
/**/      }
/**/      
/**/      bool AskStop(LPCTSTR moduleregpath, LPCTSTR srcfn, int line)
/**/      {
/**/          bool bShouldStop=false;
/**/          DWORD dwType=REG_SZ;
/**/          TCHAR data[8]={0};
/**/          DWORD cbData=sizeof(data)/(sizeof TCHAR);
/**/          TCHAR num[16]={0};
/**/          TCHAR fileregpath[MAX_PATH]={0};
/**/          bool bShowDlg=true;
/**/  
/**/          _sntprintf(fileregpath, sizeof(fileregpath)/(sizeof TCHAR),TEXT("%s\\%s"),moduleregpath,PathFindFileName(srcfn));
/**/          _itot(line,num,10);
/**/  
/**/          if(SHGetValue(HKEY_LOCAL_MACHINE, moduleregpath, TEXT(""), &dwType, data,&cbData)==ERROR_SUCCESS  && _tcsicmp(data,TEXT("no"))==0)   
/**/              bShowDlg=false;
/**/          else
/**/          {
/**/              dwType=REG_SZ;
/**/              cbData=sizeof(data)/(sizeof TCHAR);
/**/              data[0]=0;
/**/  
/**/              if(SHGetValue(HKEY_LOCAL_MACHINE, fileregpath, TEXT(""), &dwType, data,&cbData)==ERROR_SUCCESS  &&  _tcsicmp(data,TEXT("no"))==0 )
/**/                  bShowDlg=false;
/**/              else
/**/              {
/**/                  dwType=REG_SZ;
/**/                  cbData=sizeof(data)/(sizeof TCHAR);
/**/                  data[0]=0;
/**/  
/**/                  if(SHGetValue(HKEY_LOCAL_MACHINE,fileregpath, num, &dwType,data,&cbData)==ERROR_SUCCESS  &&  _tcsicmp(data,TEXT("no"))==0)
/**/                      bShowDlg=false;
/**/                  //else cont.
/**/              }//endi
/**/          }//endi
/**/  
/**/          if(bShowDlg==true)
/**/          {
/**/  #define TGLOG_USAGE TEXT("\n\n\n")                                                  \
/**/                      TEXT("    Ctrl+Shift+'否': 不再对 此模块 的错误进行提示\n")     \
/**/                      TEXT("         Shift+'否': 不再对 此文件 的错误进行提示\n")     \
/**/                      TEXT("           Alt+'否': 不再对 此处   的错误进行提示")
/**/
/**/              if(_tcslen(msgbuf)<sizeof(msgbuf)/(sizeof TCHAR)-sizeof(TGLOG_USAGE)/(sizeof TCHAR)-2)
/**/              {
/**/                  if(rand()%10==1)  //展现概率10%
/**/                      _tcscat(msgbuf,TGLOG_USAGE);
/**/                  //else cont.
/**/              }
/**/              //else cont.
/**/  
/**/              if(::MessageBox(0,msgbuf,TEXT("stop?"),MB_YESNO)==IDYES)
/**/                  bShouldStop=true;
/**/              else
/**/              {
/**/                  if(_tcsicmp(TEXT("software\\tglogfile\\") DEFAULT_MODULENAME, moduleregpath)!=0)
/**/                  {
/**/                      if((GetKeyState(VK_CONTROL)&0x8000) > 0  &&  (GetKeyState(VK_SHIFT)&0x8000) > 0)
/**/                          SHSetValue(HKEY_LOCAL_MACHINE, moduleregpath, TEXT(""),  REG_SZ, TEXT("NO"), 2*sizeof(TCHAR));                        
/**/                      else if((GetKeyState(VK_SHIFT)&0x8000) > 0)
/**/                          SHSetValue(HKEY_LOCAL_MACHINE, fileregpath,   TEXT(""),  REG_SZ, TEXT("NO"), 2*sizeof(TCHAR));
/**/                      else if((GetKeyState(VK_MENU)&0x8000) > 0)
/**/                          SHSetValue(HKEY_LOCAL_MACHINE, fileregpath,   num, REG_SZ, TEXT("NO"), 2*sizeof(TCHAR));
/**/                      //else cont.
/**/                  }
/**/                  //else cont.
/**/              }//endi
/**/          }
/**/          //else cont.
/**/  
/**/          return bShouldStop;
/**/      }
/**/  };
/**/
/**/  inline bool _writeLog(FILE* fp, TCHAR* buf)
/**/  {
/**/      fseek(fp,0,SEEK_END);
/**/      _ftprintf(fp,TEXT("%s\n"),buf);
/**/  
/**/      return true;
/**/  }
/**/  
/**/  inline bool WriteLog(TCHAR* msg, ...)
/**/  {
/**/      volatile unsigned long guard=0x771021;
/**/      TCHAR buf[1024]={0};
/**/      va_list ptr;
/**/      va_start(ptr, msg);
/**/      _vsntprintf(buf,sizeof(buf)/(sizeof TCHAR),msg,ptr);
/**/      va_end(ptr);
/**/  
/**/      TCHAR path[MAX_PATH];
/**/      MEMORY_BASIC_INFORMATION mbi;
/**/      memset(&mbi,0,sizeof(mbi));
/**/#ifdef ALTERNATE_GLOBAL_VAR
/**/      if(VirtualQuery(& ALTERNATE_GLOBAL_VAR,  //如果没有_Module, 就随便取一个本模块内的全局变量或者函数就可以
/**/#else
/**/      if(VirtualQuery(&g_LocateHelper,  //如果没有_Module, 就随便取一个本模块内的全局变量或者函数就可以
/**/#endif
/**/                   &mbi,sizeof(mbi)))
/**/      {
/**/          if(GetModuleFileName((HMODULE)mbi.AllocationBase,path,sizeof(path)/(sizeof TCHAR)))
/**/          {
/**/              TCHAR * pFileName=_tcsrchr(path,'\\'); //retain the leading '\'
/**/              if(pFileName)
/**/              {
/**/                   TCHAR tempname[MAX_PATH]={0};
/**/#ifdef _DEBUG
/**/					if(1)
/**/					{
/**/						//*pFileName = 0;
/**/						 _tcscpy(tempname, path);
/**/
/**/#else
/**/					if(GetTempPath(sizeof(tempname)/(sizeof TCHAR),tempname)< MAX_PATH-_tcslen(pFileName)-10)
/**/					{
/**/						 _tcscat(tempname, pFileName);
/**/#endif
/**/                  
/**/                   
/**/                      
/**/ 
/**/                       if(_tcslen(tempname)<MAX_PATH-10)
/**/                       {
/**/                           _tcscat(tempname,TEXT(".1.log"));
/**/                           int size=0;
/**/                           FILE* fp = _tfopen(tempname,TEXT("a+t"));
/**/                           if(fp!=NULL)
/**/                           {
/**/                               _writeLog(fp, buf);
/**/             
/**/                               size=ftell(fp);
/**/                               fclose(fp);
/**/                           }
/**/                           //else other thread or process is writing this file, just ignore it
/**/       
/**/                           if(size > 8*1024)
/**/                           {
/**/                               TCHAR log1[MAX_PATH];
/**/                               _tcsncpy(log1,tempname,sizeof(log1)/(sizeof TCHAR));
/**/                               TCHAR *pos=_tcsrchr(log1,'1');
/**/                               if(pos!=0)
/**/                               {
/**/                                   *pos='2';
/**/                                   DeleteFile(log1);
/**/                                   MoveFile(tempname,log1);
/**/                               }   
/**/                               //else impossible!!!
/**/                           }
/**/                           //else ok
/**/                      }
/**/                      else
/**/                      {
/**/#ifdef _DEBUG
/**/   		   	              ::MessageBox(0,TEXT("无效的临时目录！太长了？"),TEXT("出大错了..."),0);
/**/#endif
/**/                      }
/**/                 }
/**/                 else
/**/                 {
/**/#ifdef _DEBUG
/**/                     ::MessageBox(0,TEXT("路径超长！"),TEXT("出大错了..."),0);
/**/#endif
/**/                 }
/**/             }
/**/             else
/**/             {
/**/#ifdef _DEBUG
/**/   		   	     ::MessageBox(0,TEXT("取不到名字？"),TEXT("出大错了..."),0);
/**/#endif
/**/             }
/**/         }
/**/         else
/**/         {
/**/#ifdef _DEBUG
/**/   			::MessageBox(0,TEXT("无法取得本模块的名字！"),TEXT("出大错了..."),0);
/**/#endif
/**/         }
/**/      }
/**/      else
/**/      {
/**/#ifdef _DEBUG
/**/		 ::MessageBox(0,TEXT("VirtualQuery failed!"),TEXT("出大错了..."),0);
/**/#endif
/**/      }
/**/  
/**/      if(guard!=0x771021)
/**/      {
/**/#ifdef _DEBUG
/**/	      ::MessageBox(0,TEXT("guard error!"),TEXT("出大错了..."),0);
/**/#endif
/**/      }
/**/      //else ok
/**/
/**/      return true;
/**/  }
/**/  
/**///////////////////////////////////////////////////////////////////////////
/**///////////////////////////////////////////////////////////////////////////

