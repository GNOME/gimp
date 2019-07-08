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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _TW_LOCAL_H
#define _TW_LOCAL_H

#include "tw_func.h"

/* Functions which the platform-independent code will call */

TW_UINT16 callDSM(pTW_IDENTITY, pTW_IDENTITY,
		  TW_UINT32, TW_UINT16,
		  TW_UINT16, TW_MEMREF);

int twainIsAvailable(void);
void twainQuitApplication (void);
gboolean twainSetupCallback (pTW_SESSION twSession);

TW_HANDLE twainAllocHandle(size_t size);
TW_MEMREF twainLockHandle (TW_HANDLE handle);
void twainUnlockHandle (TW_HANDLE handle);
void twainFreeHandle (TW_HANDLE handle);

int twainMain (void);
int scanImage (void);

#endif
