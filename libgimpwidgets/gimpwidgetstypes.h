/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmawidgetstypes.h
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

#ifndef __LIGMA_WIDGETS_TYPES_H__
#define __LIGMA_WIDGETS_TYPES_H__

#include <libligmaconfig/ligmaconfigtypes.h>

#include <libligmawidgets/ligmawidgetsenums.h>

G_BEGIN_DECLS

/* For information look into the html documentation */


typedef struct _LigmaBrowser                   LigmaBrowser;
typedef struct _LigmaBusyBox                   LigmaBusyBox;
typedef struct _LigmaButton                    LigmaButton;
typedef struct _LigmaCellRendererColor         LigmaCellRendererColor;
typedef struct _LigmaCellRendererToggle        LigmaCellRendererToggle;
typedef struct _LigmaChainButton               LigmaChainButton;
typedef struct _LigmaColorArea                 LigmaColorArea;
typedef struct _LigmaColorButton               LigmaColorButton;
typedef struct _LigmaColorDisplay              LigmaColorDisplay;
typedef struct _LigmaColorDisplayStack         LigmaColorDisplayStack;
typedef struct _LigmaColorHexEntry             LigmaColorHexEntry;
typedef struct _LigmaColorNotebook             LigmaColorNotebook;
typedef struct _LigmaColorProfileChooserDialog LigmaColorProfileChooserDialog;
typedef struct _LigmaColorProfileComboBox      LigmaColorProfileComboBox;
typedef struct _LigmaColorProfileStore         LigmaColorProfileStore;
typedef struct _LigmaColorProfileView          LigmaColorProfileView;
typedef struct _LigmaColorScale                LigmaColorScale;
typedef struct _LigmaColorScaleEntry           LigmaColorScaleEntry;
typedef struct _LigmaColorScales               LigmaColorScales;
typedef struct _LigmaColorSelector             LigmaColorSelector;
typedef struct _LigmaColorSelect               LigmaColorSelect;
typedef struct _LigmaColorSelection            LigmaColorSelection;
typedef struct _LigmaController                LigmaController;
typedef struct _LigmaDialog                    LigmaDialog;
typedef struct _LigmaEnumStore                 LigmaEnumStore;
typedef struct _LigmaEnumComboBox              LigmaEnumComboBox;
typedef struct _LigmaEnumLabel                 LigmaEnumLabel;
typedef struct _LigmaFileEntry                 LigmaFileEntry;
typedef struct _LigmaFrame                     LigmaFrame;
typedef struct _LigmaHintBox                   LigmaHintBox;
typedef struct _LigmaIntComboBox               LigmaIntComboBox;
typedef struct _LigmaIntRadioFrame             LigmaIntRadioFrame;
typedef struct _LigmaIntStore                  LigmaIntStore;
typedef struct _LigmaLabeled                   LigmaLabeled;
typedef struct _LigmaLabelColor                LigmaLabelColor;
typedef struct _LigmaLabelEntry                LigmaLabelEntry;
typedef struct _LigmaLabelSpin                 LigmaLabelSpin;
typedef struct _LigmaMemsizeEntry              LigmaMemsizeEntry;
typedef struct _LigmaNumberPairEntry           LigmaNumberPairEntry;
typedef struct _LigmaOffsetArea                LigmaOffsetArea;
typedef struct _LigmaPageSelector              LigmaPageSelector;
typedef struct _LigmaPathEditor                LigmaPathEditor;
typedef struct _LigmaPickButton                LigmaPickButton;
typedef struct _LigmaPreview                   LigmaPreview;
typedef struct _LigmaPreviewArea               LigmaPreviewArea;
typedef struct _LigmaRuler                     LigmaRuler;
typedef struct _LigmaScaleEntry                LigmaScaleEntry;
typedef struct _LigmaScrolledPreview           LigmaScrolledPreview;
typedef struct _LigmaSizeEntry                 LigmaSizeEntry;
typedef struct _LigmaSpinButton                LigmaSpinButton;
typedef struct _LigmaStringComboBox            LigmaStringComboBox;
typedef struct _LigmaUnitComboBox              LigmaUnitComboBox;
typedef struct _LigmaUnitStore                 LigmaUnitStore;
typedef struct _LigmaZoomModel                 LigmaZoomModel;


/**
 * LigmaHelpFunc:
 * @help_id:   the help ID
 * @help_data: the help user data
 *
 * This is the prototype for all functions you pass as @help_func to
 * the various LIGMA dialog constructors like ligma_dialog_new(),
 * ligma_query_int_box() etc.
 *
 * Help IDs are textual identifiers the help system uses to figure
 * which page to display.
 *
 * All these dialog constructors functions call ligma_help_connect().
 *
 * In most cases it will be ok to use ligma_standard_help_func() which
 * does nothing but passing the @help_id string to ligma_help(). If
 * your plug-in needs some more sophisticated help handling you can
 * provide your own @help_func which has to call ligma_help() to
 * actually display the help.
 **/
typedef void (* LigmaHelpFunc) (const gchar *help_id,
                               gpointer     help_data);


G_END_DECLS

#endif /* __LIGMA_WIDGETS_TYPES_H__ */
