/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrushselect.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpbrush.h"

#include "pdb/procedural_db.h"

#include "gimpbrushfactoryview.h"
#include "gimpbrushselect.h"
#include "gimpcontainerbox.h"
#include "gimpwidgets-constructors.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_OPACITY,
  PROP_PAINT_MODE,
  PROP_SPACING
};


static void   gimp_brush_select_class_init (GimpBrushSelectClass *klass);

static GObject  * gimp_brush_select_constructor  (GType            type,
                                                  guint            n_params,
                                                  GObjectConstructParam *params);
static void       gimp_brush_select_set_property (GObject         *object,
                                                  guint            property_id,
                                                  const GValue    *value,
                                                  GParamSpec      *pspec);

static Argument * gimp_brush_select_run_callback (GimpPdbDialog   *dialog,
                                                  GimpObject      *object,
                                                  gboolean         closing,
                                                  gint            *n_return_vals);

static void   gimp_brush_select_opacity_changed  (GimpContext     *context,
                                                  gdouble          opacity,
                                                  GimpBrushSelect *select);
static void   gimp_brush_select_mode_changed     (GimpContext     *context,
                                                  GimpLayerModeEffects  paint_mode,
                                                  GimpBrushSelect *select);

static void   gimp_brush_select_opacity_update   (GtkAdjustment   *adj,
                                                  GimpBrushSelect *select);
static void   gimp_brush_select_mode_update      (GtkWidget       *widget,
                                                  GimpBrushSelect *select);
static void   gimp_brush_select_spacing_update   (GtkAdjustment   *adj,
                                                  GimpBrushSelect *select);


static GimpPdbDialogClass *parent_class = NULL;


GType
gimp_brush_select_get_type (void)
{
  static GType dialog_type = 0;

  if (! dialog_type)
    {
      static const GTypeInfo dialog_info =
      {
        sizeof (GimpBrushSelectClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_brush_select_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpBrushSelect),
        0,              /* n_preallocs    */
        NULL            /* instance_init  */
      };

      dialog_type = g_type_register_static (GIMP_TYPE_PDB_DIALOG,
                                            "GimpBrushSelect",
                                            &dialog_info, 0);
    }

  return dialog_type;
}

static void
gimp_brush_select_class_init (GimpBrushSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  GimpPdbDialogClass *pdb_class    = GIMP_PDB_DIALOG_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = gimp_brush_select_constructor;
  object_class->set_property = gimp_brush_select_set_property;

  pdb_class->run_callback    = gimp_brush_select_run_callback;

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity", NULL, NULL,
                                                        GIMP_OPACITY_TRANSPARENT,
                                                        GIMP_OPACITY_OPAQUE,
                                                        GIMP_OPACITY_OPAQUE,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PAINT_MODE,
                                   g_param_spec_enum ("paint-mode", NULL, NULL,
                                                      GIMP_TYPE_LAYER_MODE_EFFECTS,
                                                      GIMP_NORMAL_MODE,
                                                      G_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SPACING,
                                   g_param_spec_int ("spacing", NULL, NULL,
                                                     -G_MAXINT, 1000, -1,
                                                     G_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT));
}

static GObject *
gimp_brush_select_constructor (GType                  type,
                               guint                  n_params,
                               GObjectConstructParam *params)
{
  GObject         *object;
  GimpPdbDialog   *dialog;
  GimpBrushSelect *select;
  GtkWidget       *table;
  GtkAdjustment   *spacing_adj;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  dialog = GIMP_PDB_DIALOG (object);
  select = GIMP_BRUSH_SELECT (object);

  gimp_context_set_opacity    (dialog->context, select->initial_opacity);
  gimp_context_set_paint_mode (dialog->context, select->initial_mode);

  g_signal_connect (dialog->context, "opacity_changed",
                    G_CALLBACK (gimp_brush_select_opacity_changed),
                    dialog);
  g_signal_connect (dialog->context, "paint_mode_changed",
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
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), dialog->view);
  gtk_widget_show (dialog->view);

  /*  Create the frame and the table for the options  */
  table = GIMP_BRUSH_FACTORY_VIEW (dialog->view)->spacing_scale->parent;
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);

  /*  Create the opacity scale widget  */
  select->opacity_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                          _("Opacity:"), -1, 5,
                                          gimp_context_get_opacity (dialog->context) * 100.0,
                                          0.0, 100.0, 1.0, 10.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (select->opacity_data, "value_changed",
                    G_CALLBACK (gimp_brush_select_opacity_update),
                    select);

  /*  Create the paint mode option menu  */
  select->paint_mode_menu =
    gimp_paint_mode_menu_new (G_CALLBACK (gimp_brush_select_mode_update),
			      select,
			      TRUE,
			      gimp_context_get_paint_mode (dialog->context));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Mode:"), 0.0, 0.5,
			     select->paint_mode_menu, 2, FALSE);

  spacing_adj = GIMP_BRUSH_FACTORY_VIEW (dialog->view)->spacing_adjustment;

  /*  Use passed spacing instead of brushes default  */
  if (select->spacing >= 0)
    gtk_adjustment_set_value (spacing_adj, select->spacing);

  g_signal_connect (spacing_adj, "value_changed",
                    G_CALLBACK (gimp_brush_select_spacing_update),
                    select);

  gtk_widget_show (table);

  return object;
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

static Argument *
gimp_brush_select_run_callback (GimpPdbDialog *dialog,
                                GimpObject    *object,
                                gboolean       closing,
                                gint          *n_return_vals)
{
  GimpBrush *brush = GIMP_BRUSH (object);

  return procedural_db_run_proc (dialog->caller_context->gimp,
                                 dialog->caller_context,
                                 NULL,
                                 dialog->callback_name,
                                 n_return_vals,
                                 GIMP_PDB_STRING,    object->name,
                                 GIMP_PDB_FLOAT,     gimp_context_get_opacity (dialog->context) * 100.0,
                                 GIMP_PDB_INT32,     GIMP_BRUSH_SELECT (dialog)->spacing,
                                 GIMP_PDB_INT32,     (gint) gimp_context_get_paint_mode (dialog->context),
                                 GIMP_PDB_INT32,     brush->mask->width,
                                 GIMP_PDB_INT32,     brush->mask->height,
                                 GIMP_PDB_INT32,     (brush->mask->width *
                                                      brush->mask->height),
                                 GIMP_PDB_INT8ARRAY, temp_buf_data (brush->mask),

                                 GIMP_PDB_INT32,     closing,
                                 GIMP_PDB_END);
}

static void
gimp_brush_select_opacity_changed (GimpContext     *context,
                                   gdouble          opacity,
                                   GimpBrushSelect *select)
{
  g_signal_handlers_block_by_func (select->opacity_data,
                                   gimp_brush_select_opacity_update,
				   select);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (select->opacity_data),
			    opacity * 100.0);

  g_signal_handlers_unblock_by_func (select->opacity_data,
				     gimp_brush_select_opacity_update,
				     select);

  gimp_pdb_dialog_run_callback (GIMP_PDB_DIALOG (select), FALSE);
}

static void
gimp_brush_select_mode_changed (GimpContext          *context,
                                GimpLayerModeEffects  paint_mode,
                                GimpBrushSelect      *select)
{
  gimp_paint_mode_menu_set_history (GTK_OPTION_MENU (select->paint_mode_menu),
				    paint_mode);

  gimp_pdb_dialog_run_callback (GIMP_PDB_DIALOG (select), FALSE);
}

static void
gimp_brush_select_opacity_update (GtkAdjustment   *adjustment,
                                  GimpBrushSelect *select)
{
  gimp_context_set_opacity (GIMP_PDB_DIALOG (select)->context,
                            adjustment->value / 100.0);
}

static void
gimp_brush_select_mode_update (GtkWidget       *widget,
                               GimpBrushSelect *select)
{
  GimpLayerModeEffects  paint_mode;

  paint_mode = (GimpLayerModeEffects)
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "gimp-item-data"));

  gimp_context_set_paint_mode (GIMP_PDB_DIALOG (select)->context, paint_mode);
}

static void
gimp_brush_select_spacing_update (GtkAdjustment   *adjustment,
                                  GimpBrushSelect *select)
{
  if (select->spacing != adjustment->value)
    {
      select->spacing = adjustment->value;

      gimp_pdb_dialog_run_callback (GIMP_PDB_DIALOG (select), FALSE);
    }
}
