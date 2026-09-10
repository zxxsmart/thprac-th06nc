#include "module.h"
#include "thprac_locale_def.h"
#include "ecl_map.h"
#include <vector>
#include <cstring>
#include <utility>
#include <stdexcept>

namespace THPrac::TH06NC {
using namespace TH06;
using std::pair;
namespace {
int warpedFrame{};
int failureOffset{};
bool needsBossAssets{};
int TranslateFrame(int oldTime) {
    for(auto& map:frameMap)if(map.stage==thPracParam.stage+1 && map.oldTime>=oldTime)
        return map.newTime+(oldTime-map.oldTime);
    throw std::runtime_error("Missing timeline anchor");
}
class ECLHelper {
    struct Write { int oldOffset; std::vector<unsigned char> bytes; };
    std::vector<Write> writes;
    int position{};
public:
    void SetPos(int p) { position=p; }
    template<class T> ECLHelper& operator<<(T value) {
        static_assert(std::is_integral_v<T>);
        auto p=reinterpret_cast<unsigned char*>(&value);
        writes.push_back({position,{p,p+sizeof(T)}});
        position+=sizeof(T);return *this;
    }
    template<class T> ECLHelper& operator<<(pair<int,T> value) {
        SetPos(value.first);return *this<<value.second;
    }
    bool Apply() {
        auto script=*reinterpret_cast<unsigned char**>(imageBase+Rva::EclFile);
        if(!script)return false;
        int stage=thPracParam.stage+1;
        struct Translated { unsigned char* dest; std::vector<unsigned char> bytes; };
        std::vector<Translated> pending;
        for(auto& write:writes) {
            // NC moved this damage-enable instruction out of the final-card
            // prelude into its setup sub. There is no prelude timestamp to edit.
            if(stage==7 && write.oldOffset==0xbe90 && write.bytes.size()==4)continue;
            const EclMap* found=nullptr;
            for(auto& map:eclMap) if(map.stage==stage && write.oldOffset>=map.oldOffset &&
                write.oldOffset+int(write.bytes.size())<=map.oldOffset+map.oldSize) {found=&map;break;}
            if(!found || write.oldOffset-found->oldOffset+int(write.bytes.size())>found->newSize){failureOffset=write.oldOffset;return false;}
            pending.push_back({script+found->newOffset+(write.oldOffset-found->oldOffset),write.bytes});
            if(found->timeline && write.oldOffset==found->oldOffset && write.bytes.size()==2){
                int16_t oldTime;memcpy(&oldTime,write.bytes.data(),2);
                // Zero means skip this event, not a frame before the new stage.
                int16_t newTime=oldTime==0?0:int16_t(TranslateFrame(oldTime));
                memcpy(pending.back().bytes.data(),&newTime,2);
            }
            if(!found->timeline && write.oldOffset==found->oldOffset && write.bytes.size()==4)
                for(auto& extra:extraTimeMap)if(extra.stage==stage && extra.oldFollowing==write.oldOffset){
                    int before,after;memcpy(&before,script+extra.newOffset,4);memcpy(&after,write.bytes.data(),4);
                    if(after<before)pending.push_back({script+extra.newOffset,write.bytes});
                }
        }
        // Translate call operands after all recipe writes have supplied their new opcode.
        for(auto& map:eclMap) if(map.stage==stage && !map.timeline && map.oldSize>=12) {
            auto instruction=script+map.newOffset;
            uint16_t opcode;memcpy(&opcode,instruction+4,2);
            for(auto& p:pending) if(p.dest<=instruction+4 && p.dest+p.bytes.size()>=instruction+6)
                memcpy(&opcode,p.bytes.data()+(instruction+4-p.dest),2);
            bool subRef=opcode==35||(opcode>=37&&opcode<=42)||opcode==95||opcode==108||opcode==109||opcode==114||opcode==116;
            if(subRef)for(auto& p:pending)if(p.dest==instruction+12&&p.bytes.size()==4) {
                int sub;memcpy(&sub,p.bytes.data(),4);bool translated=sub<0;
                for(auto& entry:subMap)if(entry.stage==stage&&entry.oldSub==sub){sub=entry.newSub;translated=true;break;}
                if(!translated){failureOffset=map.oldOffset+12;return false;}
                memcpy(p.bytes.data(),&sub,4);
            }
        }
        for(auto& p:pending)memcpy(p.dest,p.bytes.data(),p.bytes.size());
        return true;
    }
};
void ECLWarp(int32_t oldTime) {
    warpedFrame=TranslateFrame(oldTime);
}
void ECLSetHealth(ECLHelper& ecl,int offset,int32_t time,int32_t hp) {
    ecl.SetPos(offset);ecl<<time<<0x0010006f<<0x00ffff00<<hp;
}
void ECLSetTime(ECLHelper& ecl,int offset,int32_t time,int32_t timer) {
    ecl.SetPos(offset);ecl<<time<<0x00100073<<0x00ffff00<<timer;
}
void ECLStall(ECLHelper& ecl,int offset) {
    ecl.SetPos(offset);ecl<<0x99999<<0x000c0000<<0x0000ff00;
}
// These assets are normally loaded by dialogue; custom practice skips it.
void ECLNameFix() { needsBossAssets=true; }
#include "practice_patches.inl"
#include "stage_warp.inl"
}
bool ApplyEcl() {
    failureOffset=0;
    needsBossAssets=false;
    warpedFrame=thPracParam.frame;
    ECLHelper ecl;
    try {
        if(thPracParam.stage<0||thPracParam.stage>6)return false;
        auto script=*reinterpret_cast<const unsigned char**>(imageBase+Rva::EclFile);
        auto guard=scriptGuards[thPracParam.stage];
        if(!script)return false;
        MEMORY_BASIC_INFORMATION memory{};
        if(!VirtualQuery(script,&memory,sizeof(memory))||memory.State!=MEM_COMMIT||
           script+guard.size>static_cast<unsigned char*>(memory.BaseAddress)+memory.RegionSize)return false;
        uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<guard.size;++i)hash=(hash^script[i])*1099511628211ULL;
        if(hash!=guard.digest)return false;
        if(thPracParam.section>=10000) {
            int portion=thPracParam.section%100;
            constexpr int counts[]={6,4,7,9,5,2,7};
            if(portion<1||portion>counts[thPracParam.stage]||thPracParam.section/100!=101+thPracParam.stage)return false;
            THStageWarp(ecl,thPracParam.stage+1,portion);
        } else if(thPracParam.section)THPatch(ecl,static_cast<th_sections_t>(thPracParam.section));
        if(!ecl.Apply())return false;
        *reinterpret_cast<int*>(imageBase+Rva::TimelineFrame)=warpedFrame;
        return true;
    } catch(...) {return false;}
}
int EclFailureOffset() { return failureOffset; }
bool EclNeedsBossAssets() { return needsBossAssets; }
int NativeTimelineFrame(int stage,int oldTime) {
    for(auto& map:frameMap)if(map.stage==stage && map.oldTime>=oldTime)return map.newTime+(oldTime-map.oldTime);
    return oldTime;
}
}
