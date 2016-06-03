/*
 * TWAIN Plug-in
 * Copyright (C) 1999 Craig Setera
 * Craig Setera <setera@home.com>
 * 03/31/1999
 *
 * Added for Win x64 support, changed data source selection
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

#ifndef _TW_PLATFORM_H
  #define _TW_PLATFORM_H

    /* Coding style violation: Don't include headers in headers */
    #include <windows.h>
    /* Coding style violation: Don't include headers in headers */
    #include "twain.h"

    /* The DLL to be loaded for TWAIN support */
    #ifdef TWH_64BIT
      #define TWAIN_DLL_NAME "TWAINDSM.DLL"
    #else
      #define TWAIN_DLL_NAME "TWAIN_32.DLL"
    #endif /* TWH_64BIT */

  /* Windows uses separate entry point */
  #define TWAIN_ALTERNATE_MAIN

  /*
   * Plug-in Definitions
   */
  #define STRINGIFY(x) #x
  #define TOSTRING(x) STRINGIFY(x)

  #define PRODUCT_FAMILY      "GNU"
  #define PRODUCT_NAME        "GIMP"
  #define PLUG_IN_NAME        "TWAIN"
  #define PLUG_IN_DESCRIPTION N_("Capture an image from a TWAIN datasource")
  #define PLUG_IN_HELP        "This plug-in will capture an image from a TWAIN datasource.\n" \
                              "Recent changes:\n" \
                              "  - Support of Win 64 Bit (requires twaindsm.dll from twain.org)\n" \
                              "  - Fixed bug while scanning multiple images\n" \
                              "  - Added easy Scanner access"
  #define PLUG_IN_AUTHOR      "Jens Plonka (jens.plonka@gmx.de)"
  #define PLUG_IN_COPYRIGHT   "Craig Setera"
  #define PLUG_IN_MAJOR       0
  #define PLUG_IN_MINOR       7
  #define PLUG_IN_DATE        "11/25/2011"
  #define PLUG_IN_VERSION     "v" TOSTRING(PLUG_IN_MAJOR) "." TOSTRING(PLUG_IN_MINOR) " (" PLUG_IN_DATE ")"
  #define PLUG_IN_HINT        PRODUCT_NAME " " PLUG_IN_NAME " v" TOSTRING(PLUG_IN_MAJOR) "." TOSTRING(PLUG_IN_MINOR)

  #define MID_SELECT          "twain-acquire"
  #define MP_SELECT           "<Image>/File/Create/Acquire"
#endif
