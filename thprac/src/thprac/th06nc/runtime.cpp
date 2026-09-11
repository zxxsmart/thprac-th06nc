#include "common.h"
#include "MinHook.h"
#include "module.h"
#include "thprac_locale_def.h"
#include <cstring>
#include <atomic>

namespace THPrac::TH06NC { uintptr_t imageBase{}; }
using namespace THPrac::TH06NC;
namespace {
uintptr_t base{}; Shared* shared{}; HANDLE mutex{}, mapping{};
Settings config{}, requested{}, initialConfig{}; Status status{};
std::atomic<int> language{1};
bool active=false, playback=false, armed=false;
uint32_t resourceLocks=0;
int lockedLives=0,lockedBombs=0,lockedPower=0;
bool retryPending=false, speedOwned=false;
double originalFps=60.0;
bool initializingPractice{}, practicePaused{};
int selectedDifficulty{};
using PlayBgm= int64_t(*)(void*,const char*); PlayBgm originalBgm{};
void* practiceMenu{};int practiceTransitionMode{};bool confirmNativePractice{};
using Transition=void(*)(void*,int,int); Transition originalTransition{};using MenuCallback=int64_t(*)(void*); MenuCallback originalMenu{};
uint32_t restartSeen=0;
uint32_t simulationTick=0;
uint32_t playbackEndTick=0;
struct Event { uint32_t tick, flags; int enabled; };
std::vector<Event> events; size_t nextEvent=0;
bool diagnostics=false;HANDLE trace=INVALID_HANDLE_VALUE;uint32_t tracedTick=UINT32_MAX;
struct ReplayHeader { uint32_t magic=0x4e435250, version=ReplayProtocol; Settings settings; uint64_t digest=0; uint32_t count=0,endTick=0; };
using Callback=int64_t(*)(void*);
Callback originalInit{}, originalUpdate{}, originalPlayer{}, originalEnemy{}, originalSupervisor{}, originalResultInit{};
using EclCallback=int64_t(*)(void*,void*);
EclCallback originalEcl{};
using VoidEcl=void(*)(void*,void*); VoidEcl originalFinal{};
using FileWriter=void(*)(const char*,void*,size_t);FileWriter originalWrite{};
template<class T> T& mem(uintptr_t rva) { return *reinterpret_cast<T*>(base+rva); }
template<class T> T& field(void* p,size_t offset) { return *reinterpret_cast<T*>(static_cast<char*>(p)+offset); }
void sync() {
    if(!shared) return;
    DWORD lock=WaitForSingleObject(mutex,0);
    if(lock!=WAIT_OBJECT_0 && lock!=WAIT_ABANDONED)return;
    if(shared->magic==Magic&&shared->version==Protocol) {
        if(valid(shared->settings))requested=shared->settings;
        if(shared->language>=0&&shared->language<=2)language.store(shared->language,std::memory_order_relaxed);
    }

    shared->status=status; ReleaseMutex(mutex);
}
void message(const wchar_t* text) { wcsncpy_s(status.message,text,_TRUNCATE); }
bool bossBgm(const Settings& settings) {
    if(settings.stage==7 && IsAddedSpell(settings.section))return true;
    return settings.section>0 && settings.section<int(std::size(THPrac::TH06::th_sections_bgm)) &&
        THPrac::TH06::th_sections_bgm[settings.section];
}
uint64_t digest(const void* data,size_t size) {
    uint64_t value=14695981039346656037ULL;
    for(size_t i=0;i<size;++i)value=(value^static_cast<const uint8_t*>(data)[i])*1099511628211ULL;
    return value;
}
bool readFile(const std::string& name,std::vector<uint8_t>& out,size_t limit) {
    HANDLE f=CreateFileA(name.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    if(f==INVALID_HANDLE_VALUE)return false;
    LARGE_INTEGER size{};bool ok=GetFileSizeEx(f,&size)&&size.QuadPart>=0&&size.QuadPart<=int64_t(limit);
    DWORD got=0;if(ok){out.resize(size_t(size.QuadPart));ok=ReadFile(f,out.data(),DWORD(out.size()),&got,nullptr)&&got==out.size();}
    CloseHandle(f);return ok;
}
bool loadReplay() {
    const char* name=reinterpret_cast<const char*>(base+Rva::ReplayPath);
    if(strnlen_s(name,260)==260)return false;
    std::vector<uint8_t> sidecar,raw;
    if(!readFile(std::string(name)+".thprac-nc",sidecar,16*1024*1024)||sidecar.size()<sizeof(ReplayHeader))return false;
    ReplayHeader header{};memcpy(&header,sidecar.data(),sizeof(header));
    if(header.magic!=0x4e435250||header.version!=ReplayProtocol||!valid(header.settings)||
       header.count>(16*1024*1024)/sizeof(Event)||sidecar.size()!=sizeof(header)+size_t(header.count)*sizeof(Event))return false;
    if(!readFile(name,raw,64*1024*1024)||digest(raw.data(),raw.size())!=header.digest)return false;
    events.resize(header.count);memcpy(events.data(),sidecar.data()+sizeof(header),events.size()*sizeof(Event));
    uint32_t tick=0;for(auto& e:events){if(e.tick<tick||(e.flags&~255u)||(e.enabled!=0&&e.enabled!=1))return false;tick=e.tick;}
    config=header.settings;nextEvent=0;playbackEndTick=header.endTick;
    return true;
}
void writeFile(const char* name,void* data,size_t size) {
    auto n=strlen(name);
    if(n>=4&&_stricmp(name+n-4,".rpy")==0&&active&&!playback) {
        ReplayHeader header;header.settings=initialConfig;header.digest=digest(data,size);header.count=uint32_t(events.size());header.endTick=simulationTick;
        std::string target=std::string(name)+".thprac-nc",tmp=target+".tmp";
        HANDLE f=CreateFileA(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);DWORD wrote=0;
        bool ok=f!=INVALID_HANDLE_VALUE;
        if(ok){ok=WriteFile(f,&header,sizeof(header),&wrote,nullptr)&&wrote==sizeof(header);
          if(ok&&!events.empty())ok=WriteFile(f,events.data(),DWORD(events.size()*sizeof(Event)),&wrote,nullptr)&&wrote==events.size()*sizeof(Event);
          FlushFileBuffers(f);CloseHandle(f);}
        if(ok)ok=MoveFileExA(tmp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
        if(!ok)message(L"录像参数文件保存失败");else {++status.exports;message(L"练习录像及参数已保存");}
    }
    originalWrite(name,data,size);
}
// TH06 routes practice exits to the native result screen's save question.
// NC normally skips that screen in Practice mode. Initialize its assets in
// state 9 (without adding a high score), then enter the native save flow.
int64_t resultInit(void* p) {
    bool practiceResult=active && !playback && mem<int>(Rva::NextState)==7;
    if(practiceResult)field<int>(p,0x9e94)=9;
    auto result=originalResultInit(p);
    if(practiceResult && result==0) {
        field<int>(p,0x9e94)=10;
        field<int>(p,0x4450)=0;
        memset(static_cast<char*>(p)+0x9e88,' ',8);
        field<char>(p,0x9e90)=0;
    }
    return result;
}
void hotkeys() {
    DWORD owner=0;GetWindowThreadProcessId(GetForegroundWindow(),&owner);
    static bool last[256]{};bool pressed[256]{};
    for(int key=0;key<256;++key){bool down=(GetAsyncKeyState(key)&0x8000)!=0;pressed[key]=down&&!last[key]&&owner==GetCurrentProcessId();last[key]=down;}
    uint32_t toggle=0;for(int i=0;i<6;++i)if(pressed[VK_F1+i])toggle|=1u<<i;
    if(pressed[VK_F7])toggle|=KeepBgm;
    bool restart=pressed['R']&&(GetAsyncKeyState(VK_ESCAPE)&0x8000);
    if(pressed[VK_BACK])ToggleQuickMenu();
    if(pressed[VK_F11])ToggleAdvancedMenu();
    if((toggle||restart)&&!playback){auto lock=WaitForSingleObject(mutex,0);if(lock==WAIT_OBJECT_0||lock==WAIT_ABANDONED){
      shared->settings.flags^=toggle;if(restart)++shared->settings.restart;requested=shared->settings;ReleaseMutex(mutex);}}

}
void speed(double fps) {
    if(mem<double>(Rva::Fps)==double(fps))return;
    auto frequency=mem<int64_t>(Rva::CounterFrequency);if(frequency<=0)return;
    auto period=std::max<int64_t>(1,int64_t(double(frequency)/fps+0.5));
    mem<double>(Rva::Fps)=double(fps);mem<int64_t>(Rva::FramePeriod)=period;
    mem<int64_t>(Rva::HalfWait)=std::min<int64_t>(period/2,int64_t(frequency*mem<double>(Rva::WaitFactor)));
    mem<uint8_t>(Rva::ResetFrameTimer)=0;
}
int64_t supervisor(void* p){
    sync();
    // Retry temporarily sets nextState to zero before registering GameInit.
    // Remember ownership before the native teardown consumes that transition.
    if(active && field<int>(p,0x428)==2 && field<int>(p,0x42c)==12)retryPending=true;
    bool inPractice=active && field<int>(p,0x428)==2 && field<int>(p,0x42c)==2;
    if(inPractice){
        hotkeys();
        if(!speedOwned){originalFps=mem<double>(Rva::Fps);speedOwned=true;}
        speed(requested.enabled?requested.fps:originalFps);
    } else if(speedOwned){speed(originalFps);speedOwned=false;}
    return originalSupervisor(p);
}
void resources() {
    mem<int8_t>(Rva::Lives)=int8_t(config.lives);mem<int8_t>(Rva::Bombs)=int8_t(config.bombs);
    mem<int16_t>(Rva::Power)=int16_t(config.power);
    mem<uint64_t>(Rva::DisplayScore)=mem<uint64_t>(Rva::Score)=config.score;
    mem<int32_t>(Rva::Graze)=mem<int32_t>(Rva::StageGraze)=config.graze;
    mem<uint16_t>(Rva::Point)=mem<uint16_t>(Rva::StagePoint)=uint16_t(config.point);
    if(config.flags&CustomRank)mem<int>(Rva::Rank)=config.rank;
    int extensions=0; while(extensions<5 && config.score>=mem<uint32_t>(Rva::ExtendScores+4*extensions))++extensions;
    mem<uint8_t>(Rva::Extends)=uint8_t(extensions);
}
void restoreResourceLocks() {
    if(resourceLocks&InfiniteLives)mem<int8_t>(Rva::Lives)=int8_t(lockedLives);
    if(resourceLocks&InfiniteBombs)mem<int8_t>(Rva::Bombs)=int8_t(lockedBombs);
    if(resourceLocks&InfinitePower)mem<int16_t>(Rva::Power)=int16_t(lockedPower);
}
void updateResourceLocks() {
    uint32_t next=active&&config.enabled?config.flags&(InfiniteLives|InfiniteBombs|InfinitePower):0;
    uint32_t rising=next&~resourceLocks;
    if(rising&InfiniteLives)lockedLives=mem<int8_t>(Rva::Lives);
    if(rising&InfiniteBombs)lockedBombs=mem<int8_t>(Rva::Bombs);
    if(rising&InfinitePower)lockedPower=mem<int16_t>(Rva::Power);
    resourceLocks=next;restoreResourceLocks();
}
void skipStageIntroduction() {
    if(config.section==0 && config.frame==0)return;
    auto gui=mem<void*>(Rva::Gui);
    if(gui){
        // The native Spell Practice initializer hides this same stage-name VM.
        auto title=static_cast<char*>(gui)+0x960;
        field<uint32_t>(title,0xc4)&=~1u;
        field<uintptr_t>(title,0xf0)=field<uintptr_t>(title,0xf8)=0;
    }
    auto playerObject=reinterpret_cast<void*>(base+Rva::Player);
    // At GameInit the player is still in state 1 (birth). Its first update
    // would enter state 3 with 240 invulnerability frames. Skip that transition.
    field<uint8_t>(playerObject,0x7898)=0;
    field<int>(playerObject,0x7854)=-999;
    field<int>(playerObject,0x7858)=0;
    field<uint32_t>(playerObject,0x798c)&=~12u;
    field<uint32_t>(playerObject,0x79b4)=0xffffffff;
    field<float>(playerObject,0x79ac)=field<float>(playerObject,0x79b0)=1.0f;
}
int64_t init(void* p) {
    sync();playback=mem<uint8_t>(Rva::Replay)!=0;resourceLocks=0;
    bool vanillaRetry=!playback && retryPending && !requested.enabled;
    if(playback)active=loadReplay();
    else {
        bool owned=retryPending || (armed && mem<uint8_t>(Rva::Practice) && !mem<uint8_t>(Rva::SpellPractice));
        if(owned)config=requested;
        active=owned && requested.enabled;
        events.clear();nextEvent=0;
    }
    armed=false;retryPending=false;practicePaused=false;ClosePause();
    initialConfig=config;simulationTick=0;restartSeen=requested.restart;
    if(active || vanillaRetry) {
        mem<int>(Rva::Stage)=config.stage-1;
        mem<int>(Rva::Difficulty)=config.stage==7?4:std::min(config.difficulty,3);
        mem<uint8_t>(Rva::Character)=uint8_t(config.shot/2);mem<uint8_t>(Rva::Shot)=uint8_t(config.shot%2);
        mem<uint8_t>(Rva::Practice)=1;
        mem<uint8_t>(Rva::SpellPractice)=0;

    }
    initializingPractice=active;
    auto result=originalInit(p);
    initializingPractice=false;
    if(active && result==0) {
        if(!(config.flags&CustomRank))config.rank=mem<int>(Rva::Rank);
        initialConfig=config;
        resources();
        thPracParam.stage=config.stage-1;thPracParam.section=config.section;thPracParam.frame=config.frame;
        thPracParam.dlg=config.dialogue;thPracParam.fakeType=config.fakeShot+1;thPracParam.phase=config.phase;
        if(!ApplyEcl()){active=false;status.error=7;message(L"脚本映射校验失败，未应用练习选段。");}
        if(!active){mem<int>(Rva::NextState)=8;sync();return result;}
        status.error=0;
        skipStageIntroduction();
        updateResourceLocks();
        if(active && EclNeedsBossAssets()) {
            using LoadAnm=int64_t(*)(void*,int,const char*,int);
            auto load=reinterpret_cast<LoadAnm>(base+Rva::LoadAnm);
            if(config.stage==6)load(nullptr,11,"data/eff06.anm",0x2d3);
            if(config.stage==7){load(nullptr,11,"data/eff07.anm",0x2d3);load(nullptr,18,"data/face12c.anm",0x4c6);}
        }
        ++status.starts;message(playback?L"已恢复练习录像参数":L"练习参数已应用");restartSeen=requested.restart;
        if(trace!=INVALID_HANDLE_VALUE){CloseHandle(trace);trace=INVALID_HANDLE_VALUE;}
        tracedTick=UINT32_MAX;
        if(diagnostics){auto name=L"practice-trace-"+std::to_wstring(status.starts)+(playback?L"-replay.csv":L"-record.csv");
          trace=CreateFileW(name.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,0,nullptr);}
    } else active=false;
    sync();return result;
}
int64_t ecl(void* manager,void* enemyObject) {
    if(!(active&&config.enabled&&config.fakeShot>=0))return originalEcl(manager,enemyObject);
    auto character=mem<uint8_t>(Rva::Character),shot=mem<uint8_t>(Rva::Shot);
    mem<uint8_t>(Rva::Character)=uint8_t(config.fakeShot/2);mem<uint8_t>(Rva::Shot)=uint8_t(config.fakeShot%2);
    auto result=originalEcl(manager,enemyObject);
    mem<uint8_t>(Rva::Character)=character;mem<uint8_t>(Rva::Shot)=shot;return result;
}
void selectParameters() {
    Settings selected{};selected.enabled=thPracParam.mode;selected.stage=thPracParam.stage+1;
    selected.difficulty=selectedDifficulty;selected.shot=MenuShot();
    selected.section=thPracParam.section;selected.frame=thPracParam.frame;
    selected.lives=int(thPracParam.life);selected.bombs=int(thPracParam.bomb);selected.power=int(thPracParam.power);
    selected.score=thPracParam.score;selected.graze=thPracParam.graze;selected.point=thPracParam.point;
    selected.rank=thPracParam.rank;selected.flags=(requested.flags&~CustomRank)|(thPracParam.customRank?CustomRank:0);
    selected.fakeShot=thPracParam.fakeType-1;selected.dialogue=thPracParam.dlg;selected.phase=thPracParam.phase;
    selected.restart=requested.restart;selected.fps=requested.fps;
    requested=selected;
    auto lock=WaitForSingleObject(mutex,100);if(lock==WAIT_OBJECT_0||lock==WAIT_ABANDONED){shared->settings=requested;ReleaseMutex(mutex);}
}
int64_t update(void* p) {
    sync();
    if(active && playback && playbackEndTick && simulationTick>=playbackEndTick){mem<int>(Rva::NextState)=8;return 3;}
    if(active) {
        uint32_t tick=simulationTick;
        if(playback){while(nextEvent<events.size()&&events[nextEvent].tick<=tick){auto e=events[nextEvent++];config.flags=e.flags;config.enabled=e.enabled;}}
        else {
            // Rank belongs to the starting parameters; pause edits apply on retry.
            auto flags=(requested.flags&~CustomRank)|(config.flags&CustomRank);
            if(config.flags!=flags||config.enabled!=requested.enabled){
                config.flags=flags;config.enabled=requested.enabled;
                events.push_back({tick,config.flags,config.enabled});
            }
        }
    }
    if(active&&!playback) {
        if(requested.restart!=restartSeen) {
            restartSeen=requested.restart;
            retryPending=true;
            mem<uint8_t>(Rva::KeepBgm)=uint8_t((config.flags&KeepBgm)&&requested.stage==config.stage&&bossBgm(config)==bossBgm(requested));
            // The native state machine handles teardown/recreation, including all callbacks.
            mem<int>(Rva::NextState)=12; // Native retry path; 3 is next-stage and reuses unloaded ANMs.
            return 3;
        }
    }
    updateResourceLocks();
    if(active && !playback && mem<int>(Rva::NextState)==2) {
        if(!practicePaused && !mem<uint8_t>(Rva::Paused) && !mem<uint8_t>(Rva::GameOver) &&
           ((mem<uint16_t>(Rva::Input)&0x400) && !(mem<uint16_t>(Rva::PreviousInput)&0x400))) {
            practicePaused=true;OpenPause();
        }
        if(practicePaused) {
            int action=TakePauseAction();
            if(action){
                practicePaused=false;ClosePause();
                mem<uint16_t>(Rva::PreviousInput)|=0x400;
                if(action==2){mem<int>(Rva::NextState)=7;return 3;}
                if(action==3){selectParameters();RequestPracticeRestart();return 3;}
            } else {
                // Keep native render preparation, and stop the update chain just
                // as the game's pause path does. Its menu state stays untouched.
                auto paused=mem<uint8_t>(Rva::Paused);
                mem<uint8_t>(Rva::Paused)=1;
                auto result=originalUpdate(p);
                mem<uint8_t>(Rva::Paused)=paused;
                return result;
            }
        }
    }
    auto result=originalUpdate(p);
    status.stage=mem<int>(Rva::Stage);status.difficulty=mem<int>(Rva::Difficulty);status.frame=mem<int>(Rva::TimelineFrame);
    status.lives=mem<int8_t>(Rva::Lives);status.bombs=mem<int8_t>(Rva::Bombs);status.power=mem<int16_t>(Rva::Power);
    status.rank=mem<int>(Rva::Rank);status.score=mem<uint64_t>(Rva::Score);
    status.flags=active?config.flags:0;
    status.misses=mem<int>(Rva::Misses);status.bombsUsed=mem<int>(Rva::BombsUsed);
    return result;
}
int64_t player(void* p) {
    bool enabled=active&&config.enabled;
    if(enabled&&(config.flags&Invincible) && (field<uint8_t>(p,0x7898)==0||field<uint8_t>(p,0x7898)==3)) {
        field<uint8_t>(p,0x7898)=3;field<int>(p,0x7858)=2;
    }
    if(enabled&&(config.flags&AutoBomb)&&field<uint8_t>(p,0x7898)==2&&mem<int8_t>(Rva::Bombs)>0) {
        mem<uint16_t>(Rva::Input)|=2;mem<uint16_t>(Rva::PreviousInput)&=~2;
    }
    // Let a locked last life take the normal miss/respawn path, without letting
    // the native zero-life branch end the practice. Restore before HUD draw.
    if((resourceLocks&InfiniteLives)&&lockedLives==0)mem<int8_t>(Rva::Lives)=1;
    auto result=originalPlayer(p);
    restoreResourceLocks();
    return result;
}
int64_t enemy(void* p) {
    // Timer locking is applied around the actual enemy callback, on the game thread.
    int timers[256]{};int timelineBefore=mem<int>(Rva::TimelineFrame);bool locked=active&&config.enabled&&(config.flags&TimeLock);
    if(locked)for(int i=0;i<256;i++)timers[i]=field<int>(p,8+i*0x10b0+4);
    auto result=originalEnemy(p);
    if(locked)for(int i=0;i<256;i++) {
        auto off=8+i*0x10b0;
        if(field<uint8_t>(p,off+0xbc)&0x80)field<int>(p,off+4)=timers[i];
    }
    if(locked && config.stage<=5 && config.stage!=3) {
        // Match TH06's midboss-only workaround. Boss introductions must keep
        // advancing so timeline interrupts can enter the actual attack.
        constexpr int starts[]={2008,2588,0,4132,3374};
        constexpr int lengths[]={48*60,32*60,0,40*60,70*60};
        constexpr int waits[]={4*60,15*60,0,12*60,5*60};
        int st=config.stage-1,start=NativeTimelineFrame(config.stage,starts[st]);
        if(timelineBefore>=start && timelineBefore<start+lengths[st])
            for(int i=0;i<256;++i)if((field<uint8_t>(p,8+i*0x10b0+0xbc)&0x80)&&(field<uint8_t>(p,8+i*0x10b0+0xbd)&8)){
                mem<int>(Rva::TimelineFrame)=std::max(timelineBefore,start+waits[st]);break;
            }
    }
    if(trace!=INVALID_HANDLE_VALUE){auto tick=simulationTick;
      if(tick%60==0&&tick!=tracedTick){tracedTick=tick;uint64_t hash=0;
        for(int i=0;i<256;++i){auto off=8+i*0x10b0;if(field<uint8_t>(p,off+0xbc)&0x80){
          hash=hash*1099511628211ULL^digest(static_cast<char*>(p)+off+0xb0,12);
          hash=hash*1099511628211ULL^uint64_t(field<int>(p,off+0x234));
          hash=hash*1099511628211ULL^uint64_t(field<int>(p,off+4));
          hash=hash*1099511628211ULL^uint64_t(field<int16_t>(p,off+0x80));}}
        char line[256];auto length=sprintf_s(line,"%u,%llu,%u,%u,%d,%d,%d,%llu\n",tick,hash,mem<uint16_t>(Rva::RandomSeed),mem<uint32_t>(Rva::RandomCount),mem<int8_t>(Rva::Lives),mem<int8_t>(Rva::Bombs),mem<int32_t>(Rva::StageGraze),mem<uint64_t>(Rva::Score));
        DWORD written;WriteFile(trace,line,DWORD(length),&written,nullptr);
      }}
    ++simulationTick;return result;
}
void finalPhase(void* enemyObject,void* instr) {
    auto age=field<int>(enemyObject,4);
    if(active&&config.enabled&&config.stage==7&&config.section==THPrac::TH06::TH06_ST7_END_S10&&config.phase==1)field<int>(enemyObject,4)=7200;
    originalFinal(enemyObject,instr);field<int>(enemyObject,4)=age;
}
void transition(void* menuObject,int origin,int mode) {
    if(origin==0x16 && field<uint8_t>(menuObject,0x168d1) && !field<uint8_t>(menuObject,0x168d3)) {
        selectedDifficulty=std::clamp(MenuDifficulty(),0,3);
        practiceMenu=menuObject;practiceTransitionMode=mode;OpenPractice();return;
    }
    originalTransition(menuObject,origin,mode);
}
int64_t playBgm(void* supervisor,const char* path) {
    if(initializingPractice && bossBgm(config)) {
        auto stageData=mem<uintptr_t>(Rva::StageData);
        if(stageData)path=reinterpret_cast<const char*>(stageData+0x310);
    }
    return originalBgm(supervisor,path);
}
int64_t menu(void* p) {
    if(practiceMenu==p) {
        int action=TakePracticeAction();
        if(!action) {
            // The native callback returns 1 to remain in the chain. Keep its
            // animation/render work running, while masking stage-menu input.
            auto input=mem<uint16_t>(Rva::Input),previous=mem<uint16_t>(Rva::PreviousInput),repeat=mem<uint16_t>(Rva::InputRepeat);
            mem<uint16_t>(Rva::Input)=mem<uint16_t>(Rva::PreviousInput)=mem<uint16_t>(Rva::InputRepeat)=0;
            auto result=originalMenu(p);
            mem<uint16_t>(Rva::Input)=input;mem<uint16_t>(Rva::PreviousInput)=previous;mem<uint16_t>(Rva::InputRepeat)=repeat;
            return result;
        }
        practiceMenu=nullptr;
        if(action==1) {
            selectParameters();
            armed=requested.enabled!=0;
            confirmNativePractice=true;
            mem<int>(Rva::Stage)=requested.stage-1;
            originalTransition(p,0x16,practiceTransitionMode);
        } else field<int>(p,0x9638)=0;
        return 1;
    }
    if(confirmNativePractice && field<int>(p,0x168b0)==0x18 && field<int>(p,0x9638)>14) {
        confirmNativePractice=false;
        auto input=mem<uint16_t>(Rva::Input),previous=mem<uint16_t>(Rva::PreviousInput);
        mem<uint16_t>(Rva::Input)=0x100;mem<uint16_t>(Rva::PreviousInput)=0;
        auto result=originalMenu(p);
        mem<uint16_t>(Rva::Input)=input;mem<uint16_t>(Rva::PreviousInput)=previous;
        return result;
    }
    if(field<int>(p,0x168b0)==3) {
        armed=false;confirmNativePractice=false;
        active=false;retryPending=false;
        if(speedOwned){speed(originalFps);speedOwned=false;}
    }
    return originalMenu(p);
}
DWORD WINAPI start(void*) {
    wchar_t diagnostic[8];diagnostics=GetEnvironmentVariableW(L"TH06NC_PRACTICE_TRACE",diagnostic,8)>0;
    imageBase=base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(Shared),mapName(GetCurrentProcessId()).c_str());
    if(!mapping)return 1;
    shared=static_cast<Shared*>(MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(Shared)));
    mutex=CreateMutexW(nullptr,FALSE,mutexName(GetCurrentProcessId()).c_str());
    if(!shared||!mutex)return 2;
    new(shared) Shared();
    if(!supportedFile(executablePath())) {status.error=1;message(L"游戏版本不匹配：仅支持已核对的 1.03 EXE");sync();return 3;}
    if(MH_Initialize()!=MH_OK) {status.error=2;message(L"初始化钩子失败");sync();return 4;}
    struct Hook {uintptr_t rva;void* target;void** original;};
    Hook hooks[]={{Rva::GameInit,(void*)init,(void**)&originalInit},{Rva::GameUpdate,(void*)update,(void**)&originalUpdate},
      {Rva::PlayerUpdate,(void*)player,(void**)&originalPlayer},{Rva::EnemyUpdate,(void*)enemy,(void**)&originalEnemy},
      {Rva::EclUpdate,(void*)ecl,(void**)&originalEcl},{Rva::FinalSpellRage,(void*)finalPhase,(void**)&originalFinal},
      {Rva::MenuUpdate,(void*)menu,(void**)&originalMenu},{Rva::MenuTransition,(void*)transition,(void**)&originalTransition},
      {Rva::PlayBgm,(void*)playBgm,(void**)&originalBgm},
      {Rva::ResultInit,(void*)resultInit,(void**)&originalResultInit},
      {Rva::WriteFile,(void*)writeFile,(void**)&originalWrite},{Rva::SupervisorUpdate,(void*)supervisor,(void**)&originalSupervisor}};
    for(auto& h:hooks)if(MH_CreateHook((void*)(base+h.rva),h.target,h.original)!=MH_OK){
        MH_Uninitialize();status.error=3;message(L"创建钩子失败，未应用补丁");sync();return 5;}
    if(!InstallOverlay()){MH_Uninitialize();status.error=6;message(L"Direct3D 11 界面挂钩初始化失败。");sync();return 7;}
    if(MH_EnableHook(MH_ALL_HOOKS)!=MH_OK){MH_Uninitialize();status.error=4;message(L"启用钩子失败");sync();return 6;}
    status.ready=1;message(L"已连接。从 Practice Start 选择单面后设置练习参数。");sync();return 0;
}
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(instance);HANDLE t=CreateThread(nullptr,0,start,nullptr,0,nullptr);if(t)CloseHandle(t);}
    return TRUE;
}

namespace THPrac::TH06NC {
int MenuStage(){return mem<int>(Rva::Stage);}
int MenuDifficulty(){return mem<int>(Rva::Difficulty);}
int NativeRank(int difficulty){return mem<int>(Rva::NativeRanks+sizeof(int)*std::clamp(difficulty,0,4));}
int MenuShot(){return mem<uint8_t>(Rva::Character)*2+mem<uint8_t>(Rva::Shot);}
bool PracticeActive(){return active && mem<int>(Rva::CurrentState)==2 && mem<int>(Rva::NextState)==2;}
uint32_t PracticeFlags(){return playback?config.flags:requested.flags;}
int PracticeLanguage(){return language.load(std::memory_order_relaxed);}
void ChangeRequested(void(*change)(Settings&)) {
    if(!active||playback)return;
    auto lock=WaitForSingleObject(mutex,100);
    if(lock==WAIT_OBJECT_0||lock==WAIT_ABANDONED){change(shared->settings);requested=shared->settings;ReleaseMutex(mutex);}
}
void RequestPracticeRestart(){ChangeRequested([](Settings& s){++s.restart;});}
int PracticeFps(){return requested.fps;}
void SetPracticeFps(int fps) {
    if(!active||playback)return;
    auto lock=WaitForSingleObject(mutex,100);
    if(lock==WAIT_OBJECT_0||lock==WAIT_ABANDONED){shared->settings.fps=std::clamp(fps,30,240);requested=shared->settings;ReleaseMutex(mutex);}
}
}
