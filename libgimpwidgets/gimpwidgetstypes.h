/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpwidgetstypes.h
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

#ifndef __GIMP_WIDGETS_TYPES_H__
#define __GIMP_WIDGETS_TYPES_H__

#include <libgimpconfig/gimpconfigtypes.h>

#include <libgimpwidgets/gimpwidgetsenums.h>

G_BEGIN_DECLS

/* For information look into the html documentation */


typedef struct _GimpBrowser                   GimpBrowser;
typedef struct _GimpBusyBox                   GimpBusyBox;
typedef struct _GimpButton                    GimpButton;
typedef struct _GimpCellRendererColor         GimpCellRendererColor;
typedef struct _GimpCellRendererToggle        GimpCellRendererToggle;
typedef struct _GimpChainButton               GimpChainButton;
typedef struct _GimpColorArea                 GimpColorArea;
typedef struct _GimpColorButton               GimpColorButton;
typedef struct _GimpColorDisplay              GimpColorDisplay;
typedef struct _GimpColorDisplayStack         GimpColorDisplayStack;
typedef struct _GimpColorHexEntry             GimpColorHexEntry;
typedef struct _GimpColorNotebook             GimpColorNotebook;
typedef struct _GimpColorProfileChooserDialog GimpColorProfileChooserDialog;
typedef struct _GimpColorProfileComboBox      GimpColorProfileComboBox;
typedef struct _GimpColorProfileStore         GimpColorProfileStore;
typedef struct _GimpColorProfileView          GimpColorProfileView;
typedef struct _GimpColorScale                GimpColorScale;
typedef struct _GimpColorScaleEntry           GimpColorScaleEntry;
typedef struct _GimpColorScales               GimpColorScales;
typedef struct _GimpColorSelector             GimpColorSelector;
typedef struct _GimpColorSelect               GimpColorSelect;
typedef struct _GimpColorSelection            GimpColorSelection;
typedef struct _GimpController                GimpController;
typedef struct _GimpDialog                    GimpDialog;
typedef struct _GimpEnumStore                 GimpEnumStore;
typedef struct _GimpEnumComboBox              GimpEnumComboBox;
typedef struct _GimpEnumLabel                 GimpEnumLabel;
typedef struct _GimpFileChooser               GimpFileChooser;
typedef struct _GimpFileEntry                 GimpFileEntry;
typedef struct _GimpFrame                     GimpFrame;
typedef struct _GimpHintBox                   GimpHintBox;
typedef struct _GimpIntComboBox               GimpIntComboBox;
typedef struct _GimpIntRadioFrame             GimpIntRadioFrame;
typedef struct _GimpIntStore                  GimpIntStore;
typedef struct _GimpLabeled                   GimpLabeled;
typedef struct _GimpLabelColor                GimpLabelColor;
typedef struct _GimpLabelEntry                GimpLabelEntry;
typedef struct _GimpLabelSpin                 GimpLabelSpin;
typedef struct _GimpMemsizeEntry              GimpMemsizeEntry;
typedef struct _GimpNumberPairEntry           GimpNumberPairEntry;
typedef struct _GimpOffsetArea                GimpOffsetArea;
typedef struct _GimpPageSelector              GimpPageSelector;
typedef struct _GimpPathEditor                GimpPathEditor;
typedef struct _GimpPickButton                GimpPickButton;
typedef struct _GimpPreview                   GimpPreview;
typedef struct _GimpPreviewArea               GimpPreviewArea;
typedef struct _GimpRuler                     GimpRuler;
typedef struct _GimpScaleEntry                GimpScaleEntry;
typedef struct _GimpScrolledPreview           GimpScrolledPreview;
typedef struct _GimpSizeEntry                 GimpSizeEntry;
typedef struct _GimpSpinButton                GimpSpinButton;
typedef struct _GimpStringComboBox            GimpStringComboBox;
typedef struct _GimpUnitComboBox              GimpUnitComboBox;
typedef struct _GimpUnitStore                 GimpUnitStore;
typedef struct _GimpZoomModel                 GimpZoomModel;


/**
 * GimpHelpFunc:
 * @help_id:   the help ID
 * @help_data: the help user data
 *
 * This is the prototype for all functions you pass as @help_func to
 * the various GIMP dialog constructors like gimp_dialog_new(),
 * gimp_query_int_box() etc.
 *
 * Help IDs are textual identifiers the help system uses to figure
 * which page to display.
 *
 * All these dialog constructors functions call gimp_help_connect().
 *
 * In most cases it will be ok to use gimp_standard_help_func() which
 * does nothing but passing the @help_id string to gimp_help(). If
 * your plug-in needs some more sophisticated help handling you can
 * provide your own @help_func which has to call gimp_help() to
 * actually display the help.
 **/
typedef void (* GimpHelpFunc) (const gchar *help_id,
                               gpointer     help_data);


G_END_DECLS

#endif /* __GIMP_WIDGETS_TYPES_H__ */
