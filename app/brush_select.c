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
#include <string.h>

#include "appenv.h"
#include "brush_scale.h"
#include "brush_edit.h"
#include "brush_select.h"
#include "dialog_handler.h"
#include "gimpbrushgenerated.h"
#include "gimpbrushlist.h"
#include "gimpbrushpipe.h"
#include "gimpbrushpipeP.h"
#include "gimpcontext.h"
#include "gimplist.h"
#include "gimprc.h"
#include "gimpui.h"
#include "paint_options.h"
#include "session.h"

#include "libgimp/gimpintl.h"

#define MIN_CELL_SIZE     25
#define MAX_CELL_SIZE     25  /*  disable variable brush preview size  */ 

#define STD_BRUSH_COLUMNS 5
#define STD_BRUSH_ROWS    5

/* how long to wait after mouse-down before showing brush popup */
#define POPUP_DELAY_MS      150

#define MAX_WIN_WIDTH(bsp)     (MIN_CELL_SIZE * ((bsp)->NUM_BRUSH_COLUMNS))
#define MAX_WIN_HEIGHT(bsp)    (MIN_CELL_SIZE * ((bsp)->NUM_BRUSH_ROWS))
#define MARGIN_WIDTH      1
#define MARGIN_HEIGHT     1

#define BRUSH_EVENT_MASK  GDK_EXPOSURE_MASK | \
                          GDK_BUTTON_PRESS_MASK | \
			  GDK_BUTTON_RELEASE_MASK | \
                          GDK_BUTTON1_MOTION_MASK | \
			  GDK_ENTER_NOTIFY_MASK

/*  the pixmaps for the [scale|pipe]_indicators  */
#define indicator_width  7
#define indicator_height 7

#define WHT {255, 255, 255}
#define BLK {  0,   0,   0}
#define RED {255, 127, 127}

static unsigned char scale_indicator_bits[7][7][3] = 
{
  { WHT, WHT, WHT, WHT, WHT, WHT, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
  { WHT, BLK, BLK, BLK, BLK, BLK, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, WHT },
  { WHT, WHT, WHT, WHT, WHT, WHT, WHT }
};

static unsigned char pipe_indicator_bits[7][7][3] = 
{
  { WHT, WHT, WHT, WHT, WHT, WHT, WHT },
  { WHT, WHT, WHT, WHT, WHT, WHT, RED },
  { WHT, WHT, WHT, WHT, WHT, RED, RED },
  { WHT, WHT, WHT, WHT, RED, RED, RED },
  { WHT, WHT, WHT, RED, RED, RED, RED },
  { WHT, WHT, RED, RED, RED, RED, RED },
  { WHT, RED, RED, RED, RED, RED, RED }
};

static unsigned char scale_pipe_indicator_bits[7][7][3] = 
{
  { WHT, WHT, WHT, WHT, WHT, WHT, WHT },
  { WHT, WHT, WHT, BLK, WHT, WHT, RED },
  { WHT, WHT, WHT, BLK, WHT, RED, RED },
  { WHT, BLK, BLK, BLK, BLK, BLK, RED },
  { WHT, WHT, WHT, BLK, RED, RED, RED },
  { WHT, WHT, RED, BLK, RED, RED, RED },
  { WHT, RED, RED, RED, RED, RED, RED }
};


/*  local function prototypes  */
static void     brush_select_brush_changed         (GimpContext      *context,
						    GimpBrush        *brush,
						    BrushSelect      *bsp);
static void     brush_select_opacity_changed       (GimpContext      *context,
						    gdouble           opacity,
						    BrushSelect      *bsp);
static void     brush_select_paint_mode_changed    (GimpContext      *context,
						    LayerModeEffects  paint_mode,
						    BrushSelect      *bsp);
static void     brush_select_select                (BrushSelect      *bsp,
						    GimpBrush        *brush);

static void     brush_select_brush_dirty_callback  (GimpBrush        *brush,
						    BrushSelect      *bsp);
static void     connect_signals_to_brush           (GimpBrush        *brush, 
						    BrushSelect      *bsp);
static void     disconnect_signals_from_brush      (GimpBrush        *brush,
						    BrushSelect      *bsp);
static void     brush_added_callback               (GimpBrushList    *list,
						    GimpBrush        *brush,
						    BrushSelect      *bsp);
static void     brush_removed_callback             (GimpBrushList    *list,
						    GimpBrush        *brush,
						    BrushSelect      *bsp);

static void     draw_brush_popup                   (GtkPreview       *preview,
						    GimpBrush        *brush,
						    gint              width,
						    gint              height);
static gint     brush_popup_anim_timeout           (gpointer          data);
static gboolean brush_popup_timeout                (gpointer          data);
static void     brush_popup_open                   (BrushSelect *,
						    gint, gint, GimpBrush *);
static void     brush_popup_close                  (BrushSelect *);

static void     display_setup                      (BrushSelect *);
static void     display_brush                      (BrushSelect *,
						    GimpBrush *, gint, gint);
static void     do_display_brush                   (GimpBrush   *brush,
						    BrushSelect *bsp);
static void     display_brushes                    (BrushSelect *);

static void     brush_select_show_selected         (BrushSelect *, gint, gint);
static void     update_active_brush_field          (BrushSelect *);
static void     preview_calc_scrollbar             (BrushSelect *);

static gint     brush_select_resize		   (GtkWidget *, GdkEvent *,
						    BrushSelect *);
static gint     brush_select_events                (GtkWidget *, GdkEvent *,
						    BrushSelect *);

/* static void  brush_select_map_callback          (GtkWidget *,
                                                    BrushSelect *); */
static void     brush_select_scroll_update         (GtkAdjustment *, gpointer);
static void     opacity_scale_update               (GtkAdjustment *, gpointer);
static void     paint_mode_menu_callback           (GtkWidget *, gpointer);
static void     spacing_scale_update               (GtkAdjustment *, gpointer);

static void     brush_select_close_callback        (GtkWidget *, gpointer);
static void     brush_select_refresh_callback      (GtkWidget *, gpointer);
static void     brush_select_new_brush_callback    (GtkWidget *, gpointer);
static void     brush_select_edit_brush_callback   (GtkWidget *, gpointer);
static void     brush_select_delete_brush_callback (GtkWidget *, gpointer);

/*  The main brush selection dialog  */
BrushSelect *brush_select_dialog = NULL;

/*  local variables  */

/*  List of active dialogs  */
GSList *brush_active_dialogs = NULL;

/*  Brush editor dialog  */
static BrushEditGeneratedWindow *brush_edit_generated_dialog;

void
brush_dialog_create (void)
{
  if (! brush_select_dialog)
    {
      /*  Create the dialog...  */
      brush_select_dialog = brush_select_new (NULL, NULL, 0.0, 0, 0);
      
      /* register this one only */
      dialog_register (brush_select_dialog->shell);
    }
  else
    {
      /*  Popup the dialog  */
      if (!GTK_WIDGET_VISIBLE (brush_select_dialog->shell))
	gtk_widget_show (brush_select_dialog->shell);
      else
	gdk_window_raise (brush_select_dialog->shell->window);
    }
  
}

void
brush_dialog_free ()
{
  if (brush_select_dialog)
    {
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
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *sbar;
  GtkWidget *sep;
  GtkWidget *table;
  GtkWidget *util_box;
  GtkWidget *option_menu;
  GtkWidget *menu;
  GtkWidget *slider;
  GtkWidget *button;

  GimpBrush *active = NULL;

  /*  gboolean gotinitbrush = FALSE;  */

  bsp = g_new (BrushSelect, 1);
  bsp->callback_name          = NULL;
  bsp->brush_popup            = NULL;
  bsp->popup_timeout_tag      = 0;
  bsp->popup_anim_timeout_tag = 0;
  bsp->scroll_offset          = 0;
  bsp->old_row                = 0;
  bsp->old_col                = 0;
  bsp->NUM_BRUSH_COLUMNS      = STD_BRUSH_COLUMNS;
  bsp->NUM_BRUSH_ROWS         = STD_BRUSH_ROWS;
  bsp->redraw                 = TRUE;
  bsp->freeze                 = FALSE;

  /*  The shell  */
  bsp->shell = gimp_dialog_new (title ? title : _("Brush Selection"),
				"brush_selection",
				gimp_standard_help_func,
				"dialogs/brush_selection.html",
				GTK_WIN_POS_NONE,
				FALSE, TRUE, FALSE,

				_("Refresh"), brush_select_refresh_callback,
				bsp, NULL, FALSE, FALSE,
				_("Close"), brush_select_close_callback,
				bsp, NULL, TRUE, TRUE,

				NULL);

  if (title)
    {
      bsp->context = gimp_context_new (title, NULL);
    }
  else
    {
      session_set_window_geometry (bsp->shell, &brush_select_session_info,
				   FALSE);
      bsp->context = gimp_context_get_user ();
    }

  if (no_data)
    brushes_init (FALSE);

  if (title && init_name && strlen (init_name))
    {
      active = gimp_brush_list_get_brush (brush_list, init_name);
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

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (bsp->brush_selection_box), frame,
		      TRUE, TRUE, 0);

  bsp->sbar_data =
    GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, MAX_WIN_HEIGHT (bsp),
					1, 1, MAX_WIN_HEIGHT (bsp)));
  sbar = gtk_vscrollbar_new (bsp->sbar_data);
  gtk_signal_connect (GTK_OBJECT (bsp->sbar_data), "value_changed",
		      GTK_SIGNAL_FUNC (brush_select_scroll_update),
		      bsp);
  gtk_box_pack_start (GTK_BOX (bsp->brush_selection_box), sbar, FALSE, FALSE, 0);

  /*  Create the brush preview window and the underlying image  */

  /*  Get the maximum brush extents  */
  bsp->cell_width  = MIN_CELL_SIZE;
  bsp->cell_height = MIN_CELL_SIZE;

  bsp->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (bsp->preview),
		    MAX_WIN_WIDTH (bsp), MAX_WIN_HEIGHT (bsp));
  gtk_widget_set_usize (bsp->preview,
			MAX_WIN_WIDTH (bsp), MAX_WIN_HEIGHT (bsp));
  gtk_preview_set_expand (GTK_PREVIEW (bsp->preview), TRUE);
  gtk_widget_set_events (bsp->preview, BRUSH_EVENT_MASK);

  gtk_signal_connect (GTK_OBJECT (bsp->preview), "event",
		      GTK_SIGNAL_FUNC (brush_select_events),
		      bsp);
  gtk_signal_connect (GTK_OBJECT(bsp->preview), "size_allocate",
		      GTK_SIGNAL_FUNC (brush_select_resize),
		      bsp);

  gtk_container_add (GTK_CONTAINER (frame), bsp->preview);
  gtk_widget_show (bsp->preview);

  gtk_widget_show (sbar);
  gtk_widget_show (frame);
  gtk_widget_show (bsp->brush_selection_box);
  gtk_widget_show (bsp->left_box);

  /*  Options box  */
  bsp->options_box = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), bsp->options_box, FALSE, FALSE, 0);

  /*  Create the active brush label  */
  util_box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (bsp->options_box), util_box, FALSE, FALSE, 2);

  bsp->brush_name = gtk_label_new (_("Active"));
  gtk_box_pack_start (GTK_BOX (util_box), bsp->brush_name, FALSE, FALSE, 4);
  bsp->brush_size = gtk_label_new (_("(0 X 0)"));
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
  gimp_table_attach_aligned (GTK_TABLE (table), 0,
			     _("Opacity:"), 1.0, 1.0,
			     slider, FALSE);

  /*  Create the paint mode option menu  */
  menu = paint_mode_menu_new (paint_mode_menu_callback, (gpointer) bsp);
  bsp->option_menu = option_menu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu),
			       gimp_context_get_paint_mode (bsp->context));
  gimp_table_attach_aligned (GTK_TABLE (table), 1,
			     _("Mode:"), 1.0, 0.5,
			     option_menu, TRUE);

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
  gimp_table_attach_aligned (GTK_TABLE (table), 0,
			     _("Spacing:"), 1.0, 1.0,
			     slider, FALSE);

  gtk_widget_show (table);

  gtk_widget_show (bsp->options_box);
  gtk_widget_show (hbox);
  gtk_widget_show (vbox);

  /*  add callbacks to keep the display area current  */
  gimp_list_foreach (GIMP_LIST (brush_list),
		     (GFunc) connect_signals_to_brush,
		     bsp);
  gtk_signal_connect (GTK_OBJECT (brush_list), "add",
		      GTK_SIGNAL_FUNC (brush_added_callback),
		      bsp);
  gtk_signal_connect (GTK_OBJECT (brush_list), "remove",
		      GTK_SIGNAL_FUNC (brush_removed_callback),
		      bsp);

  /*  Only for main dialog  */
  if (!title)
    {
      /*  set the preview's size in the callback
      gtk_signal_connect (GTK_OBJECT (bsp->shell), "map",
			  GTK_SIGNAL_FUNC (brush_select_map_callback),
			  bsp);
      */

      /*  if we are in per-tool paint options mode, hide the paint options  */
      brush_select_show_paint_options (bsp, global_paint_options);
    }

  gtk_widget_show (bsp->shell);

  preview_calc_scrollbar (bsp);

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

  /* Only main one is saved */
  if (bsp == brush_select_dialog)
    {
      session_get_window_info (bsp->shell, &brush_select_session_info);
      /*  save the size of the preview  */
      brush_select_session_info.width = bsp->preview->allocation.width;
      brush_select_session_info.height = bsp->preview->allocation.height;
    }

  gtk_signal_disconnect_by_data (GTK_OBJECT (bsp->context), bsp);

  if (bsp->brush_popup != NULL)
    gtk_widget_destroy (bsp->brush_popup);

  if (bsp->callback_name)
    {
      g_free (bsp->callback_name);
      gtk_object_unref (GTK_OBJECT (bsp->context));
    }

  gimp_list_foreach (GIMP_LIST (brush_list),
		     (GFunc) disconnect_signals_from_brush,
		     bsp);
  gtk_signal_disconnect_by_data (GTK_OBJECT (brush_list), bsp);

  g_free (bsp);
}

void
brush_select_freeze_all (void)
{
  BrushSelect *bsp;
  GSList *list;

  for (list = brush_active_dialogs; list; list = g_slist_next (list))
    {
      bsp = (BrushSelect *) list->data;

      bsp->freeze = TRUE;
    }
}
void
brush_select_thaw_all (void)
{
  BrushSelect *bsp;
  GSList *list;

  for (list = brush_active_dialogs; list; list = g_slist_next (list))
    {
      bsp = (BrushSelect *) list->data;

      bsp->freeze = FALSE;

      preview_calc_scrollbar (bsp);
    }
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

void
brush_change_callbacks (BrushSelect *bsp,
			gint         closing)
{
  gchar *name;
  ProcRecord *prec = NULL;
  GimpBrush *brush;
  gint nreturn_vals;
  static gboolean busy = FALSE;

  /* Any procs registered to callback? */
  Argument *return_vals; 

  if (!bsp || !bsp->callback_name || busy)
    return;

  busy = TRUE;
  name = bsp->callback_name;
  brush = gimp_context_get_brush (bsp->context);

  /* If its still registered run it */
  prec = procedural_db_lookup (name);

  if (prec && brush)
    {
      return_vals =
	procedural_db_run_proc (name,
				&nreturn_vals,
				PDB_STRING, brush->name,
				PDB_FLOAT, gimp_context_get_opacity (bsp->context),
				PDB_INT32, bsp->spacing_value,
				PDB_INT32, (gint)gimp_context_get_paint_mode (bsp->context),
				PDB_INT32, brush->mask->width,
				PDB_INT32, brush->mask->height,
				PDB_INT32, brush->mask->width * brush->mask->height,
				PDB_INT8ARRAY, temp_buf_data (brush->mask),
				PDB_INT32, closing,
				PDB_END);
 
      if (!return_vals || return_vals[0].value.pdb_int != PDB_SUCCESS)
	g_message ("failed to run brush callback function");
      
      procedural_db_destroy_args (return_vals, nreturn_vals);
    }
  busy = FALSE;
}

/* Close active dialogs that no longer have PDB registered for them */

void
brushes_check_dialogs (void)
{
  BrushSelect *bsp;
  GSList *list;
  gchar *name;
  ProcRecord *prec = NULL;

  list = brush_active_dialogs;

  while (list)
    {
      bsp = (BrushSelect *) list->data;
      list = list->next;

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
brush_select_brush_changed (GimpContext *context,
			    GimpBrush   *brush,
			    BrushSelect *bsp)
{
  if (brush)
    brush_select_select (bsp, brush);
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
}

static void
brush_select_paint_mode_changed (GimpContext      *context,
				 LayerModeEffects  paint_mode,
				 BrushSelect      *bsp)
{
  gtk_option_menu_set_history (GTK_OPTION_MENU (bsp->option_menu), paint_mode);
}

static void
brush_select_select (BrushSelect *bsp,
		     GimpBrush   *brush)
{
  gint index;
  gint row, col;

  index = gimp_brush_list_get_brush_index (brush_list, brush); 

  if (index >= gimp_brush_list_length (brush_list))
    index = gimp_brush_list_length (brush_list) - 1;
  if (index < 0 || index >= gimp_brush_list_length (brush_list))
    return;

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

  update_active_brush_field (bsp);

  row = index / bsp->NUM_BRUSH_COLUMNS;
  col = index - row * (bsp->NUM_BRUSH_COLUMNS);

  brush_select_show_selected (bsp, row, col);
}

static void
brush_select_brush_dirty_callback (GimpBrush   *brush,
				   BrushSelect *bsp)
{
  gint index;

  if (!bsp && bsp->freeze)
    return;

  index = gimp_brush_list_get_brush_index (brush_list, brush);

  display_brush (bsp, brush,
		 index % (bsp->NUM_BRUSH_COLUMNS),
		 index / (bsp->NUM_BRUSH_COLUMNS));
}

static void
connect_signals_to_brush (GimpBrush   *brush,
			  BrushSelect *bsp)
{
  gtk_signal_connect (GTK_OBJECT (brush), "dirty",
		      GTK_SIGNAL_FUNC (brush_select_brush_dirty_callback),
		      bsp);
  gtk_signal_connect (GTK_OBJECT (brush), "rename",
		      GTK_SIGNAL_FUNC (brush_select_brush_dirty_callback),
		      bsp);
}

static void
disconnect_signals_from_brush (GimpBrush   *brush,
			       BrushSelect *bsp)
{
  if (!GTK_OBJECT_DESTROYED (brush))
    gtk_signal_disconnect_by_data (GTK_OBJECT (brush), bsp);
}

static void
brush_added_callback (GimpBrushList *list,
		      GimpBrush     *brush, 
		      BrushSelect   *bsp)
{
  connect_signals_to_brush (brush, bsp);

  if (bsp->freeze)
    return;

  preview_calc_scrollbar (bsp);
}

static void
brush_removed_callback (GimpBrushList *list,
			GimpBrush     *brush,
			BrushSelect   *bsp)
{
  disconnect_signals_from_brush (brush, bsp);

  if (bsp->freeze)
    return;

  preview_calc_scrollbar (bsp);
}

static void
draw_brush_popup (GtkPreview *preview,
		  GimpBrush  *brush,
		  gint        width,
		  gint        height)
{
  gint x, y;
  gint brush_width, brush_height;
  gint offset_x, offset_y;
  guchar *mask, *buf, *b;
  guchar bg;
  
  brush_width = brush->mask->width;
  brush_height = brush->mask->height;
  offset_x = (width - brush_width) / 2;
  offset_y = (height - brush_height) / 2;

  mask = temp_buf_data (brush->mask);
  buf = g_new (guchar, 3 * width);
  memset (buf, 255, 3 * width);

  if (GIMP_IS_BRUSH_PIXMAP (brush)) 
    {
      guchar *pixmap = temp_buf_data (GIMP_BRUSH_PIXMAP (brush)->pixmap_mask);

      for (y = 0; y < offset_y; y++)
	gtk_preview_draw_row (preview, buf, 0, y, width); 
      for (y = offset_y; y < brush_height + offset_y; y++)
	{
	  b = buf + 3 * offset_x;
	  for (x = 0; x < brush_width ; x++)
	    {
	      bg = (255 - *mask);
	      *b++ = bg + (*mask * *pixmap++) / 255;
	      *b++ = bg + (*mask * *pixmap++) / 255; 
	      *b++ = bg + (*mask * *pixmap++) / 255;
	      mask++;
	    }
	  gtk_preview_draw_row (preview, buf, 0, y, width); 
	}
      memset (buf, 255, 3 * width);
      for (y = brush_height + offset_y; y < height; y++)
	gtk_preview_draw_row (preview, buf, 0, y, width); 
    }
  else
    {
      for (y = 0; y < offset_y; y++)
	gtk_preview_draw_row (preview, buf, 0, y, width); 
      for (y = offset_y; y < brush_height + offset_y; y++)
	{
	  b = buf + 3 * offset_x;
	  for (x = 0; x < brush_width ; x++)
	    {
	      bg = 255 - *mask++;
	      memset (b, bg, 3);
	      b += 3;
	    }   
	  gtk_preview_draw_row (preview, buf, 0, y, width); 
	}
      memset (buf, 255, 3 * width);
      for (y = brush_height + offset_y; y < height; y++)
	gtk_preview_draw_row (preview, buf, 0, y, width);    
    }

  g_free (buf);
}

typedef struct
{
  BrushSelect *bsp;
  gint         x;
  gint         y;
  GimpBrush   *brush;
} popup_timeout_args_t;

static gint
brush_popup_anim_timeout (gpointer data)
{
  popup_timeout_args_t *args = data;
  BrushSelect *bsp = args->bsp;
  GimpBrushPipe *pipe;
  GimpBrush *brush;

  if (bsp->brush_popup != NULL && !GTK_WIDGET_VISIBLE (bsp->brush_popup))
    {
      bsp->popup_anim_timeout_tag = 0;
      return (FALSE);
    }

  pipe = GIMP_BRUSH_PIPE (args->brush);
  if (++bsp->popup_pipe_index >= pipe->nbrushes)
    bsp->popup_pipe_index = 0;
  brush = GIMP_BRUSH (pipe->brushes[bsp->popup_pipe_index]);

  draw_brush_popup (GTK_PREVIEW (bsp->brush_preview), brush, args->x, args->y);
  gtk_widget_queue_draw (bsp->brush_preview);

  return (TRUE);
}

static gboolean
brush_popup_timeout (gpointer data)
{
  popup_timeout_args_t *args = data;
  BrushSelect *bsp = args->bsp;
  GimpBrush *brush = args->brush;
  gint x, y;
  gint x_org, y_org;
  gint scr_w, scr_h;
  gint width, height;

  /* timeout has gone off so our tag is now invalid  */
  bsp->popup_timeout_tag = 0;

  /* make sure the popup exists and is not visible */
  if (bsp->brush_popup == NULL)
    {
      GtkWidget *frame;

      bsp->brush_popup = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_policy (GTK_WINDOW (bsp->brush_popup),
			     FALSE, FALSE, TRUE);
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (bsp->brush_popup), frame);
      gtk_widget_show (frame);
      bsp->brush_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_container_add (GTK_CONTAINER (frame), bsp->brush_preview);
      gtk_widget_show (bsp->brush_preview);
    }
  else
    {
      gtk_widget_hide (bsp->brush_popup);
    }

  /* decide where to put the popup */
  width = brush->mask->width;
  height = brush->mask->height;
  if (GIMP_IS_REALLY_A_BRUSH_PIPE (brush))
    {      
      GimpBrushPipe *pipe = GIMP_BRUSH_PIPE (brush);
      GimpBrush *tmp_brush;
      gint i;
      
      for (i = 1; i < pipe->nbrushes; i++)
	{
	  tmp_brush = GIMP_BRUSH (pipe->brushes[i]);
	  width = MAX (width, tmp_brush->mask->width);
	  height = MAX (height, tmp_brush->mask->height);
	}
    }
  gdk_window_get_origin (bsp->preview->window, &x_org, &y_org);
  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();
  x = x_org + args->x - width * 0.5;
  y = y_org + args->y - height * 0.5;
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + width > scr_w) ? scr_w - width : x;
  y = (y + height > scr_h) ? scr_h - height : y;
  gtk_preview_size (GTK_PREVIEW (bsp->brush_preview), width, height);

  gtk_widget_popup (bsp->brush_popup, x, y);

  /*  Draw the brush preview  */
  draw_brush_popup (GTK_PREVIEW (bsp->brush_preview), brush, width, height);
  gtk_widget_queue_draw (bsp->brush_preview);

  if (GIMP_IS_REALLY_A_BRUSH_PIPE (brush) && bsp->popup_anim_timeout_tag == 0)
    {
      static popup_timeout_args_t timeout_args;
      
      timeout_args.bsp = bsp;
      timeout_args.x = width;
      timeout_args.y = height;
      timeout_args.brush = brush;

      bsp->popup_pipe_index = 0;
      bsp->popup_anim_timeout_tag =
	gtk_timeout_add (300, brush_popup_anim_timeout, &timeout_args);
    }

  return FALSE;  /* don't repeat */
}

static void
brush_popup_open (BrushSelect *bsp,
		  gint         x,
		  gint         y,
		  GimpBrush   *brush)
{
  static popup_timeout_args_t popup_timeout_args;

  /* if we've already got a timeout scheduled, then we complain */
  g_return_if_fail (bsp->popup_timeout_tag == 0);

  popup_timeout_args.bsp = bsp;
  popup_timeout_args.x = x;
  popup_timeout_args.y = y;
  popup_timeout_args.brush = brush;
  bsp->popup_timeout_tag = gtk_timeout_add (POPUP_DELAY_MS,
					    brush_popup_timeout,
					    &popup_timeout_args);
}

static void
brush_popup_close (BrushSelect *bsp)
{
  if (bsp->popup_timeout_tag != 0)
    gtk_timeout_remove (bsp->popup_timeout_tag);
  bsp->popup_timeout_tag = 0;

  if (bsp->popup_anim_timeout_tag != 0)
    gtk_timeout_remove (bsp->popup_anim_timeout_tag);
  bsp->popup_anim_timeout_tag = 0;

  if (bsp->brush_popup != NULL)
    gtk_widget_hide (bsp->brush_popup);
}

static void
display_setup (BrushSelect *bsp)
{
  guchar * buf;
  gint i;

  buf = g_new (guchar, 3 * bsp->preview->allocation.width);

  /*  Set the buffer to white  */
  memset (buf, 255, bsp->preview->allocation.width * 3);

  /*  Set the image buffer to white  */
  for (i = 0; i < bsp->preview->allocation.height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf, 0, i,
			  bsp->preview->allocation.width);

  g_free (buf);
}

static void
display_brush (BrushSelect *bsp,
	       GimpBrush   *brush,
	       gint         col,
	       gint         row)
{
  TempBuf *mask_buf, *pixmap_buf = NULL;
  guchar *mask, *buf, *b;
  guchar bg;
  gboolean scale = FALSE;
  gint cell_width, cell_height;
  gint width, height;
  gint offset_x, offset_y;
  gint yend;
  gint ystart;
  gint i, j;

  cell_width  = bsp->cell_width  - 2 * MARGIN_WIDTH;
  cell_height = bsp->cell_height - 2 * MARGIN_HEIGHT;

  mask_buf = brush->mask;
  if (GIMP_IS_BRUSH_PIXMAP (brush))
    pixmap_buf = GIMP_BRUSH_PIXMAP (brush)->pixmap_mask;

  if (mask_buf->width > cell_width || mask_buf->height > cell_height)
    {
      gdouble ratio_x = (gdouble)mask_buf->width / cell_width;
      gdouble ratio_y = (gdouble)mask_buf->height / cell_height;
   
      mask_buf = brush_scale_mask (mask_buf, 
				   (gdouble)(mask_buf->width) / MAX (ratio_x, ratio_y) + 0.5, 
				   (gdouble)(mask_buf->height) / MAX (ratio_x, ratio_y) + 0.5);
      if (GIMP_IS_BRUSH_PIXMAP (brush))
	{
	  /*  TODO: the scale function should scale the pixmap 
	            and the mask in one run                     */
 	  pixmap_buf = brush_scale_pixmap (pixmap_buf,
					   mask_buf->width, mask_buf->height);
	}
      scale = TRUE;
    }

  /*  calculate the offset into the image  */
  width = (mask_buf->width > cell_width) ? cell_width : mask_buf->width;
  height = (mask_buf->height > cell_height) ? cell_height : mask_buf->height;

  offset_x = col * bsp->cell_width + ((cell_width - width) >> 1) + MARGIN_WIDTH;
  offset_y = row * bsp->cell_height + ((cell_height - height) >> 1) - bsp->scroll_offset + MARGIN_HEIGHT;

  ystart = BOUNDS (offset_y, 0, bsp->preview->allocation.height);
  yend   = BOUNDS (offset_y + height, 0, bsp->preview->allocation.height);

  mask = temp_buf_data (mask_buf) + (ystart - offset_y) * mask_buf->width;
  buf = g_new (guchar, 3 * cell_width);

  if (GIMP_IS_BRUSH_PIXMAP (brush)) 
    { 
      guchar *pixmap = temp_buf_data (pixmap_buf) + (ystart - offset_y) * mask_buf->width * 3;
      for (i = ystart; i < yend; i++)
	{
	  b = buf;
	  for (j = 0; j < width ; j++)
	    {
	      bg = (255 - *mask);
	      *b++ = bg + (*mask * *pixmap++) / 255;
	      *b++ = bg + (*mask * *pixmap++) / 255; 
	      *b++ = bg + (*mask * *pixmap++) / 255;
	      mask++;
	    }
	  gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf,
				offset_x, i, width);
	}
    }
  else 
    {
      for (i = ystart; i < yend; i++)
	{
	  /*  Invert the mask for display.  We're doing this because
	   *  a value of 255 in the  mask means it is full intensity.
	   *  However, it makes more sense for full intensity to show
	   *  up as black in this brush preview window...
	   */
	  b = buf;
	  for (j = 0; j < width; j++) 
	    {
	      bg = 255 - *mask++;
	      memset (b, bg, 3);
	      b += 3;
	    }
	  gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf,
				offset_x, i, width);
	}
    }
  g_free (buf);

  offset_x = (col + 1) * bsp->cell_width  - indicator_width - MARGIN_WIDTH;
  offset_y = (row + 1) * bsp->cell_height - indicator_height - bsp->scroll_offset - MARGIN_HEIGHT;

  if (scale)
    {
      temp_buf_free (mask_buf);
      if (GIMP_IS_BRUSH_PIXMAP (brush))
	temp_buf_free (pixmap_buf);
      for (i = 0; i < indicator_height; i++, offset_y++)
	{ 
	  if (offset_y > 0 && offset_y < bsp->preview->allocation.height)
	    (GIMP_IS_REALLY_A_BRUSH_PIPE (brush)) ?
	      gtk_preview_draw_row (GTK_PREVIEW (bsp->preview),
				    scale_pipe_indicator_bits[i][0],
				    offset_x, offset_y, indicator_width) :
	      gtk_preview_draw_row (GTK_PREVIEW (bsp->preview),
				    scale_indicator_bits[i][0],
				    offset_x, offset_y, indicator_width);
	}
    }
  else if (GIMP_IS_REALLY_A_BRUSH_PIPE (brush))
    {
      for (i = 0; i < indicator_height; i++, offset_y++)
	{
	  if (offset_y > 0 && offset_y < bsp->preview->allocation.height)
	    gtk_preview_draw_row (GTK_PREVIEW (bsp->preview),
				  pipe_indicator_bits[i][0],
				  offset_x, offset_y, indicator_width);
	}
    }
}

static gint brush_counter = 0;

static void
do_display_brush (GimpBrush   *brush,
		  BrushSelect *bsp)
{
  display_brush (bsp, brush, brush_counter % (bsp->NUM_BRUSH_COLUMNS),
		 brush_counter / (bsp->NUM_BRUSH_COLUMNS));
  brush_counter++;
}

static void
display_brushes (BrushSelect *bsp)
{
  if (brush_list == NULL || gimp_brush_list_length (brush_list) == 0)
    {
      gtk_widget_set_sensitive (bsp->options_box, FALSE);
      return;
    }
  else
    {
      gtk_widget_set_sensitive (bsp->options_box, TRUE);
    }

  /*  setup the display area  */
  display_setup (bsp);

  brush_counter = 0;
  gimp_list_foreach (GIMP_LIST (brush_list), (GFunc) do_display_brush, bsp);
}

static void
brush_select_show_selected (BrushSelect *bsp,
			    gint         row,
			    gint         col)
{
  GdkRectangle area;
  guchar *buf;
  gint yend;
  gint ystart;
  gint offset_x, offset_y;
  gint i;

  buf = g_new (guchar, 3 * bsp->cell_width);

  if (bsp->old_col != col || bsp->old_row != row)
    {
      /*  remove the old selection  */
      offset_x = bsp->old_col * bsp->cell_width;
      offset_y = bsp->old_row * bsp->cell_height - bsp->scroll_offset;

      ystart = BOUNDS (offset_y , 0, bsp->preview->allocation.height);
      yend = BOUNDS (offset_y + bsp->cell_height, 0, bsp->preview->allocation.height);

      /*  set the buf to white  */
      memset (buf, 255, 3 * bsp->cell_width);

      for (i = ystart; i < yend; i++)
	{
	  if (i == offset_y || i == (offset_y + bsp->cell_height - 1))
	    gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf,
				  offset_x, i, bsp->cell_width);
	  else
	    {
	      gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf,
				    offset_x, i, 1);
	      gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf,
				    offset_x + bsp->cell_width - 1, i, 1);
	    }
	}

      if (bsp->redraw)
	{
	  area.x = offset_x;
	  area.y = ystart;
	  area.width = bsp->cell_width;
	  area.height = (yend - ystart);
	  gtk_widget_draw (bsp->preview, &area);
	}
    }

  /*  make the new selection  */
  offset_x = col * bsp->cell_width;
  offset_y = row * bsp->cell_height - bsp->scroll_offset;

  ystart = BOUNDS (offset_y , 0, bsp->preview->allocation.height);
  yend = BOUNDS (offset_y + bsp->cell_height, 0, bsp->preview->allocation.height);

  /*  set the buf to black  */
  memset (buf, 0, bsp->cell_width * 3);

  for (i = ystart; i < yend; i++)
    {
      if (i == offset_y || i == (offset_y + bsp->cell_height - 1))
	gtk_preview_draw_row (GTK_PREVIEW (bsp->preview),
			      buf, offset_x, i, bsp->cell_width);
      else
	{
	  gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf,
				offset_x, i, 1);
	  gtk_preview_draw_row (GTK_PREVIEW (bsp->preview), buf,
				offset_x + bsp->cell_width - 1, i, 1);
	}
    }

  if (bsp->redraw)
    {
      area.x = offset_x;
      area.y = ystart;
      area.width = bsp->cell_width;
      area.height = (yend - ystart);
      gtk_widget_draw (bsp->preview, &area);
    }

  bsp->old_row = row;
  bsp->old_col = col;

  g_free (buf);
}

static void
update_active_brush_field (BrushSelect *bsp)
{
  GimpBrush *brush;
  gchar buf[32];

  brush = gimp_context_get_brush (bsp->context);

  if (!brush)
    return;

  /*  Set brush name  */
  gtk_label_set_text (GTK_LABEL (bsp->brush_name), brush->name);

  /*  Set brush size  */
  g_snprintf (buf, sizeof (buf), _("(%d X %d)"),
	      brush->mask->width, brush->mask->height);
  gtk_label_set_text (GTK_LABEL (bsp->brush_size), buf);

  /*  Set brush spacing  */
  gtk_adjustment_set_value (GTK_ADJUSTMENT (bsp->spacing_data),
			    gimp_brush_get_spacing (brush));
}

static void
preview_calc_scrollbar (BrushSelect *bsp)
{
  gint num_rows;
  gint page_size;
  gint max;

  bsp->scroll_offset = 0;
  num_rows = ((gimp_brush_list_length (brush_list) +
	       (bsp->NUM_BRUSH_COLUMNS) - 1)
	      / (bsp->NUM_BRUSH_COLUMNS));
  max = num_rows * bsp->cell_width;
  if (!num_rows)
    num_rows = 1;
  page_size = bsp->preview->allocation.height;

  bsp->sbar_data->value          = bsp->scroll_offset;
  bsp->sbar_data->upper          = max;
  bsp->sbar_data->page_size      = ((page_size < max) ? page_size : max);
  bsp->sbar_data->page_increment = (page_size >> 1);
  bsp->sbar_data->step_increment = bsp->cell_width;

  gtk_signal_emit_by_name (GTK_OBJECT (bsp->sbar_data), "value_changed");
}

static gint 
brush_select_resize (GtkWidget   *widget, 
		     GdkEvent    *event, 
		     BrushSelect *bsp)
{
  /*  calculate the best-fit approximation...  */  
  gint wid;
  gint now;
  gint cell_size;

  wid = widget->allocation.width;

  for (now = cell_size = MIN_CELL_SIZE;
       now < MAX_CELL_SIZE; ++now)
    {
      if ((wid % now) < (wid % cell_size)) cell_size = now;
      if ((wid % cell_size) == 0)
        break;
    }

   bsp->NUM_BRUSH_COLUMNS =
     (gint) (wid / cell_size);
   bsp->NUM_BRUSH_ROWS =
     (gint) ((gimp_brush_list_length (brush_list) + bsp->NUM_BRUSH_COLUMNS - 1) /
	     bsp->NUM_BRUSH_COLUMNS);

   bsp->cell_width  = cell_size;
   bsp->cell_height = cell_size;

   /*  recalculate scrollbar extents  */
   preview_calc_scrollbar (bsp);
 
   return FALSE;
}
 
static gint
brush_select_events (GtkWidget   *widget,
		     GdkEvent    *event,
		     BrushSelect *bsp)
{
  GdkEventButton *bevent;
  GimpBrush *brush;
  gint row, col, index;

  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_2BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      col = bevent->x / bsp->cell_width;
      row = (bevent->y + bsp->scroll_offset) / bsp->cell_height;
      index = row * bsp->NUM_BRUSH_COLUMNS + col;
      
      /*  Get the brush and check if it is editable  */
      brush = gimp_brush_list_get_brush_by_index (brush_list, index);
      if (GIMP_IS_BRUSH_GENERATED (brush))
	brush_select_edit_brush_callback (NULL, bsp);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	  col = bevent->x / bsp->cell_width;
	  row = (bevent->y + bsp->scroll_offset) / bsp->cell_height;
	  index = row * bsp->NUM_BRUSH_COLUMNS + col;

	  /*  Get the brush and display the popup brush preview  */
	  if ((brush = gimp_brush_list_get_brush_by_index (brush_list, index)))
	    {
	      gdk_pointer_grab (bsp->preview->window, FALSE,
				(GDK_POINTER_MOTION_HINT_MASK |
				 GDK_BUTTON1_MOTION_MASK |
				 GDK_BUTTON_RELEASE_MASK),
				NULL, NULL, bevent->time);

	      /*  Make this brush the active brush  */
	      gimp_context_set_brush (bsp->context, brush);

	      /*  only if dialog is main one        */
	      if (bsp == brush_select_dialog &&
		  brush_edit_generated_dialog)
		{
		  brush_edit_generated_set_brush (brush_edit_generated_dialog,
						  brush);
		}
	      
	      /*  Show the brush popup window if the brush is too large  */
	      if (brush->mask->width  > bsp->cell_width ||
		  brush->mask->height > bsp->cell_height ||
		  GIMP_IS_REALLY_A_BRUSH_PIPE (brush))
		{
		  brush_popup_open (bsp, bevent->x, bevent->y, brush);
		}
	    }
	}

      /*  wheelmouse support  */
      else if (bevent->button == 4)
	{
	  GtkAdjustment *adj = bsp->sbar_data;
	  gfloat new_value = adj->value - adj->page_increment / 2;
	  new_value =
	    CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	  gtk_adjustment_set_value (adj, new_value);
	}
      else if (bevent->button == 5)
	{
	  GtkAdjustment *adj = bsp->sbar_data;
	  gfloat new_value = adj->value + adj->page_increment / 2;
	  new_value =
	    CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	  gtk_adjustment_set_value (adj, new_value);
	}

      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	  /*  Ungrab the pointer  */
	  gdk_pointer_ungrab (bevent->time);

	  /*  Close the brush popup window  */
	  brush_popup_close (bsp);

	  /* Call any callbacks registered */
	  brush_change_callbacks (bsp, 0);
	}
      break;

    case GDK_DELETE:
      /* g_warning ("test"); */
      break;

    default:
      break;
    }

  return FALSE;
}

/*  Disabled until I've figured out how gtk window resizing *really* works.
 *  I don't think that the function below is the correct way to do it
 *    --  Michael
 *
static void
brush_select_map_callback (GtkWidget   *widget,
			   BrushSelect *bsp)
{
  GtkAllocation allocation;
  gint xdiff, ydiff;

  xdiff =
    bsp->shell->allocation.width - bsp->preview->allocation.width;
  ydiff =
    bsp->shell->allocation.height - bsp->preview->allocation.height;

  allocation = bsp->shell->allocation;
  allocation.width = brush_select_session_info.width + xdiff;
  allocation.height = brush_select_session_info.height + ydiff;

  gtk_widget_size_allocate (bsp->shell, &allocation);
}
*/

static void
brush_select_scroll_update (GtkAdjustment *adjustment,
			    gpointer       data)
{
  BrushSelect *bsp;
  GimpBrush *active;
  gint row, col, index;

  bsp = (BrushSelect *) data;

  if (bsp)
    {
      bsp->scroll_offset = adjustment->value;

      display_brushes (bsp);

      active = gimp_context_get_brush (bsp->context);

      if (active)
	{
	  index = gimp_brush_list_get_brush_index (brush_list, active);
	  if (index < 0)
	    return;
	  row = index / bsp->NUM_BRUSH_COLUMNS;
	  col = index - row * bsp->NUM_BRUSH_COLUMNS;

	  brush_select_show_selected (bsp, row, col);
	}

      if (bsp->redraw)
	gtk_widget_draw (bsp->preview, NULL);
    }
}

static void
opacity_scale_update (GtkAdjustment *adjustment,
		      gpointer       data)
{
  BrushSelect *bsp;

  bsp = (BrushSelect *) data;

  gimp_context_set_opacity (bsp->context, adjustment->value / 100.0);

  if (bsp != brush_select_dialog)
    {
      brush_change_callbacks (bsp, 0); 
    }
}

static void
paint_mode_menu_callback (GtkWidget *widget,
			  gpointer   data)
{
  BrushSelect *bsp;
  
  bsp = (BrushSelect *) gtk_object_get_user_data (GTK_OBJECT (widget));

  gimp_context_set_paint_mode (bsp->context, (LayerModeEffects) data);

  if (bsp != brush_select_dialog)
    {
      brush_change_callbacks (bsp, 0);
    }
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
	  brush_change_callbacks (bsp, 0);
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
      brush_change_callbacks (bsp, 1);
      gtk_widget_destroy (bsp->shell); 
      brush_select_free (bsp); 
    }
}

static void
brush_select_refresh_callback (GtkWidget *widget,
			       gpointer   data)
{
  brush_select_freeze_all ();

  /*  re-init the brush list  */
  brushes_init (FALSE);

  brush_select_thaw_all ();
}

static void
brush_select_new_brush_callback (GtkWidget *widget,
				 gpointer   data)
{
  GimpBrushGenerated *brush;
  BrushSelect *bsp;

  bsp = (BrushSelect *) data;

  brush = gimp_brush_generated_new (10, .5, 0.0, 1.0);
  gimp_brush_list_add (brush_list, GIMP_BRUSH (brush));

  gimp_context_set_brush (bsp->context, GIMP_BRUSH (brush));

  if (brush_edit_generated_dialog)
    brush_edit_generated_set_brush (brush_edit_generated_dialog,
				    GIMP_BRUSH (brush));

  brush_select_edit_brush_callback (widget, data);
}

static void
brush_select_edit_brush_callback (GtkWidget *widget,
				  gpointer   data)
{
  BrushSelect *bsp;
  GimpBrush *brush;

  bsp = (BrushSelect *) data;
  brush = gimp_context_get_brush (bsp->context);

  if (GIMP_IS_BRUSH_GENERATED (brush))
    {
      if (!brush_edit_generated_dialog)
	{
	  /*  Create the dialog...  */
	  brush_edit_generated_dialog = brush_edit_generated_new ();
	  brush_edit_generated_set_brush (brush_edit_generated_dialog, brush);
	}
      else
	{
	  /*  Popup the dialog  */
	  if (!GTK_WIDGET_VISIBLE (brush_edit_generated_dialog->shell))
	    gtk_widget_show (brush_edit_generated_dialog->shell);
	  else
	    gdk_window_raise (brush_edit_generated_dialog->shell->window);
	}
    }
  else
    g_message (_("We are all fresh out of brush editors today,\n"
		 "please write your own or try back tomorrow\n"));
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
      gint index;

      gimp_brush_generated_delete (GIMP_BRUSH_GENERATED (brush));

      brush_select_freeze_all ();

      index = gimp_brush_list_get_brush_index (brush_list, brush);
      gimp_brush_list_remove (brush_list, GIMP_BRUSH (brush));
      gimp_context_refresh_brushes ();

      brush_select_thaw_all ();
    }
  else
    g_message (_("Wilber says: \"I don\'t know how to delete that brush.\""));
}
