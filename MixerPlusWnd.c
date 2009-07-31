/*
	Mixer Plus
	Main window procedure

	Author: John Peloquin
	Date: 2007
*/

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include "MixerPlus.h"
#include "resource.h"

LRESULT CALLBACK MixerPlus_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HMIXER hMixer = NULL;
	static DWORD dwLines = 0;						// Must always count plinfo correctly
	static PMIXERPLUS_INFO pinfo = NULL;
	static PMIXERPLUS_LINE_INFO plinfo = NULL;		// Array
	static HWND hCFader = NULL;

	static DWORD dwControlFlags = 0;
	static MIXERLINE mxlineDest;
	static MIXERPLUS_CONTROL_FEED feed;

	UINT i, j;
	RECT rcCtrl, rcCFader, rcWnd;
	SCROLLINFO si;
	MIXERCAPS mxcaps;
	MIXERLINE mxlineSrc;
	MIXERLINECONTROLS mxlctls;
	MIXERCONTROL mxctl;
	MIXERCONTROLDETAILS mxcdtls;
	TCHAR szTitle[MIXERPLUS_MAX_TITLE];

	switch(uMsg)
	{
		case MM_MIXM_LINE_CHANGE:

			for(i = 0; i < dwLines; i++)
				if(((DWORD) lParam) == plinfo[i].dwLineID)
				{
					SendMessage(plinfo[i].hCtrl, uMsg, wParam, lParam);
					return 0;
				}

			break;

		case MM_MIXM_CONTROL_CHANGE:

			for(i = 0; i < dwLines; i++)
			{
				/* Check if child contains control for which this message is intended */
				if(((plinfo[i].dwControlFlags & MIXERPLUS_LINE_CONTROL_VOLUME)
					&& ((DWORD) lParam) == plinfo[i].volume.dwID)
					|| ((plinfo[i].dwControlFlags & MIXERPLUS_LINE_CONTROL_MUTE)
					&& ((DWORD) lParam) == plinfo[i].mute.dwID))
				{
					SendMessage(plinfo[i].hCtrl, uMsg, wParam, lParam);
					return 0;
				}
			}

			if((dwControlFlags & MIXERPLUS_LINE_CONTROL_FEED)
				&& (((DWORD) lParam) == feed.dwID))
			{
				mxlctls.cbStruct = sizeof(mxlctls);
				mxlctls.cbmxctrl = sizeof(mxctl);
				mxlctls.dwControlID = feed.dwID;
				mxlctls.pamxctrl = &mxctl;

				if(MMERROR(mixerGetLineControls((HMIXEROBJ) hMixer,
													&mxlctls,
													MIXER_GETLINECONTROLSF_ONEBYID)))
				{
					return 0;
				}

				if(feed.dwSources != mxctl.cMultipleItems)
				{
					/* Reallocate */
					HeapFree(pinfo->hHeap, 0, feed.pmxcdtls_lt);
					HeapFree(pinfo->hHeap, 0, feed.pmxcdtls_b);

					feed.pmxcdtls_lt =
						(LPMIXERCONTROLDETAILS_LISTTEXT) HeapAlloc(pinfo->hHeap, 0,
							mxctl.cMultipleItems * sizeof(MIXERCONTROLDETAILS_LISTTEXT));

					feed.pmxcdtls_b =
						(LPMIXERCONTROLDETAILS_BOOLEAN) HeapAlloc(pinfo->hHeap, 0,
							mxctl.cMultipleItems * sizeof(MIXERCONTROLDETAILS_BOOLEAN));

					if(!feed.pmxcdtls_lt || !feed.pmxcdtls_b)
					{
						dwControlFlags &= ~MIXERPLUS_LINE_CONTROL_FEED;
						MixerPlus_ErrorBox(hWnd, TEXT("Cannot allocate memory. Check available memory."));
						DestroyWindow(hWnd);
						return 0;
					}

					feed.dwSources = mxctl.cMultipleItems;
				}

				mxcdtls.cbStruct = sizeof(mxcdtls);
				mxcdtls.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
				mxcdtls.dwControlID = feed.dwID;
				mxcdtls.cMultipleItems = feed.dwSources;
				mxcdtls.cChannels = 1;
				mxcdtls.paDetails = feed.pmxcdtls_lt;

				if(MMERROR(mixerGetControlDetails((HMIXEROBJ) hMixer,
													&mxcdtls,
													MIXER_GETCONTROLDETAILSF_LISTTEXT)))
				{
					return 0;
				}

				mxcdtls.cbStruct = sizeof(mxcdtls);
				mxcdtls.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
				mxcdtls.dwControlID = feed.dwID;
				mxcdtls.cMultipleItems = feed.dwSources;
				mxcdtls.cChannels = 1;
				mxcdtls.paDetails = feed.pmxcdtls_b;

				if(MMERROR(mixerGetControlDetails((HMIXEROBJ) hMixer,
													&mxcdtls,
													MIXER_GETCONTROLDETAILSF_VALUE)))
				{
					return 0;
				}

				for(i = 0; i < dwLines; i++)
				for(j = 0; j < feed.dwSources; j++)
					if(plinfo[i].dwLineID == feed.pmxcdtls_lt[j].dwParam1)
						CheckDlgButton(plinfo[i].hCtrl,
										IDB_POWER,
										feed.pmxcdtls_b[j].fValue ? BST_CHECKED : BST_UNCHECKED);
			}

			break;

		case WM_HSCROLL:

			/* Get scroll position */
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hWnd, SB_HORZ, &si);

			/* Adjust */
			switch(LOWORD(wParam))
			{
				case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
				case SB_LINELEFT:
				case SB_PAGELEFT:
					si.nPos--; break;
				case SB_LINERIGHT:
				case SB_PAGERIGHT:
					si.nPos++; break;
				case SB_LEFT: si.nPos = si.nMin; break;
				case SB_RIGHT: si.nPos = si.nMax; break;
			}

			SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

			/* Allow for adjustments by Windows */
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			GetScrollInfo(hWnd, SB_HORZ, &si);

			/* Move child windows */
			for(i = 0; i < dwLines; i++)
			{
				GetWindowRect(plinfo[i].hCtrl, &rcCtrl);
				MoveWindow(plinfo[i].hCtrl,
							(i - si.nPos) * (rcCtrl.right - rcCtrl.left), 0,
							(rcCtrl.right - rcCtrl.left), (rcCtrl.bottom - rcCtrl.top),
							TRUE);
			}

			return TRUE;

		case WM_COMMAND:

			/* Passed from crossfader */
			if(LOWORD(wParam) == IDB_FADE && HIWORD(wParam) == BN_CLICKED)
			{
				/* Get indices */
				i = (DWORD) SendDlgItemMessage(hCFader,
												IDC_SOURCE,
												CB_GETCURSEL,
												0, 0);

				j = (DWORD) SendDlgItemMessage(hCFader,
												IDC_TARGET,
												CB_GETCURSEL,
												0, 0);

				/* Do nothing if source is target */
				if(i == j)
					return TRUE;

				/* Start faders */
				SendMessage(plinfo[i].hCtrl,
								WM_COMMAND,
								MAKEWPARAM(IDB_FADEOUT, BN_CLICKED),
								(LPARAM) GetDlgItem(plinfo[i].hCtrl, IDB_FADEOUT));

				SendMessage(plinfo[j].hCtrl,
								WM_COMMAND,
								MAKEWPARAM(IDB_FADEIN, BN_CLICKED),
								(LPARAM) GetDlgItem(plinfo[j].hCtrl, IDB_FADEIN));

				return TRUE;
			}

			/* Passed from feed button */
			else if((dwControlFlags & MIXERPLUS_LINE_CONTROL_FEED)
					&& LOWORD(wParam) == IDB_POWER && HIWORD(wParam) == BN_CLICKED)
			{
				mxcdtls.cbStruct = sizeof(mxcdtls);
				mxcdtls.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
				mxcdtls.dwControlID = feed.dwID;
				mxcdtls.cMultipleItems = feed.dwSources;
				mxcdtls.cChannels = 1;
				mxcdtls.paDetails = feed.pmxcdtls_b;

				for(i = 0; i < dwLines; i++)
					if(((HWND) lParam) == GetDlgItem(plinfo[i].hCtrl, IDB_POWER))
					{
						for(j = 0; j < feed.dwSources; j++)
							if(plinfo[i].dwLineID == feed.pmxcdtls_lt[j].dwParam1)
							{
								ZeroMemory(feed.pmxcdtls_b, feed.dwSources * sizeof(MIXERCONTROLDETAILS_BOOLEAN));
								feed.pmxcdtls_b[j].fValue = !(IsDlgButtonChecked(plinfo[i].hCtrl, IDB_POWER) == BST_CHECKED);

								mixerSetControlDetails((HMIXEROBJ) hMixer,
														&mxcdtls,
														MIXER_SETCONTROLDETAILSF_VALUE);

								return TRUE;
							}

						break;
					}

				return TRUE;
			}

			switch(LOWORD(wParam))
			{
				case ID_FILE_EXIT:

					SendMessage(hWnd, WM_CLOSE, 0, 0);
					return 0;

				case ID_TOOLS_OPTIONS:

					/* Show options dialog box */
					if(DialogBoxParam(pinfo->hInstance,
										MAKEINTRESOURCE(IDD_OPTIONS),
										hWnd,
										(DLGPROC) MixerPlus_OptionsDlgProc,
										(LPARAM) pinfo) != IDOK)
					{
						return TRUE;
					}

					/* Destroy everything */
					for(i = 0; i < dwLines; i++)
						DestroyWindow(plinfo[i].hCtrl);

					if(hCFader)
					{
						DestroyWindow(hCFader);
						hCFader = NULL;
					}

					if(dwControlFlags & MIXERPLUS_LINE_CONTROL_FEED)
					{
						dwControlFlags &= ~MIXERPLUS_LINE_CONTROL_FEED;
						HeapFree(pinfo->hHeap, 0, feed.pmxcdtls_lt);
						HeapFree(pinfo->hHeap, 0, feed.pmxcdtls_b);
					}

					if(plinfo)
					{
						HeapFree(pinfo->hHeap, 0, plinfo);
						plinfo = NULL;
						dwLines = 0;
					}

					if(hMixer)
					{
						mixerClose(hMixer);
						hMixer = NULL;
					}

					/* Recreate */
					SendMessage(hWnd,
								WM_CREATE,
								0,
								0);		// Do not need to set this since pinfo not NULL

					return 0;

				case ID_HELP_ABOUT:

					MessageBox(hWnd,
								TEXT("Mixer Plus v0.2\n\nWritten by John Peloquin"),
								TEXT("Mixer Plus"),
								MB_ICONINFORMATION);

					return 0;
			}

			break;

		case WM_CREATE:

			/* NOTE: This message is also simulated when the user changes options */

			/* Get init info if we do not aleady have it */
			if(!pinfo)
				pinfo = (PMIXERPLUS_INFO)(((LPCREATESTRUCT) lParam)->lpCreateParams);

			/* Open mixer device */
			if(MMERROR(mixerOpen(&hMixer,
									pinfo->dwMixerID,
									(DWORD_PTR) hWnd, 0,
									MIXER_OBJECTF_MIXER | CALLBACK_WINDOW)))
			{
				MixerPlus_ErrorBox(hWnd, TEXT("Cannot open mixer device."));
				return 0;
			}

			if(!MMERROR(mixerGetDevCaps((UINT) hMixer, &mxcaps, sizeof(mxcaps))))
			{
				if(SUCCEEDED(StringCchPrintf(szTitle,
												LENGTHOF(szTitle),
												TEXT("Mixer Plus [%s - %s]"),
												mxcaps.szPname,
												(pinfo->mode == MIXERPLUS_MODE_PLAYBACK) ? TEXT("Playback") : TEXT("Recording"))))
				{
					SetWindowText(hWnd, szTitle);
				}
			}

			/* Get mixer destination line info */
			mxlineDest.cbStruct = sizeof(mxlineDest);
			mxlineDest.dwComponentType = (pinfo->mode == MIXERPLUS_MODE_PLAYBACK) ?
											MIXERLINE_COMPONENTTYPE_DST_SPEAKERS :
											MIXERLINE_COMPONENTTYPE_DST_WAVEIN;

			if(MMERROR(mixerGetLineInfo((HMIXEROBJ) hMixer,
											&mxlineDest,
											MIXER_GETLINEINFOF_COMPONENTTYPE)))
			{
				mixerClose(hMixer);
				MixerPlus_ErrorBox(hWnd, TEXT("Cannot open mixer destination line."));
				return 0;
			}

			/* The output destination line also has volume and other controls, so
				we wish to treat it like a source line. We do not do this for the
				recording destination line */
			dwLines = mxlineDest.cConnections
						+ ((pinfo->mode == MIXERPLUS_MODE_PLAYBACK) ? 1 : 0);

			/* Allocate info block memory for destination line and its associated source lines */
			if(!(plinfo = HeapAlloc(pinfo->hHeap,
										HEAP_ZERO_MEMORY,	// This is necessary!
										dwLines * sizeof(MIXERPLUS_LINE_INFO))))
			{
				mixerClose(hMixer);
				MixerPlus_ErrorBox(hWnd, TEXT("Cannot allocate memory. Check available memory."));
				return -1;
			}

			/* Create crossfader */
			if(dwLines > 0)
			{
				hCFader = CreateDialog(pinfo->hInstance,
											MAKEINTRESOURCE(IDD_CROSSFADER),
											hWnd,
											(DLGPROC) MixerPlus_CFaderDlgProc);
			}

			if(pinfo->mode == MIXERPLUS_MODE_PLAYBACK)
			{
				/* Put output destination line info at beginning of array */
				i = 0;
				plinfo[i].mode = pinfo->mode;
				plinfo[i].hHeap = pinfo->hHeap;
				plinfo[i].hMixer = hMixer;
				plinfo[i].dwLineID = mxlineDest.dwLineID;
				plinfo[i].dwChannels = mxlineDest.cChannels;
				StringCchCopy(plinfo[i].szName,
								LENGTHOF(plinfo[i].szName),
								mxlineDest.szName);

				/* Add to crossfader lists */
				SendDlgItemMessage(hCFader,
									IDC_SOURCE,
									CB_ADDSTRING,
									0,
									(LPARAM) plinfo[i].szName);

				SendDlgItemMessage(hCFader,
									IDC_TARGET,
									CB_ADDSTRING,
									0,
									(LPARAM) plinfo[i].szName);

				/* Create its child window */
				if(plinfo[i].hCtrl = CreateDialogParam(pinfo->hInstance,
														MAKEINTRESOURCE(IDD_MIXERCTRL),
														hWnd,
														(DLGPROC) MixerPlus_CtrlDlgProc,
														(LPARAM) &plinfo[i]))
				{
					ShowWindow(plinfo[i].hCtrl, SW_SHOW);
				}
			}

			/* Get ahold of selection control for recording destination line */
			else
			{
				mxlctls.cbStruct = sizeof(mxlctls);
				mxlctls.cbmxctrl = sizeof(mxctl);
				mxlctls.dwLineID = mxlineDest.dwLineID;
				mxlctls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUX;
				mxlctls.pamxctrl = &mxctl;

				if(!MMERROR(mixerGetLineControls((HMIXEROBJ) hMixer,
													&mxlctls,
													MIXER_GETLINECONTROLSF_ONEBYTYPE)))
				{
					feed.pmxcdtls_lt =
						(LPMIXERCONTROLDETAILS_LISTTEXT) HeapAlloc(pinfo->hHeap, 0,
							mxctl.cMultipleItems * sizeof(MIXERCONTROLDETAILS_LISTTEXT));

					feed.pmxcdtls_b =
						(LPMIXERCONTROLDETAILS_BOOLEAN) HeapAlloc(pinfo->hHeap, 0,
							mxctl.cMultipleItems * sizeof(MIXERCONTROLDETAILS_BOOLEAN));

					if(!feed.pmxcdtls_lt || !feed.pmxcdtls_b)
					{
						mixerClose(hMixer);
						MixerPlus_ErrorBox(hWnd, TEXT("Cannot allocate memory. Check available memory."));
						return -1;
					}

					dwControlFlags |= MIXERPLUS_LINE_CONTROL_FEED;
					feed.dwID = mxctl.dwControlID;
					feed.dwSources = mxctl.cMultipleItems;
				}
			}

			/* Get source line info and create child windows */
			for(i = 0; i < mxlineDest.cConnections; i++)
			{
				j = (pinfo->mode == MIXERPLUS_MODE_PLAYBACK) ? i + 1 : i;

				mxlineSrc.cbStruct = sizeof(mxlineSrc);
				mxlineSrc.dwDestination = mxlineDest.dwDestination;
				mxlineSrc.dwSource = i;
				mixerGetLineInfo((HMIXEROBJ) hMixer, &mxlineSrc, MIXER_GETLINEINFOF_SOURCE);

				plinfo[j].mode = pinfo->mode;
				plinfo[j].hHeap = pinfo->hHeap;
				plinfo[j].hMixer = hMixer;
				plinfo[j].dwLineID = mxlineSrc.dwLineID;
				plinfo[j].dwChannels = mxlineSrc.cChannels;
				StringCchCopy(plinfo[j].szName,
								LENGTHOF(plinfo[j].szName),
								mxlineSrc.szName);

				/* Add to crossfader lists */
				SendDlgItemMessage(hCFader,
									IDC_SOURCE,
									CB_ADDSTRING,
									0,
									(LPARAM) plinfo[j].szName);

				SendDlgItemMessage(hCFader,
									IDC_TARGET,
									CB_ADDSTRING,
									0,
									(LPARAM) plinfo[j].szName);

				if(plinfo[j].hCtrl = CreateDialogParam(pinfo->hInstance,
														MAKEINTRESOURCE(IDD_MIXERCTRL),
														hWnd,
														(DLGPROC) MixerPlus_CtrlDlgProc,
														(LPARAM) &plinfo[j]))
				{
					if(dwControlFlags & MIXERPLUS_LINE_CONTROL_FEED)
					{
						SetDlgItemText(plinfo[j].hCtrl, IDB_POWER, TEXT("Feed"));
						EnableWindow(GetDlgItem(plinfo[j].hCtrl, IDB_POWER), TRUE);
					}

					GetWindowRect(plinfo[j].hCtrl, &rcCtrl);
					MoveWindow(plinfo[j].hCtrl,
								j * (rcCtrl.right - rcCtrl.left), 0,
								rcCtrl.right - rcCtrl.left, rcCtrl.bottom - rcCtrl.top,
								FALSE);

					ShowWindow(plinfo[j].hCtrl, SW_SHOW);
				}
			}

			/* Adjust window */
			if(dwLines > 0 && hCFader)
			{
				/* Simulate feed control update if necessary */
				if(dwControlFlags & MIXERPLUS_LINE_CONTROL_FEED)
					SendMessage(hWnd,
								MM_MIXM_CONTROL_CHANGE,
								(WPARAM) hMixer,
								(LPARAM) feed.dwID);

				/* Initialize crossfader */
				SendDlgItemMessage(hCFader,
									IDC_SOURCE,
									CB_SETCURSEL,
									0, 0);

				SendDlgItemMessage(hCFader,
									IDC_TARGET,
									CB_SETCURSEL,
									0, 0);

				GetWindowRect(hCFader, &rcCFader);
				MoveWindow(hCFader,
							0, rcCtrl.bottom - rcCtrl.top,
							rcCFader.right - rcCFader.left, rcCFader.bottom - rcCFader.top,
							FALSE);

				ShowWindow(hCFader, SW_SHOW);

				/* Resize main window */
				rcWnd.left = 0;
				rcWnd.top = 0;
				rcWnd.right = MIXERPLUS_LINES_PER_VIEW * (rcCtrl.right - rcCtrl.left);
				rcWnd.bottom = (rcCtrl.bottom - rcCtrl.top) + (rcCFader.bottom - rcCFader.top);
				AdjustWindowRect(&rcWnd, WS_CAPTION, TRUE);
				rcWnd.bottom += GetSystemMetrics(SM_CYHSCROLL);

				SetWindowPos(hWnd,
								HWND_TOP,
								0, 0,
								rcWnd.right - rcWnd.left,
								rcWnd.bottom - rcWnd.top,
								SWP_NOMOVE);

				/* Set scroll range and position */
				si.cbSize = sizeof(si);
				si.fMask = SIF_RANGE | SIF_POS;
				si.nMin = 0;
				si.nMax = max(0, dwLines - MIXERPLUS_LINES_PER_VIEW);
				si.nPos = 0;
				SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
			}

			return 0;

		case WM_DESTROY:

			if(dwControlFlags & MIXERPLUS_LINE_CONTROL_FEED)
			{
				HeapFree(pinfo->hHeap, 0, feed.pmxcdtls_lt);
				HeapFree(pinfo->hHeap, 0, feed.pmxcdtls_b);
			}

			/* BUG FIX: intermittent access violation on exit.
				
				Do not free plinfo here, as the child windows
				reference it during their WM_DESTROY processing,
				occurs after this (see MSDN).
				
				This is not a huge leak, as the heap is destroyed
				immediately after the message loop.
			*/

			/*
			if(plinfo)
				HeapFree(pinfo->hHeap, 0, plinfo);
			*/

			if(hMixer)
				mixerClose(hMixer);

			/* Kill message loop */
			PostQuitMessage(0);
			return 0;
	}

	/* Pass message to default handler */
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}