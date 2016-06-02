.re/*
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

/*
 * Application definitions
 */
#define MAX_IMAGES 1

/* Functions which the platform-independent code will call */
TW_UINT16 callDSM (
    pTW_IDENTITY,
    pTW_IDENTITY,
    TW_UINT32,
    TW_UINT16,
    TW_UINT16,
    TW_MEMREF);

int       twainIsAvailable(void);
void      twainQuitApplication (void);
gboolean  twainSetupCallback (pTW_SESSION twSession);

TW_HANDLE twainAllocHandle(size_t size);
TW_MEMREF twainLockHandle (TW_HANDLE handle);
void      twainUnlockHandle (TW_HANDLE handle);
void      twainFreeHandle (TW_HANDLE handle);

int       twainMain (void);
int       scanImage (void);
pTW_SESSION initializeTwain (void);

void      preTransferCallback (void *);
int       beginTransferCallback (pTW_IMAGEINFO, void *);
int       dataTransferCallback (pTW_IMAGEINFO, pTW_IMAGEMEMXFER, void *);
int       endTransferCallback (int, int, void *);
void      postTransferCallback (int, void *);

extern void set_gimp_PLUG_IN_INFO_PTR(GimpPlugInInfo *);

/* Data structure holding data between runs */
/* Currently unused... Eventually may be used
 * to track dialog data.
 */
typedef struct {
  gchar  sourceName[34];
  gfloat xResolution;
  gfloat yResolution;
  gint   xOffset;
  gint   yOffset;
  gint   width;
  gint   height;
  gint   imageType;
} TwainValues;

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

#endif
