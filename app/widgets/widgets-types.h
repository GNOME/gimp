/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WIDGETS_TYPES_H__
#define __WIDGETS_TYPES_H__


#include "libgimpwidgets/gimpwidgetstypes.h"

#include "core/core-types.h"

#include "widgets/widgets-enums.h"


/*  input devices & controllers  */

typedef struct _GimpControllerInfo           GimpControllerInfo;
typedef struct _GimpControllerKeyboard       GimpControllerKeyboard;
typedef struct _GimpControllerMouse          GimpControllerMouse;
typedef struct _GimpControllerWheel          GimpControllerWheel;
typedef struct _GimpDeviceInfo               GimpDeviceInfo;
typedef struct _GimpDeviceManager            GimpDeviceManager;


/*  docks  */

typedef struct _GimpDock                     GimpDock;
typedef struct _GimpDockColumns              GimpDockColumns;
typedef struct _GimpDockContainer            GimpDockContainer; /* dummy typedef */
typedef struct _GimpDockWindow               GimpDockWindow;
typedef struct _GimpDockable                 GimpDockable;
typedef struct _GimpDockbook                 GimpDockbook;
typedef struct _GimpDocked                   GimpDocked; /* dummy typedef */
typedef struct _GimpMenuDock                 GimpMenuDock;
typedef struct _GimpPanedBox                 GimpPanedBox;
typedef struct _GimpToolbox                  GimpToolbox;


/*  GimpEditor widgets  */

typedef struct _GimpColorEditor              GimpColorEditor;
typedef struct _GimpDeviceStatus             GimpDeviceStatus;
typedef struct _GimpEditor                   GimpEditor;
typedef struct _GimpErrorConsole             GimpErrorConsole;
typedef struct _GimpToolOptionsEditor        GimpToolOptionsEditor;
typedef struct _GimpDashboard                GimpDashboard;


/*  GimpDataEditor widgets  */

typedef struct _GimpBrushEditor              GimpBrushEditor;
typedef struct _GimpDataEditor               GimpDataEditor;
typedef struct _GimpDynamicsEditor           GimpDynamicsEditor;
typedef struct _GimpGradientEditor           GimpGradientEditor;
typedef struct _GimpPaletteEditor            GimpPaletteEditor;
typedef struct _GimpToolPresetEditor         GimpToolPresetEditor;


/*  GimpImageEditor widgets  */

typedef struct _GimpColormapEditor           GimpColormapEditor;
typedef struct _GimpComponentEditor          GimpComponentEditor;
typedef struct _GimpHistogramEditor          GimpHistogramEditor;
typedef struct _GimpImageEditor              GimpImageEditor;
typedef struct _GimpSamplePointEditor        GimpSamplePointEditor;
typedef struct _GimpSelectionEditor          GimpSelectionEditor;
typedef struct _GimpSymmetryEditor           GimpSymmetryEditor;
typedef struct _GimpUndoEditor               GimpUndoEditor;


/*  GimpContainerView and its implementors  */

typedef struct _GimpChannelTreeView          GimpChannelTreeView;
typedef struct _GimpContainerBox             GimpContainerBox;
typedef struct _GimpContainerComboBox        GimpContainerComboBox;
typedef struct _GimpContainerEntry           GimpContainerEntry;
typedef struct _GimpContainerGridView        GimpContainerGridView;
typedef struct _GimpContainerIconView        GimpContainerIconView;
typedef struct _GimpContainerTreeStore       GimpContainerTreeStore;
typedef struct _GimpContainerTreeView        GimpContainerTreeView;
typedef struct _GimpContainerView            GimpContainerView; /* dummy typedef */
typedef struct _GimpDrawableTreeView         GimpDrawableTreeView;
typedef struct _GimpItemTreeView             GimpItemTreeView;
typedef struct _GimpLayerTreeView            GimpLayerTreeView;
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
typedef struct _GimpToolEditor               GimpToolEditor;


/*  GimpDataFactoryView widgets  */

typedef struct _GimpBrushFactoryView         GimpBrushFactoryView;
typedef struct _GimpDataFactoryView          GimpDataFactoryView;
typedef struct _GimpDynamicsFactoryView      GimpDynamicsFactoryView;
typedef struct _GimpPatternFactoryView       GimpPatternFactoryView;
typedef struct _GimpToolPresetFactoryView    GimpToolPresetFactoryView;


/*  menus  */

typedef struct _GimpAction                   GimpAction;
typedef struct _GimpActionFactory            GimpActionFactory;
typedef struct _GimpActionGroup              GimpActionGroup;
typedef struct _GimpEnumAction               GimpEnumAction;
typedef struct _GimpMenuFactory              GimpMenuFactory;
typedef struct _GimpProcedureAction          GimpProcedureAction;
typedef struct _GimpStringAction             GimpStringAction;
typedef struct _GimpUIManager                GimpUIManager;


/*  file dialogs  */

typedef struct _GimpExportDialog             GimpExportDialog;
typedef struct _GimpFileDialog               GimpFileDialog;
typedef struct _GimpOpenDialog               GimpOpenDialog;
typedef struct _GimpSaveDialog               GimpSaveDialog;


/*  misc dialogs  */

typedef struct _GimpColorDialog              GimpColorDialog;
typedef struct _GimpCriticalDialog           GimpCriticalDialog;
typedef struct _GimpErrorDialog              GimpErrorDialog;
typedef struct _GimpMessageDialog            GimpMessageDialog;
typedef struct _GimpProgressDialog           GimpProgressDialog;
typedef struct _GimpTextEditor               GimpTextEditor;
typedef struct _GimpViewableDialog           GimpViewableDialog;


/*  GimpPdbDialog widgets  */

typedef struct _GimpBrushSelect              GimpBrushSelect;
typedef struct _GimpFontSelect               GimpFontSelect;
typedef struct _GimpGradientSelect           GimpGradientSelect;
typedef struct _GimpPaletteSelect            GimpPaletteSelect;
typedef struct _GimpPatternSelect            GimpPatternSelect;
typedef struct _GimpPdbDialog                GimpPdbDialog;


/*  misc widgets  */

typedef struct _GimpActionEditor             GimpActionEditor;
typedef struct _GimpActionView               GimpActionView;
typedef struct _GimpBlobEditor               GimpBlobEditor;
typedef struct _GimpBufferSourceBox          GimpBufferSourceBox;
typedef struct _GimpCircle                   GimpCircle;
typedef struct _GimpColorBar                 GimpColorBar;
typedef struct _GimpColorDisplayEditor       GimpColorDisplayEditor;
typedef struct _GimpColorFrame               GimpColorFrame;
typedef struct _GimpColorHistory             GimpColorHistory;
typedef struct _GimpColorPanel               GimpColorPanel;
typedef struct _GimpComboTagEntry            GimpComboTagEntry;
typedef struct _GimpControllerEditor         GimpControllerEditor;
typedef struct _GimpControllerList           GimpControllerList;
typedef struct _GimpCurveView                GimpCurveView;
typedef struct _GimpDashEditor               GimpDashEditor;
typedef struct _GimpDeviceEditor             GimpDeviceEditor;
typedef struct _GimpDeviceInfoEditor         GimpDeviceInfoEditor;
typedef struct _GimpDial                     GimpDial;
typedef struct _GimpDynamicsOutputEditor     GimpDynamicsOutputEditor;
typedef struct _GimpFgBgEditor               GimpFgBgEditor;
typedef struct _GimpFgBgView                 GimpFgBgView;
typedef struct _GimpFileProcView             GimpFileProcView;
typedef struct _GimpFillEditor               GimpFillEditor;
typedef struct _GimpGridEditor               GimpGridEditor;
typedef struct _GimpHandleBar                GimpHandleBar;
typedef struct _GimpHighlightableButton      GimpHighlightableButton;
typedef struct _GimpHistogramBox             GimpHistogramBox;
typedef struct _GimpHistogramView            GimpHistogramView;
typedef struct _GimpIconPicker               GimpIconPicker;
typedef struct _GimpIconSizeScale            GimpIconSizeScale;
typedef struct _GimpImageCommentEditor       GimpImageCommentEditor;
typedef struct _GimpImageParasiteView        GimpImageParasiteView;
typedef struct _GimpImageProfileView         GimpImageProfileView;
typedef struct _GimpImagePropView            GimpImagePropView;
typedef struct _GimpLanguageComboBox         GimpLanguageComboBox;
typedef struct _GimpLanguageEntry            GimpLanguageEntry;
typedef struct _GimpLanguageStore            GimpLanguageStore;
typedef struct _GimpLayerModeBox             GimpLayerModeBox;
typedef struct _GimpLayerModeComboBox        GimpLayerModeComboBox;
typedef struct _GimpMessageBox               GimpMessageBox;
typedef struct _GimpMeter                    GimpMeter;
typedef struct _GimpOverlayBox               GimpOverlayBox;
typedef struct _GimpPickableButton           GimpPickableButton;
typedef struct _GimpPickablePopup            GimpPickablePopup;
typedef struct _GimpPlugInView               GimpPlugInView;
typedef struct _GimpPolar                    GimpPolar;
typedef struct _GimpPopup                    GimpPopup;
typedef struct _GimpPrefsBox                 GimpPrefsBox;
typedef struct _GimpProgressBox              GimpProgressBox;
typedef struct _GimpScaleButton              GimpScaleButton;
typedef struct _GimpSettingsBox              GimpSettingsBox;
typedef struct _GimpSettingsEditor           GimpSettingsEditor;
typedef struct _GimpSizeBox                  GimpSizeBox;
typedef struct _GimpStrokeEditor             GimpStrokeEditor;
typedef struct _GimpTagEntry                 GimpTagEntry;
typedef struct _GimpTagPopup                 GimpTagPopup;
typedef struct _GimpTemplateEditor           GimpTemplateEditor;
typedef struct _GimpTextStyleEditor          GimpTextStyleEditor;
typedef struct _GimpThumbBox                 GimpThumbBox;
typedef struct _GimpToolPalette              GimpToolPalette;
typedef struct _GimpTranslationStore         GimpTranslationStore;
typedef struct _GimpWindow                   GimpWindow;


/*  views  */

typedef struct _GimpNavigationView           GimpNavigationView;
typedef struct _GimpPaletteView              GimpPaletteView;
typedef struct _GimpView                     GimpView;


/*  view renderers  */

typedef struct _GimpViewRenderer             GimpViewRenderer;
typedef struct _GimpViewRendererBrush        GimpViewRendererBrush;
typedef struct _GimpViewRendererBuffer       GimpViewRendererBuffer;
typedef struct _GimpViewRendererDrawable     GimpViewRendererDrawable;
typedef struct _GimpViewRendererGradient     GimpViewRendererGradient;
typedef struct _GimpViewRendererImage        GimpViewRendererImage;
typedef struct _GimpViewRendererImagefile    GimpViewRendererImagefile;
typedef struct _GimpViewRendererLayer        GimpViewRendererLayer;
typedef struct _GimpViewRendererPalette      GimpViewRendererPalette;
typedef struct _GimpViewRendererVectors      GimpViewRendererVectors;


/*  cell renderers  */

typedef struct _GimpCellRendererButton       GimpCellRendererButton;
typedef struct _GimpCellRendererDashes       GimpCellRendererDashes;
typedef struct _GimpCellRendererViewable     GimpCellRendererViewable;


/*  misc objects  */

typedef struct _GimpDialogFactory            GimpDialogFactory;
typedef struct _GimpTextBuffer               GimpTextBuffer;
typedef struct _GimpUIConfigurer             GimpUIConfigurer;
typedef struct _GimpWindowStrategy           GimpWindowStrategy;


/*  session management objects and structs  */

typedef struct _GimpSessionInfo              GimpSessionInfo;
typedef struct _GimpSessionInfoAux           GimpSessionInfoAux;
typedef struct _GimpSessionInfoBook          GimpSessionInfoBook;
typedef struct _GimpSessionInfoDock          GimpSessionInfoDock;
typedef struct _GimpSessionInfoDockable      GimpSessionInfoDockable;
typedef struct _GimpSessionManaged           GimpSessionManaged;


/*  structs  */

typedef struct _GimpActionEntry              GimpActionEntry;
typedef struct _GimpEnumActionEntry          GimpEnumActionEntry;
typedef struct _GimpProcedureActionEntry     GimpProcedureActionEntry;
typedef struct _GimpRadioActionEntry         GimpRadioActionEntry;
typedef struct _GimpStringActionEntry        GimpStringActionEntry;
typedef struct _GimpToggleActionEntry        GimpToggleActionEntry;

typedef struct _GimpDialogFactoryEntry       GimpDialogFactoryEntry;


/*  function types  */

typedef GtkWidget * (* GimpDialogRestoreFunc)        (GimpDialogFactory *factory,
                                                      GdkMonitor        *monitor,
                                                      GimpSessionInfo   *info);
typedef void        (* GimpActionGroupSetupFunc)     (GimpActionGroup   *group);
typedef void        (* GimpActionGroupUpdateFunc)    (GimpActionGroup   *group,
                                                      gpointer           data);

typedef void        (* GimpUIManagerSetupFunc)       (GimpUIManager     *manager,
                                                      const gchar       *ui_path);

typedef void        (* GimpMenuPositionFunc)         (GtkMenu           *menu,
                                                      gint              *x,
                                                      gint              *y,
                                                      gpointer           data);
typedef gboolean    (* GimpPanedBoxDroppedFunc)      (GtkWidget         *source,
                                                      gint               insert_index,
                                                      gpointer           data);


#endif /* __WIDGETS_TYPES_H__ */
