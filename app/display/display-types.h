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

#ifndef __DISPLAY_TYPES_H__
#define __DISPLAY_TYPES_H__


#include "propgui/propgui-types.h"

#include "display/display-enums.h"


typedef struct _LigmaCanvas               LigmaCanvas;
typedef struct _LigmaCanvasGroup          LigmaCanvasGroup;
typedef struct _LigmaCanvasItem           LigmaCanvasItem;

typedef struct _LigmaDisplayShell         LigmaDisplayShell;
typedef struct _LigmaMotionBuffer         LigmaMotionBuffer;

typedef struct _LigmaImageWindow          LigmaImageWindow;
typedef struct _LigmaMultiWindowStrategy  LigmaMultiWindowStrategy;
typedef struct _LigmaSingleWindowStrategy LigmaSingleWindowStrategy;

typedef struct _LigmaCursorView           LigmaCursorView;
typedef struct _LigmaNavigationEditor     LigmaNavigationEditor;
typedef struct _LigmaScaleComboBox        LigmaScaleComboBox;
typedef struct _LigmaStatusbar            LigmaStatusbar;

typedef struct _LigmaToolDialog           LigmaToolDialog;
typedef struct _LigmaToolGui              LigmaToolGui;
typedef struct _LigmaToolWidget           LigmaToolWidget;
typedef struct _LigmaToolWidgetGroup      LigmaToolWidgetGroup;

typedef struct _LigmaDisplayXfer          LigmaDisplayXfer;
typedef struct _Selection                Selection;

typedef struct _LigmaModifiersManager     LigmaModifiersManager;


#endif /* __DISPLAY_TYPES_H__ */
