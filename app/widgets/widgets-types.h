/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __WIDGETS_TYPES_H__
#define __WIDGETS_TYPES_H__


#include "libgimpwidgets/gimpwidgetstypes.h"

#include "core/core-types.h"


/*  enums  */

typedef enum
{
  GIMP_DROP_NONE,
  GIMP_DROP_ABOVE,
  GIMP_DROP_BELOW
} GimpDropType;

typedef enum
{
  GIMP_MOUSE_CURSOR = (GDK_LAST_CURSOR + 2),
  GIMP_CROSSHAIR_CURSOR,
  GIMP_CROSSHAIR_SMALL_CURSOR,
  GIMP_BAD_CURSOR,
  GIMP_ZOOM_CURSOR,
  GIMP_COLOR_PICKER_CURSOR,
  GIMP_LAST_CURSOR_ENTRY
} GimpCursorType;

typedef enum
{
  GIMP_TOOL_CURSOR_NONE,
  GIMP_RECT_SELECT_TOOL_CURSOR,
  GIMP_ELLIPSE_SELECT_TOOL_CURSOR,
  GIMP_FREE_SELECT_TOOL_CURSOR,
  GIMP_FUZZY_SELECT_TOOL_CURSOR,
  GIMP_BEZIER_SELECT_TOOL_CURSOR,
  GIMP_SCISSORS_TOOL_CURSOR,
  GIMP_MOVE_TOOL_CURSOR,
  GIMP_ZOOM_TOOL_CURSOR,
  GIMP_CROP_TOOL_CURSOR,
  GIMP_RESIZE_TOOL_CURSOR,
  GIMP_ROTATE_TOOL_CURSOR,
  GIMP_SHEAR_TOOL_CURSOR,
  GIMP_PERSPECTIVE_TOOL_CURSOR,
  GIMP_FLIP_HORIZONTAL_TOOL_CURSOR,
  GIMP_FLIP_VERTICAL_TOOL_CURSOR,
  GIMP_TEXT_TOOL_CURSOR,
  GIMP_COLOR_PICKER_TOOL_CURSOR,
  GIMP_BUCKET_FILL_TOOL_CURSOR,
  GIMP_BLEND_TOOL_CURSOR,
  GIMP_PENCIL_TOOL_CURSOR,
  GIMP_PAINTBRUSH_TOOL_CURSOR,
  GIMP_AIRBRUSH_TOOL_CURSOR,
  GIMP_INK_TOOL_CURSOR,
  GIMP_CLONE_TOOL_CURSOR,
  GIMP_ERASER_TOOL_CURSOR,
  GIMP_SMUDGE_TOOL_CURSOR,
  GIMP_BLUR_TOOL_CURSOR,
  GIMP_DODGE_TOOL_CURSOR,
  GIMP_BURN_TOOL_CURSOR,
  GIMP_MEASURE_TOOL_CURSOR,
  GIMP_LAST_STOCK_TOOL_CURSOR_ENTRY
} GimpToolCursorType;

typedef enum
{
  GIMP_CURSOR_MODIFIER_NONE,
  GIMP_CURSOR_MODIFIER_PLUS,
  GIMP_CURSOR_MODIFIER_MINUS,
  GIMP_CURSOR_MODIFIER_INTERSECT,
  GIMP_CURSOR_MODIFIER_MOVE,
  GIMP_CURSOR_MODIFIER_RESIZE,
  GIMP_CURSOR_MODIFIER_CONTROL,
  GIMP_CURSOR_MODIFIER_ANCHOR,
  GIMP_CURSOR_MODIFIER_FOREGROUND,
  GIMP_CURSOR_MODIFIER_BACKGROUND,
  GIMP_CURSOR_MODIFIER_PATTERN,
  GIMP_CURSOR_MODIFIER_HAND,
  GIMP_LAST_CURSOR_MODIFIER_ENTRY
} GimpCursorModifier;


/*  non-widget objects  */

typedef struct _GimpDialogFactory     GimpDialogFactory;


/*  widgets  */

typedef struct _GimpPreview           GimpPreview;
typedef struct _GimpImagePreview      GimpImagePreview;
typedef struct _GimpDrawablePreview   GimpDrawablePreview;
typedef struct _GimpBrushPreview      GimpBrushPreview;
typedef struct _GimpPatternPreview    GimpPatternPreview;
typedef struct _GimpPalettePreview    GimpPalettePreview;
typedef struct _GimpGradientPreview   GimpGradientPreview;
typedef struct _GimpToolInfoPreview   GimpToolInfoPreview;

typedef struct _GimpContainerMenu     GimpContainerMenu;
typedef struct _GimpContainerMenuImpl GimpContainerMenuImpl;

typedef struct _GimpMenuItem          GimpMenuItem;

typedef struct _GimpContainerView     GimpContainerView;
typedef struct _GimpContainerListView GimpContainerListView;
typedef struct _GimpContainerGridView GimpContainerGridView;
typedef struct _GimpDataFactoryView   GimpDataFactoryView;
typedef struct _GimpDrawableListView  GimpDrawableListView;
typedef struct _GimpLayerListView     GimpLayerListView;
typedef struct _GimpChannelListView   GimpChannelListView;

typedef struct _GimpListItem          GimpListItem;
typedef struct _GimpChannelListItem   GimpChannelListItem;
typedef struct _GimpDrawableListItem  GimpDrawableListItem;
typedef struct _GimpLayerListItem     GimpLayerListItem;
typedef struct _GimpComponentListItem GimpComponentListItem;

typedef struct _GimpDock              GimpDock;
typedef struct _GimpImageDock         GimpImageDock;
typedef struct _GimpDockable          GimpDockable;
typedef struct _GimpDockbook          GimpDockbook;

typedef struct _HistogramWidget       HistogramWidget;


/*  function types  */

typedef gchar * (* GimpItemGetNameFunc) (GtkWidget *widget);


#endif /* __WIDGETS_TYPES_H__ */
