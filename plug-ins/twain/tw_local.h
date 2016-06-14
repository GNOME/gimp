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

#ifndef _TW_LOCAL_H
#define _TW_LOCAL_H

/*
 * Plug-in Parameter definitions
 */
#define NUMBER_IN_ARGS 1
#define IN_ARGS \
    { GIMP_PDB_INT32, "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" }
#define NUMBER_OUT_ARGS 2
#define OUT_ARGS \
    { GIMP_PDB_INT32, "image-count", "Number of acquired images" }, \
    { GIMP_PDB_INT32ARRAY, "image-ids", "Array of acquired image identifiers" }

/* Functions which the platform-independent code will call */
TW_UINT16 callDSM (
  pTW_IDENTITY pOrigin,
  pTW_IDENTITY pDest,
  TW_UINT32    DG,
  TW_UINT16    DAT,
  TW_UINT16    MSG,
  TW_MEMREF    pData);

int       twainIsAvailable(void);
void      twainQuitApplication (void);
gboolean  twainSetupCallback (pTW_SESSION twSession);

TW_HANDLE twainAllocHandle(size_t size);
TW_MEMREF twainLockHandle (TW_HANDLE handle);
void      twainUnlockHandle (TW_HANDLE handle);
void      twainFreeHandle (TW_HANDLE handle);

int       twainMain (const gchar *name);
int       scanImage (void);
pTW_SESSION initializeTwain (void);

void      preTransferCallback (void *);
int       beginTransferCallback (pTW_IMAGEINFO, void *);
int       dataTransferCallback (pTW_IMAGEINFO, pTW_IMAGEMEMXFER, void *);
void      endTransferCallback (int, void *);
void      postTransferCallback (void *);
void      register_menu (pTW_IDENTITY dsIdentity);
void      register_scanner_menus (void);

/* Data used to carry data between each
 * of the callback function calls.
 */
typedef struct
{
  gint32 image_id;
  gint32 layer_id;
  GimpPixelRgn pixel_rgn;
  GimpDrawable *drawable;
  pTW_PALETTE8 paletteData;
  int totalPixels;
  int completedPixels;
} ClientDataStruct, *pClientDataStruct;
#endif /* _TW_LOCAL_H */
