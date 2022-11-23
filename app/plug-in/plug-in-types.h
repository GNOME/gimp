/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __PLUG_IN_TYPES_H__
#define __PLUG_IN_TYPES_H__


#include "core/core-types.h"

#include "plug-in/plug-in-enums.h"


#define LIGMA_PLUG_IN_TILE_WIDTH  128
#define LIGMA_PLUG_IN_TILE_HEIGHT 128


typedef struct _LigmaPlugIn           LigmaPlugIn;
typedef struct _LigmaPlugInDebug      LigmaPlugInDebug;
typedef struct _LigmaPlugInDef        LigmaPlugInDef;
typedef struct _LigmaPlugInManager    LigmaPlugInManager;
typedef struct _LigmaPlugInMenuBranch LigmaPlugInMenuBranch;
typedef struct _LigmaPlugInProcFrame  LigmaPlugInProcFrame;
typedef struct _LigmaPlugInShm        LigmaPlugInShm;


#endif /* __PLUG_IN_TYPES_H__ */
