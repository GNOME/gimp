/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabrushselect.c
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "gegl/ligma-babl-compat.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmabrush.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmatempbuf.h"

#include "pdb/ligmapdb.h"

#include "ligmabrushfactoryview.h"
#include "ligmabrushselect.h"
#include "ligmacontainerbox.h"
#include "ligmalayermodebox.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_OPACITY,
  PROP_PAINT_MODE,
  PROP_SPACING
};


static void             ligma_brush_select_constructed  (GObject         *object);
static void             ligma_brush_select_set_property (GObject         *object,
                                                        guint            property_id,
                                                        const GValue    *value,
                                                        GParamSpec      *pspec);

static LigmaValueArray * ligma_brush_select_run_callback (LigmaPdbDialog   *dialog,
                                                        LigmaObject      *object,
                                                        gboolean         closing,
                                                        GError         **error);

static void          ligma_brush_select_opacity_changed (LigmaContext     *context,
                                                        gdouble          opacity,
                                                        LigmaBrushSelect *select);
static void          ligma_brush_select_mode_changed    (LigmaContext     *context,
                                                        LigmaLayerMode    paint_mode,
                                                        LigmaBrushSelect *select);

static void          ligma_brush_select_opacity_update  (GtkAdjustment   *adj,
                                                        LigmaBrushSelect *select);
static void          ligma_brush_select_spacing_update  (LigmaBrushFactoryView *view,
                                                        LigmaBrushSelect      *select);


G_DEFINE_TYPE (LigmaBrushSelect, ligma_brush_select, LIGMA_TYPE_PDB_DIALOG)

#define parent_class ligma_brush_select_parent_class


static void
ligma_brush_select_class_init (LigmaBrushSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  LigmaPdbDialogClass *pdb_class    = LIGMA_PDB_DIALOG_CLASS (klass);

  object_class->constructed  = ligma_brush_select_constructed;
  object_class->set_property = ligma_brush_select_set_property;

  pdb_class->run_callback    = ligma_brush_select_run_callback;

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity", NULL, NULL,
                                                        LIGMA_OPACITY_TRANSPARENT,
                                                        LIGMA_OPACITY_OPAQUE,
                                                        LIGMA_OPACITY_OPAQUE,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PAINT_MODE,
                                   g_param_spec_enum ("paint-mode", NULL, NULL,
                                                      LIGMA_TYPE_LAYER_MODE,
                                                      LIGMA_LAYER_MODE_NORMAL,
                                                      LIGMA_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SPACING,
                                   g_param_spec_int ("spacing", NULL, NULL,
                                                     -G_MAXINT, 1000, -1,
                                                     LIGMA_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT));
}

static void
ligma_brush_select_init (LigmaBrushSelect *select)
{
}

static void
ligma_brush_select_constructed (GObject *object)
{
  LigmaPdbDialog   *dialog = LIGMA_PDB_DIALOG (object);
  LigmaBrushSelect *select = LIGMA_BRUSH_SELECT (object);
  GtkWidget       *content_area;
  GtkWidget       *vbox;
  GtkWidget       *scale;
  GtkWidget       *hbox;
  GtkWidget       *label;
  GtkAdjustment   *spacing_adj;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_context_set_opacity    (dialog->context, select->initial_opacity);
  ligma_context_set_paint_mode (dialog->context, select->initial_mode);

  g_signal_connect (dialog->context, "opacity-changed",
                    G_CALLBACK (ligma_brush_select_opacity_changed),
                    dialog);
  g_signal_connect (dialog->context, "paint-mode-changed",
                    G_CALLBACK (ligma_brush_select_mode_changed),
                    dialog);

  dialog->view =
    ligma_brush_factory_view_new (LIGMA_VIEW_TYPE_GRID,
                                 dialog->context->ligma->brush_factory,
                                 dialog->context,
                                 FALSE,
                                 LIGMA_VIEW_SIZE_MEDIUM, 1,
                                 dialog->menu_factory);

  ligma_container_box_set_size_request (LIGMA_CONTAINER_BOX (LIGMA_CONTAINER_EDITOR (dialog->view)->view),
                                       5 * (LIGMA_VIEW_SIZE_MEDIUM + 2),
                                       5 * (LIGMA_VIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (dialog->view), 12);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_pack_start (GTK_BOX (content_area), dialog->view, TRUE, TRUE, 0);
  gtk_widget_show (dialog->view);

  vbox = GTK_WIDGET (LIGMA_CONTAINER_EDITOR (dialog->view)->view);

  /*  Create the opacity scale widget  */
  select->opacity_data =
    gtk_adjustment_new (ligma_context_get_opacity (dialog->context) * 100.0,
                        0.0, 100.0,
                        1.0, 10.0, 0.0);

  scale = ligma_spin_scale_new (select->opacity_data,
                               _("Opacity"), 1);
  ligma_spin_scale_set_constrain_drag (LIGMA_SPIN_SCALE (scale), TRUE);
  gtk_box_pack_end (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (select->opacity_data, "value-changed",
                    G_CALLBACK (ligma_brush_select_opacity_update),
                    select);

  /*  Create the paint mode option menu  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Mode:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  select->layer_mode_box = ligma_layer_mode_box_new (LIGMA_LAYER_MODE_CONTEXT_PAINT);
  gtk_box_pack_start (GTK_BOX (hbox), select->layer_mode_box, TRUE, TRUE, 0);
  gtk_widget_show (select->layer_mode_box);

  g_object_bind_property (G_OBJECT (dialog->context),        "paint-mode",
                          G_OBJECT (select->layer_mode_box), "layer-mode",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);

  spacing_adj = LIGMA_BRUSH_FACTORY_VIEW (dialog->view)->spacing_adjustment;

  /*  Use passed spacing instead of brushes default  */
  if (select->spacing >= 0)
    gtk_adjustment_set_value (spacing_adj, select->spacing);

  g_signal_connect (dialog->view, "spacing-changed",
                    G_CALLBACK (ligma_brush_select_spacing_update),
                    select);
}

static void
ligma_brush_select_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaPdbDialog   *dialog = LIGMA_PDB_DIALOG (object);
  LigmaBrushSelect *select = LIGMA_BRUSH_SELECT (object);

  switch (property_id)
    {
    case PROP_OPACITY:
      if (dialog->view)
        ligma_context_set_opacity (dialog->context, g_value_get_double (value));
      else
        select->initial_opacity = g_value_get_double (value);
      break;
    case PROP_PAINT_MODE:
      if (dialog->view)
        ligma_context_set_paint_mode (dialog->context, g_value_get_enum (value));
      else
        select->initial_mode = g_value_get_enum (value);
      break;
    case PROP_SPACING:
      if (dialog->view)
        {
          if (g_value_get_int (value) >= 0)
            gtk_adjustment_set_value (LIGMA_BRUSH_FACTORY_VIEW (dialog->view)->spacing_adjustment,
                                      g_value_get_int (value));
        }
      else
        {
          select->spacing = g_value_get_int (value);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static LigmaValueArray *
ligma_brush_select_run_callback (LigmaPdbDialog  *dialog,
                                LigmaObject     *object,
                                gboolean        closing,
                                GError        **error)
{
  LigmaBrush      *brush = LIGMA_BRUSH (object);
  LigmaTempBuf    *mask  = ligma_brush_get_mask (brush);
  const Babl     *format;
  gpointer        data;
  LigmaArray      *array;
  LigmaValueArray *return_vals;

  format = ligma_babl_compat_u8_mask_format (ligma_temp_buf_get_format (mask));
  data   = ligma_temp_buf_lock (mask, format, GEGL_ACCESS_READ);

  array = ligma_array_new (data,
                          ligma_temp_buf_get_width         (mask) *
                          ligma_temp_buf_get_height        (mask) *
                          babl_format_get_bytes_per_pixel (format),
                          TRUE);

  return_vals =
    ligma_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        G_TYPE_STRING,         ligma_object_get_name (object),
                                        G_TYPE_DOUBLE,         ligma_context_get_opacity (dialog->context) * 100.0,
                                        G_TYPE_INT,            LIGMA_BRUSH_SELECT (dialog)->spacing,
                                        LIGMA_TYPE_LAYER_MODE,  ligma_context_get_paint_mode (dialog->context),
                                        G_TYPE_INT,            ligma_brush_get_width  (brush),
                                        G_TYPE_INT,            ligma_brush_get_height (brush),
                                        G_TYPE_INT,            array->length,
                                        LIGMA_TYPE_UINT8_ARRAY, array->data,
                                        G_TYPE_BOOLEAN,        closing,
                                        G_TYPE_NONE);

  ligma_array_free (array);

  ligma_temp_buf_unlock (mask, data);

  return return_vals;
}

static void
ligma_brush_select_opacity_changed (LigmaContext     *context,
                                   gdouble          opacity,
                                   LigmaBrushSelect *select)
{
  g_signal_handlers_block_by_func (select->opacity_data,
                                   ligma_brush_select_opacity_update,
                                   select);

  gtk_adjustment_set_value (select->opacity_data, opacity * 100.0);

  g_signal_handlers_unblock_by_func (select->opacity_data,
                                     ligma_brush_select_opacity_update,
                                     select);

  ligma_pdb_dialog_run_callback ((LigmaPdbDialog **) &select, FALSE);
}

static void
ligma_brush_select_mode_changed (LigmaContext     *context,
                                LigmaLayerMode    paint_mode,
                                LigmaBrushSelect *select)
{
  ligma_pdb_dialog_run_callback ((LigmaPdbDialog **) &select, FALSE);
}

static void
ligma_brush_select_opacity_update (GtkAdjustment   *adjustment,
                                  LigmaBrushSelect *select)
{
  ligma_context_set_opacity (LIGMA_PDB_DIALOG (select)->context,
                            gtk_adjustment_get_value (adjustment) / 100.0);
}

static void
ligma_brush_select_spacing_update (LigmaBrushFactoryView *view,
                                  LigmaBrushSelect      *select)
{
  gdouble value = gtk_adjustment_get_value (view->spacing_adjustment);

  if (select->spacing != value)
    {
      select->spacing = value;

      ligma_pdb_dialog_run_callback ((LigmaPdbDialog **) &select, FALSE);
    }
}
