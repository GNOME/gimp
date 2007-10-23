/* GIMP - The GNU Image Manipulation Program
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

#include "widgets/widgets-enums.h"


/*  input devices & controllers  */

typedef struct _GimpDeviceInfo               GimpDeviceInfo;
typedef struct _GimpControllerInfo           GimpControllerInfo;
typedef struct _GimpControllerKeyboard       GimpControllerKeyboard;
typedef struct _GimpControllerWheel          GimpControllerWheel;


/*  docks  */

typedef struct _GimpDock                     GimpDock;
typedef struct _GimpDockSeparator            GimpDockSeparator; /* not a dock */
typedef struct _GimpImageDock                GimpImageDock;
typedef struct _GimpMenuDock                 GimpMenuDock;
typedef struct _GimpToolbox                  GimpToolbox;
typedef struct _GimpDockbook                 GimpDockbook;
typedef struct _GimpDockable                 GimpDockable;
typedef struct _GimpDocked                   GimpDocked; /* dummy typedef */


/*  GimpEditor widgets  */

typedef struct _GimpEditor                   GimpEditor;
typedef struct _GimpColorEditor              GimpColorEditor;
typedef struct _GimpCursorView               GimpCursorView;
typedef struct _GimpDeviceStatus             GimpDeviceStatus;
typedef struct _GimpErrorConsole             GimpErrorConsole;
typedef struct _GimpToolOptionsEditor        GimpToolOptionsEditor;


/*  GimpDataEditor widgets  */

typedef struct _GimpDataEditor               GimpDataEditor;
typedef struct _GimpBrushEditor              GimpBrushEditor;
typedef struct _GimpGradientEditor           GimpGradientEditor;
typedef struct _GimpPaletteEditor            GimpPaletteEditor;


/*  GimpImageEditor widgets  */

typedef struct _GimpImageEditor              GimpImageEditor;
typedef struct _GimpColormapEditor           GimpColormapEditor;
typedef struct _GimpComponentEditor          GimpComponentEditor;
typedef struct _GimpHistogramEditor          GimpHistogramEditor;
typedef struct _GimpSamplePointEditor        GimpSamplePointEditor;
typedef struct _GimpSelectionEditor          GimpSelectionEditor;
typedef struct _GimpUndoEditor               GimpUndoEditor;


/*  GimpContainerView and its implementors  */

typedef struct _GimpContainerView            GimpContainerView; /* dummy typedef */
typedef struct _GimpContainerBox             GimpContainerBox;
typedef struct _GimpContainerComboBox        GimpContainerComboBox;
typedef struct _GimpContainerEntry           GimpContainerEntry;
typedef struct _GimpContainerGridView        GimpContainerGridView;
typedef struct _GimpContainerTreeView        GimpContainerTreeView;
typedef struct _GimpItemTreeView             GimpItemTreeView;
typedef struct _GimpDrawableTreeView         GimpDrawableTreeView;
typedef struct _GimpLayerTreeView            GimpLayerTreeView;
typedef struct _GimpChannelTreeView          GimpChannelTreeView;
typedef struct _GimpVectorsTreeView          GimpVectorsTreeView;

typedef struct _GimpContainerPopup           GimpContainerPopup;
typedef struct _GimpViewableButton           GimpViewableButton;


/*  GimpContainerEditor widgets  */

typedef struct _GimpContainerEditor          GimpContainerEditor;
typedef struct _GimpBufferView               GimpBufferView;
typedef struct _GimpDocumentView             GimpDocumentView;
typedef struct _GimpFontView                 GimpFontView;
typedef struct _GimpImageView                GimpImageView;
typedef struct _GimpTemplateView             GimpTemplateView;
typedef struct _GimpToolView                 GimpToolView;


/*  GimpDataFactoryView widgets  */

typedef struct _GimpDataFactoryView          GimpDataFactoryView;
typedef struct _GimpBrushFactoryView         GimpBrushFactoryView;
typedef struct _GimpPatternFactoryView       GimpPatternFactoryView;


/*  menus  */

typedef struct _GimpActionFactory            GimpActionFactory;
typedef struct _GimpActionGroup              GimpActionGroup;
typedef struct _GimpAction                   GimpAction;
typedef struct _GimpEnumAction               GimpEnumAction;
typedef struct _GimpPlugInAction             GimpPlugInAction;
typedef struct _GimpStringAction             GimpStringAction;
typedef struct _GimpMenuFactory              GimpMenuFactory;
typedef struct _GimpUIManager                GimpUIManager;


/*  misc dialogs  */

typedef struct _GimpColorDialog              GimpColorDialog;
typedef struct _GimpErrorDialog              GimpErrorDialog;
typedef struct _GimpFileDialog               GimpFileDialog;
typedef struct _GimpMessageDialog            GimpMessageDialog;
typedef struct _GimpProfileChooserDialog     GimpProfileChooserDialog;
typedef struct _GimpProgressDialog           GimpProgressDialog;
typedef struct _GimpTextEditor               GimpTextEditor;
typedef struct _GimpToolDialog               GimpToolDialog;
typedef struct _GimpViewableDialog           GimpViewableDialog;


/*  GimpPdbDialog widgets  */

typedef struct _GimpPdbDialog                GimpPdbDialog;
typedef struct _GimpBrushSelect              GimpBrushSelect;
typedef struct _GimpGradientSelect           GimpGradientSelect;
typedef struct _GimpPaletteSelect            GimpPaletteSelect;
typedef struct _GimpPatternSelect            GimpPatternSelect;
typedef struct _GimpFontSelect               GimpFontSelect;


/*  misc widgets  */

typedef struct _GimpActionView               GimpActionView;
typedef struct _GimpBlobEditor               GimpBlobEditor;
typedef struct _GimpColorBar                 GimpColorBar;
typedef struct _GimpColorDisplayEditor       GimpColorDisplayEditor;
typedef struct _GimpColorFrame               GimpColorFrame;
typedef struct _GimpColorPanel               GimpColorPanel;
typedef struct _GimpControllerEditor         GimpControllerEditor;
typedef struct _GimpControllerList           GimpControllerList;
typedef struct _GimpDashEditor               GimpDashEditor;
typedef struct _GimpFgBgEditor               GimpFgBgEditor;
typedef struct _GimpFgBgView                 GimpFgBgView;
typedef struct _GimpFileProcView             GimpFileProcView;
typedef struct _GimpGridEditor               GimpGridEditor;
typedef struct _GimpHistogramBox             GimpHistogramBox;
typedef struct _GimpHistogramView            GimpHistogramView;
typedef struct _GimpImageCommentEditor       GimpImageCommentEditor;
typedef struct _GimpImageParasiteView        GimpImageParasiteView;
typedef struct _GimpImagePropView            GimpImagePropView;
typedef struct _GimpImageProfileView         GimpImageProfileView;
typedef struct _GimpMessageBox               GimpMessageBox;
typedef struct _GimpProgressBox              GimpProgressBox;
typedef struct _GimpSizeBox                  GimpSizeBox;
typedef struct _GimpStrokeEditor             GimpStrokeEditor;
typedef struct _GimpTemplateEditor           GimpTemplateEditor;
typedef struct _GimpThumbBox                 GimpThumbBox;


/*  views  */

typedef struct _GimpView                     GimpView;
typedef struct _GimpPaletteView              GimpPaletteView;
typedef struct _GimpNavigationView           GimpNavigationView;


/*  view renderers  */

typedef struct _GimpViewRenderer             GimpViewRenderer;
typedef struct _GimpViewRendererBrush        GimpViewRendererBrush;
typedef struct _GimpViewRendererBuffer       GimpViewRendererBuffer;
typedef struct _GimpViewRendererDrawable     GimpViewRendererDrawable;
typedef struct _GimpViewRendererGradient     GimpViewRendererGradient;
typedef struct _GimpViewRendererPalette      GimpViewRendererPalette;
typedef struct _GimpViewRendererLayer        GimpViewRendererLayer;
typedef struct _GimpViewRendererImage        GimpViewRendererImage;
typedef struct _GimpViewRendererImagefile    GimpViewRendererImagefile;
typedef struct _GimpViewRendererVectors      GimpViewRendererVectors;


/*  cell renderers  */

typedef struct _GimpCellRendererDashes       GimpCellRendererDashes;
typedef struct _GimpCellRendererViewable     GimpCellRendererViewable;


/*  misc utilities & constructors  */

typedef struct _GimpDialogFactory            GimpDialogFactory;
typedef struct _GimpUnitStore                GimpUnitStore;
typedef struct _GimpUnitComboBox             GimpUnitComboBox;


/*  structs  */

typedef struct _GimpActionEntry              GimpActionEntry;
typedef struct _GimpToggleActionEntry        GimpToggleActionEntry;
typedef struct _GimpRadioActionEntry         GimpRadioActionEntry;
typedef struct _GimpEnumActionEntry          GimpEnumActionEntry;
typedef struct _GimpStringActionEntry        GimpStringActionEntry;
typedef struct _GimpPlugInActionEntry        GimpPlugInActionEntry;
typedef struct _GimpDialogFactoryEntry       GimpDialogFactoryEntry;
typedef struct _GimpSessionInfo              GimpSessionInfo;
typedef struct _GimpSessionInfoBook          GimpSessionInfoBook;
typedef struct _GimpSessionInfoDockable      GimpSessionInfoDockable;
typedef struct _GimpSessionInfoAux           GimpSessionInfoAux;


/*  function types  */

typedef void (* GimpActionGroupSetupFunc)  (GimpActionGroup *group);
typedef void (* GimpActionGroupUpdateFunc) (GimpActionGroup *group,
                                            gpointer         data);

typedef void (* GimpUIManagerSetupFunc)    (GimpUIManager   *manager,
                                            const gchar     *ui_path);

typedef void (* GimpMenuPositionFunc)      (GtkMenu         *menu,
                                            gint            *x,
                                            gint            *y,
                                            gpointer         data);


#endif /* __WIDGETS_TYPES_H__ */
