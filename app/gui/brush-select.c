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

#include "widgets/widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"

#include "pdb/procedural_db.h"

#include "widgets/gimpbrushfactoryview.h"
#include "widgets/gimpwidgets-constructors.h"

#include "brush-select.h"
#include "dialogs-constructors.h"

#include "gimprc.h"

#include "libgimp/gimpintl.h"


#define MIN_CELL_SIZE     25
#define STD_BRUSH_COLUMNS  5
#define STD_BRUSH_ROWS     5


/*  local function prototypes  */
static void     brush_select_change_callbacks       (BrushSelect      *bsp,
						     gboolean          closing);

static void     brush_select_brush_changed          (GimpContext      *context,
						     GimpBrush        *brush,
						     BrushSelect      *bsp);
static void     brush_select_opacity_changed        (GimpContext      *context,
						     gdouble           opacity,
						     BrushSelect      *bsp);
static void     brush_select_paint_mode_changed     (GimpContext      *context,
						     LayerModeEffects  paint_mode,
						     BrushSelect      *bsp);

static void     opacity_scale_update                (GtkAdjustment    *adj,
						     gpointer          data);
static void     paint_mode_menu_callback            (GtkWidget        *widget,
						     gpointer          data);
static void     spacing_scale_update                (GtkAdjustment    *adj,
						     gpointer          data);

static void     brush_select_close_callback         (GtkWidget       *widget,
						     gpointer         data);


/*  list of active dialogs  */
static GSList *brush_active_dialogs = NULL;

/*  the main brush selection dialog  */
static BrushSelect *brush_select_dialog = NULL;


/*  public functions  */

GtkWidget *
brush_dialog_create (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (! brush_select_dialog)
    {
      brush_select_dialog = brush_select_new (gimp, NULL, NULL, 0.0, 0, 0, NULL);
    }

  return brush_select_dialog->shell;
}

void
brush_dialog_free (void)
{
  if (brush_select_dialog)
    {
      brush_select_free (brush_select_dialog);
      brush_select_dialog = NULL;
    }
}


/*  If title == NULL then it is the main brush dialog  */
BrushSelect *
brush_select_new (Gimp        *gimp,
                  const gchar *title,
		  /*  These are the required initial vals
		   *  If init_name == NULL then use current brush
		   */
		  const gchar *init_name,
		  gdouble      init_opacity,
		  gint         init_spacing,
		  gint         init_mode,
                  const gchar *callback_name)
{
  BrushSelect *bsp;
  GtkWidget   *main_vbox;
  GtkWidget   *sep;
  GtkWidget   *table;
  GtkWidget   *slider;

  GimpBrush *active = NULL;

  static gboolean first_call = TRUE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  bsp = g_new0 (BrushSelect, 1);

  bsp->callback_name = g_strdup (callback_name);

  /*  The shell  */
  bsp->shell = gimp_dialog_new (title ? title : _("Brush Selection"),
				"brush_selection",
				gimp_standard_help_func,
				"dialogs/brush_selection.html",
				title ? GTK_WIN_POS_MOUSE : GTK_WIN_POS_NONE,
				FALSE, TRUE, FALSE,

				"_delete_event_", brush_select_close_callback,
				bsp, NULL, NULL, TRUE, TRUE,

				NULL);

  gtk_dialog_set_has_separator (GTK_DIALOG (bsp->shell), FALSE);
  gtk_widget_hide (GTK_DIALOG (bsp->shell)->action_area);

  if (title)
    {
      bsp->context = gimp_create_context (gimp, title, NULL);
    }
  else
    {
      bsp->context = gimp_get_user_context (gimp);
    }

  if (gimp->no_data && first_call)
    gimp_data_factory_data_init (gimp->brush_factory, FALSE);

  first_call = FALSE;

  if (title && init_name && strlen (init_name))
    {
      active = (GimpBrush *)
	gimp_container_get_child_by_name (gimp->brush_factory->container,
					  init_name);
    }
  else
    {
      active = gimp_context_get_brush (gimp_get_user_context (gimp));
    }

  if (!active)
    active = gimp_context_get_brush (gimp_get_standard_context (gimp));

  if (title)
    {
      gimp_context_set_brush (bsp->context, active);
      gimp_context_set_paint_mode (bsp->context, init_mode);
      gimp_context_set_opacity (bsp->context, init_opacity);
      bsp->spacing_value = init_spacing;
    }

  /*  The main vbox  */
  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (bsp->shell)->vbox), main_vbox);

  /*  The Brush Grid  */
  bsp->view = gimp_brush_factory_view_new (GIMP_VIEW_TYPE_GRID,
					   gimp->brush_factory,
					   dialogs_edit_brush_func,
					   bsp->context,
					   title ? FALSE : TRUE,
					   MIN_CELL_SIZE,
					   STD_BRUSH_COLUMNS,
					   STD_BRUSH_ROWS,
					   NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), bsp->view, TRUE, TRUE, 0);
  gtk_widget_show (bsp->view);

  /*  The vbox for the paint options  */
  bsp->paint_options_box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (bsp->view), bsp->paint_options_box,
		      FALSE, FALSE, 0);

  /*  Create the frame and the table for the options  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (bsp->paint_options_box), table, FALSE, FALSE, 2);

  /*  Create the opacity scale widget  */
  bsp->opacity_data = 
    GTK_ADJUSTMENT (gtk_adjustment_new
		    (gimp_context_get_opacity (bsp->context) * 100.0,
		     0.0, 100.0, 1.0, 1.0, 0.0));
  slider = gtk_hscale_new (bsp->opacity_data);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  g_signal_connect (G_OBJECT (bsp->opacity_data), "value_changed",
                    G_CALLBACK (opacity_scale_update),
                    bsp);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Opacity:"), 1.0, 1.0,
			     slider, 1, FALSE);

  /*  Create the paint mode option menu  */
  bsp->option_menu =
    gimp_paint_mode_menu_new (G_CALLBACK (paint_mode_menu_callback),
			      bsp,
			      TRUE,
			      gimp_context_get_paint_mode (bsp->context));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Mode:"), 1.0, 0.5,
			     bsp->option_menu, 1, TRUE);

  gtk_widget_show (table);
  gtk_widget_show (bsp->paint_options_box);

  /*  A separator after the paint options  */
  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (bsp->paint_options_box), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

  if (title && init_spacing >= 0)
    {
      GtkAdjustment *adj;

      adj = GIMP_BRUSH_FACTORY_VIEW (bsp->view)->spacing_adjustment;

      /*  Use passed spacing instead of brushes default  */
      gtk_adjustment_set_value (adj, init_spacing);
    }

  g_signal_connect
    (G_OBJECT (GIMP_BRUSH_FACTORY_VIEW (bsp->view)->spacing_adjustment),
     "value_changed",
     G_CALLBACK (spacing_scale_update),
     bsp);

  gtk_widget_show (table);

  gtk_widget_show (main_vbox);

  /*  Only for main dialog  */
  if (! title)
    {
      /*  if we are in per-tool paint options mode, hide the paint options  */
      brush_select_show_paint_options (bsp, gimprc.global_paint_options);
    }

  gtk_widget_show (bsp->shell);

  g_signal_connect (G_OBJECT (bsp->context), "brush_changed",
                    G_CALLBACK (brush_select_brush_changed),
                    bsp);
  g_signal_connect (G_OBJECT (bsp->context), "opacity_changed",
                    G_CALLBACK (brush_select_opacity_changed),
                    bsp);
  g_signal_connect (G_OBJECT (bsp->context), "paint_mode_changed",
                    G_CALLBACK (brush_select_paint_mode_changed),
                    bsp);

  /*  Add to active brush dialogs list  */
  brush_active_dialogs = g_slist_append (brush_active_dialogs, bsp);

  return bsp;
}

void
brush_select_free (BrushSelect *bsp)
{
  g_return_if_fail (bsp != NULL);

  gtk_widget_destroy (bsp->shell); 

  /* remove from active list */
  brush_active_dialogs = g_slist_remove (brush_active_dialogs, bsp);

  g_signal_handlers_disconnect_by_func (G_OBJECT (bsp->context), 
                                        brush_select_brush_changed,
                                        bsp);
  g_signal_handlers_disconnect_by_func (G_OBJECT (bsp->context), 
                                        brush_select_opacity_changed,
                                        bsp);
  g_signal_handlers_disconnect_by_func (G_OBJECT (bsp->context), 
                                        brush_select_paint_mode_changed,
                                        bsp);
  
  if (bsp->callback_name)
    {
      g_free (bsp->callback_name);
      g_object_unref (G_OBJECT (bsp->context));
    }

  g_free (bsp);
}

void
brush_select_show_paint_options (BrushSelect *bsp,
				 gboolean     show)
{
  if ((bsp == NULL) && ((bsp = brush_select_dialog) == NULL))
    return;

  if (show)
    {
      if (! GTK_WIDGET_VISIBLE (bsp->paint_options_box))
	gtk_widget_show (bsp->paint_options_box);
    }
  else
    {
      if (GTK_WIDGET_VISIBLE (bsp->paint_options_box))
	gtk_widget_hide (bsp->paint_options_box);
    }
}

/*  call this dialog's PDB callback  */

static void
brush_select_change_callbacks (BrushSelect *bsp,
			       gboolean     closing)
{
  gchar      *name;
  ProcRecord *prec = NULL;
  GimpBrush  *brush;
  gint        nreturn_vals;

  static gboolean busy = FALSE;

  /* Any procs registered to callback? */
  Argument *return_vals; 

  if (!bsp || !bsp->callback_name || busy)
    return;

  busy  = TRUE;
  name  = bsp->callback_name;
  brush = gimp_context_get_brush (bsp->context);

  /* If its still registered run it */
  prec = procedural_db_lookup (bsp->context->gimp, name);

  if (prec && brush)
    {
      return_vals =
	procedural_db_run_proc (bsp->context->gimp,
				name,
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
				GIMP_PDB_INT32,     (gint) closing,
				GIMP_PDB_END);
 
      if (!return_vals || return_vals[0].value.pdb_int != GIMP_PDB_SUCCESS)
	g_message ("failed to run brush callback function");
      
      procedural_db_destroy_args (return_vals, nreturn_vals);
    }
  busy = FALSE;
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

/* Close active dialogs that no longer have PDB registered for them */

void
brush_select_dialogs_check (void)
{
  BrushSelect *bsp;
  GSList      *list;
  gchar       *name;
  ProcRecord  *prec = NULL;

  list = brush_active_dialogs;

  while (list)
    {
      bsp = (BrushSelect *) list->data;
      list = g_slist_next (list);

      name = bsp->callback_name;

      if (name)
	{
	  prec = procedural_db_lookup (bsp->context->gimp, name);

	  if (!prec)
	    {
	      /*  Can alter brush_active_dialogs list  */
	      brush_select_close_callback (NULL, bsp);
	    }
	}
    }
}

/*
 *  Local functions
 */

static void
brush_select_brush_changed (GimpContext *context,
			    GimpBrush   *brush,
			    BrushSelect *bsp)
{
  if (brush && bsp->callback_name)
    {
      brush_select_change_callbacks (bsp, FALSE);
    }
}

static void
brush_select_opacity_changed (GimpContext *context,
			      gdouble      opacity,
			      BrushSelect *bsp)
{
  g_signal_handlers_block_by_func (G_OBJECT (bsp->opacity_data),
				   opacity_scale_update,
				   bsp);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (bsp->opacity_data),
			    opacity * 100.0);

  g_signal_handlers_unblock_by_func (G_OBJECT (bsp->opacity_data),
				     opacity_scale_update,
				     bsp);

  if (bsp->callback_name)
    brush_select_change_callbacks (bsp, FALSE);
}

static void
brush_select_paint_mode_changed (GimpContext      *context,
				 LayerModeEffects  paint_mode,
				 BrushSelect      *bsp)
{
  gimp_option_menu_set_history (GTK_OPTION_MENU (bsp->option_menu),
				GINT_TO_POINTER (paint_mode));

  if (bsp->callback_name)
    brush_select_change_callbacks (bsp, FALSE);
}

static void
opacity_scale_update (GtkAdjustment *adjustment,
		      gpointer       data)
{
  BrushSelect *bsp;

  bsp = (BrushSelect *) data;

  g_signal_handlers_block_by_func (G_OBJECT (bsp->context),
				   brush_select_opacity_changed,
				   data);

  gimp_context_set_opacity (bsp->context, adjustment->value / 100.0);

  g_signal_handlers_unblock_by_func (G_OBJECT (bsp->context),
				     brush_select_opacity_changed,
				     data);
}

static void
paint_mode_menu_callback (GtkWidget *widget,
			  gpointer   data)
{
  BrushSelect      *bsp;
  LayerModeEffects  paint_mode;

  bsp = (BrushSelect *) data;

  paint_mode = (LayerModeEffects)
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "user_data"));

  gimp_context_set_paint_mode (bsp->context, paint_mode);
}

static void
spacing_scale_update (GtkAdjustment *adjustment,
		      gpointer       data)
{
  BrushSelect *bsp;

  bsp = (BrushSelect *) data;

  if (bsp->callback_name && bsp->spacing_value != adjustment->value)
    {
      bsp->spacing_value = adjustment->value;
      brush_select_change_callbacks (bsp, FALSE);
    }
}

static void
brush_select_close_callback (GtkWidget *widget,
			     gpointer   data)
{
  BrushSelect *bsp;

  bsp = (BrushSelect *) data;

  if (GTK_WIDGET_VISIBLE (bsp->shell))
    gtk_widget_hide (bsp->shell);

  /* Free memory if poping down dialog which is not the main one */
  if (bsp != brush_select_dialog)
    {
      /* Send data back */
      brush_select_change_callbacks (bsp, TRUE);
      brush_select_free (bsp); 
    }
}
