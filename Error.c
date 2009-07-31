/*
	Mixer Plus
	Error handler

	Author: John Peloquin
	Date: 2007
*/

#include <windows.h>
#include <tchar.h>

VOID MixerPlus_ErrorBox(HWND hWnd, LPCTSTR strText)
{
	MessageBox(hWnd, strText, TEXT("Mixer Plus Error"), MB_OK | MB_ICONERROR);
}