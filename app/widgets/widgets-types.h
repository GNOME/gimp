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
