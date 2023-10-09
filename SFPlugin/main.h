#pragma once

#include "SAMPFUNCS_API.h"
#include "game_api.h"

void WalkEngine(bool eng);
bool EngineWork();
int GetNearWheat();
void cameraset(float X, float Y);
void EmulAlt();
float distance(int x1, int y1, int x2, int y2);
bool __stdcall onRecvRpc(stRakNetHookParams* param);

extern SAMPFUNCS *SF;
