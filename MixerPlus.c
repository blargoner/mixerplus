/*
	Mixer Plus
	Startup code

	Author: John Peloquin
	Date: 2007
*/

#include <windows.h>
#include <tchar.h>
#include "MixerPlus.h"
#include "resource.h"

INT WINAPI _tWinMain(HINSTANCE hInst,
						HINSTANCE hInstPrev,
						LPTSTR strCmdLine,
						INT nCmdShow)
{
	BOOL bResult;
	HANDLE hHeap;
	HWND hWnd;
	WNDCLASSEX wcx;
	MSG msg;
	MIXERPLUS_INFO info;

	/* Create heap */
	if(!(hHeap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0)))
	{
		MixerPlus_ErrorBox(NULL, TEXT("Cannot create memory heap. Check available memory."));
		return 1;
	}

	/* Initialize info (temporary) */
	info.hInstance = hInst;
	info.hHeap = hHeap;
	info.dwMixerID = 0;
	info.mode = MIXERPLUS_MODE_PLAYBACK;

	/* Initialize window class */
	ZeroMemory(&wcx, sizeof(wcx));
	wcx.cbSize = sizeof(wcx);
	wcx.hInstance = hInst;
	wcx.lpszClassName = MIXERPLUS_WINDOW_CLASS;
	wcx.lpfnWndProc = MixerPlus_MainWndProc;
	wcx.hbrBackground = (HBRUSH) COLOR_BACKGROUND + 1;
	wcx.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MIXERPLUS));

	if(!RegisterClassEx(&wcx))
	{
		MixerPlus_ErrorBox(NULL, TEXT("Cannot register window class."));
		return 1;
	}

	hWnd = CreateWindowEx(0,
							MIXERPLUS_WINDOW_CLASS,
							TEXT("Mixer Plus"),
							MIXERPLUS_WINDOW_STYLE,
							CW_USEDEFAULT, CW_USEDEFAULT,
							CW_USEDEFAULT, CW_USEDEFAULT,
							NULL,
							LoadMenu(hInst, MAKEINTRESOURCE(IDM_MIXERPLUS)),
							hInst,
							&info);

	if(!hWnd)
		return 1; /* Ignore */

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	while((bResult = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if(bResult == -1 || !IsWindow(hWnd))
			return 1; /* Ignore */

		if(!IsDialogMessage(hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	HeapDestroy(hHeap);

	return 0;
}