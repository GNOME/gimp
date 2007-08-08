/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpwidgetstypes.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_WIDGETS_TYPES_H__
#define __GIMP_WIDGETS_TYPES_H__

#include <libgimpconfig/gimpconfigtypes.h>

#include <libgimpwidgets/gimpwidgetsenums.h>

G_BEGIN_DECLS

/* For information look into the html documentation */


typedef struct _GimpBrowser            GimpBrowser;
typedef struct _GimpButton             GimpButton;
typedef struct _GimpCellRendererColor  GimpCellRendererColor;
typedef struct _GimpCellRendererToggle GimpCellRendererToggle;
typedef struct _GimpChainButton        GimpChainButton;
typedef struct _GimpColorArea          GimpColorArea;
typedef struct _GimpColorButton        GimpColorButton;
typedef struct _GimpColorDisplay       GimpColorDisplay;
typedef struct _GimpColorDisplayStack  GimpColorDisplayStack;
typedef struct _GimpColorHexEntry      GimpColorHexEntry;
typedef struct _GimpColorNotebook      GimpColorNotebook;
typedef struct _GimpColorScale         GimpColorScale;
typedef struct _GimpColorScales        GimpColorScales;
typedef struct _GimpColorSelector      GimpColorSelector;
typedef struct _GimpColorSelect        GimpColorSelect;
typedef struct _GimpColorSelection     GimpColorSelection;
typedef struct _GimpController         GimpController;
typedef struct _GimpDialog             GimpDialog;
typedef struct _GimpEnumStore          GimpEnumStore;
typedef struct _GimpEnumComboBox       GimpEnumComboBox;
typedef struct _GimpEnumLabel          GimpEnumLabel;
typedef struct _GimpFileEntry          GimpFileEntry;
typedef struct _GimpFrame              GimpFrame;
typedef struct _GimpIntComboBox        GimpIntComboBox;
typedef struct _GimpIntStore           GimpIntStore;
typedef struct _GimpMemsizeEntry       GimpMemsizeEntry;
typedef struct _GimpNumberPairEntry    GimpNumberPairEntry;
typedef struct _GimpOffsetArea         GimpOffsetArea;
typedef struct _GimpPageSelector       GimpPageSelector;
typedef struct _GimpPathEditor         GimpPathEditor;
typedef struct _GimpPickButton         GimpPickButton;
typedef struct _GimpPreview            GimpPreview;
typedef struct _GimpPreviewArea        GimpPreviewArea;
typedef struct _GimpPixmap             GimpPixmap;
typedef struct _GimpScrolledPreview    GimpScrolledPreview;
typedef struct _GimpSizeEntry          GimpSizeEntry;
typedef struct _GimpStringComboBox     GimpStringComboBox;
typedef struct _GimpUnitMenu           GimpUnitMenu;
typedef struct _GimpZoomModel          GimpZoomModel;


typedef void (* GimpHelpFunc) (const gchar *help_id,
                               gpointer     help_data);


G_END_DECLS

#endif /* __GIMP_WIDGETS_TYPES_H__ */
