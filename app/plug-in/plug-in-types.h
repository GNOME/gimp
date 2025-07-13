/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#pragma once

#include "core/core-types.h"

#include "plug-in/plug-in-enums.h"


#define GIMP_PLUG_IN_TILE_WIDTH  128
#define GIMP_PLUG_IN_TILE_HEIGHT 128


typedef struct _GimpPlugIn           GimpPlugIn;
typedef struct _GimpPlugInDebug      GimpPlugInDebug;
typedef struct _GimpPlugInDef        GimpPlugInDef;
typedef struct _GimpPlugInManager    GimpPlugInManager;
typedef struct _GimpPlugInMenuBranch GimpPlugInMenuBranch;
typedef struct _GimpPlugInProcFrame  GimpPlugInProcFrame;
typedef struct _GimpPlugInShm        GimpPlugInShm;
