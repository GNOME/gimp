/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Magnification-win32.h
 * Copyright (C) 2018 Gil Eliyahu
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

#include <winapifamily.h>

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#ifndef __wincodec_h__
#include <wincodec.h>
#endif

#define WC_MAGNIFIERA             "Magnifier"
#define WC_MAGNIFIERW             L"Magnifier"

#ifdef UNICODE
#define WC_MAGNIFIER              WC_MAGNIFIERW
#else
#define WC_MAGNIFIER              WC_MAGNIFIERA
#endif

#else
#define WC_MAGNIFIER              "Magnifier"
#endif

/* Magnifier Window Styles */
#define MS_SHOWMAGNIFIEDCURSOR      0x0001L
#define MS_CLIPAROUNDCURSOR         0x0002L
#define MS_INVERTCOLORS             0x0004L


/* Filter Modes */
#define MW_FILTERMODE_EXCLUDE   0
#define MW_FILTERMODE_INCLUDE   1


/* Structures */
typedef struct tagMAGTRANSFORM
{
  float v[3][3];
} MAGTRANSFORM, *PMAGTRANSFORM;

typedef struct tagMAGIMAGEHEADER
{
  UINT width;
  UINT height;
  WICPixelFormatGUID format;
  UINT stride;
  UINT offset;
  SIZE_T cbSize;
} MAGIMAGEHEADER, *PMAGIMAGEHEADER;

typedef struct tagMAGCOLOREFFECT
{
  float transform[5][5];
} MAGCOLOREFFECT, *PMAGCOLOREFFECT;


/* Proptypes for the public functions */
typedef BOOL (WINAPI* MAGINITIALIZE) ();
MAGINITIALIZE MagInitialize;

typedef BOOL (WINAPI* MAGUNINITIALIZE) ();
MAGUNINITIALIZE MagUninitialize;

typedef BOOL (WINAPI* MAGSETWINDOWSOURCE) (HWND, RECT);
MAGSETWINDOWSOURCE MagSetWindowSource;

typedef BOOL (WINAPI* MAGSETWINDOWFILTERLIST) (HWND, DWORD, int, HWND*);
MAGSETWINDOWFILTERLIST MagSetWindowFilterList;

typedef BOOL (CALLBACK* MagImageScalingCallback) (HWND hwnd, void * srcdata, MAGIMAGEHEADER srcheader, void * destdata, MAGIMAGEHEADER destheader, RECT unclipped, RECT clipped, HRGN dirty);

typedef BOOL (WINAPI* MAGSETIMAGESCALINGCALLBACK) (HWND, MagImageScalingCallback);
MAGSETIMAGESCALINGCALLBACK MagSetImageScalingCallback;


/* Library DLL */
static HINSTANCE magnificationLibrary;


void UnLoadMagnificationLibrary (void);
BOOL LoadMagnificationLibrary (void);


void
UnLoadMagnificationLibrary (void)
{
  if (!magnificationLibrary)
    return;

  FreeLibrary (magnificationLibrary);
}

BOOL
LoadMagnificationLibrary (void)
{
  if (magnificationLibrary)
    return TRUE;

  magnificationLibrary = LoadLibraryW (L"Magnification");
  if (!magnificationLibrary)
    return FALSE;

  MagInitialize = (MAGINITIALIZE) GetProcAddress (magnificationLibrary, "MagInitialize");
  if (!MagInitialize)
    {
      UnLoadMagnificationLibrary ();
      return FALSE;
    }

  MagUninitialize = (MAGUNINITIALIZE) GetProcAddress (magnificationLibrary, "MagUninitialize");
  if (!MagUninitialize)
    {
      UnLoadMagnificationLibrary ();
      return FALSE;
    }

  MagSetWindowSource = (MAGSETWINDOWSOURCE) GetProcAddress (magnificationLibrary, "MagSetWindowSource");
  if (!MagSetWindowSource)
    {
      UnLoadMagnificationLibrary ();
      return FALSE;
    }

  MagSetWindowFilterList = (MAGSETWINDOWFILTERLIST) GetProcAddress (magnificationLibrary, "MagSetWindowFilterList");
  if (!MagSetWindowFilterList)
    {
      UnLoadMagnificationLibrary ();
      return FALSE;
    }

  MagSetImageScalingCallback = (MAGSETIMAGESCALINGCALLBACK) GetProcAddress (magnificationLibrary, "MagSetImageScalingCallback");
  if (!MagSetImageScalingCallback)
    {
      UnLoadMagnificationLibrary ();
      return FALSE;
    }

  return TRUE;
}
