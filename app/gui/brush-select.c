/* The GIMP -- an image manipulation program
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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"

#include "pdb/procedural_db.h"

#include "widgets/gimpbrushfactoryview.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-constructors.h"

#include "brush-select.h"
#include "dialogs-constructors.h"
#include "menus.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void     brush_select_change_callbacks   (BrushSelect          *bsp,
                                                 gboolean              closing);

static void     brush_select_brush_changed      (GimpContext          *context,
                                                 GimpBrush            *brush,
                                                 BrushSelect          *bsp);
static void     brush_select_opacity_changed    (GimpContext          *context,
                                                 gdouble               opacity,
                                                 BrushSelect          *bsp);
static void     brush_select_paint_mode_changed (GimpContext          *context,
                                                 GimpLayerModeEffects  paint_mode,
                                                 BrushSelect          *bsp);

static void     opacity_scale_update            (GtkAdjustment        *adj,
                                                 BrushSelect          *bsp);
static void     paint_mode_menu_callback        (GtkWidget            *widget,
                                                 BrushSelect          *bsp);
static void     spacing_scale_update            (GtkAdjustment        *adj,
                                                 BrushSelect          *bsp);

static void     brush_select_response           (GtkWidget            *widget,
                                                 gint                  response_id,
                                                 BrushSelect          *bsp);


/*  list of active dialogs  */
static GSList *brush_active_dialogs = NULL;


/*  public functions  */

BrushSelect *
brush_select_new (Gimp                 *gimp,
                  GimpContext          *context,
                  const gchar          *title,
		  const gchar          *initial_brush,
                  const gchar          *callback_name,
		  gdouble               initial_opacity,
		  GimpLayerModeEffects  initial_mode,
		  gint                  initial_spacing)
{
  BrushSelect   *bsp;
  GtkWidget     *table;
  GtkAdjustment *spacing_adj;
  GimpBrush     *active = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (title != NULL, NULL);

  if (gimp->no_data)
    {
      static gboolean first_call = TRUE;

      if (first_call)
        gimp_data_factory_data_init (gimp->brush_factory, FALSE);

      first_call = FALSE;
    }

  if (initial_brush && strlen (initial_brush))
    {
      active = (GimpBrush *)
	gimp_container_get_child_by_name (gimp->brush_factory->container,
					  initial_brush);
    }

  if (! active)
    active = gimp_context_get_brush (context);

  if (! active)
    return NULL;

  bsp = g_new0 (BrushSelect, 1);

  /*  Add to active brush dialogs list  */
  brush_active_dialogs = g_slist_append (brush_active_dialogs, bsp);

  bsp->context       = gimp_context_new (gimp, title, NULL);
  bsp->callback_name = g_strdup (callback_name);

  gimp_context_set_brush (bsp->context, active);
  gimp_context_set_paint_mode (bsp->context, initial_mode);
  gimp_context_set_opacity (bsp->context, initial_opacity);
  bsp->spacing_value = initial_spacing;

  g_signal_connect (bsp->context, "brush_changed",
                    G_CALLBACK (brush_select_brush_changed),
                    bsp);
  g_signal_connect (bsp->context, "opacity_changed",
                    G_CALLBACK (brush_select_opacity_changed),
                    bsp);
  g_signal_connect (bsp->context, "paint_mode_changed",
                    G_CALLBACK (brush_select_paint_mode_changed),
                    bsp);

  /*  The shell  */
  bsp->shell = gimp_dialog_new (title, "brush_selection",
                                NULL, 0,
				gimp_standard_help_func,
				GIMP_HELP_BRUSH_DIALOG,

				GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

                                NULL);

  g_signal_connect (bsp->shell, "response",
                    G_CALLBACK (brush_select_response),
                    bsp);

  /*  The Brush Grid  */
  bsp->view = gimp_brush_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                           gimp->brush_factory,
                                           dialogs_edit_brush_func,
                                           bsp->context,
                                           FALSE,
                                           GIMP_PREVIEW_SIZE_MEDIUM, 1,
                                           global_menu_factory);

  gimp_container_view_set_size_request (GIMP_CONTAINER_VIEW (GIMP_CONTAINER_EDITOR (bsp->view)->view),
                                        5 * (GIMP_PREVIEW_SIZE_MEDIUM + 2),
                                        5 * (GIMP_PREVIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (bsp->view), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (bsp->shell)->vbox), bsp->view);
  gtk_widget_show (bsp->view);

  /*  Create the frame and the table for the options  */
  table = GIMP_BRUSH_FACTORY_VIEW (bsp->view)->spacing_scale->parent;
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);

  /*  Create the opacity scale widget  */
  bsp->opacity_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                          _("Opacity:"), -1, 5,
                                          gimp_context_get_opacity (bsp->context) * 100.0,
                                          0.0, 100.0, 1.0, 10.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (bsp->opacity_data, "value_changed",
                    G_CALLBACK (opacity_scale_update),
                    bsp);

  /*  Create the paint mode option menu  */
  bsp->paint_mode_menu =
    gimp_paint_mode_menu_new (G_CALLBACK (paint_mode_menu_callback),
			      bsp,
			      TRUE,
			      gimp_context_get_paint_mode (bsp->context));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Mode:"), 1.0, 0.5,
			     bsp->paint_mode_menu, 2, TRUE);

  spacing_adj = GIMP_BRUSH_FACTORY_VIEW (bsp->view)->spacing_adjustment;

  /*  Use passed spacing instead of brushes default  */
  if (initial_spacing >= 0)
    gtk_adjustment_set_value (spacing_adj, initial_spacing);

  g_signal_connect (spacing_adj, "value_changed",
                    G_CALLBACK (spacing_scale_update),
                    bsp);

  gtk_widget_show (table);

  gtk_widget_show (bsp->shell);

  return bsp;
}

void
brush_select_free (BrushSelect *bsp)
{
  g_return_if_fail (bsp != NULL);

  gtk_widget_destroy (bsp->shell);

  /* remove from active list */
  brush_active_dialogs = g_slist_remove (brush_active_dialogs, bsp);

  if (bsp->callback_name)
    g_free (bsp->callback_name);

  if (bsp->context)
    g_object_unref (bsp->context);

  g_free (bsp);
}

BrushSelect *
brush_select_get_by_callback (const gchar *callback_name)
{
  GSList      *list;
  BrushSelect *bsp;

  for (list = brush_active_dialogs; list; list = g_slist_next (list))
    {
      bsp = (BrushSelect *) list->data;

      if (bsp->callback_name && ! strcmp (callback_name, bsp->callback_name))
	return bsp;
    }

  return NULL;
}

void
brush_select_dialogs_check (void)
{
  GSList *list = brush_active_dialogs;

  while (list)
    {
      BrushSelect *bsp = list->data;

      list = g_slist_next (list);

      if (bsp->callback_name)
        {
          if (! procedural_db_lookup (bsp->context->gimp, bsp->callback_name))
            brush_select_response (NULL, GTK_RESPONSE_CLOSE, bsp);
        }
    }
}


/*  private functions  */

static void
brush_select_change_callbacks (BrushSelect *bsp,
			       gboolean     closing)
{
  ProcRecord *proc;
  GimpBrush  *brush;

  static gboolean busy = FALSE;

  if (! (bsp && bsp->callback_name) || busy)
    return;

  busy  = TRUE;

  brush = gimp_context_get_brush (bsp->context);

  /* If its still registered run it */
  proc = procedural_db_lookup (bsp->context->gimp, bsp->callback_name);

  if (proc && brush)
    {
      Argument *return_vals;
      gint      nreturn_vals;

      return_vals =
	procedural_db_run_proc (bsp->context->gimp,
                                bsp->context,
				bsp->callback_name,
				&nreturn_vals,
				GIMP_PDB_STRING,    GIMP_OBJECT (brush)->name,
				GIMP_PDB_FLOAT,     gimp_context_get_opacity (bsp->context) * 100.0,
				GIMP_PDB_INT32,     bsp->spacing_value,
				GIMP_PDB_INT32,     (gint) gimp_context_get_paint_mode (bsp->context),
				GIMP_PDB_INT32,     brush->mask->width,
				GIMP_PDB_INT32,     brush->mask->height,
				GIMP_PDB_INT32,     (brush->mask->width *
						     brush->mask->height),
				GIMP_PDB_INT8ARRAY, temp_buf_data (brush->mask),
				GIMP_PDB_INT32,     closing,
				GIMP_PDB_END);

      if (!return_vals || return_vals[0].value.pdb_int != GIMP_PDB_SUCCESS)
	g_message (_("Unable to run brush callback. "
                     "The corresponding plug-in may have crashed."));

      if (return_vals)
        procedural_db_destroy_args (return_vals, nreturn_vals);
    }

  busy = FALSE;
}

static void
brush_select_brush_changed (GimpContext *context,
			    GimpBrush   *brush,
			    BrushSelect *bsp)
{
  if (brush)
    brush_select_change_callbacks (bsp, FALSE);
}

static void
brush_select_opacity_changed (GimpContext *context,
			      gdouble      opacity,
			      BrushSelect *bsp)
{
  g_signal_handlers_block_by_func (bsp->opacity_data,
				   opacity_scale_update,
				   bsp);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (bsp->opacity_data),
			    opacity * 100.0);

  g_signal_handlers_unblock_by_func (bsp->opacity_data,
				     opacity_scale_update,
				     bsp);

  brush_select_change_callbacks (bsp, FALSE);
}

static void
brush_select_paint_mode_changed (GimpContext          *context,
				 GimpLayerModeEffects  paint_mode,
				 BrushSelect          *bsp)
{
  gimp_paint_mode_menu_set_history (GTK_OPTION_MENU (bsp->paint_mode_menu),
				    paint_mode);

  brush_select_change_callbacks (bsp, FALSE);
}

static void
opacity_scale_update (GtkAdjustment *adjustment,
		      BrushSelect   *bsp)
{
  g_signal_handlers_block_by_func (bsp->context,
				   brush_select_opacity_changed,
				   bsp);

  gimp_context_set_opacity (bsp->context, adjustment->value / 100.0);

  g_signal_handlers_unblock_by_func (bsp->context,
				     brush_select_opacity_changed,
				     bsp);

  brush_select_change_callbacks (bsp, FALSE);
}

static void
paint_mode_menu_callback (GtkWidget   *widget,
			  BrushSelect *bsp)
{
  GimpLayerModeEffects  paint_mode;

  paint_mode = (GimpLayerModeEffects)
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "gimp-item-data"));

  gimp_context_set_paint_mode (bsp->context, paint_mode);
}

static void
spacing_scale_update (GtkAdjustment *adjustment,
		      BrushSelect   *bsp)
{
  if (bsp->spacing_value != adjustment->value)
    {
      bsp->spacing_value = adjustment->value;
      brush_select_change_callbacks (bsp, FALSE);
    }
}

static void
brush_select_response (GtkWidget   *widget,
                       gint         response_id,
                       BrushSelect *bsp)
{
  brush_select_change_callbacks (bsp, TRUE);
  brush_select_free (bsp);
}
