/* gap_mov_dialog.c
 *   by hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * Dialog Window for Move Path (gap_mov)
 *
 */

/* code was mainly inspired by SuperNova plug-in
 * Copyright (C) 1997 Eiichi Takamori <taka@ma1.seikyou.ne.jp>,
 *		      Spencer Kimball, Federico Mena Quintero
 */
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

/* revision history:
 * gimp    1.1.17b; 2000/02/23  hof: bugfix: dont flatten the preview, just merge visible layers
 *                                   bugfix: for current frame never use diskfile for the preview
 *                                           (to avoid inconsitencies, and to speed up a little)
 *                                   added "Show Path", pick and drag Controlpoints
 * gimp    1.1.17a; 2000/02/20  hof: use gimp_help_set_help_data for tooltips
 *                                   added spinbuttons, and more layout cosmetics.
 * gimp    1.1.15a; 2000/01/26  hof: removed gimp 1.0.x support
 * gimp    1.1.13b; 1999/12/04  hof: some cosmetic gtk fixes
 *                                   changed border_width spacing and Buttons in action area
 *                                   to same style as used in dialogs of the gimp 1.1.13 main dialogs
 * gimp   1.1.8a;   1999/08/31  hof: accept anim framenames without underscore '_'
 * gimp   1.1.5a;   1999/05/08  hof: call fileselect in gtk+1.2 style 
 * version 0.99.00; 1999.03.03  hof: bugfix: update of the preview (did'nt work with gimp1.1.2)
 * version 0.98.00; 1998.11.28  hof: Port to GIMP 1.1: replaced buildmenu.h, apply layermask (before rotate)
 *                                   mov_imglayer_constrain must check for drawable_id -1
 * version 0.97.00; 1998.10.19  hof: Set window title to "Move Path"
 * version 0.96.02; 1998.07.30  hof: added clip to frame option and tooltips
 * version 0.96.00; 1998.07.09  hof: bugfix (filesel did not reopen after cancel)
 * version 0.95.00; 1998.05.12  hof: added rotatation capabilities
 * version 0.94.00; 1998.04.25  hof: use only one point as default
 *                                   bugfix: initial value for src_paintmode
 *                                           fixes the problem reported in p_my_layer_copy (cant get new layer)
 * version 0.90.00; 1997.12.14  hof: 1.st (pre) release
 */

/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "config.h"
#include "libgimp/stdplugins-intl.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

/* GAP includes */
#include "gap_layer_copy.h"
#include "gap_lib.h"
#include "gap_mov_exec.h"
#include "gap_mov_dialog.h"
#include "gap_pdb_calls.h"
 
 
/* Some useful macros */

extern      int gap_debug; /* ==0  ... dont print debug infos */

#define ENTRY_WIDTH 60
#define SCALE_WIDTH 125
#define PREVIEW_SIZE 256
#define RADIUS           3

#define PREVIEW	      0x1
#define CURSOR        0x2
#define PATH_LINE     0x4
#define ALL	      0xf

/*  event masks for the preview widget */
#define PREVIEW_MASK   GDK_EXPOSURE_MASK | \
		       GDK_BUTTON_PRESS_MASK |\
		       GDK_BUTTON_RELEASE_MASK |\
		       GDK_BUTTON_MOTION_MASK


#define LABEL_LENGTH 256

typedef struct {
  gint run;
} t_mov_interface;

typedef struct {
  GtkObject	*adjustment;
  GtkWidget	*entry;
  gint		constraint;
} t_mov_EntryScaleData;

typedef struct
{
  GDrawable	*drawable;
  gint		dwidth, dheight;
  gint		bpp;
  GtkWidget	*preview;
  GPixelRgn	 src_rgn;
  gint           PixelRgnIsInitialized;
  gint           show_path;
  gint           startup;

  gint		pwidth, pheight;
  gint		cursor;
  gint		curx, cury;		 /* x,y of cursor in preview */
  gint		oldx, oldy;

  GtkWidget     *filesel;
  GtkAdjustment *x_adj;
  GtkAdjustment *y_adj;
  GtkAdjustment *wres_adj;
  GtkAdjustment *hres_adj;
  GtkAdjustment *opacity_adj;
  GtkAdjustment *rotation_adj;
  gchar          PointIndex_Label[LABEL_LENGTH];
  GtkWidget     *PointIndex_LabelPtr;
  gint           p_x, p_y;
  gint           opacity;
  gint           w_resize;
  gint           h_resize;
  gint           rotation;

  gint           preview_frame_nr;      /* default: current frame */
  gint           old_preview_frame_nr;
  t_anim_info   *ainfo_ptr;
 	
  gint          in_call;
  char         *pointfile_name;
} t_mov_path_preview;

typedef struct {
  t_mov_path_preview *path_ptr;
  GtkWidget          *dlg;
} t_ok_data;


/* p_buildmenu Structures */

typedef struct _MenuItem   MenuItem;

typedef void (*MenuItemCallback) (GtkWidget *widget,
				  gpointer   user_data);
struct _MenuItem
{
  char *label;
  char  unused_accelerator_key;
  gint  unused_accelerator_mods;
  MenuItemCallback callback;
  gpointer user_data;
  MenuItem *unused_subitems;
  GtkWidget *widget;
};

/* Declare a local function.
 */
GtkWidget       *  p_buildmenu (MenuItem *);

       long        p_move_dialog            (t_mov_data *mov_ptr);
static void        p_update_point_labels    (t_mov_path_preview *path_ptr);
static void        p_points_from_tab        (t_mov_path_preview *path_ptr);
static void        p_points_to_tab          (t_mov_path_preview *path_ptr);
static void        p_point_refresh          (t_mov_path_preview *path_ptr);
static void        p_pick_nearest_point     (gint px, gint py);
static void        p_reset_points           ();
static void        p_clear_point            ();
static void        p_load_points            (char *filename);
static void        p_save_points            (char *filename);

static GDrawable * p_get_flattened_drawable (gint32 image_id);
static GDrawable * p_get_prevw_drawable (t_mov_path_preview *path_ptr);

static gint	   mov_dialog ( GDrawable *drawable, t_mov_path_preview *path_ptr,
                                gint min, gint max);
static GtkWidget * mov_path_prevw_create ( GDrawable *drawable,
                                           t_mov_path_preview *path_ptr);
static GtkWidget * mov_src_sel_create ();

static void	   mov_path_prevw_destroy  ( GtkWidget *widget, gpointer data );
static void	   mov_path_prevw_preview_init ( t_mov_path_preview *path_ptr );
static void	   mov_path_prevw_draw ( t_mov_path_preview *path_ptr, gint update );
static void	   mov_path_x_adjustment_update ( GtkWidget *widget, gpointer data );
static void	   mov_path_y_adjustment_update ( GtkWidget *widget, gpointer data );
static void	   mov_path_prevw_cursor_update ( t_mov_path_preview *path_ptr );
static gint	   mov_path_prevw_preview_expose ( GtkWidget *widget, GdkEvent *event );
static gint	   mov_path_prevw_preview_events ( GtkWidget *widget, GdkEvent *event );

static void	mov_padd_callback        (GtkWidget *widget,gpointer data);
static void	mov_pins_callback        (GtkWidget *widget,gpointer data);
static void	mov_pdel_callback        (GtkWidget *widget,gpointer data);
static void     mov_pnext_callback       (GtkWidget *widget,gpointer data);
static void     mov_pprev_callback       (GtkWidget *widget,gpointer data);
static void     mov_pfirst_callback      (GtkWidget *widget,gpointer data);
static void     mov_plast_callback       (GtkWidget *widget,gpointer data);
static void	mov_pres_callback        (GtkWidget *widget,gpointer data);
static void	mov_pclr_callback        (GtkWidget *widget,gpointer data);
static void     mov_pload_callback       (GtkWidget *widget,gpointer data);
static void     mov_psave_callback       (GtkWidget *widget,gpointer data);
static void     p_points_load_from_file  (GtkWidget *widget,gpointer data);
static void     p_points_save_to_file    (GtkWidget *widget,gpointer data);

static void	mov_close_callback	 (GtkWidget *widget,gpointer data);
static void	mov_ok_callback	         (GtkWidget *widget,gpointer data);
static void     mov_upvw_callback        (GtkWidget *widget,gpointer data);
static void	p_filesel_close_cb       (GtkWidget *widget, t_mov_path_preview *path_ptr);

static gint mov_imglayer_constrain      (gint32 image_id, gint32 drawable_id, gpointer data);
static void mov_imglayer_menu_callback  (gint32 id, gpointer data);
static void mov_paintmode_menu_callback (GtkWidget *, gpointer);
static void mov_handmode_menu_callback  (GtkWidget *, gpointer);
static void mov_stepmode_menu_callback  (GtkWidget *, gpointer);
static void mov_gint_toggle_callback    (GtkWidget *, gpointer);
static void mov_show_path_callback      (GtkWidget *, gpointer);


/*  the option menu items -- the paint modes  */
static MenuItem option_paint_items[] =
{
  { N_("Normal"), 0, 0, mov_paintmode_menu_callback, (gpointer) NORMAL_MODE, NULL, NULL },
  { N_("Dissolve"), 0, 0, mov_paintmode_menu_callback, (gpointer) DISSOLVE_MODE, NULL, NULL },
  { N_("Multiply"), 0, 0, mov_paintmode_menu_callback, (gpointer) MULTIPLY_MODE, NULL, NULL },
  { N_("Screen"), 0, 0, mov_paintmode_menu_callback, (gpointer) SCREEN_MODE, NULL, NULL },
  { N_("Overlay"), 0, 0, mov_paintmode_menu_callback, (gpointer) OVERLAY_MODE, NULL, NULL },
  { N_("Difference"), 0, 0, mov_paintmode_menu_callback, (gpointer) DIFFERENCE_MODE, NULL, NULL },
  { N_("Addition"), 0, 0, mov_paintmode_menu_callback, (gpointer) ADDITION_MODE, NULL, NULL },
  { N_("Subtract"), 0, 0, mov_paintmode_menu_callback, (gpointer) SUBTRACT_MODE, NULL, NULL },
  { N_("Darken Only"), 0, 0, mov_paintmode_menu_callback, (gpointer) DARKEN_ONLY_MODE, NULL, NULL },
  { N_("Lighten Only"), 0, 0, mov_paintmode_menu_callback, (gpointer) LIGHTEN_ONLY_MODE, NULL, NULL },
  { N_("Hue"), 0, 0, mov_paintmode_menu_callback, (gpointer) HUE_MODE, NULL, NULL },
  { N_("Saturation"), 0, 0, mov_paintmode_menu_callback, (gpointer) SATURATION_MODE, NULL, NULL },
  { N_("Color"), 0, 0, mov_paintmode_menu_callback, (gpointer) COLOR_MODE, NULL, NULL },
  { N_("Value"), 0, 0, mov_paintmode_menu_callback, (gpointer) VALUE_MODE, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

/*  the option menu items -- the handle modes  */
static MenuItem option_handle_items[] =
{
  { N_("Left  Top"),    0, 0, mov_handmode_menu_callback, (gpointer) GAP_HANDLE_LEFT_TOP, NULL, NULL },
  { N_("Left  Bottom"), 0, 0, mov_handmode_menu_callback, (gpointer) GAP_HANDLE_LEFT_BOT, NULL, NULL },
  { N_("Right Top"),    0, 0, mov_handmode_menu_callback, (gpointer) GAP_HANDLE_RIGHT_TOP, NULL, NULL },
  { N_("Right Bottom"), 0, 0, mov_handmode_menu_callback, (gpointer) GAP_HANDLE_RIGHT_BOT, NULL, NULL },
  { N_("Center"),       0, 0, mov_handmode_menu_callback, (gpointer) GAP_HANDLE_CENTER, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};


/*  the option menu items -- the loop step modes  */
static MenuItem option_step_items[] =
{
  { N_("Loop"),         0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_LOOP, NULL, NULL },
  { N_("Loop Reverse"), 0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_LOOP_REV, NULL, NULL },
  { N_("Once"),         0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_ONCE, NULL, NULL },
  { N_("OnceReverse"),  0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_ONCE_REV, NULL, NULL },
  { N_("PingPong"),     0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_PING_PONG, NULL, NULL },
  { N_("None"),         0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_NONE, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};



static t_mov_values *pvals;

static t_mov_interface mov_int =
{
  FALSE	    /* run */
};


/* ============================================================================
 **********************
 *                    *
 *  Dialog interface  *
 *                    *
 **********************
 * ============================================================================
 */

long      p_move_dialog    (t_mov_data *mov_ptr)
{
  GDrawable *l_drawable_ptr;
  gint       l_first, l_last;  
  char      *l_str;
  t_mov_path_preview *path_ptr;


  if(gap_debug) printf("GAP-DEBUG: START p_move_dialog\n");

  path_ptr = g_new( t_mov_path_preview, 1 );
  if(path_ptr == NULL)
  {
    printf("error cant alloc path_preview structure\n");
    return -1;
  }
  path_ptr->show_path = TRUE;
  path_ptr->startup = TRUE;
  path_ptr->PixelRgnIsInitialized = FALSE;
  
  pvals = mov_ptr->val_ptr;

  l_str = p_strdup_del_underscore(mov_ptr->dst_ainfo_ptr->basename);
  path_ptr->pointfile_name  = g_strdup_printf("%s.path_points", l_str);
  g_free(l_str);

  
  l_first = mov_ptr->dst_ainfo_ptr->first_frame_nr;
  l_last  = mov_ptr->dst_ainfo_ptr->last_frame_nr;

  /* init parameter values */ 
  pvals->dst_image_id = mov_ptr->dst_ainfo_ptr->image_id;
  pvals->tmp_image_id = -1;
  pvals->src_image_id = -1;
  pvals->src_layer_id = -1;
  pvals->src_paintmode = NORMAL_MODE;
  pvals->src_handle = GAP_HANDLE_LEFT_TOP;
  pvals->src_stepmode = GAP_STEP_LOOP;
  pvals->src_only_visible  = 0;
  pvals->clip_to_img  = 0;

  p_reset_points();
  
  /* pvals->point[1].p_x = 100; */  /* default: move from 0/0 to 100/0 */

  pvals->dst_range_start = mov_ptr->dst_ainfo_ptr->curr_frame_nr;
  pvals->dst_range_end   = l_last;
  pvals->dst_layerstack = 0;   /* 0 ... insert layer on top of stack */

  path_ptr->filesel = NULL;   /* fileselector is not open */
  path_ptr->ainfo_ptr            = mov_ptr->dst_ainfo_ptr;
  path_ptr->preview_frame_nr     = mov_ptr->dst_ainfo_ptr->curr_frame_nr;
  path_ptr->old_preview_frame_nr = path_ptr->preview_frame_nr;
  path_ptr->PointIndex_LabelPtr = NULL;
  
  p_points_from_tab(path_ptr);
  p_update_point_labels(path_ptr);

  /* duplicate the curerent image (for flatten & preview)  */
  pvals->tmp_image_id = p_gimp_channel_ops_duplicate(pvals->dst_image_id);

  /* flatten image, and get the (only) resulting drawable */
  l_drawable_ptr = p_get_prevw_drawable(path_ptr);

  /* do DIALOG window */
  mov_dialog(l_drawable_ptr, path_ptr, l_first, l_last);
  p_points_to_tab(path_ptr);

  /* destroy the tmp image */
  gimp_image_delete(pvals->tmp_image_id);

/*  g_free (path_ptr); */ /* DONT free path_ptr it leads to sigsegv ERROR */


  if(gap_debug) printf("GAP-DEBUG: END p_move_dialog\n");

  if(mov_int.run == TRUE)  return 0;
  else                     return  -1;
}


/* ============================================================================
 *******************
 *                 *
 *   Main Dialog   *
 *                 *
 *******************
 * ============================================================================
 */

static gint
mov_dialog ( GDrawable *drawable, t_mov_path_preview *path_ptr,
             gint first_nr, gint last_nr )
{
  GtkWidget *vbox;
  GtkWidget *hbox_table;
  GtkWidget *hbbox;
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *path_prevw_frame;
  GtkWidget *src_sel_frame;
  GtkWidget *check_button;
  GtkObject *adj;
  guchar *color_cube;
  t_ok_data  ok_data;

  gchar **argv;
  gint	argc;

  if(gap_debug) printf("GAP-DEBUG: START mov_dialog\n");

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("gap_move");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());


  gdk_set_use_xshm (gimp_use_xshm ());
  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  /* dialog */
  dlg = gtk_dialog_new ();
  ok_data.dlg = dlg;
  ok_data.path_ptr = path_ptr;
  gtk_window_set_title (GTK_WINDOW (dlg), _("Move Path"));
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) mov_close_callback,
		      NULL);

  /* Initialize Tooltips */
  gimp_help_init ();

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);

  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
      
  button = gtk_button_new_with_label ( _("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mov_ok_callback,
		      &ok_data);
  gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( _("UpdPreview"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mov_upvw_callback,
		      path_ptr);
  gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
  gimp_help_set_help_data(button,
                       _("Show PreviewFrame with Selected       \nSrcLayer at current Controlpoint")
                       , NULL);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( _("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new ( _("Copy moving source-layer(s) into frames"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  /* the vbox */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* the source select frame */  
  src_sel_frame = mov_src_sel_create ();
  gtk_box_pack_start (GTK_BOX (vbox), src_sel_frame, TRUE, TRUE, 0);

  /* the path preview frame (with all the controlpoint widgets) */  
  path_prevw_frame = mov_path_prevw_create ( drawable, path_ptr);
  gtk_box_pack_start (GTK_BOX (vbox), path_prevw_frame, TRUE, TRUE, 0);

  /* the hbox_table (1 row) */
  hbox_table = gtk_table_new (5, 3, FALSE);
  gtk_widget_show (hbox_table);
  gtk_box_pack_start (GTK_BOX (vbox), hbox_table, TRUE, TRUE, 0);
  
  /* table with 5 rows */
  table = gtk_table_new (5, 3, FALSE);
  gtk_table_attach(GTK_TABLE(hbox_table), table, 0, 1, 0, 1, GTK_FILL|GTK_EXPAND, GTK_FILL, 4, 0);

  /* the start frame scale_entry */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 0, 0,        /* table col, row */
			  _("Start Frame:"),                  /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gfloat)pvals->dst_range_start,     /* value */
			  (gfloat)first_nr, (gfloat)last_nr,  /* lower, upper */
			  1, 10,                              /* step, page */
			  0,                                  /* digits */
			  TRUE,                               /* constrain */
			  (gfloat)first_nr, (gfloat)last_nr,  /* lower, upper (unconstrained) */
			  _("first handled frame"), NULL);    /* tooltip privatetip */
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &pvals->dst_range_start);

  /* the end frame scale_entry */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 0, 1,        /* table col, row */
			  _("End Frame:"),                    /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gfloat)pvals->dst_range_end,       /* value */
			  (gfloat)first_nr, (gfloat)last_nr,  /* lower, upper */
			  1, 10,                              /* step, page */
			  0,                                  /* digits */
			  TRUE,                               /* constrain */
			  (gfloat)first_nr, (gfloat)last_nr,  /* lower, upper (unconstrained) */
			  _("last handled frame"), NULL);     /* tooltip privatetip */
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &pvals->dst_range_end);

  /* the Preview Frame scale_entry */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 0, 2,        /* table col, row */
			  _("Preview Frame:"),                /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gfloat)path_ptr->preview_frame_nr, /* value */
			  (gfloat)first_nr, (gfloat)last_nr,  /* lower, upper */
			  1, 10,                              /* step, page */
			  0,                                  /* digits */
			  TRUE,                               /* constrain */
			  (gfloat)first_nr, (gfloat)last_nr,  /* lower, upper (unconstrained) */
			  _("frame to show when UpdPreview\nbutton is pressed"),
			  NULL);                              /* tooltip privatetip */
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &path_ptr->preview_frame_nr);
			  
  /* the Layerstack scale_entry */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 0, 3,        /* table col, row */
			  _("Layerstack:"),                   /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gfloat)pvals->dst_layerstack,      /* value */
			  0.0, 99.0,                          /* lower, upper */
			  1, 10,                              /* step, page */
			  0,                                  /* digits */
			  FALSE,                              /* constrain */
			  0.0, 999999.0,                      /* lower, upper (unconstrained) */
			  _("How to insert SrcLayer into the\nDst.Frame's Layerstack\n0 means on top i.e in front"),
			  NULL);                              /* tooltip privatetip */
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &pvals->dst_layerstack);

  /* toggle clip_to_image */
  check_button = gtk_check_button_new_with_label ( _("Clip To Frame"));
  gtk_signal_connect (GTK_OBJECT (check_button), "toggled",
                      (GtkSignalFunc) mov_gint_toggle_callback,
                       &pvals->clip_to_img);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (check_button), pvals->clip_to_img);
  gimp_help_set_help_data(check_button,
                       _("Clip all copied Src-Layers\nat Frame Boundaries")
                       , NULL);
  gtk_widget_show (check_button);
  gtk_table_attach(GTK_TABLE(hbox_table), check_button, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 4, 0);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  path_ptr->startup = FALSE;

  gtk_main ();
  gdk_flush ();

  if(gap_debug) printf("GAP-DEBUG: END mov_dialog\n");

  return mov_int.run;
}

/* ============================================================================
 * implementation of CALLBACK procedures
 * ============================================================================
 */

static void
mov_close_callback (GtkWidget *widget,
			 gpointer   data)
{
  gtk_main_quit ();
}

static void
mov_ok_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_ok_data   *ok_data_ptr;

  ok_data_ptr = data;


  if(pvals != NULL)
  {
    if(pvals->src_layer_id < 0)
    {

       p_msg_win(RUN_INTERACTIVE,
                 _("No Source Image was selected\n(Please open a 2nd Image of the same type before opening Move Path)\n"));
       return;
    }
  }

  mov_int.run = TRUE;
 
  if(pvals->point_idx_max == 0)
  {
    /* if we have only one point duplicate that point
     * (move algorithm needs at least 2 points)
     */
    mov_padd_callback(NULL, ok_data_ptr->path_ptr);
  }
  gtk_widget_destroy (ok_data_ptr->dlg);
}

static void
mov_upvw_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_path_preview *path_ptr;
  char               *l_filename;
  long                l_frame_nr;
  gint32              l_new_tmp_image_id;
  gint32              l_old_tmp_image_id;

  path_ptr = data;

  if(gap_debug) printf("mov_upvw_callback nr: %d old_nr: %d\n",
         (int)path_ptr->preview_frame_nr , (int)path_ptr->old_preview_frame_nr);

/*  if( path_ptr->preview_frame_nr != path_ptr->old_preview_frame_nr)
 * {
 */
     l_frame_nr = (long)path_ptr->preview_frame_nr;
     l_filename = p_alloc_fname(path_ptr->ainfo_ptr->basename,
                                l_frame_nr,
                                path_ptr->ainfo_ptr->extension);
     if(l_filename != NULL)
     {
        /* replace the temporary image */
        if(path_ptr->preview_frame_nr  == path_ptr->ainfo_ptr->curr_frame_nr)
        {
          l_new_tmp_image_id = p_gimp_channel_ops_duplicate(path_ptr->ainfo_ptr->image_id);
        }
        else
        {
           l_new_tmp_image_id = p_load_image(l_filename);
        }
        g_free(l_filename);
        if (l_new_tmp_image_id >= 0)
        {
           /* use the new loaded temporary image */
           l_old_tmp_image_id  = pvals->tmp_image_id;
           pvals->tmp_image_id = l_new_tmp_image_id;

           /* flatten image, and get the (only) resulting drawable */
           path_ptr->drawable = p_get_prevw_drawable(path_ptr);

           /* re initialize preview image */
           mov_path_prevw_preview_init(path_ptr);
           p_point_refresh(path_ptr);

           path_ptr->old_preview_frame_nr = path_ptr->preview_frame_nr;

           gtk_widget_draw(path_ptr->preview, NULL);
	   mov_path_prevw_draw ( path_ptr, CURSOR | PATH_LINE );
           gdk_flush();  

           /* destroy the old tmp image */
           gimp_image_delete(l_old_tmp_image_id);
        }
       
     }
/* } */
}

static void
p_copy_point(gint from_idx, gint to_idx)
{
    pvals->point[from_idx].p_x = pvals->point[to_idx].p_x;
    pvals->point[from_idx].p_y = pvals->point[to_idx].p_y;
    pvals->point[from_idx].opacity = pvals->point[to_idx].opacity;
    pvals->point[from_idx].w_resize = pvals->point[to_idx].w_resize;
    pvals->point[from_idx].h_resize = pvals->point[to_idx].h_resize;
    pvals->point[from_idx].rotation = pvals->point[to_idx].rotation;
}


static void
mov_padd_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_path_preview *path_ptr = data;
  gint l_idx;
  
  if(gap_debug) printf("mov_padd_callback\n");
  l_idx = pvals->point_idx_max;
  if (l_idx < GAP_MOV_MAX_POINT -2)
  {
    /* advance to next point */
    p_points_to_tab(path_ptr);
    pvals->point_idx_max++;
    pvals->point_idx = pvals->point_idx_max;
    
    /* copy values from previous point to current (new) point */
    p_copy_point(pvals->point_idx_max, l_idx);
    p_point_refresh(path_ptr);
  }
}

static void
mov_pins_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_path_preview *path_ptr = data;
  gint l_idx;
  
  if(gap_debug) printf("mov_pins_callback\n");
  l_idx = pvals->point_idx_max;
  if (l_idx < GAP_MOV_MAX_POINT -2)
  {
    /* advance to next point */
    p_points_to_tab(path_ptr);
    pvals->point_idx_max++;

    for(l_idx = pvals->point_idx_max; l_idx >  pvals->point_idx; l_idx--)
    {
      /* copy values from next point */
      p_copy_point(l_idx, l_idx-1);
    }
     
    pvals->point_idx++;
    p_point_refresh(path_ptr);
  }
}

static void
mov_pdel_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_path_preview *path_ptr = data;
  gint l_idx;
  
  if(gap_debug) printf("mov_pdel_callback\n");
  
  l_idx = pvals->point_idx_max;
  if(pvals->point_idx_max == 0)
  {
    /* This is the las t point to delete */
    p_reset_points();
  }
  else
  {
    for(l_idx = pvals->point_idx; l_idx <  pvals->point_idx_max; l_idx++)
    {
      /* copy values from next point */
      p_copy_point(l_idx, l_idx+1);
    }
    pvals->point_idx_max--;
    pvals->point_idx = MIN(pvals->point_idx, pvals->point_idx_max);
  }
  p_point_refresh(path_ptr);
}

static void
mov_pnext_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_path_preview *path_ptr = data;
  
  if(gap_debug) printf("mov_pnext_callback\n");
  if (pvals->point_idx < pvals->point_idx_max)
  {
    /* advance to next point */
    p_points_to_tab(path_ptr);
    pvals->point_idx++;
    p_point_refresh(path_ptr);
  }
}


static void
mov_pprev_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_path_preview *path_ptr = data;
  
  if(gap_debug) printf("mov_pprev_callback\n");
  if (pvals->point_idx > 0)
  {
    /* advance to next point */
    p_points_to_tab(path_ptr);
    pvals->point_idx--;
    p_point_refresh(path_ptr);
  }
}

static void
mov_pfirst_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_path_preview *path_ptr = data;
  
  if(gap_debug) printf("mov_pfirst_callback\n");

  /* advance to first point */
  p_points_to_tab(path_ptr);
  pvals->point_idx = 0;
  p_point_refresh(path_ptr);
}


static void
mov_plast_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_path_preview *path_ptr = data;
  
  if(gap_debug) printf("mov_plast_callback\n");

  /* advance to first point */
  p_points_to_tab(path_ptr);
  pvals->point_idx = pvals->point_idx_max;
  p_point_refresh(path_ptr);
}


static void
mov_pclr_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_path_preview *path_ptr = data;
  
  if(gap_debug) printf("mov_pclr_callback\n");
  p_clear_point();
  p_point_refresh(path_ptr);
}

static void
mov_pres_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_path_preview *path_ptr = data;
  
  if(gap_debug) printf("mov_pres_callback\n");
  p_reset_points();
  p_point_refresh(path_ptr);
}

static void
p_filesel_close_cb(GtkWidget *widget,
		   t_mov_path_preview *path_ptr)
{
  if(path_ptr->filesel == NULL) return;

  gtk_widget_destroy(GTK_WIDGET(path_ptr->filesel));
  path_ptr->filesel = NULL;   /* now filesel is closed */
}

static void
mov_pload_callback (GtkWidget *widget,
		      gpointer	 data)
{
  GtkWidget *filesel;
  t_mov_path_preview *path_ptr = data;

  if(path_ptr->filesel != NULL) return;  /* filesel is already open */
  
  filesel = gtk_file_selection_new ( _("Load Path Points from file"));
  path_ptr->filesel = filesel;

  gtk_window_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked", (GtkSignalFunc) p_points_load_from_file,
		      path_ptr);

  gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
		     "clicked", (GtkSignalFunc) p_filesel_close_cb,
	             path_ptr);
	             
  /* "destroy" has to be the last signal, 
   * (otherwise the other callbacks are never called)
   */
  gtk_signal_connect (GTK_OBJECT (filesel), "destroy",
		      (GtkSignalFunc) p_filesel_close_cb,
		      path_ptr);

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				   path_ptr->pointfile_name);

  gtk_widget_show (filesel);
}


static void
mov_psave_callback (GtkWidget *widget,
		      gpointer	 data)
{
  GtkWidget *filesel;
  t_mov_path_preview *path_ptr = data;

  if(path_ptr->filesel != NULL) return;  /* filesel is already open */
  
  filesel = gtk_file_selection_new ( _("Save Path Points to file"));
  path_ptr->filesel = filesel;

  gtk_window_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked", (GtkSignalFunc) p_points_save_to_file,
		      path_ptr);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
		     "clicked", (GtkSignalFunc) p_filesel_close_cb,
		     path_ptr);

  /* "destroy" has to be the last signal, 
   * (otherwise the other callbacks are never called)
   */
  gtk_signal_connect (GTK_OBJECT (filesel), "destroy",
		      (GtkSignalFunc) p_filesel_close_cb,
		      path_ptr);

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				   path_ptr->pointfile_name);

  gtk_widget_show (filesel);
}


static void 
p_points_load_from_file (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_path_preview *path_ptr = data;
  gchar	    *filename;
  
  if(gap_debug) printf("p_points_load_from_file\n");
  if(path_ptr->filesel == NULL) return;
  
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (path_ptr->filesel));
  g_free(path_ptr->pointfile_name);
  path_ptr->pointfile_name = g_strdup(filename);

  if(gap_debug) printf("p_points_load_from_file %s\n", path_ptr->pointfile_name);

  gtk_widget_destroy(GTK_WIDGET(path_ptr->filesel));
  path_ptr->filesel = NULL;

  p_load_points(path_ptr->pointfile_name);
  p_point_refresh(path_ptr);
}


static void
p_points_save_to_file (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_path_preview *path_ptr = data;
  char	    *filename;
  
  if(gap_debug) printf("p_points_save_to_file\n");
  if(path_ptr->filesel == NULL) return;
  
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (path_ptr->filesel));
  g_free(path_ptr->pointfile_name);
  path_ptr->pointfile_name = g_strdup(filename);

  if(gap_debug) printf("p_points_save_to_file %s\n", path_ptr->pointfile_name);

  gtk_widget_destroy(GTK_WIDGET(path_ptr->filesel));
  path_ptr->filesel = NULL;

  p_points_to_tab(path_ptr);
  p_save_points(path_ptr->pointfile_name);
  p_point_refresh(path_ptr);
}


static void
p_point_refresh(t_mov_path_preview *path_ptr)
{
  p_points_from_tab(path_ptr);
  p_update_point_labels(path_ptr);

  if(gap_debug) printf("p_point_refresh:newval in_call=%d\n", path_ptr->in_call );
  if( !path_ptr->in_call )
  {
      mov_path_prevw_cursor_update( path_ptr );
      mov_path_prevw_draw ( path_ptr, CURSOR | PATH_LINE );
  }
  path_ptr->in_call = TRUE;

  gtk_adjustment_set_value (path_ptr->x_adj,
			    (gfloat)path_ptr->p_x);
  gtk_adjustment_set_value (path_ptr->y_adj,
			    (gfloat)path_ptr->p_y);    
  gtk_adjustment_set_value (path_ptr->wres_adj,
			    (gfloat)path_ptr->w_resize);
  gtk_adjustment_set_value (path_ptr->hres_adj,
			    (gfloat)path_ptr->h_resize);
  gtk_adjustment_set_value (path_ptr->opacity_adj,
			    (gfloat)path_ptr->opacity);
  gtk_adjustment_set_value (path_ptr->rotation_adj,
			    (gfloat)path_ptr->rotation);
  path_ptr->in_call = FALSE;
}	/* end p_point_refresh */

static void
p_pick_nearest_point(gint px, gint py)
{
  gint l_idx;
  gint l_idx_min;
  gdouble l_sq_dist;
  gdouble l_dx, l_dy;
  gdouble l_sq_dist_min;
  
  l_idx_min = 0;
  l_sq_dist_min = G_MAXDOUBLE;

  if(gap_debug) printf("\np_pick_nearest_point: near to %4d %4d\n", (int)px, (int)py);

  for(l_idx = pvals->point_idx_max; l_idx >= 0; l_idx--)
  {
    /* calculate x and y distance */
    l_dx = pvals->point[l_idx].p_x - px;
    l_dy = pvals->point[l_idx].p_y - py;
    
    /* calculate square of the distance */
    l_sq_dist = (l_dx * l_dx) + (l_dy * l_dy);
    if(l_sq_dist < l_sq_dist_min)
    {
      l_sq_dist_min = l_sq_dist;
      l_idx_min = l_idx;
    }

    if(gap_debug)
    {
       printf("  [%2d] %4d %4d %f\n",
             (int)l_idx, 
             (int)pvals->point[l_idx].p_x,
             (int)pvals->point[l_idx].p_y,
             (float)l_sq_dist
             );
    }
  }
  if(gap_debug) printf("p_pick_nearest_point: selected %d\n", (int)l_idx_min);
     
  pvals->point_idx = l_idx_min;
  pvals->point[pvals->point_idx].p_x = px;
  pvals->point[pvals->point_idx].p_y = py;
}	/* end p_pick_nearest_point */



static void
mov_imglayer_menu_callback(gint32 id, gpointer data)
{
	pvals->src_layer_id = id;
	pvals->src_image_id = gimp_layer_get_image_id(id);

        if(gap_debug) printf("mov_imglayer_menu_callback: image_id=%ld layer_id=%ld\n",
               (long)pvals->src_image_id, (long)pvals->src_layer_id);
	/* TODO:
	 * if any remove old src layer from preview
	 * add this layer to preview (at current point koords)
	 * update_preview 
	 */
	
} /* end mov_imglayer_menu_callback */

static gint
mov_imglayer_constrain(gint32 image_id, gint32 drawable_id, gpointer data)
{
  gint32 l_src_image_id;

  if(gap_debug) printf("GAP-DEBUG: mov_imglayer_constrain PROCEDURE\n");

  if(drawable_id < 0)
  {
     /* gimp 1.1 makes a first call of the constraint procedure
      * with drawable_id = -1, and skips the whole image if FALSE is returned
      */
     return(TRUE);
  }
  
  l_src_image_id = gimp_layer_get_image_id(drawable_id);
  
   /* dont accept layers from within the destination image id 
    * or layers within the tmp preview image
    * conversions between different base_types are not supported in this version
    */
   return((l_src_image_id != pvals->dst_image_id) &&
          (l_src_image_id != pvals->tmp_image_id) &&
          (gimp_image_base_type(l_src_image_id) == gimp_image_base_type(pvals->tmp_image_id)) );
} /* end mov_imglayer_constrain */

static void
mov_paintmode_menu_callback (GtkWidget *w,  gpointer   client_data)
{
  pvals->src_paintmode = (gint)client_data;
}

static void
mov_handmode_menu_callback (GtkWidget *w,  gpointer   client_data)
{
  pvals->src_handle = (gint)client_data;
}

static void
mov_stepmode_menu_callback (GtkWidget *w, gpointer   client_data)
{
  pvals->src_stepmode = (gint)client_data;
}

static void
mov_gint_toggle_callback(GtkWidget *w, gpointer   client_data)
{
  gint *data;
  
  data = (gint*)client_data;
  
  if (GTK_TOGGLE_BUTTON (w)->active)
  {
    *data = 1;
  }
  else
  {
    *data = 0;
  }
}

static void
mov_show_path_callback(GtkWidget *widget, gpointer   client_data)
{
  t_mov_path_preview *path_ptr;
  
  path_ptr = (t_mov_path_preview *)client_data;
  mov_gint_toggle_callback(widget, &path_ptr->show_path);

  if(path_ptr == NULL) return;
  if(path_ptr->startup) return;
  if(path_ptr->preview == NULL) return;
  if(path_ptr->drawable == NULL) return;
  
  p_point_refresh(path_ptr);
  mov_path_prevw_draw ( path_ptr, CURSOR | PATH_LINE );
  gtk_widget_draw(path_ptr->preview, NULL);
  gdk_flush();  
}

/* ============================================================================
 * procedures to handle POINTS - TABLE
 * ============================================================================
 */
static void
p_points_from_tab(t_mov_path_preview *path_ptr)
{
  path_ptr->p_x      = pvals->point[pvals->point_idx].p_x;
  path_ptr->p_y      = pvals->point[pvals->point_idx].p_y;
  path_ptr->opacity  = pvals->point[pvals->point_idx].opacity;
  path_ptr->w_resize = pvals->point[pvals->point_idx].w_resize;
  path_ptr->h_resize = pvals->point[pvals->point_idx].h_resize;
  path_ptr->rotation = pvals->point[pvals->point_idx].rotation;
}

static void
p_points_to_tab(t_mov_path_preview *path_ptr)
{
  pvals->point[pvals->point_idx].p_x       = path_ptr->p_x;
  pvals->point[pvals->point_idx].p_y       = path_ptr->p_y;
  pvals->point[pvals->point_idx].opacity   = path_ptr->opacity;
  pvals->point[pvals->point_idx].w_resize  = path_ptr->w_resize;
  pvals->point[pvals->point_idx].h_resize  = path_ptr->h_resize;
  pvals->point[pvals->point_idx].rotation  = path_ptr->rotation;
}

void
p_update_point_labels(t_mov_path_preview *path_ptr)
{ 
  g_snprintf (&path_ptr->PointIndex_Label[0], LABEL_LENGTH, _("Current Point: [ %3d ] of [ %3d ]"), 
	      pvals->point_idx + 1, pvals->point_idx_max +1);

  if(NULL != path_ptr->PointIndex_LabelPtr)
  {  gtk_label_set(GTK_LABEL(path_ptr->PointIndex_LabelPtr),
                   &path_ptr->PointIndex_Label[0]);
  }
}


/* ============================================================================
 * p_reset_points
 *   Init point table with identical 2 Points
 * ============================================================================
 */
void p_clear_point()
{
  gint l_idx;
  
  l_idx = pvals->point_idx; 
  if((l_idx >= 0) && (l_idx <= pvals->point_idx_max))
  {
    pvals->point[l_idx].opacity  = 100; /* 100 percent (no transparecy) */
    pvals->point[l_idx].w_resize = 100; /* 100%  no resizize (1:1) */
    pvals->point[l_idx].h_resize = 100; /* 100%  no resizize (1:1) */
    pvals->point[l_idx].rotation = 0;   /* no rotation (0 degree) */
  }
}	/* end p_clear_point */

void p_reset_points()
{
  pvals->point_idx = 0;        /* 0 == current point */
  pvals->point_idx_max = 0;    /* 0 == there is only one valid point */
  p_clear_point();
  pvals->point[0].p_x = 0;
  pvals->point[0].p_y = 0;
}	/* end p_reset_points */

/* ============================================================================
 * p_load_points
 *   load point table (from named file into global pvals)
 *   (reset points if load failed)
 * ============================================================================
 */

void p_load_points(char *filename)
{
#define POINT_REC_MAX 128

  FILE *l_fp;
  gint   l_idx;
  char  l_buff[POINT_REC_MAX +1 ];
  char *l_ptr;
  gint   l_cnt;
  gint   l_v1, l_v2, l_v3, l_v4, l_v5, l_v6;

  if(filename == NULL) return;
  
  l_fp = fopen(filename, "r");
  if(l_fp != NULL)
  {
    l_idx = -1;
    while (NULL != fgets (l_buff, POINT_REC_MAX, l_fp))
    {
       /* skip leading blanks */
       l_ptr = l_buff;
       while(*l_ptr == ' ') { l_ptr++; }
       
       /* check if line empty or comment only (starts with '#') */
       if((*l_ptr != '#') && (*l_ptr != '\n') && (*l_ptr != '\0'))
       {
         l_cnt = sscanf(l_ptr, "%d%d%d%d%d%d", &l_v1, &l_v2, &l_v3, &l_v4, &l_v5, &l_v6);
         if(l_idx == -1)
         {
           if((l_cnt < 2) || (l_v2 > GAP_MOV_MAX_POINT) || (l_v1 > l_v2))
           {
             break;
            }
           pvals->point_idx     = l_v1;
           pvals->point_idx_max = l_v2 -1;
           l_idx = 0;
         }
         else
         {
           if(l_cnt != 6)
           {
             p_reset_points();
             break;
           }
           pvals->point[l_idx].p_x      = l_v1;
           pvals->point[l_idx].p_y      = l_v2;
           pvals->point[l_idx].w_resize = l_v3;
           pvals->point[l_idx].h_resize = l_v4;
           pvals->point[l_idx].opacity  = l_v5;
           pvals->point[l_idx].rotation = l_v6;
           l_idx ++;
         }

         if(l_idx > pvals->point_idx_max) break;
       }
    }
     
    fclose(l_fp);
  }
}

/* ============================================================================
 * p_save_points
 *   save point table (from global pvals into named file)
 * ============================================================================
 */
void p_save_points(char *filename)
{
  FILE *l_fp;
  gint l_idx;
  
  if(filename == NULL) return;
  
  l_fp = fopen(filename, "w+");
  if(l_fp != NULL)
  {
    fprintf(l_fp, "# GAP file contains saved Move Path Point Table\n");
    fprintf(l_fp, "%d  %d  # current_point  points\n",
                  (int)pvals->point_idx,
                  (int)pvals->point_idx_max + 1);
    fprintf(l_fp, "# x  y   width height opacity rotation\n");
    for(l_idx = 0; l_idx <= pvals->point_idx_max; l_idx++)
    {
      fprintf(l_fp, "%04d %04d  %03d %03d  %03d %d\n",
                   (int)pvals->point[l_idx].p_x,
                   (int)pvals->point[l_idx].p_y,
                   (int)pvals->point[l_idx].w_resize,
                   (int)pvals->point[l_idx].h_resize,
                   (int)pvals->point[l_idx].opacity,
                   (int)pvals->point[l_idx].rotation);
    }
     
    fclose(l_fp);
  }
}	/* end p_save_points */





/* ============================================================================
 * Create new source selection table Frame, and return it.
 *   A frame that contains:
 *   - 2x2 menus (src_image/layer, handle, stepmode, paintmode)
 * ============================================================================
 */

static GtkWidget *
mov_src_sel_create()
{
  GtkWidget	 *frame;
  GtkWidget	 *table;
  GtkWidget  *option_menu;
  GtkWidget  *menu;
  GtkWidget  *label;
  gint gettextize_loop;


  frame = gtk_frame_new ( _("Source Select") );
/*
  gtk_signal_connect( GTK_OBJECT( frame ), "destroy",
		      (GtkSignalFunc) mov_src_sel_destroy,
		      path_ptr );
*/
  gtk_frame_set_shadow_type( GTK_FRAME( frame ) ,GTK_SHADOW_ETCHED_IN );
  gtk_container_border_width( GTK_CONTAINER( frame ), 2 );
  

  table = gtk_table_new ( 2, 4, FALSE );
  gtk_container_border_width (GTK_CONTAINER (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);

  /* Source Layer menu */
  label = gtk_label_new( _("SourceImage/Layer:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  option_menu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), option_menu, 1, 2, 0, 1,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gimp_help_set_help_data(option_menu,
                       _("Source Object to insert into Framerange")
                       , NULL);

  gtk_widget_show(option_menu);

  menu = gimp_layer_menu_new(mov_imglayer_constrain,
			     mov_imglayer_menu_callback,
			     NULL,
			     pvals->src_layer_id);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_widget_show(option_menu);


  /* Paintmode menu */

  label = gtk_label_new( _("Mode:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  option_menu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), option_menu, 3, 4, 0, 1,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gimp_help_set_help_data(option_menu,
                       _("Paintmode")
                       , NULL);
  gtk_widget_show(option_menu);

  for (gettextize_loop = 0; option_paint_items[gettextize_loop].label != NULL;
       gettextize_loop++)
    option_paint_items[gettextize_loop].label =
      gettext(option_paint_items[gettextize_loop].label);

  menu = p_buildmenu (option_paint_items);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_widget_show(option_menu);

  /* Loop Stepmode menu */

  label = gtk_label_new( _("Stepmode:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  option_menu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), option_menu, 1, 2, 1, 2,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(option_menu);

  for (gettextize_loop = 0; option_step_items[gettextize_loop].label != NULL;
       gettextize_loop++)
    option_step_items[gettextize_loop].label =
      gettext(option_step_items[gettextize_loop].label);

  menu = p_buildmenu (option_step_items);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gimp_help_set_help_data(option_menu,
                       _("How to fetch the next SrcLayer   \nat the next handled frame")
                       , NULL);
  gtk_widget_show(option_menu);

  /* Source Image Handle menu */

  label = gtk_label_new( _("Handle:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 1, 2, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  option_menu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), option_menu, 3, 4, 1, 2,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(option_menu);

  for (gettextize_loop = 0; option_handle_items[gettextize_loop].label != NULL;
       gettextize_loop++)
    option_handle_items[gettextize_loop].label =
      gettext(option_handle_items[gettextize_loop].label);

  menu = p_buildmenu (option_handle_items);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gimp_help_set_help_data(option_menu,
                       _("How to place the SrcLayer at   \nControlpoint Koordinates")
                       , NULL);
  gtk_widget_show(option_menu);

  gtk_widget_show( table );
  gtk_widget_show( frame );
  
  return frame;
}	/* end mov_src_sel_create */


/* ============================================================================
 * Create new path_preview Frame, and return it (GtkFrame).
 *   A frame that contains one preview and the entries of the current point
 *   One "Point" has:
 *   - 2 entrys X/Y, used for positioning
 *   - Resize 2x Scale + integer entry (for resizing Width + Height)
 *   - Opacity   Scale + integr entry  (0 to 100 %)
 *   - Rotation  Scale + ineger entry  (-360 to 360 degrees)
 * ============================================================================
 */

static GtkWidget *
mov_path_prevw_create ( GDrawable *drawable, t_mov_path_preview *path_ptr)
{
  GtkWidget	 *frame;
  GtkWidget	 *vbox;
  GtkWidget	 *hbox;
  GtkWidget	 *table;
  GtkWidget	 *label;
  GtkWidget	 *pframe;
  GtkWidget	 *preview;
  GtkWidget      *button_table;
  GtkWidget      *pv_table;
  GtkWidget      *button;
  GtkWidget      *check_button;
  GtkObject      *adj;
  gint           row;

  path_ptr->drawable = drawable;
  path_ptr->dwidth   = gimp_drawable_width(drawable->id );
  path_ptr->dheight  = gimp_drawable_height(drawable->id );
  path_ptr->bpp	   = gimp_drawable_bpp(drawable->id);
  if ( gimp_drawable_has_alpha(drawable->id) )
    path_ptr->bpp--;
  path_ptr->cursor = FALSE;
  path_ptr->curx = 0;
  path_ptr->cury = 0;
  path_ptr->oldx = 0;
  path_ptr->oldy = 0;
  path_ptr->in_call = TRUE;  /* to avoid side effects while initialization */

  /* the frame */
  frame = gtk_frame_new ( _("Move Path Preview") );
  gtk_signal_connect( GTK_OBJECT( frame ), "destroy",
		      (GtkSignalFunc) mov_path_prevw_destroy,
		      path_ptr );
  gtk_frame_set_shadow_type( GTK_FRAME( frame ) ,GTK_SHADOW_ETCHED_IN );
  gtk_container_border_width( GTK_CONTAINER( frame ), 2 );

  /* the vbox */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* the table (3 rows) */
  table = gtk_table_new ( 3, 6, FALSE );
  gtk_container_border_width (GTK_CONTAINER (table), 2 );
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);


  /* X */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 0, 0,          /* table col, row */
			  _("X:"),                              /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,             /* scalesize spinsize */
			  (gfloat)path_ptr->p_x,                /* value */
			  (gfloat)0, (gfloat)path_ptr->dwidth,  /* lower, upper */
			  1, 10,                                /* step, page */
			  0,                                    /* digits */
			  FALSE,                                /* constrain */
			  (gfloat)(-GIMP_MAX_IMAGE_SIZE),
			  (gfloat)GIMP_MAX_IMAGE_SIZE,          /* lower, upper (unconstrained) */
			  _("X Coordinate"),
			  NULL);    /* tooltip privatetip */
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (mov_path_x_adjustment_update),
		      path_ptr);
  path_ptr->x_adj = GTK_ADJUSTMENT(adj);

  /* Y */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 3, 0,          /* table col, row */
			  _("Y:"),                              /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,             /* scalesize spinsize */
			  (gfloat)path_ptr->p_y,                /* value */
			  (gfloat)0, (gfloat)path_ptr->dheight, /* lower, upper */
			  1, 10,                                /* step, page */
			  0,                                    /* digits */
			  FALSE,                                /* constrain */
			  (gfloat)(-GIMP_MAX_IMAGE_SIZE),
			  (gfloat)GIMP_MAX_IMAGE_SIZE,          /* lower, upper (unconstrained) */
			  _("Y Coordinate"),
			  NULL);    /* tooltip privatetip */
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (mov_path_y_adjustment_update),
		      path_ptr);
  path_ptr->y_adj = GTK_ADJUSTMENT(adj);

  /* Widht Scale */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 0, 1,        /* table col, row */
			  _("Width:"),                        /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gfloat)path_ptr->w_resize,         /* value */
			  (gfloat)1, (gfloat)200,             /* lower, upper */
			  1, 10,                              /* step, page */
			  0,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gfloat)1, (gfloat)1000,            /* lower, upper (unconstrained) */
			  _("Scale SrcLayer's Width\nin percent"),
			  NULL);    /* tooltip privatetip */
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &path_ptr->w_resize);
  path_ptr->wres_adj = GTK_ADJUSTMENT(adj);

  /* Height Scale */		      
  adj = gimp_scale_entry_new( GTK_TABLE (table), 3, 1,        /* table col, row */
			  _("Height:"),                       /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gfloat)path_ptr->h_resize,         /* value */
			  (gfloat)1, (gfloat)200,             /* lower, upper */
			  1, 10,                              /* step, page */
			  0,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gfloat)1, (gfloat)1000,            /* lower, upper (unconstrained) */
			  _("Scale SrcLayer's Height\nin percent"),
			  NULL);    /* tooltip privatetip */
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &path_ptr->h_resize);
  path_ptr->hres_adj = GTK_ADJUSTMENT(adj);
		      
  /* Opacity */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 0, 2,        /* table col, row */
			  _("Opacity:"),                      /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gfloat)path_ptr->opacity,          /* value */
			  (gfloat)0, (gfloat)100,             /* lower, upper */
			  1, 10,                              /* step, page */
			  0,                                  /* digits */
			  TRUE,                               /* constrain */
			  (gfloat)0, (gfloat)100,             /* lower, upper (unconstrained) */
			  _("SrcLayer's Opacity\nin percent"),
			  NULL);    /* tooltip privatetip */
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &path_ptr->opacity);
  path_ptr->opacity_adj = GTK_ADJUSTMENT(adj);
		      
  /* Rotation */ 
  adj = gimp_scale_entry_new( GTK_TABLE (table), 3, 2,        /* table col, row */
			  _("Rotate:"),                       /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gfloat)path_ptr->rotation,         /* value */
			  (gfloat)-360, (gfloat)360,          /* lower, upper */
			  1, 10,                              /* step, page */
			  0,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gfloat)-3600, (gfloat)3600,        /* lower, upper (unconstrained) */
			  _("Rotate SrcLayer (in degree)"),
			  NULL);    /* tooltip privatetip */
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &path_ptr->rotation);
  path_ptr->rotation_adj = GTK_ADJUSTMENT(adj);
  gtk_widget_show( table );

  /* the hbox (for preview table and button_table) */
  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* the preview table (1 rows) */
  pv_table = gtk_table_new ( 1, 1, FALSE );
  gtk_container_border_width (GTK_CONTAINER (pv_table), 2 );
  gtk_table_set_row_spacings (GTK_TABLE (pv_table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (pv_table), 5);
  gtk_box_pack_start (GTK_BOX (hbox), pv_table, TRUE, TRUE, 0);

  /* frame (shadow_in) that contains preview */
  pframe = gtk_frame_new ( NULL );
  gtk_frame_set_shadow_type( GTK_FRAME( pframe ), GTK_SHADOW_IN );
  gtk_table_attach( GTK_TABLE(pv_table), pframe, 0, 1, 0, 1,
		    0, 0, 0, 0 );

  /* PREVIEW */
  path_ptr->preview = preview = gtk_preview_new( path_ptr->bpp==3 ? GTK_PREVIEW_COLOR : GTK_PREVIEW_GRAYSCALE );
  gtk_object_set_user_data( GTK_OBJECT(preview), path_ptr );
  gtk_widget_set_events( GTK_WIDGET(preview), PREVIEW_MASK );
  gtk_signal_connect_after( GTK_OBJECT(preview), "expose_event",
		      (GtkSignalFunc) mov_path_prevw_preview_expose,
		      path_ptr );
  gtk_signal_connect( GTK_OBJECT(preview), "event",
		      (GtkSignalFunc) mov_path_prevw_preview_events,
		      path_ptr );
  gtk_container_add( GTK_CONTAINER( pframe ), path_ptr->preview );

  /*
   * Resize the greater one of dwidth and dheight to PREVIEW_SIZE
   */
  if ( path_ptr->dwidth > path_ptr->dheight ) {
    path_ptr->pheight = path_ptr->dheight * PREVIEW_SIZE / path_ptr->dwidth;
    path_ptr->pwidth = PREVIEW_SIZE;
  } else {
    path_ptr->pwidth = path_ptr->dwidth * PREVIEW_SIZE / path_ptr->dheight;
    path_ptr->pheight = PREVIEW_SIZE;
  }
  gtk_preview_size( GTK_PREVIEW( preview ), path_ptr->pwidth, path_ptr->pheight );



  /* Draw the contents of preview, that is saved in the preview widget */
  mov_path_prevw_preview_init( path_ptr );
  gtk_widget_show(preview);

  /* button_table 7 rows */
  button_table = gtk_table_new (7, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (button_table), 0);
  gtk_table_set_col_spacings (GTK_TABLE (button_table), 0);

  row = 0;

  /* the PointIndex label */
  label = gtk_label_new ( &path_ptr->PointIndex_Label[0] );  /* "Current Point: 1" */
  gtk_misc_set_alignment( GTK_MISC(label), 0.0, 0.5 );
  gtk_table_attach( GTK_TABLE(button_table), label, 0, 2, row, row+1,
		    GTK_FILL, 0, 4, 0 );
  gtk_widget_show(label);
  path_ptr->PointIndex_LabelPtr = label;  /* store label ptr for later update */

  row++;

  button = gtk_button_new_with_label ( _("Add Point"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mov_padd_callback,
		      path_ptr);
  gtk_table_attach( GTK_TABLE(button_table), button, 0, 1, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Add Controlpoint at end        \n(the last Point is duplicated)")
                       , NULL);
  gtk_widget_show (button);

  /* toggle clip_to_image */
  check_button = gtk_check_button_new_with_label ( _("Show Path"));
  gtk_signal_connect (GTK_OBJECT (check_button), "toggled",
                      (GtkSignalFunc) mov_show_path_callback,
                       path_ptr);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (check_button), path_ptr->show_path);
  gimp_help_set_help_data(check_button,
                       _("Show Path Lines and enable\n"
                         "pick/drag with left button\n"
                         "or move with right button")
                       , NULL);
  gtk_widget_show (check_button);
  gtk_table_attach(GTK_TABLE(button_table), check_button, 1, 2, row, row+1,
                   0, 0, 0, 0);

  row++;

  button = gtk_button_new_with_label ( _("Insert Point"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mov_pins_callback,
		      path_ptr);
  gtk_table_attach( GTK_TABLE(button_table), button, 0, 1, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Insert Controlpoint          \n(the current Point is duplicated)")
                       , NULL);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( _("Delete Point"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mov_pdel_callback,
		      path_ptr);
  gtk_table_attach( GTK_TABLE(button_table), button, 1, 2, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Delete current Controlpoint")
                       , NULL);
  gtk_widget_show (button);

  row++;

  button = gtk_button_new_with_label ( _("Prev Point"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mov_pprev_callback,
		      path_ptr);
  gtk_table_attach( GTK_TABLE(button_table), button, 0, 1, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Show Previous Controlpoint")
                       , NULL);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( _("Next Point"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mov_pnext_callback,
		      path_ptr);
  gtk_table_attach( GTK_TABLE(button_table), button, 1, 2, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Show Next Controlpoint")
                       , NULL);
  gtk_widget_show (button);

  row++;

  button = gtk_button_new_with_label ( _("First Point"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mov_pfirst_callback,
		      path_ptr);
  gtk_table_attach( GTK_TABLE(button_table), button, 0, 1, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Show First Controlpoint")
                       , NULL);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( _("Last Point"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mov_plast_callback,
		      path_ptr);
  gtk_table_attach( GTK_TABLE(button_table), button, 1, 2, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Show Last Controlpoint")
                       , NULL);
  gtk_widget_show (button);

  row++;

  button = gtk_button_new_with_label ( _("Clear Point"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mov_pclr_callback,
		      path_ptr);
  gtk_table_attach( GTK_TABLE(button_table), button, 0, 1, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Reset the current Controlpoint\nto default Values")
                       , NULL);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( _("Reset Points"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mov_pres_callback,
		      path_ptr);
  gtk_table_attach( GTK_TABLE(button_table), button, 1, 2, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Reset Controlpoints  \nto one Defaultpoint")
                       , NULL);
  gtk_widget_show (button);

  row++;

  button = gtk_button_new_with_label ( _("Load Points"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mov_pload_callback,
		      path_ptr);
  gtk_table_attach( GTK_TABLE(button_table), button, 0, 1, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Load Controlpoints from file")
                       , NULL);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( _("Save Points"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) mov_psave_callback,
		      path_ptr);
  gtk_table_attach( GTK_TABLE(button_table), button, 1, 2, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Save Controlpoints to file")
                       , NULL);
  gtk_widget_show (button);

  row++;

  gtk_box_pack_start (GTK_BOX (hbox), button_table, TRUE, TRUE, 0);

  gtk_widget_show (button_table);
  gtk_widget_show( pframe );
  gtk_widget_show( pv_table );
  gtk_widget_show( frame );

  mov_path_prevw_cursor_update( path_ptr );

  path_ptr->cursor = FALSE;    /* Make sure that the cursor has not been drawn */
  path_ptr->in_call = FALSE;   /* End of initialization */
  if(gap_debug) printf("pvals path_ptr=%d,%d\n", path_ptr->p_x, path_ptr->p_y );
  if(gap_debug) printf("path_ptr cur=%d,%d\n", path_ptr->curx, path_ptr->cury );
  return frame;
}

static void
mov_path_prevw_destroy ( GtkWidget *widget,
		      gpointer data )
{
  t_mov_path_preview *path_ptr = data;
  g_free( path_ptr );
}

static void render_preview ( GtkWidget *preview, GPixelRgn *srcrgn );

/* ============================================================================
 *  mov_path_prevw_preview_init
 *    Initialize preview
 *    Draw the contents into the internal buffer of the preview widget
 * ============================================================================
 */

static void
mov_path_prevw_preview_init ( t_mov_path_preview *path_ptr )
{
  gimp_pixel_rgn_init ( &path_ptr->src_rgn, path_ptr->drawable, 0, 0,
			path_ptr->dwidth, path_ptr->dheight, FALSE, FALSE );
  path_ptr->PixelRgnIsInitialized = TRUE;
  render_preview( path_ptr->preview, &path_ptr->src_rgn );
}


/* ============================================================================
 * render_preview
 *		Preview Rendering Util routine
 * ============================================================================
 */

#define CHECKWIDTH 8
#define LIGHTCHECK 192
#define DARKCHECK  128
#ifndef OPAQUE
#define OPAQUE	   255
#endif

static void
render_preview ( GtkWidget *preview, GPixelRgn *srcrgn )
{
  guchar	 *src_row, *dest_row, *src, *dest;
  gint		 row, col;
  gint		 dwidth, dheight, pwidth, pheight;
  gint		 *src_col;
  gint		 bpp, alpha, has_alpha, b;
  guchar	 check;

  dwidth  = srcrgn->w;
  dheight = srcrgn->h;
  if( GTK_PREVIEW(preview)->buffer )
    {
      pwidth  = GTK_PREVIEW(preview)->buffer_width;
      pheight = GTK_PREVIEW(preview)->buffer_height;
    }
  else
    {
      pwidth  = preview->requisition.width;
      pheight = preview->requisition.height;
    }

  bpp = srcrgn->bpp;
  alpha = bpp;
  has_alpha = gimp_drawable_has_alpha( srcrgn->drawable->id );
  if( has_alpha ) alpha--;
  /*  printf("render_preview: %d %d %d", bpp, alpha, has_alpha);
      printf(" (%d %d %d %d)\n", dwidth, dheight, pwidth, pheight); */

  src_row = g_new ( guchar, dwidth * bpp );
  dest_row = g_new ( guchar, pwidth * bpp );
  src_col = g_new ( gint, pwidth );

  for ( col = 0; col < pwidth; col++ )
    src_col[ col ] = ( col * dwidth / pwidth ) * bpp;

  for ( row = 0; row < pheight; row++ )
    {
      gimp_pixel_rgn_get_row ( srcrgn, src_row,
			       0, row * dheight / pheight, dwidth );
      dest = dest_row;
      for ( col = 0; col < pwidth; col++ )
	{
	  src = &src_row[ src_col[col] ];
	  if( !has_alpha || src[alpha] == OPAQUE )
	    {
	      /* no alpha channel or opaque -- simple way */
	      for ( b = 0; b < alpha; b++ )
		dest[b] = src[b];
	    }
	  else
	    {
	      /* more or less transparent */
	      if( ( col % (CHECKWIDTH*2) < CHECKWIDTH ) ^
		  ( row % (CHECKWIDTH*2) < CHECKWIDTH ) )
		check = LIGHTCHECK;
	      else
		check = DARKCHECK;

	      if ( src[alpha] == 0 )
		{
		  /* full transparent -- check */
		  for ( b = 0; b < alpha; b++ )
		    dest[b] = check;
		}
	      else
		{
		  /* middlemost transparent -- mix check and src */
		  for ( b = 0; b < alpha; b++ )
		    dest[b] = ( src[b]*src[alpha] + check*(OPAQUE-src[alpha]) ) / OPAQUE;
		}
	    }
	  dest += alpha;
	}
      gtk_preview_draw_row( GTK_PREVIEW( preview ), dest_row,
			    0, row, pwidth );
    }

  g_free ( src_col );
  g_free ( src_row );
  g_free ( dest_row );
}	/* end render_preview */

/* ============================================================================
 * mov_path_prevw_draw
 *   Preview Rendering Util routine End
 *     if update & PATH_LINE, draw the path lines
 *     if update & CURSOR,  draw cross cursor
 * ============================================================================
 */

static void
mov_path_prevw_draw ( t_mov_path_preview *path_ptr, gint update )
{
  gint l_idx;
  GdkColor fg;
  guchar l_red, l_green, l_blue;

  if( update & PREVIEW )
    {
      path_ptr->cursor = FALSE;
      if(gap_debug) printf("draw-preview\n");
    }


  /* alternate cross cursor OR path graph */

  if((path_ptr->show_path)
  && ( pvals != NULL )
  && (update & PATH_LINE))
    {
      if(gap_debug) printf("draw-preview re-render for PATH draw\n");
      if((path_ptr->PixelRgnIsInitialized)
      && (path_ptr->preview))
      {
         /* redraw the preview
          * (to clear path lines and cross cursor)
          */
         gtk_widget_draw(path_ptr->preview, NULL);
      }
 
      gimp_palette_get_foreground(&l_red, &l_green, &l_blue);
      fg.pixel = gdk_rgb_xpixel_from_rgb ((l_red << 16) | (l_green << 8) | l_blue);
        
      gdk_gc_set_foreground (path_ptr->preview->style->black_gc, &fg);

      p_points_to_tab(path_ptr);
      for(l_idx = 0; l_idx < pvals->point_idx_max; l_idx++)
        {
           /* draw the path line(s) */
	   gdk_draw_line (path_ptr->preview->window,
			  path_ptr->preview->style->black_gc,
			  (pvals->point[l_idx].p_x    * path_ptr->pwidth) / path_ptr->dwidth,
			  (pvals->point[l_idx].p_y    * path_ptr->pheight) / path_ptr->dheight, 
			  (pvals->point[l_idx +1].p_x * path_ptr->pwidth) / path_ptr->dwidth,
			  (pvals->point[l_idx +1].p_y * path_ptr->pheight) / path_ptr->dheight
			  );
           /* draw the path point(s) */
	   gdk_draw_arc (path_ptr->preview->window, path_ptr->preview->style->black_gc, TRUE,
			    (pvals->point[l_idx +1].p_x  * path_ptr->pwidth / path_ptr->dwidth) -RADIUS,
			    (pvals->point[l_idx +1].p_y  * path_ptr->pheight / path_ptr->dheight) -RADIUS,
			    RADIUS * 2, RADIUS * 2, 0, 23040);
        }
        /* draw the start point */
	gdk_draw_arc (path_ptr->preview->window, path_ptr->preview->style->black_gc, TRUE,
		     (pvals->point[0].p_x * path_ptr->pwidth / path_ptr->dwidth) -RADIUS,
		     (pvals->point[0].p_y * path_ptr->pheight / path_ptr->dheight) -RADIUS,
		     RADIUS * 2, RADIUS * 2, 0, 23040);

        /* restore black gc */
        fg.pixel = gdk_rgb_xpixel_from_rgb (0);
        gdk_gc_set_foreground (path_ptr->preview->style->black_gc, &fg);
    }


  if( update & CURSOR )
    {
      if(gap_debug) printf("draw-cursor %d old=%d,%d cur=%d,%d\n",
	     path_ptr->cursor, path_ptr->oldx, path_ptr->oldy, path_ptr->curx, path_ptr->cury);
      gdk_gc_set_function ( path_ptr->preview->style->black_gc, GDK_INVERT);
      if( path_ptr->cursor )
	{
	  gdk_draw_line ( path_ptr->preview->window,
			  path_ptr->preview->style->black_gc,
			  path_ptr->oldx, 1, path_ptr->oldx, path_ptr->pheight-1 );
	  gdk_draw_line ( path_ptr->preview->window,
			  path_ptr->preview->style->black_gc,
			  1, path_ptr->oldy, path_ptr->pwidth-1, path_ptr->oldy );
	}
      gdk_draw_line ( path_ptr->preview->window,
		      path_ptr->preview->style->black_gc,
		      path_ptr->curx, 1, path_ptr->curx, path_ptr->pheight-1 );
      gdk_draw_line ( path_ptr->preview->window,
		      path_ptr->preview->style->black_gc,
		      1, path_ptr->cury, path_ptr->pwidth-1, path_ptr->cury );
      /* current position of cursor is updated */
      path_ptr->oldx = path_ptr->curx;
      path_ptr->oldy = path_ptr->cury;
      path_ptr->cursor = TRUE;
      gdk_gc_set_function ( path_ptr->preview->style->black_gc, GDK_COPY);
    }
}


/*
 *  mov_path_xy_adjustment_update
 */

static void
mov_path_x_adjustment_update( GtkWidget *widget,
			      gpointer data )
{
  t_mov_path_preview *path_ptr;
  gint old_val;

  path_ptr = (t_mov_path_preview *)data;
  if(path_ptr == NULL) return;
  
  old_val = path_ptr->p_x;
  gimp_int_adjustment_update(GTK_ADJUSTMENT(widget), &path_ptr->p_x);
  if( old_val != path_ptr->p_x )
  {
      if( !path_ptr->in_call )
      {
	  mov_path_prevw_cursor_update( path_ptr );
	  mov_path_prevw_draw ( path_ptr, CURSOR | PATH_LINE );
      }
  }
}
static void
mov_path_y_adjustment_update( GtkWidget *widget,
			      gpointer data )
{
  t_mov_path_preview *path_ptr;
  gint old_val;

  path_ptr = (t_mov_path_preview *)data;
  if(path_ptr == NULL) return;
  
  old_val = path_ptr->p_y;
  gimp_int_adjustment_update(GTK_ADJUSTMENT(widget), &path_ptr->p_y);
  if( old_val != path_ptr->p_y )
  {
      if( !path_ptr->in_call )
      {
	  mov_path_prevw_cursor_update( path_ptr );
	  mov_path_prevw_draw ( path_ptr, CURSOR | PATH_LINE );
      }
  }
}




/*
 *  Update the cross cursor's  coordinates accoding to pvals->[xy]path_prevw
 *  but not redraw it
 */

static void
mov_path_prevw_cursor_update ( t_mov_path_preview *path_ptr )
{
  path_ptr->curx = path_ptr->p_x * path_ptr->pwidth / path_ptr->dwidth;
  path_ptr->cury = path_ptr->p_y * path_ptr->pheight / path_ptr->dheight;

  if( path_ptr->curx < 0 )		     path_ptr->curx = 0;
  else if( path_ptr->curx >= path_ptr->pwidth )  path_ptr->curx = path_ptr->pwidth-1;
  if( path_ptr->cury < 0 )		     path_ptr->cury = 0;
  else if( path_ptr->cury >= path_ptr->pheight)  path_ptr->cury = path_ptr->pheight-1;

}

/*
 *    Handle the expose event on the preview
 */
static gint
mov_path_prevw_preview_expose( GtkWidget *widget,
			    GdkEvent *event )
{
  t_mov_path_preview *path_ptr;

  path_ptr = gtk_object_get_user_data( GTK_OBJECT(widget) );

  if((!path_ptr->PixelRgnIsInitialized)
  || (path_ptr->in_call))
    {
       return FALSE;
    }

  path_ptr->in_call = TRUE;
  mov_path_prevw_draw( path_ptr, ALL );
  path_ptr->in_call = FALSE;
  return FALSE;
}


/*
 *    Handle other events on the preview
 */

static gint
mov_path_prevw_preview_events ( GtkWidget *widget,
			     GdkEvent *event )
{
  t_mov_path_preview *path_ptr;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint upd_flag;
  gint mouse_button;

  path_ptr = gtk_object_get_user_data ( GTK_OBJECT(widget) );

 /* HINT:
  * smooth update of both CURSOR and PATH_LINE
  * on every mousemove works fine on machines with 300MHz.
  * for slower machines it is better to paint just the cross cursor,
  * and refresh the path lines only at
  * button press and release events
  */
  /* upd_flag = CURSOR | PATH_LINE; */
  upd_flag = CURSOR;
       
  mouse_button = 0;
  
  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;
      mouse_button = 0 - bevent->button;
      goto mbuttons;
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      mouse_button = bevent->button;
    mbuttons:
      path_ptr->curx = bevent->x;
      path_ptr->cury = bevent->y;
      upd_flag = CURSOR | PATH_LINE;    
      goto mouse;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      if ( !mevent->state ) break;
      path_ptr->curx = mevent->x;
      path_ptr->cury = mevent->y;
    mouse:
      if((mouse_button == 1)
      && (path_ptr->show_path))
      {
         /* Picking of pathpoints is done only if
          *   the left mousebutton goes down (mouse_button == 1)
          *   and only if Path is visible
          */
         p_points_to_tab(path_ptr);
         path_ptr->p_x = path_ptr->curx * path_ptr->dwidth / path_ptr->pwidth;     
         path_ptr->p_y = path_ptr->cury * path_ptr->dheight / path_ptr->pheight;
         p_pick_nearest_point(path_ptr->p_x, path_ptr->p_y);
         p_point_refresh(path_ptr);
      }
      else
      {
         path_ptr->p_x = path_ptr->curx * path_ptr->dwidth / path_ptr->pwidth;     
         path_ptr->p_y = path_ptr->cury * path_ptr->dheight / path_ptr->pheight;
         p_points_to_tab(path_ptr);
         mov_path_prevw_cursor_update( path_ptr );
      }
      mov_path_prevw_draw( path_ptr, upd_flag);
      path_ptr->in_call = TRUE;
      gtk_adjustment_set_value (path_ptr->x_adj,
				(gfloat)path_ptr->p_x);
      gtk_adjustment_set_value (path_ptr->y_adj,
				(gfloat)path_ptr->p_y);    
      
      path_ptr->in_call = FALSE;
      break;

    default:
      break;
    }

  return FALSE;
}

/* ============================================================================
 * p_get_flattened_drawable
 *   flatten the given image and return pointer to the
 *   (only) remaining drawable. 
 * ============================================================================
 */
GDrawable *
p_get_flattened_drawable(gint32 image_id)
{
  GDrawable *l_drawable_ptr ;
  GImageType l_type;
  guint   l_width, l_height;
  gint32  l_layer_id;
 
  /* get info about the image */
  l_width  = gimp_image_width(image_id);
  l_height = gimp_image_height(image_id);
  l_type   = gimp_image_base_type(image_id);

  l_type   = (l_type * 2); /* convert from GImageType to GDrawableType */

  /* add 2 full transparent dummy layers at top
   * (because gimp_image_merge_visible_layers complains
   * if there are less than 2 visible layers)
   */
  l_layer_id = gimp_layer_new(image_id, "dummy",
                                 l_width, l_height,  l_type,
                                 0.0,       /* Opacity full transparent */     
                                 0);        /* NORMAL */
  gimp_image_add_layer(image_id, l_layer_id, 0);

  l_layer_id = gimp_layer_new(image_id, "dummy",
                                 10, 10,  l_type,
                                 0.0,       /* Opacity full transparent */     
                                 0);        /* NORMAL */
  gimp_image_add_layer(image_id, l_layer_id, 0);

  l_drawable_ptr = gimp_drawable_get (gimp_image_merge_visible_layers (image_id, GIMP_CLIP_TO_IMAGE));
  return l_drawable_ptr;
}	/* end p_get_flattened_drawable */



/* ============================================================================
 *   add the selected source layer to the temp. preview image
 *   (modified accordung to current settings)
 *   then flatten the temporary preview image, 
 *   and return pointer to the (only) remaining drawable. 
 * ============================================================================
 */

GDrawable * 
p_get_prevw_drawable (t_mov_path_preview *path_ptr)
{
  t_mov_current l_curr;
  gint      l_nlayers;
 

  /* check if we have a source layer (to add to the preview) */
  if((pvals->src_layer_id >= 0) && (pvals->src_image_id >= 0))
  {
    p_points_to_tab(path_ptr);
    
    /* calculate current settings */
    l_curr.dst_frame_nr    = 0;
    l_curr.deltaX          = 0.0;
    l_curr.deltaY          = 0.0;
    l_curr.deltaOpacity    = 0.0;
    l_curr.deltaWidth      = 0.0;
    l_curr.deltaHeight     = 0.0;
    l_curr.deltaRotation   = 0.0;


    l_curr.currX         = (gdouble)path_ptr->p_x;
    l_curr.currY         = (gdouble)path_ptr->p_y;
    l_curr.currOpacity   = (gdouble)path_ptr->opacity;
    l_curr.currWidth     = (gdouble)path_ptr->w_resize;
    l_curr.currHeight    = (gdouble)path_ptr->h_resize;
    l_curr.currRotation  = (gdouble)path_ptr->rotation;

    l_curr.src_layer_idx   = 0;
    l_curr.src_layers      = gimp_image_get_layers (pvals->src_image_id, &l_nlayers);
    if((l_curr.src_layers != NULL) && (l_nlayers > 0))
    {
      l_curr.src_last_layer  = l_nlayers -1;
      /* findout index of src_layer_id */
      for(l_curr.src_layer_idx = 0; 
          l_curr.src_layer_idx  < l_nlayers;
          l_curr.src_layer_idx++)
      {
         if(l_curr.src_layers[l_curr.src_layer_idx] == pvals->src_layer_id)
            break;
      }
      
    }
    /* set offsets (in cur_ptr) 
     *  according to handle_mode and src_img dimension (pvals) 
     */
    p_set_handle_offsets(pvals,  &l_curr);
    
    /* render: add source layer to (temporary) preview image */
    p_mov_render(pvals->tmp_image_id, pvals, &l_curr);

    if(l_curr.src_layers != NULL) g_free(l_curr.src_layers);
  }

  /* flatten image, and get the (only) resulting drawable */
  return(p_get_flattened_drawable(pvals->tmp_image_id));

}	/* end p_get_prevw_drawable */


/* ============================================================================
 * p_set_handle_offsets
 *  set handle offsets according to handle mode and src image dimensions
 * ============================================================================
 */
void p_set_handle_offsets(t_mov_values *val_ptr, t_mov_current *cur_ptr)
{
  guint    l_src_width, l_src_height;         /* dimensions of the source image */

   /* get dimensions of source image */
   l_src_width  = gimp_image_width(val_ptr->src_image_id);
   l_src_height = gimp_image_height(val_ptr->src_image_id);

   cur_ptr->l_handleX = 0.0;
   cur_ptr->l_handleY = 0.0;
   switch(val_ptr->src_handle)
   {
      case GAP_HANDLE_LEFT_BOT:
         cur_ptr->l_handleY += l_src_height;
         break;
      case GAP_HANDLE_RIGHT_TOP:
         cur_ptr->l_handleX += l_src_width;
         break;
      case GAP_HANDLE_RIGHT_BOT:
         cur_ptr->l_handleX += l_src_width;
         cur_ptr->l_handleY += l_src_height;
         break;
      case GAP_HANDLE_CENTER:
         cur_ptr->l_handleX += (l_src_width  / 2);
         cur_ptr->l_handleY += (l_src_height / 2);
         break;
      case GAP_HANDLE_LEFT_TOP:
      default:
         break;
   }
}	/* end p_set_handle_offsets */


#define BOUNDS(a,x,y)  ((a < x) ? x : ((a > y) ? y : a))

/* ============================================================================
 * p_mov_render
 * insert current source layer into image
 *    at current settings (position, size opacity ...)
 * ============================================================================
 */

  /*  why dont use: l_cp_layer_id = gimp_layer_copy(src_id);
   *  ==> Sorry this procedure works only for layers within the same image !!
   *  Workaround:
   *    use my 'private' version of layercopy
   */

gint
p_mov_render(gint32 image_id, t_mov_values *val_ptr, t_mov_current *cur_ptr)
{
  gint32       l_cp_layer_id;
  gint32       l_cp_layer_mask_id;
  gint         l_offset_x, l_offset_y;            /* new offsets within dest. image */
  gint         l_src_offset_x, l_src_offset_y;    /* layeroffsets as they were in src_image */
  guint        l_new_width;
  guint        l_new_height;
  guint        l_orig_width;
  guint        l_orig_height;
  gint         l_resized_flag;
  gint32       l_interpolation;   
  gint          lx1, ly1, lx2, ly2;
  guint        l_image_width;
  guint        l_image_height;
 
  if(gap_debug) printf("p_mov_render: frame/layer: %ld/%ld  X=%f, Y=%f\n"
                "       Width=%f Height=%f\n"
                "       Opacity=%f  Rotate=%f  clip_to_img = %d\n",
                     cur_ptr->dst_frame_nr, cur_ptr->src_layer_idx,
                     cur_ptr->currX, cur_ptr->currY,
                     cur_ptr->currWidth,
                     cur_ptr->currHeight,
                     cur_ptr->currOpacity,
                     cur_ptr->currRotation,
                     val_ptr->clip_to_img );



  /* make a copy of the current source layer
   * (using current opacity  & paintmode values)
   */
   l_cp_layer_id = p_my_layer_copy(image_id,
                                   cur_ptr->src_layers[cur_ptr->src_layer_idx],
                                   cur_ptr->currOpacity,
                                   val_ptr->src_paintmode,
                                   &l_src_offset_x,
                                   &l_src_offset_y);

  /* add the copied layer to current destination image */
  if(gap_debug) printf("p_mov_render: after layer copy layer_id=%d\n", (int)l_cp_layer_id);
  if(l_cp_layer_id < 0)
  {
     return -1;
  }  

  gimp_image_add_layer(image_id, l_cp_layer_id, 
                       val_ptr->dst_layerstack);

  if(gap_debug) printf("p_mov_render: after add layer\n");

  /* check for layermask */
  l_cp_layer_mask_id = gimp_layer_get_mask_id(l_cp_layer_id);
  if(l_cp_layer_mask_id >= 0)
  {
     /* apply the layermask
      *   some transitions (especially rotate) cant operate proper on
      *   layers with masks !
      *   (tests with gimp-rotate resulted in trashed images,
      *    even if the mask was rotated too)
      */
      gimp_image_remove_layer_mask(image_id, l_cp_layer_id, 0 /* 0==APPLY */ );
  }

  l_resized_flag = 0;
  l_orig_width  = gimp_layer_width(l_cp_layer_id);
  l_orig_height = gimp_layer_height(l_cp_layer_id);
  l_new_width  = l_orig_width;
  l_new_height = l_orig_height;
  
  if((cur_ptr->currWidth  > 100.01) || (cur_ptr->currWidth < 99.99)
  || (cur_ptr->currHeight > 100.01) || (cur_ptr->currHeight < 99.99))
  {
     /* have to scale layer */
     l_resized_flag = 1;

     l_new_width  = (l_orig_width  * cur_ptr->currWidth) / 100;
     l_new_height = (l_orig_height * cur_ptr->currHeight) / 100;
     gimp_layer_scale(l_cp_layer_id, l_new_width, l_new_height, 0);
     
  }

  if((cur_ptr->currRotation  > 0.5) || (cur_ptr->currRotation < -0.5))
  {
    l_resized_flag = 1;
    l_interpolation = 1;  /* rotate always with smoothing option turned on */

    /* have to rotate the layer (rotation also changes size as needed) */
    p_gimp_rotate(l_cp_layer_id, l_interpolation, cur_ptr->currRotation);

    
    l_new_width  = gimp_layer_width(l_cp_layer_id);
    l_new_height = gimp_layer_height(l_cp_layer_id);
  }
  
  if(l_resized_flag == 1)
  {
     /* adjust offsets according to handle and change of size */
     switch(val_ptr->src_handle)
     {
        case GAP_HANDLE_LEFT_BOT:
           l_src_offset_y += ((gint)l_orig_height - (gint)l_new_height);
           break;
        case GAP_HANDLE_RIGHT_TOP:
           l_src_offset_x += ((gint)l_orig_width - (gint)l_new_width);
           break;
        case GAP_HANDLE_RIGHT_BOT:
           l_src_offset_x += ((gint)l_orig_width - (gint)l_new_width);
           l_src_offset_y += ((gint)l_orig_height - (gint)l_new_height);
           break;
        case GAP_HANDLE_CENTER:
           l_src_offset_x += (((gint)l_orig_width - (gint)l_new_width) / 2);
           l_src_offset_y += (((gint)l_orig_height - (gint)l_new_height) / 2);
           break;
        case GAP_HANDLE_LEFT_TOP:
        default:
           break;
     }
  }
  
  /* calculate offsets in destination image */
  l_offset_x = (cur_ptr->currX - cur_ptr->l_handleX) + l_src_offset_x;
  l_offset_y = (cur_ptr->currY - cur_ptr->l_handleY) + l_src_offset_y;
  
  /* modify koordinate offsets of the copied layer within dest. image */
  gimp_layer_set_offsets(l_cp_layer_id, l_offset_x, l_offset_y);

  /* clip the handled layer to image size if desired */
  if(val_ptr->clip_to_img != 0)
  {
     l_image_width = gimp_image_width(image_id);
     l_image_height = gimp_image_height(image_id);
     
     lx1 = BOUNDS (l_offset_x, 0, l_image_width);
     ly1 = BOUNDS (l_offset_y, 0, l_image_height);
     lx2 = BOUNDS ((l_new_width + l_offset_x), 0, l_image_width);
     ly2 = BOUNDS ((l_new_height + l_offset_y), 0, l_image_height);

     l_new_width = lx2 - lx1;
     l_new_height = ly2 - ly1;
     
     if (l_new_width && l_new_height)
     {
       gimp_layer_resize(l_cp_layer_id, l_new_width, l_new_height,
			-(lx1 - l_offset_x),
			-(ly1 - l_offset_y));
     }
     else
     {
       /* no part of the layer is inside of the current frame (this image)
        * instead of removing we make the layer small and move him outside
        * the image.
        * (that helps to keep the layerstack position of the inserted layer(s)
        * constant in all handled anim_frames) 
        */
       gimp_layer_resize(l_cp_layer_id, 2, 2, -3, -3);
     }
  }

  if(gap_debug) printf("GAP p_mov_render: exit OK\n");
  
  return 0;
}	/* end p_mov_render */

/* ============================================================================
 * p_buildmenu
 *    build menu widget for all Items passed in the MenuItems Parameter
 *    MenuItems is an array of Pointers to Structure MenuItem.
 *    The End is marked by a Structure Member where the label is a NULL pointer
 *    (simplifyed version of GIMP 1.0.2 bulid_menu procedur)
 * ============================================================================
 */

GtkWidget *
p_buildmenu (MenuItem            *items)
{
  GtkWidget *menu;
  GtkWidget *menu_item;

  menu = gtk_menu_new ();

  while (items->label)
  {
      menu_item = gtk_menu_item_new_with_label (items->label);
      gtk_container_add (GTK_CONTAINER (menu), menu_item);

      if (items->callback)
	gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			    (GtkSignalFunc) items->callback,
			    items->user_data);

      gtk_widget_show (menu_item);
      items->widget = menu_item;

      items++;
  }

  return menu;
}	/* end p_buildmenu */
