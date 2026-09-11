#pragma once
#include "practice_param.h"
#include "addresses.h"
#include "sections.h"
#include <Windows.h>
namespace THPrac::TH06NC {
extern uintptr_t imageBase;
extern THPracParam thPracParam;
int MenuStage();
int MenuDifficulty();
int MenuShot();
int NativeRank(int difficulty);
int PracticeLanguage();
bool InstallOverlay();
void OpenPractice();
void ClosePractice();
bool PracticeIsOpen();
// Called on the game thread. 0 = pending, 1 = confirm, 2 = cancel.
int TakePracticeAction();
void DrawPractice();
bool ApplyEcl();
int EclFailureOffset();
bool EclNeedsBossAssets();
int NativeTimelineFrame(int stage,int oldTime);
bool PracticeActive();
uint32_t PracticeFlags();
void RequestPracticeRestart();
int PracticeFps();
void SetPracticeFps(int fps);
void ToggleQuickMenu();
void ToggleAdvancedMenu();
void OpenPause();
void ClosePause();
int TakePauseAction();
}
