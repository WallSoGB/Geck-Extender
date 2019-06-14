#pragma once

struct NiPoint3
{
	float x, y, z;
};

struct ViewMatrix
{
	float x1, x2, x3, x4, x5, x6, x7, x8, x9;
};

struct posEtc {
	void* unk0[0xD];
	ViewMatrix viewMatrix; // 38
	NiPoint3 pos; // 58
	void* unk64[0xC];
	NiPoint3 lastPos; // 8C
};

struct lpMem {
	void* unk0;
	posEtc* unk4;
	posEtc* unk8;
};


lpMem* GetLPMem();

// functions for getting and setting the render window camera
void GetCameraViewMatrix(ViewMatrix* out);
void SetCameraViewMatrix(ViewMatrix* newView);

void GetCameraPos(NiPoint3* outPosition);
void SetCameraPos(NiPoint3* newPos);

void GetCamera(NiPoint3* outPosition, ViewMatrix* outDirection);
void SetCamera(NiPoint3* pos, ViewMatrix* direction);

void SetTimeOfDay(float time);

void ToggleRenderWindowAllowCellLoads(bool toggle);
bool GetIsRenderWindowAllowCellLoads();

void SetIsShowWater(bool state);
bool GetIsShowWater();

bool GetIsShowPortalsAndRooms();
void SetIsShowPortalsAndRooms(bool state);

long GetPreviousRenderWindowSize();
int GetPreviousRenderWindowWidth();
int GetPreviousRenderWindowHeight();

bool GetIsShowLightMarkers();
void SetIsShowLightMarkers(bool state);

HWND g_renderWindowHwnd = (HWND)0xE0C1AA;
HWND g_mainWindowToolbar = (HWND)0xECFC14;

UInt32* (*LookupGeckFormByID)(UInt32 formID) = (UInt32 * (*)(UInt32))0x4F9620;