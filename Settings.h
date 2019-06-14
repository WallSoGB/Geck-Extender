#pragma once
int bPatchScriptEditorFont;
int bHighResLandscapeLOD;
int bListEditFix;
int bVersionControlMode;
int bFastExit;
int bIgnoreNAMFiles;
int bEnableSpellChecker;
int bAutoScroll;
int bRenderWindowUncap;
int bPreviewWindowUncap;
int bRemoveStatusBarLoadingText;
int bPlaySoundEndOfLoading;
int bNoDXSoundCaptureErrorPopup;
int bNoPreviewWindowAutoFocus;
int bNoLODMeshMessage;
int bSwapRenderCYKeys;
int bAutoLoadFiles;
int bShowLoadFilesAtStartup;
int bScriptCompileWarningPopup;
int bNoVersionControlWarning;
int bShowTimeOfDaySlider;
int bSkipVanillaLipGen;
int bShowAdditionalToolbarButtons;
int bAllowMultipleSearchAndReplace;

int bUseAltShiftMultipliers;
float fMovementAltMultiplier;
float fMovementShiftMultiplier;

int bSmoothFlycamRotation;
int bFlycamUpDownRelativeToWorld;
float fFlycamRotationSpeed;
float fFlycamNormalMovementSpeed;
float fFlycamShiftMovementSpeed;
float fFlycamAltMovementSpeed;

char filename[MAX_PATH];

#define INI_SETTING_NOT_FOUND -1
int GetOrCreateINIInt(char* sectionName, char* keyName, int defaultValue, char* filename) {
	int settingValue = GetPrivateProfileIntA(sectionName, keyName, INI_SETTING_NOT_FOUND, filename);
	if (settingValue == INI_SETTING_NOT_FOUND) {
		char arr[11];
		WritePrivateProfileString(sectionName, keyName, itoa(defaultValue, arr, 10), filename);
		settingValue = defaultValue;
	}
	return settingValue;
}
#undef INI_SETTING_NOT_FOUND
