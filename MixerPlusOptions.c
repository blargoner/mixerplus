/*
	Mixer Plus
	Options dialog

	Author: John Peloquin
	Date: 2007
*/

#include <windows.h>
#include <tchar.h>
#include "MixerPlus.h"
#include "resource.h"

BOOL CALLBACK MixerPlus_OptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static PMIXERPLUS_INFO pinfo = NULL;

	UINT i;
	MIXERCAPS mxcaps;

	switch(uMsg)
	{
		case WM_COMMAND:

			switch(LOWORD(wParam))
			{
				case IDOK:

					pinfo->dwMixerID = (DWORD) SendDlgItemMessage(hDlg,
																	IDC_DEVICE,
																	CB_GETCURSEL, 0, 0);

					if(IsDlgButtonChecked(hDlg, IDC_MODE_PLAYBACK) == BST_CHECKED)
						pinfo->mode = MIXERPLUS_MODE_PLAYBACK;

					else pinfo->mode = MIXERPLUS_MODE_RECORDING;

					EndDialog(hDlg, IDOK);
					return TRUE;

				case IDCANCEL:

					EndDialog(hDlg, IDCANCEL);
					return TRUE;
			}

			break;

		case WM_INITDIALOG:

			pinfo = (PMIXERPLUS_INFO) lParam;

			for(i = 0; i < mixerGetNumDevs(); i++)
				if(!MMERROR(mixerGetDevCaps(i, &mxcaps, sizeof(mxcaps))))
				{
					SendDlgItemMessage(hDlg,
										IDC_DEVICE,
										CB_ADDSTRING,
										0,
										(LPARAM) mxcaps.szPname);
				}

			SendDlgItemMessage(hDlg,
								IDC_DEVICE,
								CB_SETCURSEL,
								pinfo->dwMixerID,
								0);

			if(pinfo->mode == MIXERPLUS_MODE_PLAYBACK)
				CheckDlgButton(hDlg, IDC_MODE_PLAYBACK, BST_CHECKED);

			else CheckDlgButton(hDlg, IDC_MODE_RECORDING, BST_CHECKED);

			return TRUE;
	}

	return FALSE;
}