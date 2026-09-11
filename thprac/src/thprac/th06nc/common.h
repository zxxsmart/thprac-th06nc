#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include "sections.h"

constexpr uint32_t Magic = 0x4e435031, Protocol = 8, ReplayProtocol = 7;
constexpr wchar_t ExpectedHash[] = L"07850c8c6e469c0e82c13423e6d0d096a88d693455bdacacbb44c0aa3bcce473";
enum Flags : uint32_t { Invincible=1, InfiniteLives=2, InfiniteBombs=4, InfinitePower=8,
    TimeLock=16, AutoBomb=32, CustomRank=64, KeepBgm=128 };
struct Settings {
    int enabled=1, stage=1, difficulty=1, shot=0, spell=-1, frame=0;
    int lives=8, bombs=8, power=128, graze=0, point=0, rank=32;
    uint64_t score=0;
    uint32_t flags=0, restart=0;
    int fakeShot=-1, section=0, phase=0, dialogue=0;
    int fps=60;
    uint32_t reserved=0;
};
struct Status {
    int ready=0, error=0, stage=0, difficulty=0, frame=0, lives=0, bombs=0, power=0, rank=0;
    uint32_t starts=0;
    uint64_t score=0;
    uint32_t flags=0, exports=0;
    int misses=0, bombsUsed=0;
    wchar_t message[160]{};
};
struct Shared { uint32_t magic=Magic, version=Protocol; Settings settings{}; Status status{}; };
inline std::wstring mapName(DWORD pid) { return L"Local\\Th06NcPractice_"+std::to_wstring(pid); }
inline std::wstring mutexName(DWORD pid) { return mapName(pid)+L"_lock"; }
inline std::wstring executablePath(HMODULE module=nullptr) {
    std::wstring s(32768,L'\0'); auto n=GetModuleFileNameW(module,s.data(),DWORD(s.size())); s.resize(n); return s;
}
inline std::wstring directory(const std::wstring& p) { return p.substr(0,p.find_last_of(L"\\/")); }
inline bool supportedFile(const std::wstring& path) {
    HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    if(file==INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm{}; BCRYPT_HASH_HANDLE hash{}; bool ok=false;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0 &&
       BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0) {
        std::vector<unsigned char> buffer(65536); DWORD n=0; bool readOk=true;
        while(true) { if(!ReadFile(file,buffer.data(),DWORD(buffer.size()),&n,nullptr)) {readOk=false;break;}
          if(!n) break; if(BCryptHashData(hash,buffer.data(),n,0)<0){readOk=false;break;} }
        unsigned char digest[32]{}; if(readOk && BCryptFinishHash(hash,digest,32,0)>=0) {
          std::wstring value; const wchar_t* hex=L"0123456789abcdef";
          for(auto c:digest){value+=hex[c>>4];value+=hex[c&15];} ok=value==ExpectedHash;
        }
    }
    if(hash)BCryptDestroyHash(hash); if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0); CloseHandle(file);return ok;
}
inline bool valid(const Settings& s) {
    constexpr int lastSections[]={6,12,21,30,38,49,73};
    if(s.stage<1||s.stage>7)return false;
    bool portion=s.section/100==100+s.stage && s.section%100>=1 && s.section%100<=THPrac::TH06NC::PortionCounts[s.stage-1];
    bool boss=s.section>=(s.stage==1?1:lastSections[s.stage-2]+1) && s.section<=lastSections[s.stage-1];
    if(s.section!=0&&!portion&&!boss)return false;
    return s.stage>=1&&s.stage<=7 && s.difficulty>=0&&s.difficulty<=4 && s.shot>=0&&s.shot<=3 &&
      s.spell>=-1&&s.spell<134 && s.frame>=0&&s.frame<=32766 && s.lives>=0&&s.lives<=8 &&
      s.bombs>=0&&s.bombs<=8 && s.power>=0&&s.power<=128 && s.graze>=0&&s.graze<=99999 &&
      s.point>=0&&s.point<=65535 && s.rank>=0&&s.rank<=99 && s.score<=9999999990ULL &&
      s.fakeShot>=-1&&s.fakeShot<=3 && s.fps>=30&&s.fps<=240 && s.phase>=0&&s.phase<=1 &&
      (s.flags&~255u)==0 && (s.dialogue==0||s.dialogue==1) && (s.enabled==0||s.enabled==1);
}
