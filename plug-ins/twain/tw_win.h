/*
 * TWAIN Plug-in
 * Copyright (C) 1999 Craig Setera
 * Craig Setera <setera@home.com>
 * 03/31/1999
 *
 * Updated for Mac OS X support
 * Brion Vibber <brion@pobox.com>
 * 07/22/2004
 *
 * Added for Win x64 support, changed data source selection.
 * Jens M. Plonka <jens.plonka@gmx.de>
 * 11/25/2011
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Revision history
 *  (02/07/99)  v0.1   First working version (internal)
 *  (02/09/99)  v0.2   First release to anyone other than myself
 *  (02/15/99)  v0.3   Added image dump and read support for debugging
 *  (03/31/99)  v0.5   Added support for multi-byte samples and paletted
 *                     images.
 *  (07/23/04)  v0.6   Added Mac OS X support.
 *  (11/25/11)  v0.7   Added Win x64 support, changed data source selection.
 */

/*
 * Windows platform-specific code
 */
#ifndef _TW_WIN_H
#define _TW_WIN_H

#define SHOW_WINDOW 0
#define WM_TRANSFER_IMAGE (WM_USER + 100)

TW_UINT16 callDSM (
  pTW_IDENTITY pOrigin,
  pTW_IDENTITY pDest,
  TW_UINT32    DG,
  TW_UINT16    DAT,
  TW_UINT16    MSG,
  TW_MEMREF    pData);

extern GimpPlugInInfo PLUG_IN_INFO;

LRESULT CALLBACK WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
int     twainMessageLoop (pTW_SESSION);
int     TwainProcessMessage (LPMSG lpMsg, pTW_SESSION twSession);
void    LogLastWinError (void);
void    twainQuitApplication (void);
BOOL    InitApplication (HINSTANCE hInstance);
BOOL    InitInstance (HINSTANCE hInstance, int nCmdShow, pTW_SESSION twSession);
int     twainMain (void);
extern  pTW_SESSION initializeTwain (void);

#endif /* _TW_WIN_H */
