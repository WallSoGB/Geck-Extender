#pragma once

#define UI_CMD_ADDLOGTEXT	(WM_APP + 1)
#define UI_CMD_CLEARLOGTEXT (WM_APP + 2)
#define UI_CMD_AUTOSCROLL	(WM_APP + 3)

#define UI_EXTMENU_ID			51001
#define UI_EXTMENU_SHOWLOG		51002
#define UI_EXTMENU_CLEARLOG		51003
#define UI_EXTMENU_AUTOSCROLL	51004
#define UI_EXTMENU_SPACER		51005
#define UI_EXTMENU_SPELLCHECK	51006
#define UI_EXTMENU_RENDER		51007
#define UI_EXTMENU_PREVIEW		51008
#define UI_EXTMENU_LAUNCHGAME   51009
#define UI_EXTMENU_SAVEPOSITION 51010
#define UI_EXTMENU_LOADPOSITION 51011
#define ID_TRACKBAR				51012
#define ID_TIMEOFDAYTEXT		51013
#define ID_RENDERWINDOWCELLLOADS_CHECKBOX 51014
#define ID_RENDERWINDOW_SHOWWATER_CHECKBOX	51015
#define ID_RENDERWINDOW_SHOWPORTALS_CHECKBOX	51016

// unused button in vanilla menu
#define VIEW_RENDER_WINDOW 0x9D06

HWND g_MainHwnd;
HWND g_ConsoleHwnd;
HMENU g_ExtensionMenu;
HMENU g_MainMenu;
WNDPROC OldEditorUI_WndProc;

/* Additional Toolbar Controls */
HWND g_trackBarHwnd;
HWND g_timeOfDayTextHwnd;
HWND g_allowCellWindowLoadsButtonHwnd;
HWND g_renderWindowShowWaterButtonHwnd;
HWND g_renderWindowShowPortalsButtonHwnd;

void EditorUI_LogVa(const char* Format, va_list Va);
void EditorUI_Log(const char* Format, ...);
int __cdecl EditorUI_Log2(const char* Format, ...);
bool EditorUI_CreateLogWindow();
LRESULT CALLBACK EditorUI_WndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam);