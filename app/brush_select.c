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

#include "apptypes.h"

#include "appenv.h"
#include "context_manager.h"
#include "brush_edit.h"
#include "brush_select.h"
#include "brushes.h"
#include "dialog_handler.h"
#include "gimpbrushgenerated.h"
#include "gimpcontainer.h"
#include "gimpcontainergridview.h"
#include "gimpcontext.h"
#include "gimpdata.h"
#include "gimpdnd.h"
#include "gimprc.h"
#include "session.h"
#include "temp_buf.h"

#include "tools/paint_options.h"

#include "pdb/procedural_db.h"

#include "libgimp/gimpintl.h"


#define MIN_CELL_SIZE     25
#define STD_BRUSH_COLUMNS  5
#define STD_BRUSH_ROWS     5


/*  local function prototypes  */
static void     brush_select_change_callbacks       (BrushSelect      *bsp,
						     gboolean          closing);

static void     brush_select_drop_brush             (GtkWidget        *widget,
						     GimpViewable     *viewable,
						     gpointer          data);
static void     brush_select_brush_changed          (GimpContext      *context,
						     GimpBrush        *brush,
						     BrushSelect      *bsp);
static void     brush_select_opacity_changed        (GimpContext      *context,
						     gdouble           opacity,
						     BrushSelect      *bsp);
static void     brush_select_paint_mode_changed     (GimpContext      *context,
						     LayerModeEffects  paint_mode,
						     BrushSelect      *bsp);
static void     brush_select_select                 (BrushSelect      *bsp,
						     GimpBrush        *brush);

static void     brush_select_brush_renamed_callback (GimpBrush        *brush,
						     BrushSelect      *bsp);

static void     brush_select_update_active_brush_field (BrushSelect   *bsp);

static void     opacity_scale_update                (GtkAdjustment    *adj,
						     gpointer          data);
static void     paint_mode_menu_callback            (GtkWidget        *widget,
						     gpointer          data);
static void     spacing_scale_update                (GtkAdjustment    *adj,
						     gpointer          data);

static void     brush_select_close_callback         (GtkWidget       *widget,
						     gpointer         data);
static void     brush_select_refresh_callback       (GtkWidget       *widget,
						     gpointer         data);
static void     brush_select_new_brush_callback     (GtkWidget       *widget,
						     gpointer         data);
static void     brush_select_edit_brush_callback    (GtkWidget       *widget,
						     gpointer         data);
static void     brush_select_delete_brush_callback  (GtkWidget       *widget,
						     gpointer         data);


/*  The main brush selection dialog  */
BrushSelect *brush_select_dialog = NULL;

/*  List of active dialogs  */
GSList *brush_active_dialogs = NULL;

/*  Brush editor dialog  */
static BrushEditGeneratedWindow *brush_edit_generated_dialog;


void
brush_dialog_create (void)
{
  if (! brush_select_dialog)
    {
      brush_select_dialog = brush_select_new (NULL, NULL, 0.0, 0, 0);
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (brush_select_dialog->shell))
	gtk_widget_show (brush_select_dialog->shell);
      else
	gdk_window_raise (brush_select_dialog->shell->window);
    }
}

void
brush_dialog_free (void)
{
  if (brush_select_dialog)
    {
      session_get_window_info (brush_select_dialog->shell,
			       &brush_select_session_info);

      /*  save the size of the preview  */
      brush_select_session_info.width =
	brush_select_dialog->view->allocation.width;
      brush_select_session_info.height =
	brush_select_dialog->view->allocation.height;

      brush_select_free (brush_select_dialog);
      brush_select_dialog = NULL;
    }
}


/*  If title == NULL then it is the main brush dialog  */
BrushSelect *
brush_select_new (gchar   *title,
		  /*  These are the required initial vals
		   *  If init_name == NULL then use current brush
		   */
		  gchar   *init_name,
		  gdouble  init_opacity,
		  gint     init_spacing,
		  gint     init_mode)
{
  BrushSelect *bsp;
  GtkWidget   *vbox;
  GtkWidget   *hbox;
  GtkWidget   *sep;
  GtkWidget   *table;
  GtkWidget   *util_box;
  GtkWidget   *slider;
  GtkWidget   *button;

  GimpBrush *active = NULL;

  static gboolean first_call = TRUE;

  bsp = g_new0 (BrushSelect, 1);

  /*  The shell  */
  bsp->shell = gimp_dialog_new (title ? title : _("Brush Selection"),
				"brush_selection",
				gimp_standard_help_func,
				"dialogs/brush_selection.html",
				title ? GTK_WIN_POS_MOUSE : GTK_WIN_POS_NONE,
				FALSE, TRUE, FALSE,

				_("Refresh"), brush_select_refresh_callback,
				bsp, NULL, NULL, FALSE, FALSE,
				_("Close"), brush_select_close_callback,
				bsp, NULL, NULL, TRUE, TRUE,

				NULL);

  if (title)
    {
      bsp->context = gimp_context_new (title, NULL);
    }
  else
    {
      bsp->context = gimp_context_get_user ();

      session_set_window_geometry (bsp->shell, &brush_select_session_info,
				   FALSE);
      dialog_register (bsp->shell);
    }

  if (no_data && first_call)
    brushes_init (FALSE);

  first_call = FALSE;

  if (title && init_name && strlen (init_name))
    {
      active = (GimpBrush *)
	gimp_container_get_child_by_name (GIMP_CONTAINER (global_brush_list),
					  init_name);
    }
  else
    {
      active = gimp_context_get_brush (gimp_context_get_user ());
    }

  if (!active)
    {
      active = gimp_context_get_brush (gimp_context_get_standard ());
    }

  if (title)
    {
      gimp_context_set_brush (bsp->context, active);
      gimp_context_set_paint_mode (bsp->context, init_mode);
      gimp_context_set_opacity (bsp->context, init_opacity);
      bsp->spacing_value = init_spacing;
    }

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (bsp->shell)->vbox), vbox);

  /*  The horizontal box containing the brush list & options box */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (vbox), hbox);

  /*  A place holder for paint mode switching  */
  bsp->left_box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), bsp->left_box, TRUE, TRUE, 0);

  /*  The horizontal box containing preview & scrollbar  */
  bsp->brush_selection_box = gtk_hbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (bsp->left_box), bsp->brush_selection_box);

  /*  The Brush Grid  */
  bsp->view = gimp_container_grid_view_new (global_brush_list,
                                            bsp->context,
                                            MIN_CELL_SIZE,
                                            STD_BRUSH_COLUMNS,
                                            STD_BRUSH_ROWS);
  gtk_box_pack_start (GTK_BOX (bsp->brush_selection_box), bsp->view,
		      TRUE, TRUE, 0);
  gtk_widget_show (bsp->view);

  gimp_gtk_drag_dest_set_by_type (bsp->view,
                                  GTK_DEST_DEFAULT_ALL,
                                  GIMP_TYPE_BRUSH,
                                  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (bsp->view),
                              GIMP_TYPE_BRUSH,
                              brush_select_drop_brush,
                              bsp);

  gtk_widget_show (bsp->brush_selection_box);
  gtk_widget_show (bsp->left_box);

  /*  Options box  */
  bsp->options_box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), bsp->options_box, FALSE, FALSE, 0);

  /*  Create the active brush label  */
  util_box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (bsp->options_box), util_box, FALSE, FALSE, 2);

  bsp->brush_name = gtk_label_new (_("No Brushes available"));
  gtk_box_pack_start (GTK_BOX (util_box), bsp->brush_name, FALSE, FALSE, 4);
  bsp->brush_size = gtk_label_new ("(0 x 0)");
  gtk_box_pack_start (GTK_BOX (util_box), bsp->brush_size, FALSE, FALSE, 2);

  gtk_widget_show (bsp->brush_name);
  gtk_widget_show (bsp->brush_size);
  gtk_widget_show (util_box);

  /*  A place holder for paint mode switching  */
  bsp->right_box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (bsp->options_box), bsp->right_box, TRUE, TRUE, 0);

  /*  The vbox for the paint options  */
  bsp->paint_options_box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (bsp->right_box), bsp->paint_options_box,
		      FALSE, FALSE, 0);

  /*  A separator before the paint options  */
  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (bsp->paint_options_box), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

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
  gtk_signal_connect (GTK_OBJECT (bsp->opacity_data), "value_changed",
		      GTK_SIGNAL_FUNC (opacity_scale_update),
		      bsp);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Opacity:"), 1.0, 1.0,
			     slider, 1, FALSE);

  /*  Create the paint mode option menu  */
  bsp->option_menu =
    paint_mode_menu_new (paint_mode_menu_callback, (gpointer) bsp,
			 gimp_context_get_paint_mode (bsp->context));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Mode:"), 1.0, 0.5,
			     bsp->option_menu, 1, TRUE);

  gtk_widget_show (table);
  gtk_widget_show (bsp->paint_options_box);
  gtk_widget_show (bsp->right_box);

  /*  Create the edit/new buttons  */
  util_box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (bsp->options_box), util_box, FALSE, FALSE, 4);

  button =  gtk_button_new_with_label (_("New"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (brush_select_new_brush_callback),
		      bsp);
  gtk_box_pack_start (GTK_BOX (util_box), button, TRUE, TRUE, 6);

  bsp->edit_button =  gtk_button_new_with_label (_("Edit"));
  gtk_signal_connect (GTK_OBJECT (bsp->edit_button), "clicked",
		      GTK_SIGNAL_FUNC (brush_select_edit_brush_callback),
		      bsp);
  gtk_box_pack_start (GTK_BOX (util_box), bsp->edit_button, TRUE, TRUE, 5);

  bsp->delete_button =  gtk_button_new_with_label (_("Delete"));
  gtk_signal_connect (GTK_OBJECT (bsp->delete_button), "clicked",
		      GTK_SIGNAL_FUNC (brush_select_delete_brush_callback),
		      bsp);
  gtk_box_pack_start (GTK_BOX (util_box), bsp->delete_button, TRUE, TRUE, 5);

  gtk_widget_show (bsp->edit_button);    
  gtk_widget_show (button);
  gtk_widget_show (bsp->delete_button);    
  gtk_widget_show (util_box);

  /*  Create the spacing scale widget  */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_box_pack_end (GTK_BOX (bsp->options_box), table, FALSE, FALSE, 2);

  bsp->spacing_data =
    GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 1.0, 1000.0, 1.0, 1.0, 0.0));
  slider = gtk_hscale_new (bsp->spacing_data);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  if (title && init_spacing >= 0)
    {
      /*  Use passed spacing instead of brushes default  */
      gtk_adjustment_set_value (GTK_ADJUSTMENT (bsp->spacing_data),
				init_spacing);
    }
  gtk_signal_connect (GTK_OBJECT (bsp->spacing_data), "value_changed",
		      GTK_SIGNAL_FUNC (spacing_scale_update),
		      bsp);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Spacing:"), 1.0, 1.0,
			     slider, 1, FALSE);

  gtk_widget_show (table);

  gtk_widget_show (bsp->options_box);
  gtk_widget_show (hbox);
  gtk_widget_show (vbox);

  /*  add callbacks to keep the display area current  */
  bsp->name_changed_handler_id =
    gimp_container_add_handler
    (GIMP_CONTAINER (global_brush_list), "name_changed",
     GTK_SIGNAL_FUNC (brush_select_brush_renamed_callback),
     bsp);

  /*  Only for main dialog  */
  if (!title)
    {
      /*  if we are in per-tool paint options mode, hide the paint options  */
      brush_select_show_paint_options (bsp, global_paint_options);
    }

  gtk_widget_show (bsp->shell);

  gtk_signal_connect (GTK_OBJECT (bsp->context), "brush_changed",
		      GTK_SIGNAL_FUNC (brush_select_brush_changed),
		      bsp);
  gtk_signal_connect (GTK_OBJECT (bsp->context), "opacity_changed",
		      GTK_SIGNAL_FUNC (brush_select_opacity_changed),
		      bsp);
  gtk_signal_connect (GTK_OBJECT (bsp->context), "paint_mode_changed",
		      GTK_SIGNAL_FUNC (brush_select_paint_mode_changed),
		      bsp);

  if (active)
    brush_select_select (bsp, active);

  /*  Add to active brush dialogs list  */
  brush_active_dialogs = g_slist_append (brush_active_dialogs, bsp);

  return bsp;
}

void
brush_select_free (BrushSelect *bsp)
{
  if (!bsp)
    return;

  /* remove from active list */
  brush_active_dialogs = g_slist_remove (brush_active_dialogs, bsp);

  gtk_signal_disconnect_by_data (GTK_OBJECT (bsp->context), bsp);

  if (bsp->callback_name)
    {
      g_free (bsp->callback_name);
      gtk_object_unref (GTK_OBJECT (bsp->context));
    }

  gimp_container_remove_handler (GIMP_CONTAINER (global_brush_list),
				 bsp->name_changed_handler_id);

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

      if (bsp->brush_selection_box->parent != bsp->left_box)
	gtk_widget_reparent (bsp->brush_selection_box, bsp->left_box);
      gtk_box_set_child_packing (GTK_BOX (bsp->options_box->parent),
				 bsp->options_box,
				 FALSE, FALSE, 0, GTK_PACK_START);
      gtk_box_set_child_packing (GTK_BOX (bsp->left_box->parent),
				 bsp->left_box,
				 TRUE, TRUE, 0, GTK_PACK_START);
      gtk_box_set_spacing (GTK_BOX (bsp->left_box->parent), 2);
    }
  else
    {
      if (GTK_WIDGET_VISIBLE (bsp->paint_options_box))
	gtk_widget_hide (bsp->paint_options_box);

      if (bsp->brush_selection_box->parent != bsp->right_box)
	gtk_widget_reparent (bsp->brush_selection_box, bsp->right_box);
      gtk_box_set_child_packing (GTK_BOX (bsp->left_box->parent),
				 bsp->left_box,
				 FALSE, FALSE, 0, GTK_PACK_START);
      gtk_box_set_child_packing (GTK_BOX (bsp->options_box->parent),
				 bsp->options_box,
				 TRUE, TRUE, 0, GTK_PACK_START);
      gtk_box_set_spacing (GTK_BOX (bsp->left_box->parent), 0);
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
  prec = procedural_db_lookup (name);

  if (prec && brush)
    {
      return_vals =
	procedural_db_run_proc (name,
				&nreturn_vals,
				PDB_STRING,    GIMP_OBJECT (brush)->name,
				PDB_FLOAT,     gimp_context_get_opacity (bsp->context),
				PDB_INT32,     bsp->spacing_value,
				PDB_INT32,     (gint) gimp_context_get_paint_mode (bsp->context),
				PDB_INT32,     brush->mask->width,
				PDB_INT32,     brush->mask->height,
				PDB_INT32,     (brush->mask->width *
						brush->mask->height),
				PDB_INT8ARRAY, temp_buf_data (brush->mask),
				PDB_INT32,     (gint) closing,
				PDB_END);
 
      if (!return_vals || return_vals[0].value.pdb_int != PDB_SUCCESS)
	g_message ("failed to run brush callback function");
      
      procedural_db_destroy_args (return_vals, nreturn_vals);
    }
  busy = FALSE;
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
	  prec = procedural_db_lookup (name);

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
brush_select_drop_brush (GtkWidget    *widget,
			 GimpViewable *viewable,
			 gpointer      data)
{
  BrushSelect *bsp;

  bsp = (BrushSelect *) data;

  gimp_context_set_brush (bsp->context, GIMP_BRUSH (viewable));
}

static void
brush_select_brush_changed (GimpContext *context,
			    GimpBrush   *brush,
			    BrushSelect *bsp)
{
  if (brush)
    {
      brush_select_select (bsp, brush);

      if (bsp->callback_name)
	brush_select_change_callbacks (bsp, FALSE);
    }
}

static void
brush_select_opacity_changed (GimpContext *context,
			      gdouble      opacity,
			      BrushSelect *bsp)
{
  gtk_signal_handler_block_by_data (GTK_OBJECT (bsp->opacity_data), bsp);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (bsp->opacity_data),
			    opacity * 100.0);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (bsp->opacity_data), bsp);

  if (bsp->callback_name)
    brush_select_change_callbacks (bsp, FALSE);
}

static void
brush_select_paint_mode_changed (GimpContext      *context,
				 LayerModeEffects  paint_mode,
				 BrushSelect      *bsp)
{
  gimp_option_menu_set_history (GTK_OPTION_MENU (bsp->option_menu),
				(gpointer) paint_mode);

  if (bsp->callback_name)
    brush_select_change_callbacks (bsp, FALSE);
}

static void
brush_select_select (BrushSelect *bsp,
		     GimpBrush   *brush)
{
  if (! gimp_container_have (global_brush_list, GIMP_OBJECT (brush)));

  if (GIMP_IS_BRUSH_GENERATED (brush))
    {
      gtk_widget_set_sensitive (bsp->edit_button, TRUE);
      gtk_widget_set_sensitive (bsp->delete_button, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (bsp->edit_button, FALSE);
      gtk_widget_set_sensitive (bsp->delete_button, FALSE);
    }

  brush_select_update_active_brush_field (bsp);
}

static void
brush_select_brush_renamed_callback (GimpBrush   *brush,
				     BrushSelect *bsp)
{
  brush_select_update_active_brush_field (bsp);
}

static void
brush_select_update_active_brush_field (BrushSelect *bsp)
{
  GimpBrush *brush;
  gchar      buf[32];

  brush = gimp_context_get_brush (bsp->context);

  if (!brush)
    return;

  gtk_label_set_text (GTK_LABEL (bsp->brush_name), GIMP_OBJECT (brush)->name);

  g_snprintf (buf, sizeof (buf), "(%d x %d)",
	      brush->mask->width,
	      brush->mask->height);
  gtk_label_set_text (GTK_LABEL (bsp->brush_size), buf);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (bsp->spacing_data),
			    gimp_brush_get_spacing (brush));
}

static void
opacity_scale_update (GtkAdjustment *adjustment,
		      gpointer       data)
{
  BrushSelect *bsp;

  bsp = (BrushSelect *) data;

  gtk_signal_handler_block_by_data (GTK_OBJECT (bsp->context), data);
  gimp_context_set_opacity (bsp->context, adjustment->value / 100.0);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (bsp->context), data);
}

static void
paint_mode_menu_callback (GtkWidget *widget,
			  gpointer   data)
{
  BrushSelect      *bsp;
  LayerModeEffects  paint_mode;

  bsp = (BrushSelect *) data;

  paint_mode = (LayerModeEffects) gtk_object_get_user_data (GTK_OBJECT (widget));

  gimp_context_set_paint_mode (bsp->context, paint_mode);
}

static void
spacing_scale_update (GtkAdjustment *adjustment,
		      gpointer       data)
{
  BrushSelect *bsp;

  bsp = (BrushSelect *) data;

  if (bsp == brush_select_dialog)
    {
      gimp_brush_set_spacing (gimp_context_get_brush (bsp->context),
			      (gint) adjustment->value);
    }
  else
    {
      if (bsp->spacing_value != adjustment->value)
	{
	  bsp->spacing_value = adjustment->value;
	  brush_select_change_callbacks (bsp, FALSE);
	}
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
      gtk_widget_destroy (bsp->shell); 
      brush_select_free (bsp); 
    }
}

static void
brush_select_refresh_callback (GtkWidget *widget,
			       gpointer   data)
{
  /*  re-init the brush list  */
  brushes_init (FALSE);
}

static void
brush_select_new_brush_callback (GtkWidget *widget,
				 gpointer   data)
{
  BrushSelect *bsp;
  GimpBrush   *brush;

  bsp = (BrushSelect *) data;

  brush = gimp_brush_generated_new (10, .5, 0.0, 1.0);

  gimp_container_add (GIMP_CONTAINER (global_brush_list),
		      GIMP_OBJECT (brush));

  gimp_context_set_brush (bsp->context, brush);

  if (brush_edit_generated_dialog)
    brush_edit_generated_set_brush (brush_edit_generated_dialog, brush);

  brush_select_edit_brush_callback (widget, data);
}

static void
brush_select_edit_brush_callback (GtkWidget *widget,
				  gpointer   data)
{
  BrushSelect *bsp;
  GimpBrush   *brush;

  bsp = (BrushSelect *) data;
  brush = gimp_context_get_brush (bsp->context);

  if (GIMP_IS_BRUSH_GENERATED (brush))
    {
      if (! brush_edit_generated_dialog)
	{
	  brush_edit_generated_dialog = brush_edit_generated_new ();

	  brush_edit_generated_set_brush (brush_edit_generated_dialog, brush);
	}
      else
	{
	  if (! GTK_WIDGET_VISIBLE (brush_edit_generated_dialog->shell))
	    gtk_widget_show (brush_edit_generated_dialog->shell);
	  else
	    gdk_window_raise (brush_edit_generated_dialog->shell->window);
	}
    }
  else
    {
      /* this should never happen */
      g_message (_("Sorry, this brush can't be edited."));
    }
}

static void
brush_select_delete_brush_callback (GtkWidget *widget,
				    gpointer   data)
{
  BrushSelect *bsp;
  GimpBrush *brush;

  bsp = (BrushSelect *) data;
  brush = gimp_context_get_brush (bsp->context);

  if (GIMP_IS_BRUSH_GENERATED (brush))
    {
      if (GIMP_DATA (brush)->filename)
	gimp_data_delete_from_disk (GIMP_DATA (brush));

      gimp_container_remove (global_brush_list, GIMP_OBJECT (brush));
    }
  else
    {
      /* this should never happen */
      g_message (_("Sorry, this brush can't be deleted."));
    }
}
