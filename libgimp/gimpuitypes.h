/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpuitypes.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_UI_TYPES_H__
#define __GIMP_UI_TYPES_H__

#include <libgimpwidgets/gimpwidgetstypes.h>

G_BEGIN_DECLS

/* For information look into the html documentation */


typedef struct _GimpAspectPreview        GimpAspectPreview;
typedef struct _GimpDrawablePreview      GimpDrawablePreview;
typedef struct _GimpProcBrowserDialog    GimpProcBrowserDialog;
typedef struct _GimpProgressBar          GimpProgressBar;
typedef struct _GimpZoomPreview          GimpZoomPreview;

typedef struct _GimpDrawableComboBox     GimpDrawableComboBox;
typedef struct _GimpChannelComboBox      GimpChannelComboBox;
typedef struct _GimpLayerComboBox        GimpLayerComboBox;
typedef struct _GimpVectorsComboBox      GimpVectorsComboBox;
typedef struct _GimpImageComboBox        GimpImageComboBox;

typedef struct _GimpSelectButton         GimpSelectButton;
typedef struct _GimpBrushSelectButton    GimpBrushSelectButton;
typedef struct _GimpFontSelectButton     GimpFontSelectButton;
typedef struct _GimpGradientSelectButton GimpGradientSelectButton;
typedef struct _GimpPaletteSelectButton  GimpPaletteSelectButton;
typedef struct _GimpPatternSelectButton  GimpPatternSelectButton;


G_END_DECLS

#endif /* __GIMP_UI_TYPES_H__ */
