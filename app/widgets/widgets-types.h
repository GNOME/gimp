/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __WIDGETS_TYPES_H__
#define __WIDGETS_TYPES_H__


#include "libligmawidgets/ligmawidgetstypes.h"

#include "core/core-types.h"

#include "widgets/widgets-enums.h"


/*  input devices & controllers  */

typedef struct _LigmaControllerInfo           LigmaControllerInfo;
typedef struct _LigmaControllerKeyboard       LigmaControllerKeyboard;
typedef struct _LigmaControllerMouse          LigmaControllerMouse;
typedef struct _LigmaControllerWheel          LigmaControllerWheel;
typedef struct _LigmaDeviceInfo               LigmaDeviceInfo;
typedef struct _LigmaDeviceManager            LigmaDeviceManager;


/*  docks  */

typedef struct _LigmaDock                     LigmaDock;
typedef struct _LigmaDockColumns              LigmaDockColumns;
typedef struct _LigmaDockContainer            LigmaDockContainer; /* dummy typedef */
typedef struct _LigmaDockWindow               LigmaDockWindow;
typedef struct _LigmaDockable                 LigmaDockable;
typedef struct _LigmaDockbook                 LigmaDockbook;
typedef struct _LigmaDocked                   LigmaDocked; /* dummy typedef */
typedef struct _LigmaMenuDock                 LigmaMenuDock;
typedef struct _LigmaPanedBox                 LigmaPanedBox;
typedef struct _LigmaToolbox                  LigmaToolbox;


/*  LigmaEditor widgets  */

typedef struct _LigmaColorEditor              LigmaColorEditor;
typedef struct _LigmaDeviceStatus             LigmaDeviceStatus;
typedef struct _LigmaEditor                   LigmaEditor;
typedef struct _LigmaErrorConsole             LigmaErrorConsole;
typedef struct _LigmaToolOptionsEditor        LigmaToolOptionsEditor;
typedef struct _LigmaDashboard                LigmaDashboard;


/*  LigmaDataEditor widgets  */

typedef struct _LigmaBrushEditor              LigmaBrushEditor;
typedef struct _LigmaDataEditor               LigmaDataEditor;
typedef struct _LigmaDynamicsEditor           LigmaDynamicsEditor;
typedef struct _LigmaGradientEditor           LigmaGradientEditor;
typedef struct _LigmaPaletteEditor            LigmaPaletteEditor;
typedef struct _LigmaToolPresetEditor         LigmaToolPresetEditor;


/*  LigmaImageEditor widgets  */

typedef struct _LigmaColormapEditor           LigmaColormapEditor;
typedef struct _LigmaComponentEditor          LigmaComponentEditor;
typedef struct _LigmaHistogramEditor          LigmaHistogramEditor;
typedef struct _LigmaImageEditor              LigmaImageEditor;
typedef struct _LigmaSamplePointEditor        LigmaSamplePointEditor;
typedef struct _LigmaSelectionEditor          LigmaSelectionEditor;
typedef struct _LigmaSymmetryEditor           LigmaSymmetryEditor;
typedef struct _LigmaUndoEditor               LigmaUndoEditor;


/*  LigmaContainerView and its implementors  */

typedef struct _LigmaChannelTreeView          LigmaChannelTreeView;
typedef struct _LigmaContainerBox             LigmaContainerBox;
typedef struct _LigmaContainerComboBox        LigmaContainerComboBox;
typedef struct _LigmaContainerEntry           LigmaContainerEntry;
typedef struct _LigmaContainerIconView        LigmaContainerIconView;
typedef struct _LigmaContainerTreeStore       LigmaContainerTreeStore;
typedef struct _LigmaContainerTreeView        LigmaContainerTreeView;
typedef struct _LigmaContainerView            LigmaContainerView; /* dummy typedef */
typedef struct _LigmaDrawableTreeView         LigmaDrawableTreeView;
typedef struct _LigmaItemTreeView             LigmaItemTreeView;
typedef struct _LigmaLayerTreeView            LigmaLayerTreeView;
typedef struct _LigmaVectorsTreeView          LigmaVectorsTreeView;

typedef struct _LigmaContainerPopup           LigmaContainerPopup;
typedef struct _LigmaViewableButton           LigmaViewableButton;


/*  LigmaContainerEditor widgets  */

typedef struct _LigmaContainerEditor          LigmaContainerEditor;
typedef struct _LigmaBufferView               LigmaBufferView;
typedef struct _LigmaDocumentView             LigmaDocumentView;
typedef struct _LigmaFontView                 LigmaFontView;
typedef struct _LigmaImageView                LigmaImageView;
typedef struct _LigmaTemplateView             LigmaTemplateView;
typedef struct _LigmaToolEditor               LigmaToolEditor;


/*  LigmaDataFactoryView widgets  */

typedef struct _LigmaBrushFactoryView         LigmaBrushFactoryView;
typedef struct _LigmaDataFactoryView          LigmaDataFactoryView;
typedef struct _LigmaDynamicsFactoryView      LigmaDynamicsFactoryView;
typedef struct _LigmaFontFactoryView          LigmaFontFactoryView;
typedef struct _LigmaPatternFactoryView       LigmaPatternFactoryView;
typedef struct _LigmaToolPresetFactoryView    LigmaToolPresetFactoryView;


/*  menus  */

typedef struct _LigmaAction                   LigmaAction;
typedef struct _LigmaActionFactory            LigmaActionFactory;
typedef struct _LigmaActionGroup              LigmaActionGroup;
typedef struct _LigmaDoubleAction             LigmaDoubleAction;
typedef struct _LigmaEnumAction               LigmaEnumAction;
typedef struct _LigmaMenuFactory              LigmaMenuFactory;
typedef struct _LigmaProcedureAction          LigmaProcedureAction;
typedef struct _LigmaStringAction             LigmaStringAction;
typedef struct _LigmaUIManager                LigmaUIManager;


/*  file dialogs  */

typedef struct _LigmaExportDialog             LigmaExportDialog;
typedef struct _LigmaFileDialog               LigmaFileDialog;
typedef struct _LigmaOpenDialog               LigmaOpenDialog;
typedef struct _LigmaSaveDialog               LigmaSaveDialog;


/*  misc dialogs  */

typedef struct _LigmaColorDialog              LigmaColorDialog;
typedef struct _LigmaCriticalDialog           LigmaCriticalDialog;
typedef struct _LigmaErrorDialog              LigmaErrorDialog;
typedef struct _LigmaMessageDialog            LigmaMessageDialog;
typedef struct _LigmaProgressDialog           LigmaProgressDialog;
typedef struct _LigmaTextEditor               LigmaTextEditor;
typedef struct _LigmaViewableDialog           LigmaViewableDialog;


/*  LigmaPdbDialog widgets  */

typedef struct _LigmaBrushSelect              LigmaBrushSelect;
typedef struct _LigmaFontSelect               LigmaFontSelect;
typedef struct _LigmaGradientSelect           LigmaGradientSelect;
typedef struct _LigmaPaletteSelect            LigmaPaletteSelect;
typedef struct _LigmaPatternSelect            LigmaPatternSelect;
typedef struct _LigmaPdbDialog                LigmaPdbDialog;


/*  misc widgets  */

typedef struct _LigmaAccelLabel               LigmaAccelLabel;
typedef struct _LigmaActionEditor             LigmaActionEditor;
typedef struct _LigmaActionView               LigmaActionView;
typedef struct _LigmaBlobEditor               LigmaBlobEditor;
typedef struct _LigmaBufferSourceBox          LigmaBufferSourceBox;
typedef struct _LigmaCircle                   LigmaCircle;
typedef struct _LigmaColorBar                 LigmaColorBar;
typedef struct _LigmaColorDisplayEditor       LigmaColorDisplayEditor;
typedef struct _LigmaColorFrame               LigmaColorFrame;
typedef struct _LigmaColorHistory             LigmaColorHistory;
typedef struct _LigmaColormapSelection        LigmaColormapSelection;
typedef struct _LigmaColorPanel               LigmaColorPanel;
typedef struct _LigmaComboTagEntry            LigmaComboTagEntry;
typedef struct _LigmaCompressionComboBox      LigmaCompressionComboBox;
typedef struct _LigmaControllerEditor         LigmaControllerEditor;
typedef struct _LigmaControllerList           LigmaControllerList;
typedef struct _LigmaCurveView                LigmaCurveView;
typedef struct _LigmaDashEditor               LigmaDashEditor;
typedef struct _LigmaDeviceEditor             LigmaDeviceEditor;
typedef struct _LigmaDeviceInfoEditor         LigmaDeviceInfoEditor;
typedef struct _LigmaDial                     LigmaDial;
typedef struct _LigmaDynamicsOutputEditor     LigmaDynamicsOutputEditor;
typedef struct _LigmaExtensionDetails         LigmaExtensionDetails;
typedef struct _LigmaExtensionList            LigmaExtensionList;
typedef struct _LigmaFgBgEditor               LigmaFgBgEditor;
typedef struct _LigmaFgBgView                 LigmaFgBgView;
typedef struct _LigmaFileProcView             LigmaFileProcView;
typedef struct _LigmaFillEditor               LigmaFillEditor;
typedef struct _LigmaGridEditor               LigmaGridEditor;
typedef struct _LigmaHandleBar                LigmaHandleBar;
typedef struct _LigmaHighlightableButton      LigmaHighlightableButton;
typedef struct _LigmaHistogramBox             LigmaHistogramBox;
typedef struct _LigmaHistogramView            LigmaHistogramView;
typedef struct _LigmaIconPicker               LigmaIconPicker;
typedef struct _LigmaImageCommentEditor       LigmaImageCommentEditor;
typedef struct _LigmaImageParasiteView        LigmaImageParasiteView;
typedef struct _LigmaImageProfileView         LigmaImageProfileView;
typedef struct _LigmaImagePropView            LigmaImagePropView;
typedef struct _LigmaLanguageComboBox         LigmaLanguageComboBox;
typedef struct _LigmaLanguageEntry            LigmaLanguageEntry;
typedef struct _LigmaLanguageStore            LigmaLanguageStore;
typedef struct _LigmaLayerModeBox             LigmaLayerModeBox;
typedef struct _LigmaLayerModeComboBox        LigmaLayerModeComboBox;
typedef struct _LigmaMessageBox               LigmaMessageBox;
typedef struct _LigmaMeter                    LigmaMeter;
typedef struct _LigmaModifiersEditor          LigmaModifiersEditor;
typedef struct _LigmaOverlayBox               LigmaOverlayBox;
typedef struct _LigmaPickableButton           LigmaPickableButton;
typedef struct _LigmaPickablePopup            LigmaPickablePopup;
typedef struct _LigmaPivotSelector            LigmaPivotSelector;
typedef struct _LigmaPlugInView               LigmaPlugInView;
typedef struct _LigmaPolar                    LigmaPolar;
typedef struct _LigmaPopup                    LigmaPopup;
typedef struct _LigmaPrefsBox                 LigmaPrefsBox;
typedef struct _LigmaProgressBox              LigmaProgressBox;
typedef struct _LigmaScaleButton              LigmaScaleButton;
typedef struct _LigmaSettingsBox              LigmaSettingsBox;
typedef struct _LigmaSettingsEditor           LigmaSettingsEditor;
typedef struct _LigmaShortcutButton           LigmaShortcutButton;
typedef struct _LigmaSizeBox                  LigmaSizeBox;
typedef struct _LigmaStrokeEditor             LigmaStrokeEditor;
typedef struct _LigmaTagEntry                 LigmaTagEntry;
typedef struct _LigmaTagPopup                 LigmaTagPopup;
typedef struct _LigmaTemplateEditor           LigmaTemplateEditor;
typedef struct _LigmaTextStyleEditor          LigmaTextStyleEditor;
typedef struct _LigmaThumbBox                 LigmaThumbBox;
typedef struct _LigmaToolButton               LigmaToolButton;
typedef struct _LigmaToolPalette              LigmaToolPalette;
typedef struct _LigmaTranslationStore         LigmaTranslationStore;
typedef struct _LigmaWindow                   LigmaWindow;


/*  views  */

typedef struct _LigmaNavigationView           LigmaNavigationView;
typedef struct _LigmaPaletteView              LigmaPaletteView;
typedef struct _LigmaView                     LigmaView;


/*  view renderers  */

typedef struct _LigmaViewRenderer             LigmaViewRenderer;
typedef struct _LigmaViewRendererBrush        LigmaViewRendererBrush;
typedef struct _LigmaViewRendererBuffer       LigmaViewRendererBuffer;
typedef struct _LigmaViewRendererDrawable     LigmaViewRendererDrawable;
typedef struct _LigmaViewRendererGradient     LigmaViewRendererGradient;
typedef struct _LigmaViewRendererImage        LigmaViewRendererImage;
typedef struct _LigmaViewRendererImagefile    LigmaViewRendererImagefile;
typedef struct _LigmaViewRendererLayer        LigmaViewRendererLayer;
typedef struct _LigmaViewRendererPalette      LigmaViewRendererPalette;
typedef struct _LigmaViewRendererVectors      LigmaViewRendererVectors;


/*  cell renderers  */

typedef struct _LigmaCellRendererButton       LigmaCellRendererButton;
typedef struct _LigmaCellRendererDashes       LigmaCellRendererDashes;
typedef struct _LigmaCellRendererViewable     LigmaCellRendererViewable;


/*  misc objects  */

typedef struct _LigmaDialogFactory            LigmaDialogFactory;
typedef struct _LigmaTextBuffer               LigmaTextBuffer;
typedef struct _LigmaUIConfigurer             LigmaUIConfigurer;
typedef struct _LigmaWindowStrategy           LigmaWindowStrategy;


/*  session management objects and structs  */

typedef struct _LigmaSessionInfo              LigmaSessionInfo;
typedef struct _LigmaSessionInfoAux           LigmaSessionInfoAux;
typedef struct _LigmaSessionInfoBook          LigmaSessionInfoBook;
typedef struct _LigmaSessionInfoDock          LigmaSessionInfoDock;
typedef struct _LigmaSessionInfoDockable      LigmaSessionInfoDockable;
typedef struct _LigmaSessionManaged           LigmaSessionManaged;


/*  structs  */

typedef struct _LigmaActionEntry              LigmaActionEntry;
typedef struct _LigmaDoubleActionEntry        LigmaDoubleActionEntry;
typedef struct _LigmaEnumActionEntry          LigmaEnumActionEntry;
typedef struct _LigmaProcedureActionEntry     LigmaProcedureActionEntry;
typedef struct _LigmaRadioActionEntry         LigmaRadioActionEntry;
typedef struct _LigmaStringActionEntry        LigmaStringActionEntry;
typedef struct _LigmaToggleActionEntry        LigmaToggleActionEntry;

typedef struct _LigmaDialogFactoryEntry       LigmaDialogFactoryEntry;

typedef struct _LigmaDashboardLogParams       LigmaDashboardLogParams;


/*  function types  */

typedef GtkWidget * (* LigmaDialogRestoreFunc)        (LigmaDialogFactory *factory,
                                                      GdkMonitor        *monitor,
                                                      LigmaSessionInfo   *info);
typedef void        (* LigmaActionGroupSetupFunc)     (LigmaActionGroup   *group);
typedef void        (* LigmaActionGroupUpdateFunc)    (LigmaActionGroup   *group,
                                                      gpointer           data);

typedef void        (* LigmaUIManagerSetupFunc)       (LigmaUIManager     *manager,
                                                      const gchar       *ui_path);

typedef void        (* LigmaMenuPositionFunc)         (GtkMenu           *menu,
                                                      gint              *x,
                                                      gint              *y,
                                                      gpointer           data);
typedef gboolean    (* LigmaPanedBoxDroppedFunc)      (GtkWidget         *notebook,
                                                      GtkWidget         *child,
                                                      gint               insert_index,
                                                      gpointer           data);

typedef GtkWidget * (* LigmaToolOptionsGUIFunc)       (LigmaToolOptions   *tool_options);


#endif /* __WIDGETS_TYPES_H__ */
