#include "common.h"
#include "presentation.h"
#include <tlhelp32.h>
#include <shellapi.h>
namespace {
Shared* shared{}; HANDLE mapping{},mutex{}; DWORD targetPid{};
void failure(const std::wstring& text) { MessageBoxW(nullptr,text.c_str(),L"thprac - th06nc",MB_ICONERROR); }
void disconnect() { if(shared)UnmapViewOfFile(shared);if(mapping)CloseHandle(mapping);if(mutex)CloseHandle(mutex);shared=nullptr;mapping=nullptr;mutex=nullptr;targetPid=0; }
const wchar_t* localized(int language,const wchar_t* zh,const wchar_t* en,const wchar_t* ja) {
    return language==0?zh:language==2?ja:en;
}
struct LaunchMapping {
    HANDLE handle{};LaunchOptions* view{};
    ~LaunchMapping(){if(view)UnmapViewOfFile(view);if(handle)CloseHandle(handle);}
    bool create(DWORD pid,const LaunchOptions& options){
        handle=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(LaunchOptions),launchName(pid).c_str());
        if(!handle||GetLastError()==ERROR_ALREADY_EXISTS)return false;
        view=static_cast<LaunchOptions*>(MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,sizeof(LaunchOptions)));
        if(!view)return false;
        *view=options;return true;
    }
};
uintptr_t remoteModule(DWORD pid,const std::wstring& name) {
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
    if(snapshot==INVALID_HANDLE_VALUE)return 0;
    MODULEENTRY32W item{sizeof(item)};uintptr_t result=0;
    if(Module32FirstW(snapshot,&item))do{if(_wcsicmp(item.szModule,name.c_str())==0){result=(uintptr_t)item.modBaseAddr;break;}}while(Module32NextW(snapshot,&item));
    CloseHandle(snapshot);return result;
}
bool attach(DWORD pid,const LaunchOptions& options) {
    if(shared){if(pid==targetPid)return true;disconnect();}
    HANDLE process=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,pid);
    if(!process){failure(L"无法打开游戏进程，错误 "+std::to_wstring(GetLastError()));return false;}
    wchar_t path[32768];DWORD length=32768;
    if(!QueryFullProcessImageNameW(process,0,path,&length)||!supportedFile(path)){
      CloseHandle(process);failure(L"EXE 的 SHA-256 与本地已核对的 th06nc 1.03 不一致，未注入。");return false;}
    mapping=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,mapName(pid).c_str());
    LaunchMapping launch;
    if(!mapping) {
      if(!launch.create(pid,options)){CloseHandle(process);failure(L"Unable to prepare the game launch options. Please close the game and try again.");return false;}
      for (const wchar_t* dependency : {L"freetype.dll", L"thprac_th06nc.dll"}) {
      std::wstring dll=directory(executablePath())+L"\\"+dependency;
      if(GetFileAttributesW(dll.c_str())==INVALID_FILE_ATTRIBUTES){CloseHandle(process);failure(L"缺少同目录 thprac_th06nc.dll");return false;}
      auto load=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
      MEMORY_BASIC_INFORMATION info{};VirtualQuery((void*)load,&info,sizeof(info));
      std::wstring localOwner=executablePath((HMODULE)info.AllocationBase);
      auto ownerName=localOwner.substr(localOwner.find_last_of(L"\\")+1);
      auto remoteOwner=remoteModule(pid,ownerName);
      // Steam may expose the PID before its loader has populated the module
      // list (ERROR_PARTIAL_COPY). Wait for LoadLibraryW's owning module;
      // ASLR and process visibility alone do not imply injection readiness.
      for(int i=0;!remoteOwner && i<250;++i) {
        if(WaitForSingleObject(process,20)!=WAIT_TIMEOUT)break;
        remoteOwner=remoteModule(pid,ownerName);
      }
      if(!remoteOwner){CloseHandle(process);failure(L"The game loader is not ready. Please launch the game again.");return false;}
      auto remoteLoad=remoteOwner+((uintptr_t)load-(uintptr_t)info.AllocationBase);
      SIZE_T bytes=(dll.size()+1)*sizeof(wchar_t);void* remote=VirtualAllocEx(process,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
      SIZE_T written=0;bool ok=remote&&WriteProcessMemory(process,remote,dll.c_str(),bytes,&written)&&written==bytes;
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
    if(options.practice && !shared->practiceEnabled) {
      failure(localized(options.language,L"本次游戏仅启用了低延迟显示。请退出游戏，勾选“应用 thprac”后重新启动。",
        L"This game was started without practice features. Close it, enable Apply thprac, and launch again.",
        L"練習機能なしで起動されています。ゲームを終了し、「thpracを適用する」を有効にして起動し直してください。"));
      disconnect();return false;
    }
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
 LaunchOptions options;
 int language=1;
 switch(PRIMARYLANGID(GetUserDefaultUILanguage())) {case LANG_CHINESE:language=0;break;case LANG_JAPANESE:language=2;break;}
 for(int i=1;i<argc;++i) {
   if(std::wstring(argv[i])==L"--attach"&&i+1<argc)pid=wcstoul(argv[++i],nullptr,10);
   else if(std::wstring(argv[i])==L"--steam")launch=true;
   else if(std::wstring(argv[i])==L"--low-latency")options.lowLatency=1;
   else if(std::wstring(argv[i])==L"--no-practice")options.practice=0;
   else if(std::wstring(argv[i])==L"--language") {
     if(i+1>=argc||wcslen(argv[i+1])!=1||argv[i+1][0]<L'0'||argv[i+1][0]>L'2') {
       LocalFree(argv);failure(L"Invalid --language (0=Chinese, 1=English, 2=Japanese).");return 4;
     }
     language=argv[++i][0]-L'0';
   }
 }
 LocalFree(argv);
 options.language=language;
 if(launch) {
   if(options.lowLatency && findGame()) {
     failure(localized(language,L"低延迟选项在启动时生效。请先退出正在运行的新典，再从启动器启动。",
       L"Low latency is a launch option. Close the running game, then launch it again from thprac.",
       L"低遅延モードは起動時に適用されます。起動中のゲームを終了し、thpracから起動し直してください。"));return 5;
   }
   if(!findGame() && (INT_PTR)ShellExecuteW(nullptr,L"open",L"steam://rungameid/4659620",nullptr,nullptr,SW_SHOWNORMAL)<=32) {failure(L"无法启动 Steam。");return 1;}
   for(int i=0;i<600&&!pid;++i) {pid=findGame();if(!pid)Sleep(100);}
 }
 if(!pid)pid=findGame();
 if(!pid){failure(L"未找到 th06nc 游戏进程。");return 2;}
 bool ok=attach(pid,options);
 if(ok) {
   auto lock=WaitForSingleObject(mutex,1000);
   if(lock==WAIT_OBJECT_0||lock==WAIT_ABANDONED){shared->language=language;ReleaseMutex(mutex);}
   else {failure(L"Unable to synchronize the practice UI language.");ok=false;}
 }
 if(ok && options.lowLatency) {
   for(int i=0;i<400 && shared->lowLatencyState==static_cast<int>(THPrac::TH06NC::DisplayState::Pending);++i)Sleep(50);
   if(shared->lowLatencyState!=static_cast<int>(THPrac::TH06NC::DisplayState::Active)) {
     const wchar_t* text=shared->lowLatencyState==static_cast<int>(THPrac::TH06NC::DisplayState::Unavailable)
       ?localized(language,L"低延迟显示未能启用，游戏将使用原本的显示方式。请使用 Borderless 或 Window；当前系统及显卡驱动也需支持此显示模式。",
         L"Low-latency display could not be enabled. The game will use its original display path. Use Borderless or Window; support from Windows and the graphics driver is also required.",
         L"低遅延表示を有効にできなかったため、通常の表示方式を使用します。Borderless または Window を選択してください。Windowsとグラフィックスドライバーの対応も必要です。")
       :localized(language,L"尚未确认低延迟显示已就绪。请关闭游戏后重新启动。",
         L"Low-latency display is not ready. Please close the game and launch it again.",
         L"低遅延表示の準備を確認できませんでした。ゲームを終了して起動し直してください。");
     MessageBoxW(nullptr,text,L"thprac - th06nc",MB_ICONWARNING);
   }
 }
 disconnect();return ok?0:3;
}
