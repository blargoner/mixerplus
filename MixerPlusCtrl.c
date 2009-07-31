/*
	Mixer Plus
	Control child window procedure

	Author: John Peloquin
	Date: 2007
*/

#include <windows.h>
#include <tchar.h>
#include "MixerPlus.h"
#include "resource.h"

BOOL CALLBACK MixerPlus_CtrlDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PMIXERPLUS_LINE_INFO plinfo;

	MIXERLINECONTROLS mxlctls;
	MIXERCONTROL mxctl;
	MIXERCONTROLDETAILS mxcdtls;
	MIXERCONTROLDETAILS_BOOLEAN mxcdtls_b;

	UINT i, j;
	HWND hCtrl;
	BOOL bUniformly;
	SCROLLINFO si, sir;

	/* Initialize line info structure before processing messages */
	if(uMsg == WM_INITDIALOG)
	{
		plinfo = (PMIXERPLUS_LINE_INFO) lParam;
		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) plinfo);
	}
	else
	{
		plinfo = (PMIXERPLUS_LINE_INFO) GetWindowLongPtr(hDlg, DWLP_USER);
	}

	switch(uMsg)
	{
		case MM_MIXM_CONTROL_CHANGE:

			/* Volume control */
			if((plinfo->dwControlFlags & MIXERPLUS_LINE_CONTROL_VOLUME)
				&& (((DWORD) lParam) == plinfo->volume.dwID))
			{
				mxlctls.cbStruct = sizeof(mxcdtls);
				mxlctls.cbmxctrl = sizeof(mxctl);
				mxlctls.dwControlID = plinfo->volume.dwID;
				mxlctls.pamxctrl = &mxctl;

				if(MMERROR(mixerGetLineControls((HMIXEROBJ) plinfo->hMixer,
												&mxlctls,
												MIXER_GETLINECONTROLSF_ONEBYID)))
				{
					return TRUE;
				}

				/* We do not support weird volume controls */
				if(mxctl.fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE)
				{
					plinfo->dwControlFlags &= ~MIXERPLUS_LINE_CONTROL_VOLUME;

					EnableWindow(GetDlgItem(hDlg, IDC_LVOLUME), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_RVOLUME), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDB_FADEIN), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDB_FADEOUT), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDB_UNIFORM), FALSE);

					return TRUE;
				}

				plinfo->volume.bUniform = (plinfo->dwChannels == 1)
											|| (mxctl.fdwControl & MIXERCONTROL_CONTROLF_UNIFORM);
				plinfo->volume.dwMin = mxctl.Bounds.dwMinimum;
				plinfo->volume.dwMax = mxctl.Bounds.dwMaximum;

				mxcdtls.cbStruct = sizeof(mxcdtls);
				mxcdtls.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
				mxcdtls.dwControlID = plinfo->volume.dwID;
				mxcdtls.cMultipleItems = 0;
				mxcdtls.cChannels = plinfo->volume.bUniform ? 1 : plinfo->dwChannels;
				mxcdtls.paDetails = plinfo->volume.pmxcdtls_u;

				if(MMERROR(mixerGetControlDetails((HMIXEROBJ) plinfo->hMixer,
													&mxcdtls,
													MIXER_GETCONTROLDETAILSF_VALUE)))
				{
					return TRUE;
				}

				/* Update volume numeric displays and scroll positions */
				for(i = 0; i < 2; i++)
				{
					j = plinfo->volume.bUniform ? 0 : i;

					si.cbSize = sizeof(si);
					si.fMask = SIF_RANGE;
					GetScrollInfo(GetDlgItem(hDlg, IDC_LVOLUME + i), SB_CTL, &si);

					si.cbSize = sizeof(si);
					si.fMask = SIF_POS;
					si.nPos = si.nMax
								- (DWORD)(FLOAT)((FLOAT)(si.nMax - si.nMin)
								* (FLOAT)(plinfo->volume.pmxcdtls_u[j].dwValue - plinfo->volume.dwMin)
								/ (FLOAT)(plinfo->volume.dwMax - plinfo->volume.dwMin));

					SetScrollInfo(GetDlgItem(hDlg, IDC_LVOLUME + i),
									SB_CTL,
									&si,
									TRUE);

					/* Allow for adjustments by Windows */
					si.cbSize = sizeof(si);
					si.fMask = SIF_POS | SIF_RANGE;
					GetScrollInfo(GetDlgItem(hDlg, IDC_LVOLUME + i), SB_CTL, &si);

					SetDlgItemInt(hDlg,
									IDC_LVOLUMEVALUE + i,
									(UINT) si.nMax - si.nPos,
									TRUE);
				}

				return TRUE;
			}

			/* Mute control */
			else if((plinfo->dwControlFlags & MIXERPLUS_LINE_CONTROL_MUTE)
						&& (((DWORD) lParam) == plinfo->mute.dwID))
			{
				mxlctls.cbStruct = sizeof(mxlctls);
				mxlctls.cbmxctrl = sizeof(mxctl);
				mxlctls.dwControlID = plinfo->mute.dwID;
				mxlctls.pamxctrl = &mxctl;

				if(MMERROR(mixerGetLineControls((HMIXEROBJ) plinfo->hMixer,
												&mxlctls,
												MIXER_GETLINECONTROLSF_ONEBYID)))
				{
					return TRUE;
				}

				/* We do not support weird mute controls */
				if(mxctl.fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE)
				{
					plinfo->dwControlFlags &= ~MIXERPLUS_LINE_CONTROL_MUTE;
					plinfo->mute.dwID = 0;
					EnableWindow(GetDlgItem(hDlg, IDB_POWER), FALSE);
					return TRUE;
				}

				mxcdtls.cbStruct = sizeof(mxcdtls);
				mxcdtls.cbDetails = sizeof(mxcdtls_b);
				mxcdtls.dwControlID = plinfo->mute.dwID;
				mxcdtls.cMultipleItems = 0;
				mxcdtls.cChannels = 1;
				mxcdtls.paDetails = &mxcdtls_b;

				if(MMERROR(mixerGetControlDetails((HMIXEROBJ) plinfo->hMixer,
													&mxcdtls,
													MIXER_GETCONTROLDETAILSF_VALUE)))
				{
					return TRUE;
				}

				CheckDlgButton(hDlg, IDB_POWER, mxcdtls_b.fValue ? BST_CHECKED : BST_UNCHECKED);
			}

			break;

		case WM_VSCROLL:

			hCtrl = (HWND) lParam;

			/* One of the volume scrolls */
			if((plinfo->dwControlFlags & MIXERPLUS_LINE_CONTROL_VOLUME) &&
					(hCtrl == GetDlgItem(hDlg, IDC_LVOLUME) ||
					 hCtrl == GetDlgItem(hDlg, IDC_RVOLUME)))
			{
				/* Get position */
				si.cbSize = sizeof(si);
				si.fMask = SIF_ALL;
				GetScrollInfo(hCtrl, SB_CTL, &si);

				/* Adjust */
				switch(LOWORD(wParam))
				{
					case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
					case SB_LINEUP: si.nPos -= min(MIXERPLUS_LINE_VOLUME_STEP, si.nPos - si.nMin); break;
					case SB_LINEDOWN: si.nPos += min(MIXERPLUS_LINE_VOLUME_STEP, si.nMax - si.nPos); break;
					case SB_PAGEUP: si.nPos -= min(MIXERPLUS_LINE_VOLUME_PAGE, si.nPos - si.nMin); break;
					case SB_PAGEDOWN: si.nPos += min(MIXERPLUS_LINE_VOLUME_PAGE, si.nMax - si.nPos); break;
					case SB_TOP: si.nPos = si.nMin; break;
					case SB_BOTTOM: si.nPos = si.nMax; break;
				}

				bUniformly = plinfo->volume.bUniform
								|| (IsDlgButtonChecked(hDlg, IDB_UNIFORM) == BST_CHECKED);

				/* Update controls */
				mxcdtls.cbStruct = sizeof(mxcdtls);
				mxcdtls.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
				mxcdtls.dwControlID = plinfo->volume.dwID;
				mxcdtls.cMultipleItems = 0;
				mxcdtls.cChannels = bUniformly ? 1 : plinfo->dwChannels;
				mxcdtls.paDetails = plinfo->volume.pmxcdtls_u;

				i = bUniformly ? 0 : ((hCtrl == GetDlgItem(hDlg, IDC_LVOLUME)) ? 0 : 1);

				plinfo->volume.pmxcdtls_u[i].dwValue = plinfo->volume.dwMax
														- (DWORD)(FLOAT)((FLOAT)(plinfo->volume.dwMax - plinfo->volume.dwMin)
														* (FLOAT)(si.nPos - si.nMin)
														/ (FLOAT)(si.nMax - si.nMin));

				mixerSetControlDetails((HMIXEROBJ) plinfo->hMixer,
										&mxcdtls,
										MIXER_SETCONTROLDETAILSF_VALUE);

				return TRUE;
			}

			break;

		case WM_TIMER:

			if(wParam == MIXERPLUS_LINE_FADER_TIMER)
			{
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS | SIF_RANGE;
				GetScrollInfo(GetDlgItem(hDlg, IDC_LVOLUME), SB_CTL, &si);

				sir.cbSize = sizeof(sir);
				sir.fMask = SIF_POS | SIF_RANGE;
				GetScrollInfo(GetDlgItem(hDlg, IDC_RVOLUME), SB_CTL, &sir);

				switch(plinfo->fmode)
				{
					case MIXERPLUS_LINE_FADER_MODE_IN:

						/* If done fading in commit suicide */
						if(si.nPos == si.nMin && sir.nPos == sir.nMin)
						{
							KillTimer(hDlg, MIXERPLUS_LINE_FADER_TIMER);
							plinfo->fmode = MIXERPLUS_LINE_FADER_MODE_NONE;
							return TRUE;
						}

						/* Otherwise simulate scrolling as necessary */
						if(si.nPos > si.nMin)
						{
							SendMessage(hDlg,
										WM_VSCROLL,
										MAKEWPARAM(SB_LINEUP, 0),
										(LPARAM) GetDlgItem(hDlg, IDC_LVOLUME));
						}

						if(sir.nPos > sir.nMin)
						{
							SendMessage(hDlg,
										WM_VSCROLL,
										MAKEWPARAM(SB_LINEUP, 0),
										(LPARAM) GetDlgItem(hDlg, IDC_RVOLUME));
						}

						return TRUE;

					case MIXERPLUS_LINE_FADER_MODE_OUT:

						/* If done fading out commit suicide */
						if(si.nPos == si.nMax && sir.nPos == sir.nMax)
						{
							KillTimer(hDlg, MIXERPLUS_LINE_FADER_TIMER);
							plinfo->fmode = MIXERPLUS_LINE_FADER_MODE_NONE;
							return TRUE;
						}

						/* Otherwise simulate scrolling as necessary */
						if(si.nPos < si.nMax)
						{
							SendMessage(hDlg,
										WM_VSCROLL,
										MAKEWPARAM(SB_LINEDOWN, 0),
										(LPARAM) GetDlgItem(hDlg, IDC_LVOLUME));
						}

						if(sir.nPos < sir.nMax)
						{
							SendMessage(hDlg,
										WM_VSCROLL,
										MAKEWPARAM(SB_LINEDOWN, 0),
										(LPARAM) GetDlgItem(hDlg, IDC_RVOLUME));
						}

						return TRUE;
				}
			}

			return TRUE; // Absorb any timers

		case WM_COMMAND:

			switch(LOWORD(wParam))
			{
				/* Fade in */
				case IDB_FADEIN:

					switch(plinfo->fmode)
					{
						/* If currently doing nothing, start fading in */
						case MIXERPLUS_LINE_FADER_MODE_NONE:

							plinfo->fmode = MIXERPLUS_LINE_FADER_MODE_IN;
							SetTimer(hDlg,
										MIXERPLUS_LINE_FADER_TIMER,
										MIXERPLUS_LINE_FADER_PERIOD,
										NULL);

							break;

						/* If currently fading in, stop */
						case MIXERPLUS_LINE_FADER_MODE_IN:

							KillTimer(hDlg, MIXERPLUS_LINE_FADER_TIMER);
							plinfo->fmode = MIXERPLUS_LINE_FADER_MODE_NONE;
							break;

						/* If currently fading out, change to fading in */
						case MIXERPLUS_LINE_FADER_MODE_OUT:

							/* No need to kill and reset timer */
							plinfo->fmode = MIXERPLUS_LINE_FADER_MODE_IN;
							break;
					}

					return TRUE;

				case IDB_FADEOUT:

					switch(plinfo->fmode)
					{
						/* If currently doing nothing, start fading out */
						case MIXERPLUS_LINE_FADER_MODE_NONE:

							plinfo->fmode = MIXERPLUS_LINE_FADER_MODE_OUT;
							SetTimer(hDlg,
										MIXERPLUS_LINE_FADER_TIMER,
										MIXERPLUS_LINE_FADER_PERIOD,
										NULL);

							break;

						/* If currently fading in, change to fading out */
						case MIXERPLUS_LINE_FADER_MODE_IN:

							plinfo->fmode = MIXERPLUS_LINE_FADER_MODE_OUT;
							break;

						/* If currently fading out, stop */
						case MIXERPLUS_LINE_FADER_MODE_OUT:

							KillTimer(hDlg, MIXERPLUS_LINE_FADER_TIMER);
							plinfo->fmode = MIXERPLUS_LINE_FADER_MODE_NONE;
							break;
					}

					return TRUE;

				/* Mute or select */
				case IDB_POWER:

					if(plinfo->dwControlFlags & MIXERPLUS_LINE_CONTROL_MUTE)
					{
						/* Toggle mute */
						mxcdtls.cbStruct = sizeof(mxcdtls);
						mxcdtls.cbDetails = sizeof(mxcdtls_b);
						mxcdtls.dwControlID = plinfo->mute.dwID;
						mxcdtls.cMultipleItems = 0;
						mxcdtls.cChannels = 1;
						mxcdtls.paDetails = &mxcdtls_b;
						mxcdtls_b.fValue = !(IsDlgButtonChecked(hDlg, IDB_POWER) == BST_CHECKED);

						mixerSetControlDetails((HMIXEROBJ) plinfo->hMixer,
												&mxcdtls,
												MIXER_SETCONTROLDETAILSF_VALUE);

						return TRUE;
					}
					else
					{
						/* Pass message to parent */
						SendMessage(GetParent(hDlg), uMsg, wParam, lParam);
					}

					return TRUE;

				/* Lock volume faders (this is not an auto check button) */
				case IDB_UNIFORM:

					if(IsDlgButtonChecked(hDlg, IDB_UNIFORM) == BST_CHECKED)
					{
						/* Do not uncheck if uniform */
						if(!plinfo->volume.bUniform)
							CheckDlgButton(hDlg, IDB_UNIFORM, BST_UNCHECKED);
					}
					else
					{
						CheckDlgButton(hDlg, IDB_UNIFORM, BST_CHECKED);
					}

					return TRUE;
			}

			break;

		case WM_INITDIALOG:

			/* Set some text */
			SetDlgItemText(hDlg, IDC_TITLE, plinfo->szName);

			/* Look for volume control */
			mxlctls.cbStruct = sizeof(mxlctls);
			mxlctls.cbmxctrl = sizeof(mxctl);
			mxlctls.dwLineID = plinfo->dwLineID;
			mxlctls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
			mxlctls.pamxctrl = &mxctl;

			if(!MMERROR(mixerGetLineControls((HMIXEROBJ) plinfo->hMixer,
												&mxlctls,
												MIXER_GETLINECONTROLSF_ONEBYTYPE)))
			{
				/* We do not support volume controls with multiple items */
				if(!(mxctl.fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE))
				{
					/* NOTE: Eventually this will probably need to be moved to MM_MIXM_LINE_CHANGE */
					if(!(plinfo->volume.pmxcdtls_u
							= (LPMIXERCONTROLDETAILS_UNSIGNED) HeapAlloc(plinfo->hHeap,
								0,
								plinfo->dwChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED))))
					{
						MixerPlus_ErrorBox(hDlg, TEXT("Cannot allocate memory. Check available memory."));
						DestroyWindow(hDlg);
						return TRUE;
					}

					/* Store info */
					plinfo->dwControlFlags |= MIXERPLUS_LINE_CONTROL_VOLUME;
					plinfo->volume.dwID = mxctl.dwControlID;

					/* Set volume scrolls range */
					si.cbSize = sizeof(si);
					si.fMask = SIF_RANGE;
					si.nMin = MIXERPLUS_LINE_VOLUME_MIN;
					si.nMax = MIXERPLUS_LINE_VOLUME_MAX;
					SetScrollInfo(GetDlgItem(hDlg, IDC_LVOLUME), SB_CTL, &si, FALSE);
					SetScrollInfo(GetDlgItem(hDlg, IDC_RVOLUME), SB_CTL, &si, FALSE);

					/* Enable scrolls, fader, and lock buttons */
					EnableWindow(GetDlgItem(hDlg, IDC_LVOLUME), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_RVOLUME), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDB_FADEIN), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDB_FADEOUT), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDB_UNIFORM), TRUE);

					CheckDlgButton(hDlg, IDB_UNIFORM, BST_CHECKED);

					/* Simulate volume change to induce scroll update */
					SendMessage(hDlg,
									MM_MIXM_CONTROL_CHANGE,
									(WPARAM) plinfo->hMixer,
									(LPARAM) plinfo->volume.dwID);
				}
			}

			if(plinfo->mode == MIXERPLUS_MODE_PLAYBACK)
			{
				/* Look for mute control */
				mxlctls.cbStruct = sizeof(mxlctls);
				mxlctls.cbmxctrl = sizeof(mxctl);
				mxlctls.dwLineID = plinfo->dwLineID;
				mxlctls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
				mxlctls.pamxctrl = &mxctl;

				if(!MMERROR(mixerGetLineControls((HMIXEROBJ) plinfo->hMixer,
													&mxlctls,
													MIXER_GETLINECONTROLSF_ONEBYTYPE)))
				{
					/* We do not support weird controls */
					if(!(mxctl.fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE))
					{
						/* Store info */
						plinfo->dwControlFlags |= MIXERPLUS_LINE_CONTROL_MUTE;
						plinfo->mute.dwID = mxctl.dwControlID;

						/* Enable button */
						SetDlgItemText(hDlg, IDB_POWER, TEXT("Mute"));
						EnableWindow(GetDlgItem(hDlg, IDB_POWER),TRUE);

						SendMessage(hDlg,
									MM_MIXM_CONTROL_CHANGE,
									(WPARAM) plinfo->hMixer,
									(LPARAM) plinfo->mute.dwID);
					}
				}
			}

			return TRUE;

		case WM_DESTROY:

			/* Cleanup */
			if(plinfo->dwControlFlags & MIXERPLUS_LINE_CONTROL_VOLUME)
				HeapFree(plinfo->hHeap, 0, plinfo->volume.pmxcdtls_u);

			return TRUE;
	}

	return FALSE;
}