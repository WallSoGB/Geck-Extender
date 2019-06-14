#pragma once
void InsertListViewItem(HWND ListViewHandle, void* Parameter, bool UseImage, int ItemIndex);
void InsertComboBoxItem(HWND ComboBoxHandle, const char* DisplayText, void* Value, bool AllowResize);
void BeginUIDefer();
void EndUIDefer();