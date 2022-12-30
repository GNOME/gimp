/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * screenshot-win32-dwm-api.h
 * Copyright (C) 2018 Gil Eliyahu <gileli121@gmail.com>
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

void UnloadRequiredDwmFunctions (void);
BOOL LoadRequiredDwmFunctions (void);

typedef HRESULT (WINAPI* DWMGETWINDOWATTRIBUTE)(HWND, DWORD, _Out_ PVOID, DWORD);
DWMGETWINDOWATTRIBUTE DwmGetWindowAttribute;

typedef enum _DWMWINDOWATTRIBUTE {
  DWMWA_NCRENDERING_ENABLED = 1,
  DWMWA_NCRENDERING_POLICY,
  DWMWA_TRANSITIONS_FORCEDISABLED,
  DWMWA_ALLOW_NCPAINT,
  DWMWA_CAPTION_BUTTON_BOUNDS,
  DWMWA_NONCLIENT_RTL_LAYOUT,
  DWMWA_FORCE_ICONIC_REPRESENTATION,
  DWMWA_FLIP3D_POLICY,
  DWMWA_EXTENDED_FRAME_BOUNDS,
  DWMWA_HAS_ICONIC_BITMAP,
  DWMWA_DISALLOW_PEEK,
  DWMWA_EXCLUDED_FROM_PEEK,
  DWMWA_CLOAK,
  DWMWA_CLOAKED,
  DWMWA_FREEZE_REPRESENTATION,
  DWMWA_LAST
} DWMWINDOWATTRIBUTE;

typedef HRESULT (WINAPI* DWMISCOMPOSITIONENABLED) (BOOL *pfEnabled);
DWMISCOMPOSITIONENABLED DwmIsCompositionEnabled;

static HMODULE dwmApi = NULL;

void
UnloadRequiredDwmFunctions (void)
{
  if (! dwmApi) return;
  FreeLibrary(dwmApi);
  dwmApi = NULL;
}

BOOL
LoadRequiredDwmFunctions (void)
{
  if (dwmApi) return TRUE;

  dwmApi = LoadLibraryW (L"dwmapi");
  if (! dwmApi) return FALSE;

  DwmGetWindowAttribute = (DWMGETWINDOWATTRIBUTE) GetProcAddress (dwmApi, "DwmGetWindowAttribute");
  DwmIsCompositionEnabled = (DWMISCOMPOSITIONENABLED) GetProcAddress (dwmApi, "DwmIsCompositionEnabled");
  if (! (DwmGetWindowAttribute && DwmIsCompositionEnabled))
  {
    UnloadRequiredDwmFunctions ();
    return FALSE;
  }

  return TRUE;
}
