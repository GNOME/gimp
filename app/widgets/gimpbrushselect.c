/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrushselect.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gegl/gimp-babl-compat.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpbrush.h"
#include "core/gimpparamspecs.h"
#include "core/gimptempbuf.h"

#include "pdb/gimppdb.h"

#include "gimpbrushfactoryview.h"
#include "gimpbrushselect.h"
#include "gimpcontainerbox.h"
#include "gimplayermodebox.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_OPACITY,
  PROP_PAINT_MODE,
  PROP_SPACING
};


static void             gimp_brush_select_constructed  (GObject         *object);
static void             gimp_brush_select_set_property (GObject         *object,
                                                        guint            property_id,
                                                        const GValue    *value,
                                                        GParamSpec      *pspec);

static GimpValueArray * gimp_brush_select_run_callback (GimpPdbDialog   *dialog,
                                                        GimpObject      *object,
                                                        gboolean         closing,
                                                        GError         **error);

static void          gimp_brush_select_opacity_changed (GimpContext     *context,
                                                        gdouble          opacity,
                                                        GimpBrushSelect *select);
static void          gimp_brush_select_mode_changed    (GimpContext     *context,
                                                        GimpLayerMode    paint_mode,
                                                        GimpBrushSelect *select);

static void          gimp_brush_select_opacity_update  (GtkAdjustment   *adj,
                                                        GimpBrushSelect *select);
static void          gimp_brush_select_spacing_update  (GimpBrushFactoryView *view,
                                                        GimpBrushSelect      *select);


G_DEFINE_TYPE (GimpBrushSelect, gimp_brush_select, GIMP_TYPE_PDB_DIALOG)

#define parent_class gimp_brush_select_parent_class


static void
gimp_brush_select_class_init (GimpBrushSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  GimpPdbDialogClass *pdb_class    = GIMP_PDB_DIALOG_CLASS (klass);

  object_class->constructed  = gimp_brush_select_constructed;
  object_class->set_property = gimp_brush_select_set_property;

  pdb_class->run_callback    = gimp_brush_select_run_callback;

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity", NULL, NULL,
                                                        GIMP_OPACITY_TRANSPARENT,
                                                        GIMP_OPACITY_OPAQUE,
                                                        GIMP_OPACITY_OPAQUE,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PAINT_MODE,
                                   g_param_spec_enum ("paint-mode", NULL, NULL,
                                                      GIMP_TYPE_LAYER_MODE,
                                                      GIMP_LAYER_MODE_NORMAL,
                                                      GIMP_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SPACING,
                                   g_param_spec_int ("spacing", NULL, NULL,
                                                     -G_MAXINT, 1000, -1,
                                                     GIMP_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT));
}

static void
gimp_brush_select_init (GimpBrushSelect *select)
{
}

static void
gimp_brush_select_constructed (GObject *object)
{
  GimpPdbDialog   *dialog = GIMP_PDB_DIALOG (object);
  GimpBrushSelect *select = GIMP_BRUSH_SELECT (object);
  GtkWidget       *content_area;
  GtkWidget       *vbox;
  GtkWidget       *scale;
  GtkWidget       *hbox;
  GtkWidget       *label;
  GtkAdjustment   *spacing_adj;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_context_set_opacity    (dialog->context, select->initial_opacity);
  gimp_context_set_paint_mode (dialog->context, select->initial_mode);

  g_signal_connect (dialog->context, "opacity-changed",
                    G_CALLBACK (gimp_brush_select_opacity_changed),
                    dialog);
  g_signal_connect (dialog->context, "paint-mode-changed",
                    G_CALLBACK (gimp_brush_select_mode_changed),
                    dialog);

  dialog->view =
    gimp_brush_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                 dialog->context->gimp->brush_factory,
                                 dialog->context,
                                 FALSE,
                                 GIMP_VIEW_SIZE_MEDIUM, 1,
                                 dialog->menu_factory);

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (GIMP_CONTAINER_EDITOR (dialog->view)->view),
                                       5 * (GIMP_VIEW_SIZE_MEDIUM + 2),
                                       5 * (GIMP_VIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (dialog->view), 12);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_pack_start (GTK_BOX (content_area), dialog->view, TRUE, TRUE, 0);
  gtk_widget_show (dialog->view);

  vbox = GTK_WIDGET (GIMP_CONTAINER_EDITOR (dialog->view)->view);

  /*  Create the opacity scale widget  */
  select->opacity_data =
    gtk_adjustment_new (gimp_context_get_opacity (dialog->context) * 100.0,
                        0.0, 100.0,
                        1.0, 10.0, 0.0);

  scale = gimp_spin_scale_new (select->opacity_data,
                               _("Opacity"), 1);
  gimp_spin_scale_set_constrain_drag (GIMP_SPIN_SCALE (scale), TRUE);
  gtk_box_pack_end (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (select->opacity_data, "value-changed",
                    G_CALLBACK (gimp_brush_select_opacity_update),
                    select);

  /*  Create the paint mode option menu  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Mode:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  select->layer_mode_box = gimp_layer_mode_box_new (GIMP_LAYER_MODE_CONTEXT_PAINT);
  gtk_box_pack_start (GTK_BOX (hbox), select->layer_mode_box, TRUE, TRUE, 0);
  gtk_widget_show (select->layer_mode_box);

  g_object_bind_property (G_OBJECT (dialog->context),        "paint-mode",
                          G_OBJECT (select->layer_mode_box), "layer-mode",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);

  spacing_adj = GIMP_BRUSH_FACTORY_VIEW (dialog->view)->spacing_adjustment;

  /*  Use passed spacing instead of brushes default  */
  if (select->spacing >= 0)
    gtk_adjustment_set_value (spacing_adj, select->spacing);

  g_signal_connect (dialog->view, "spacing-changed",
                    G_CALLBACK (gimp_brush_select_spacing_update),
                    select);
}

static void
gimp_brush_select_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpPdbDialog   *dialog = GIMP_PDB_DIALOG (object);
  GimpBrushSelect *select = GIMP_BRUSH_SELECT (object);

  switch (property_id)
    {
    case PROP_OPACITY:
      if (dialog->view)
        gimp_context_set_opacity (dialog->context, g_value_get_double (value));
      else
        select->initial_opacity = g_value_get_double (value);
      break;
    case PROP_PAINT_MODE:
      if (dialog->view)
        gimp_context_set_paint_mode (dialog->context, g_value_get_enum (value));
      else
        select->initial_mode = g_value_get_enum (value);
      break;
    case PROP_SPACING:
      if (dialog->view)
        {
          if (g_value_get_int (value) >= 0)
            gtk_adjustment_set_value (GIMP_BRUSH_FACTORY_VIEW (dialog->view)->spacing_adjustment,
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

static GimpValueArray *
gimp_brush_select_run_callback (GimpPdbDialog  *dialog,
                                GimpObject     *object,
                                gboolean        closing,
                                GError        **error)
{
  GimpBrush      *brush = GIMP_BRUSH (object);
  GimpTempBuf    *mask  = gimp_brush_get_mask (brush);
  const Babl     *format;
  gpointer        data;
  GBytes         *bytes;
  GimpValueArray *return_vals;

  format = gimp_babl_compat_u8_mask_format (gimp_temp_buf_get_format (mask));
  data   = gimp_temp_buf_lock (mask, format, GEGL_ACCESS_READ);

  bytes = g_bytes_new_static (data,
                              gimp_temp_buf_get_width         (mask) *
                              gimp_temp_buf_get_height        (mask) *
                              babl_format_get_bytes_per_pixel (format));

  return_vals =
    gimp_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        GIMP_TYPE_RESOURCE,    object,
                                        G_TYPE_DOUBLE,         gimp_context_get_opacity (dialog->context) * 100.0,
                                        G_TYPE_INT,            GIMP_BRUSH_SELECT (dialog)->spacing,
                                        GIMP_TYPE_LAYER_MODE,  gimp_context_get_paint_mode (dialog->context),
                                        G_TYPE_INT,            gimp_brush_get_width  (brush),
                                        G_TYPE_INT,            gimp_brush_get_height (brush),
                                        G_TYPE_BYTES,          bytes,
                                        G_TYPE_BOOLEAN,        closing,
                                        G_TYPE_NONE);

  g_bytes_unref (bytes);

  gimp_temp_buf_unlock (mask, data);

  return return_vals;
}

static void
gimp_brush_select_opacity_changed (GimpContext     *context,
                                   gdouble          opacity,
                                   GimpBrushSelect *select)
{
  g_signal_handlers_block_by_func (select->opacity_data,
                                   gimp_brush_select_opacity_update,
                                   select);

  gtk_adjustment_set_value (select->opacity_data, opacity * 100.0);

  g_signal_handlers_unblock_by_func (select->opacity_data,
                                     gimp_brush_select_opacity_update,
                                     select);

  gimp_pdb_dialog_run_callback ((GimpPdbDialog **) &select, FALSE);
}

static void
gimp_brush_select_mode_changed (GimpContext     *context,
                                GimpLayerMode    paint_mode,
                                GimpBrushSelect *select)
{
  gimp_pdb_dialog_run_callback ((GimpPdbDialog **) &select, FALSE);
}

static void
gimp_brush_select_opacity_update (GtkAdjustment   *adjustment,
                                  GimpBrushSelect *select)
{
  gimp_context_set_opacity (GIMP_PDB_DIALOG (select)->context,
                            gtk_adjustment_get_value (adjustment) / 100.0);
}

static void
gimp_brush_select_spacing_update (GimpBrushFactoryView *view,
                                  GimpBrushSelect      *select)
{
  gdouble value = gtk_adjustment_get_value (view->spacing_adjustment);

  if (select->spacing != value)
    {
      select->spacing = value;

      gimp_pdb_dialog_run_callback ((GimpPdbDialog **) &select, FALSE);
    }
}
