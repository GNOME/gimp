/*
 * TWAIN Plug-in
 * Copyright (C) 1999 Craig Setera
 * Craig Setera <setera@home.com>
 * 03/31/1999
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

#ifndef _TW_PLATFORM_H
#define _TW_PLATFORM_H

#include <windows.h>
#include "twain.h"

/* The DLL to be loaded for TWAIN support */
#define TWAIN_DLL_NAME_W L"TWAIN_32.DLL"
#define TWAIN_DLL_NAME "TWAIN_32.DLL"
#define DEBUG_LOGFILE "c:\\twain.log"
#define DUMP_FILE "C:\\TWAINCAP.BIN"
#define DUMP_NAME "DTWAIN.EXE"
#define READDUMP_NAME "RTWAIN.EXE"

#endif
