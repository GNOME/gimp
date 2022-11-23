/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmauitypes.h
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

#ifndef __LIGMA_UI_TYPES_H__
#define __LIGMA_UI_TYPES_H__

#include <libligmawidgets/ligmawidgetstypes.h>

G_BEGIN_DECLS

/* For information look into the html documentation */


typedef struct _LigmaProcedureDialog      LigmaProcedureDialog;
typedef struct _LigmaSaveProcedureDialog  LigmaSaveProcedureDialog;

typedef struct _LigmaAspectPreview        LigmaAspectPreview;
typedef struct _LigmaDrawablePreview      LigmaDrawablePreview;
typedef struct _LigmaProcBrowserDialog    LigmaProcBrowserDialog;
typedef struct _LigmaProgressBar          LigmaProgressBar;
typedef struct _LigmaZoomPreview          LigmaZoomPreview;

typedef struct _LigmaDrawableComboBox     LigmaDrawableComboBox;
typedef struct _LigmaChannelComboBox      LigmaChannelComboBox;
typedef struct _LigmaLayerComboBox        LigmaLayerComboBox;
typedef struct _LigmaVectorsComboBox      LigmaVectorsComboBox;
typedef struct _LigmaImageComboBox        LigmaImageComboBox;

typedef struct _LigmaSelectButton         LigmaSelectButton;
typedef struct _LigmaBrushSelectButton    LigmaBrushSelectButton;
typedef struct _LigmaFontSelectButton     LigmaFontSelectButton;
typedef struct _LigmaGradientSelectButton LigmaGradientSelectButton;
typedef struct _LigmaPaletteSelectButton  LigmaPaletteSelectButton;
typedef struct _LigmaPatternSelectButton  LigmaPatternSelectButton;


G_END_DECLS

#endif /* __LIGMA_UI_TYPES_H__ */
