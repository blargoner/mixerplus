/*
	Mixer Plus
	Crossfader

	Author: John Peloquin
	Date: 2007
*/

#include <windows.h>
#include <tchar.h>
#include "MixerPlus.h"
#include "resource.h"

BOOL CALLBACK MixerPlus_CFaderDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_COMMAND:

			/* Pass this message to parent */
			if(LOWORD(wParam) == IDB_FADE && HIWORD(wParam) == BN_CLICKED)
			{
				SendMessage(GetParent(hDlg), uMsg, wParam, lParam);
				return TRUE;
			}

			break;
	}

	return FALSE;
}