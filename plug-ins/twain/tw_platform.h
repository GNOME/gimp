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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TW_PLATFORM_H
#define _TW_PLATFORM_H

    /* Coding style violation: Don't include headers in headers */
    #include <windows.h>
    /* Coding style violation: Don't include headers in headers */
    #define _MSWIN_ /* Definition for TWAIN.H */  
    #include "twain.h"

/* The DLL to be loaded for TWAIN support */
#define TWAIN_DLL_NAME "TWAIN_32.DLL"

/* Windows uses separate entry point */
#define TWAIN_ALTERNATE_MAIN

  /*
   * Plug-in Definitions
   */
  #define PRODUCT_FAMILY      "GNU"
  #define PRODUCT_NAME        "GIMP"
  #define PLUG_IN_NAME        "TWAIN"
  #define PLUG_IN_DESCRIPTION N_("Capture an image from a TWAIN datasource")
  #define PLUG_IN_HELP        "This plug-in will capture an image from a TWAIN datasource"
  #define PLUG_IN_AUTHOR      "Craig Setera (setera@home.com)"
  #define PLUG_IN_COPYRIGHT   "Craig Setera"
  #define PLUG_IN_MAJOR       0
  #define PLUG_IN_MINOR       6
  #define PLUG_IN_VERSION     "v0.6 (07/22/2004)"
  #define MID_SELECT          "twain-acquire"
#endif
