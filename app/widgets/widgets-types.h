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

#include "display/display-types.h"

#include "widgets/widgets-enums.h"


/*  non-widget objects  */

typedef struct _GimpDeviceInfo          GimpDeviceInfo;
typedef struct _GimpDialogFactory       GimpDialogFactory;
typedef struct _GimpItemFactory         GimpItemFactory;


/*  widgets  */

typedef struct _GimpPreview             GimpPreview;
typedef struct _GimpImagePreview        GimpImagePreview;
typedef struct _GimpDrawablePreview     GimpDrawablePreview;
typedef struct _GimpImagefilePreview    GimpImagefilePreview;
typedef struct _GimpBrushPreview        GimpBrushPreview;
typedef struct _GimpNavigationPreview   GimpNavigationPreview;
typedef struct _GimpPatternPreview      GimpPatternPreview;
typedef struct _GimpPalettePreview      GimpPalettePreview;
typedef struct _GimpGradientPreview     GimpGradientPreview;
typedef struct _GimpToolInfoPreview     GimpToolInfoPreview;
typedef struct _GimpBufferPreview       GimpBufferPreview;

typedef struct _GimpContainerMenu       GimpContainerMenu;
typedef struct _GimpContainerMenuImpl   GimpContainerMenuImpl;

typedef struct _GimpMenuItem            GimpMenuItem;
typedef struct _GimpEnumMenu            GimpEnumMenu;

typedef struct _GimpEditor              GimpEditor;
typedef struct _GimpColormapEditor      GimpColormapEditor;
typedef struct _GimpDataEditor          GimpDataEditor;
typedef struct _GimpBrushEditor         GimpBrushEditor;
typedef struct _GimpGradientEditor      GimpGradientEditor;
typedef struct _GimpPaletteEditor       GimpPaletteEditor;

typedef struct _GimpContainerView       GimpContainerView;
typedef struct _GimpContainerListView   GimpContainerListView;
typedef struct _GimpContainerGridView   GimpContainerGridView;

typedef struct _GimpItemListView        GimpItemListView;
typedef struct _GimpDrawableListView    GimpDrawableListView;
typedef struct _GimpLayerListView       GimpLayerListView;
typedef struct _GimpChannelListView     GimpChannelListView;
typedef struct _GimpVectorsListView     GimpVectorsListView;

typedef struct _GimpContainerEditor     GimpContainerEditor;
typedef struct _GimpBufferView          GimpBufferView;
typedef struct _GimpDocumentView        GimpDocumentView;
typedef struct _GimpDataFactoryView     GimpDataFactoryView;
typedef struct _GimpBrushFactoryView    GimpBrushFactoryView;

typedef struct _GimpListItem            GimpListItem;
typedef struct _GimpItemListItem        GimpItemListItem;
typedef struct _GimpChannelListItem     GimpChannelListItem;
typedef struct _GimpDrawableListItem    GimpDrawableListItem;
typedef struct _GimpLayerListItem       GimpLayerListItem;
typedef struct _GimpComponentListItem   GimpComponentListItem;

typedef struct _GimpDock                GimpDock;
typedef struct _GimpToolbox             GimpToolbox;
typedef struct _GimpImageDock           GimpImageDock;
typedef struct _GimpDockable            GimpDockable;
typedef struct _GimpDockbook            GimpDockbook;

typedef struct _GimpFontSelection       GimpFontSelection;
typedef struct _GimpFontSelectionDialog GimpFontSelectionDialog;

typedef struct _GimpHistogramView       GimpHistogramView;


/*  function types  */

typedef gchar * (* GimpItemGetNameFunc) (GtkWidget  *widget,
					 gchar     **tooltip);


#endif /* __WIDGETS_TYPES_H__ */
