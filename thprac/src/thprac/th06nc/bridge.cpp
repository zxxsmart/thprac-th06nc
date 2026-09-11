#include "common.h"
#include <tlhelp32.h>
#include <shellapi.h>
namespace {
Shared* shared{}; HANDLE mapping{},mutex{}; DWORD targetPid{};
void failure(const std::wstring& text) { MessageBoxW(nullptr,text.c_str(),L"thprac - th06nc",MB_ICONERROR); }
void disconnect() { if(shared)UnmapViewOfFile(shared);if(mapping)CloseHandle(mapping);if(mutex)CloseHandle(mutex);shared=nullptr;mapping=nullptr;mutex=nullptr;targetPid=0; }
uintptr_t remoteModule(DWORD pid,const std::wstring& name) {
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
    if(snapshot==INVALID_HANDLE_VALUE)return 0;
    MODULEENTRY32W item{sizeof(item)};uintptr_t result=0;
    if(Module32FirstW(snapshot,&item))do{if(_wcsicmp(item.szModule,name.c_str())==0){result=(uintptr_t)item.modBaseAddr;break;}}while(Module32NextW(snapshot,&item));
    CloseHandle(snapshot);return result;
}
bool attach(DWORD pid) {
    if(shared){if(pid==targetPid)return true;disconnect();}
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,pid);
    if(!process){failure(L"无法打开游戏进程，错误 "+std::to_wstring(GetLastError()));return false;}
    wchar_t path[32768];DWORD length=32768;
    if(!QueryFullProcessImageNameW(process,0,path,&length)||!supportedFile(path)){
      CloseHandle(process);failure(L"EXE 的 SHA-256 与本地已核对的 th06nc 1.03 不一致，未注入。");return false;}
    mapping=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,mapName(pid).c_str());
    if(!mapping) {
      for (const wchar_t* dependency : {L"freetype.dll", L"thprac_th06nc.dll"}) {
      std::wstring dll=directory(executablePath())+L"\\"+dependency;
      if(GetFileAttributesW(dll.c_str())==INVALID_FILE_ATTRIBUTES){CloseHandle(process);failure(L"缺少同目录 thprac_th06nc.dll");return false;}
      SIZE_T bytes=(dll.size()+1)*sizeof(wchar_t);void* remote=VirtualAllocEx(process,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
      SIZE_T written=0;bool ok=remote&&WriteProcessMemory(process,remote,dll.c_str(),bytes,&written)&&written==bytes;
      auto load=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
      MEMORY_BASIC_INFORMATION info{};VirtualQuery((void*)load,&info,sizeof(info));
      std::wstring localOwner=executablePath((HMODULE)info.AllocationBase);
      auto remoteOwner=remoteModule(pid,localOwner.substr(localOwner.find_last_of(L"\\")+1));
      auto remoteLoad=remoteOwner+((uintptr_t)load-(uintptr_t)info.AllocationBase);
      HANDLE thread=ok&&remoteOwner?CreateRemoteThread(process,nullptr,0,(LPTHREAD_START_ROUTINE)remoteLoad,remote,0,nullptr):nullptr;
      DWORD wait=thread?WaitForSingleObject(thread,10000):WAIT_FAILED;
      if(thread)CloseHandle(thread);
      // Do not free the argument if LoadLibrary is still using it.
      if(remote&&wait!=WAIT_TIMEOUT)VirtualFreeEx(process,remote,0,MEM_RELEASE);
      if(wait!=WAIT_OBJECT_0){CloseHandle(process);failure(L"注入失败或超时，错误 "+std::to_wstring(GetLastError()));return false;}
      }
      for(int i=0;i<100&&!mapping;i++){Sleep(50);mapping=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,mapName(pid).c_str());}
    }
    CloseHandle(process);
    if(!mapping){failure(L"DLL 未建立通信通道。");return false;}
    shared=(Shared*)MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(Shared));
    mutex=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,mutexName(pid).c_str());
    if(!shared||!mutex){failure(L"无法打开通信通道。");return false;}
    for(int i=0;i<100&&shared->magic!=Magic;++i)Sleep(25);
    if(shared->magic!=Magic||shared->version!=Protocol){failure(L"游戏中已加载其他协议版本的练习器，请退出游戏后重新连接。");disconnect();return false;}
    for(int i=0;i<100&&!shared->status.ready&&!shared->status.error;++i)Sleep(25);
    if(!shared->status.ready){failure(shared->status.message);disconnect();return false;}
    targetPid=pid;return true;
}
DWORD findGame() {
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);PROCESSENTRY32W entry{sizeof(entry)};DWORD pid=0;
    if(Process32FirstW(snapshot,&entry))do{if(_wcsicmp(entry.szExeFile,L"th06nc.exe")==0){if(pid){pid=0;break;}pid=entry.th32ProcessID;}}while(Process32NextW(snapshot,&entry));
    CloseHandle(snapshot);return pid;
}
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,LPWSTR,int) {
 int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);DWORD pid=0;bool launch=false;
 int language=1;
 switch(PRIMARYLANGID(GetUserDefaultUILanguage())) {case LANG_CHINESE:language=0;break;case LANG_JAPANESE:language=2;break;}
 for(int i=1;i<argc;++i) {
   if(std::wstring(argv[i])==L"--attach"&&i+1<argc)pid=wcstoul(argv[++i],nullptr,10);
   else if(std::wstring(argv[i])==L"--steam")launch=true;
   else if(std::wstring(argv[i])==L"--language") {
     if(i+1>=argc||wcslen(argv[i+1])!=1||argv[i+1][0]<L'0'||argv[i+1][0]>L'2') {
       LocalFree(argv);failure(L"Invalid --language (0=Chinese, 1=English, 2=Japanese).");return 4;
     }
     language=argv[++i][0]-L'0';
   }
 }
 LocalFree(argv);
 if(launch) {
   if(!findGame() && (INT_PTR)ShellExecuteW(nullptr,L"open",L"steam://rungameid/4659620",nullptr,nullptr,SW_SHOWNORMAL)<=32) {failure(L"无法启动 Steam。");return 1;}
   for(int i=0;i<600&&!pid;++i) {pid=findGame();if(!pid)Sleep(100);}
 }
 if(!pid)pid=findGame();
 if(!pid){failure(L"未找到 th06nc 游戏进程。");return 2;}
 bool ok=attach(pid);
 if(ok) {
   auto lock=WaitForSingleObject(mutex,1000);
   if(lock==WAIT_OBJECT_0||lock==WAIT_ABANDONED){shared->language=language;ReleaseMutex(mutex);}
   else {failure(L"Unable to synchronize the practice UI language.");ok=false;}
 }
 disconnect();return ok?0:3;
}
