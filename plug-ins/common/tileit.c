/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Tileit - This plugin will take an image an make repeated
 * copies of it the stepping is 1/(2**n); 1<=n<=6
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 * 
 * A fair proprotion of this code was taken from the Whirl plug-in
 * which was copyrighted by Federico Mena Quintero (as below).
 * 
 * Whirl plug-in --- distort an image into a whirlpool
 * Copyright (C) 1997 Federico Mena Quintero           
 *
 */

/* Change log:-
 * 0.2  Added new functions to allow "editing" of the tile patten.
 *
 * 0.1 First version released.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimp/gimplimits.h>

#include "libgimp/stdplugins-intl.h"

/***** Magic numbers *****/

#define PREVIEW_SIZE 128 
#define SCALE_WIDTH  80
#define ENTRY_WIDTH  25

#define MAX_SEGS 6

#define PREVIEW_MASK   GDK_EXPOSURE_MASK | \
                       GDK_BUTTON_PRESS_MASK | \
		       GDK_BUTTON_MOTION_MASK

/* Variables set in dialog box */
typedef struct data {
    gint numtiles;
} TileItVals;

typedef struct {
  GtkWidget *preview;
  guchar     preview_row[PREVIEW_SIZE * 4];
  gint run;
  guchar * pv_cache;
  gint img_bpp;
} TileItInterface;

static TileItInterface tint =
{
  NULL,  /* Preview */
  {'4','u'},    /* Preview_row */
  FALSE, /* run */
  NULL,
  4      /* bpp of drawable */
};

GDrawable *tileitdrawable;
static gint   tile_width, tile_height;
static GTile *the_tile = NULL;
static gint   img_width, img_height,img_bpp;

static void      query  (void);
static void      run    (gchar    *name,
			 gint      nparams,
			 GParam   *param,
			 gint     *nreturn_vals,
			 GParam  **return_vals);
/* static void      check  (GDrawable * drawable); */

static gint      tileit_dialog (void);
static void      tileit_ok_callback (GtkWidget *widget, gpointer   data);
static void      tileit_scale_update (GtkAdjustment *adjustment, gint *size_val);
static void      tileit_entry_update(GtkWidget *widget, gint *value);
static void      tileit_exp_update(GtkWidget *widget, gpointer value);
static void      tileit_exp_update_f(GtkWidget *widget, gpointer value);
static void      tileit_reset(GtkWidget *widget, gpointer value);
static void      tileit_toggle_update(GtkWidget *widget, gpointer   data);
static void      tileit_hvtoggle_update(GtkWidget *widget, gpointer   data);

static void      do_tiles(void);
static gint      tiles_xy(gint width, gint height,gint x,gint y,gint *nx,gint *ny);
static void      all_update(void);
static void      alt_update(void);
static void      explict_update(gint);

static void      dialog_update_preview(void);
static void	 cache_preview(void);
static gint      tileit_preview_expose ( GtkWidget *widget,GdkEvent *event );
static gint      tileit_preview_events ( GtkWidget *widget,GdkEvent *event );

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

/* Values when first invoked */
static TileItVals itvals =
{
  2
};

/* Structures for call backs... */
/* The "explict tile" & family */
typedef enum {
  ALL,
  ALT,
  EXPLICT
} AppliedTo;

typedef struct {
  AppliedTo type;
  gint x; /* X - pos of tile */
  gint y; /* Y - pos of tile */
  GtkWidget *r_label; /* row label */
  GtkWidget *r_entry; /* row entry */
  GtkWidget *c_label; /* column label */
  GtkWidget *c_entry; /* column entry */
  GtkWidget *applybut; /* The apply button */
} Exp_Call;

Exp_Call exp_call = {
  ALL,
  -1,
  -1,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
};

/* The reset button needs to know some toggle widgets.. */

typedef struct {
  GtkWidget *htoggle;
  GtkWidget *vtoggle;
} Reset_Call;

Reset_Call res_call = {
  NULL,
  NULL,
};
  
/* 2D - Array that holds the actions for each tile */
/* Action type on cell */
#define HORIZONTAL 0x1
#define VERTICAL   0x2

gint tileactions[MAX_SEGS][MAX_SEGS];

/* What actions buttons toggled */
gint do_horz = FALSE;
gint do_vert = FALSE;
gint opacity = 100;

/* Stuff for the preview bit */
static gint   sel_x1, sel_y1, sel_x2, sel_y2;
static gint   sel_width, sel_height;
static gint   preview_width, preview_height;
static gint   has_alpha;

MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "number_of_tiles", "Number of tiles to make" } 
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_small_tiles",
			  _("Tiles image into smaller versions of the orginal"),
			  "More here later",
			  "Andy Thomas",
			  "Andy Thomas",
			  "1997",
			  N_("<Image>/Filters/Map/Small Tiles..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run    (gchar    *name,
	gint      nparams,
	GParam   *param,
	gint     *nreturn_vals,
	GParam  **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  int           pwidth, pheight;


  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  tileitdrawable = 
    drawable = 
    gimp_drawable_get (param[2].data.d_drawable);

  tile_width  = gimp_tile_width();
  tile_height = gimp_tile_height();

  gimp_drawable_mask_bounds(drawable->id, &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width  = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;
  
  /* Calculate preview size */
  
  if (sel_width > sel_height) {
    pwidth  = MIN(sel_width, PREVIEW_SIZE);
    pheight = sel_height * pwidth / sel_width;
  } else {
    pheight = MIN(sel_height, PREVIEW_SIZE);
    pwidth  = sel_width * pheight / sel_height;
  }
  
  preview_width  = MAX(pwidth, 2);  /* Min size is 2 */
  preview_height = MAX(pheight, 2); 

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data ("plug_in_tileit", &itvals);
      if (! tileit_dialog())
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case RUN_NONINTERACTIVE:
      if (nparams != 4)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  itvals.numtiles = param[3].data.d_int32;
	}
      INIT_I18N();
      break;

    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      gimp_get_data ("plug_in_tileit", &itvals);
      break;

    default:
      break;
    }

  if (gimp_drawable_is_rgb (drawable->id) || gimp_drawable_is_gray (drawable->id))
    {
      /* Set the tile cache size */

      gimp_tile_cache_ntiles((drawable->width + gimp_tile_width() - 1) / gimp_tile_width());

      gimp_progress_init ( _("Tiling..."));

      do_tiles();
   
      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_tileit", &itvals, sizeof (TileItVals));
    }
  else
    {
      status = STATUS_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/* Build the dialog up. This was the hard part! */
static gint
tileit_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *frame;
  GtkWidget *xframe;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *slider;
  GtkObject *size_data;
  GtkObject *op_data;
  GtkWidget *toggle;
  GSList  *orientation_group = NULL;
  guchar  *color_cube;
  gchar **argv;
  gint    argc;
  gchar   buf[256];

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("tileit");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  /* Get the stuff for the preview window...*/
  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]);
  
  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  cache_preview (); /* Get the preview image and store it also set has_alpha */

  /* Start buildng the dialog up */
  dlg = gimp_dialog_new ( _("TileIt"), "tileit",
			 gimp_plugin_help_func, "filters/tileit.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), tileit_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gtk_frame_new ( _("Preview"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (TRUE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  xframe = gtk_frame_new (NULL);
  gtk_container_set_border_width (GTK_CONTAINER (xframe), 2);
  gtk_frame_set_shadow_type (GTK_FRAME (xframe), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox2), xframe, TRUE, FALSE, 0);
  gtk_widget_show (xframe);

  tint.preview = gtk_preview_new (GTK_PREVIEW_COLOR);

  gtk_widget_set_events (GTK_WIDGET (tint.preview), PREVIEW_MASK);
  gtk_signal_connect_after (GTK_OBJECT (tint.preview), "expose_event",
			    (GtkSignalFunc) tileit_preview_expose,
			    NULL);
  gtk_signal_connect (GTK_OBJECT (tint.preview), "event",
		      (GtkSignalFunc) tileit_preview_events,
		      NULL);

  gtk_preview_size (GTK_PREVIEW (tint.preview), preview_width, preview_height);
  gtk_container_add (GTK_CONTAINER (xframe), tint.preview);

  /* Area for buttons etc */

  frame = gtk_frame_new ( _("Flipping"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  toggle = gtk_check_button_new_with_label ( _("Horizontal"));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) tileit_hvtoggle_update,
		      &do_horz);
  gtk_widget_show (toggle);
  res_call.htoggle = toggle;

  toggle = gtk_check_button_new_with_label ( _("Vertical"));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) tileit_hvtoggle_update,
		      &do_vert);
  gtk_widget_show (toggle);
  res_call.vtoggle = toggle;

  xframe = gtk_frame_new( _("Applied to Tile"));
  gtk_frame_set_shadow_type (GTK_FRAME (xframe), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), xframe, FALSE, FALSE, 0);
  gtk_widget_show(xframe);

  /* Table for the inner widgets..*/
  table = gtk_table_new (5, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (xframe), table);
  gtk_widget_show (table);

  toggle = gtk_radio_button_new_with_label (orientation_group, _("All Tiles"));
  orientation_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 4, 0, 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) tileit_toggle_update,
		      (gpointer)ALL);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (orientation_group,
					    _("Alternate Tiles"));
  orientation_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 4, 1, 2,
		    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) tileit_toggle_update,
		      (gpointer)ALT);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (orientation_group,
                                            _("Explicit Tile"));
  orientation_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));  
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 2, 4,
		    GTK_FILL | GTK_SHRINK, GTK_FILL, 0, 0);
  gtk_widget_show (toggle);

  label = gtk_label_new (_("Row:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 2, 3,
		    GTK_FILL | GTK_SHRINK , GTK_FILL, 0, 0);
  gtk_widget_show (label); 
  gtk_widget_set_sensitive (label, FALSE);
  exp_call.r_label = label;

  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  g_snprintf(buf, sizeof (buf), "%.1d", 2);
  gtk_entry_set_text (GTK_ENTRY (entry), buf);
  gtk_widget_show (entry);
  gtk_table_attach (GTK_TABLE (table), entry, 2, 3, 2, 3,
		    GTK_FILL | GTK_SHRINK, GTK_FILL, 0, 0);
  gtk_widget_set_sensitive (entry, FALSE);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) tileit_exp_update_f,
		      &exp_call);
  exp_call.r_entry = entry;

  label = gtk_label_new ( _("Column:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_widget_show (label); 
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 3, 4,
		    GTK_FILL , GTK_FILL, 0, 0);
  gtk_widget_set_sensitive (label, FALSE);
  exp_call.c_label = label;

  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  g_snprintf (buf, sizeof (buf), "%.1d", 2);
  gtk_entry_set_text (GTK_ENTRY (entry), buf);
  gtk_widget_show (entry);
  gtk_widget_set_sensitive (entry, FALSE);
  gtk_table_attach (GTK_TABLE (table), entry, 2, 3, 3, 4,
		    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc) tileit_exp_update_f,
		     &exp_call);
  exp_call.c_entry = entry;

  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) tileit_toggle_update,
		      (gpointer)EXPLICT);

  button = gtk_button_new_with_label ( _("Apply"));
  gtk_widget_set_sensitive (button, FALSE);
  gtk_table_attach (GTK_TABLE (table), button, 3, 4, 2, 4, 0, 0, 0, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) tileit_exp_update,
		      (gpointer)&exp_call);
  exp_call.applybut = button;

  /* Widget for selecting the Opacity */
  label = gtk_label_new ( _("Opacity:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
		    GTK_FILL | GTK_SHRINK, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  op_data = gtk_adjustment_new (100, 0, 100, 1, 1, 0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (op_data));
  gtk_widget_set_usize (slider, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), slider, 1 ,3, 4, 5,
		    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);

  gtk_signal_connect (GTK_OBJECT (op_data), "value_changed",
		      (GtkSignalFunc) tileit_scale_update,
		      &opacity);
  if (!has_alpha)
    gtk_widget_set_sensitive (slider,FALSE);
  gtk_widget_show (slider);

  entry = gtk_entry_new ();
  gtk_object_set_user_data (GTK_OBJECT (entry), op_data);
  gtk_object_set_user_data (op_data, entry);
  gtk_widget_set_usize (entry, 3 * ENTRY_WIDTH / 2, 0);
  g_snprintf (buf, sizeof (buf), "%.1d", opacity);
  gtk_entry_set_text (GTK_ENTRY (entry), buf);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) tileit_entry_update,
		      &opacity);
  gtk_table_attach (GTK_TABLE (table), entry, 3, 4, 4, 5,
		    GTK_FILL, GTK_FILL, 0, 0);
  if (!has_alpha)
    gtk_widget_set_sensitive (entry,FALSE);
  gtk_widget_show (entry);

  button = gtk_button_new_with_label ( _("Reset"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) tileit_reset,
		     (gpointer)&res_call);

  gtk_widget_show(frame); 

  /* Lower frame saying howmany segments */

  frame = gtk_frame_new ( _("Segment Setting"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  g_snprintf (buf, sizeof (buf), "1 / (%d ** n) ", 2);
  label = gtk_label_new (buf);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  size_data = gtk_adjustment_new (itvals.numtiles, 2, MAX_SEGS, 1, 1, 0);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (size_data));
  gtk_widget_set_usize (slider, SCALE_WIDTH, 0);
  gtk_box_pack_start (GTK_BOX (hbox), slider, TRUE, TRUE, 0);
  gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);
  gtk_signal_connect (GTK_OBJECT (size_data), "value_changed",
		      (GtkSignalFunc) tileit_scale_update,
		      &itvals.numtiles);
  gtk_widget_show (slider);

  entry = gtk_entry_new();
  gtk_object_set_user_data (GTK_OBJECT (entry), size_data);
  gtk_object_set_user_data (size_data, entry);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  g_snprintf (buf, sizeof (buf), "%.1d", itvals.numtiles);
  gtk_entry_set_text (GTK_ENTRY (entry), buf);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) tileit_entry_update,
		      &itvals.numtiles);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show(entry);

  gtk_widget_show (tint.preview);

  gtk_widget_show (dlg);
  dialog_update_preview ();

  gtk_main ();
  gdk_flush ();

  return tint.run;
}

static void
tileit_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  tint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
tileit_hvtoggle_update(GtkWidget *widget,
                      gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
  {
    /* Only do for event that sets a toggle button to true */
    /* This will break if any more toggles are added? */
    *toggle_val = TRUE;
  }
  else
    *toggle_val = FALSE;

  switch(exp_call.type)
    {
    case ALL:
      /* Clear current settings */
      memset(tileactions,0,sizeof(tileactions));
      all_update();
      break;
    case ALT:
      /* Clear current settings */
      memset(tileactions,0,sizeof(tileactions));
      alt_update();
      break;
    case EXPLICT:
      break;
    }
  dialog_update_preview();
}

static void 
draw_explict_sel(void)
{
  if(exp_call.type == EXPLICT)
    {
      gdouble x,y;
      gdouble width = (gdouble)preview_width/(gdouble)itvals.numtiles;
      gdouble height = (gdouble)preview_height/(gdouble)itvals.numtiles;

      x = width*(exp_call.x - 1);
      y = height*(exp_call.y - 1);

      gdk_gc_set_function ( tint.preview->style->black_gc, GDK_INVERT);

      gdk_draw_rectangle(tint.preview->window,
			 tint.preview->style->black_gc,
			 0,
			 (gint)x,
			 (gint)y,
			 (gint)width,
			 (gint)height);
      gdk_draw_rectangle(tint.preview->window,
			 tint.preview->style->black_gc,
			 0,
			 (gint)x+1,
			 (gint)y+1,
			 (gint)width-2,
			 (gint)height-2);
      gdk_draw_rectangle(tint.preview->window,
			 tint.preview->style->black_gc,
			 0,
			 (gint)x+2,
			 (gint)y+2,
			 (gint)width-4,
			 (gint)height-4);
      gdk_gc_set_function ( tint.preview->style->black_gc, GDK_COPY);

    }
}

static gint
tileit_preview_expose( GtkWidget *widget,
			    GdkEvent *event )
{
  draw_explict_sel();
  return FALSE;
}

static void
exp_need_update(gint nx, gint ny)
{
  gchar buf[256];

  if (nx <= 0 || nx > itvals.numtiles || ny <= 0 || ny > itvals.numtiles)
    return;

  if( nx != exp_call.x ||
       ny != exp_call.y )
    {
      draw_explict_sel(); /* Clear old 'un */
      exp_call.x = nx;
      exp_call.y = ny;
      draw_explict_sel();
      
      sprintf(buf,"%d",nx);
      gtk_signal_handler_block_by_data(GTK_OBJECT(exp_call.c_entry),&exp_call);
      gtk_entry_set_text(GTK_ENTRY(exp_call.c_entry), buf);
      gtk_signal_handler_unblock_by_data(GTK_OBJECT(exp_call.c_entry), &exp_call);
      sprintf(buf,"%d",ny);
      gtk_signal_handler_block_by_data(GTK_OBJECT(exp_call.r_entry),&exp_call);
      gtk_entry_set_text(GTK_ENTRY(exp_call.r_entry), buf);
      gtk_signal_handler_unblock_by_data(GTK_OBJECT(exp_call.r_entry), &exp_call);
    }
}

static gint
tileit_preview_events ( GtkWidget *widget,
			     GdkEvent *event )
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint nx,ny;
  gint twidth = preview_width/itvals.numtiles;
  gint theight = preview_height/itvals.numtiles;

  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      nx = bevent->x/twidth + 1;
      ny = bevent->y/theight + 1;
      exp_need_update(nx,ny);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      if ( !mevent->state ) 
	break;
      if(mevent->x < 0 || mevent->y < 0)
	break;
      nx = mevent->x/twidth + 1;
      ny = mevent->y/theight + 1;
      exp_need_update(nx,ny);
      break;

    default:
      break;
    }

  return FALSE;
}

static void 
explict_update(gint settile)
{
  int x,y;

  /* Make sure bounds are OK */
  y = atoi(gtk_entry_get_text(GTK_ENTRY(exp_call.r_entry)));
  if(y > itvals.numtiles || y <= 0)
    {
      y = itvals.numtiles;
    }
  x = atoi(gtk_entry_get_text(GTK_ENTRY(exp_call.c_entry)));
  if(x > itvals.numtiles || x <= 0)
    {
      x = itvals.numtiles;
    }

  /* Set it */
  if(settile == TRUE)
    tileactions[x-1][y-1] = (((do_horz)?HORIZONTAL:0)|((do_vert)?VERTICAL:0));

  exp_call.x = x;
  exp_call.y = y;

}

static void 
all_update(void)
{
  int x,y;
  for(x = 0 ; x < MAX_SEGS; x++)
    for(y = 0 ; y < MAX_SEGS; y++)
      tileactions[x][y] |= (((do_horz)?HORIZONTAL:0)|((do_vert)?VERTICAL:0));
}

static void
alt_update(void)
{
  int x,y;
  for(x = 0 ; x < MAX_SEGS; x++)
    for(y = 0 ; y < MAX_SEGS; y++)
      if(!((x+y)%2))
	tileactions[x][y] |= 
	  (((do_horz)?HORIZONTAL:0)|((do_vert)?VERTICAL:0));
}

static void
tileit_toggle_update(GtkWidget *widget,
                      gpointer   data)
{
  AppliedTo type = (AppliedTo)data;
  
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      switch(type)
	{
	case ALL:
	  /* Clear current settings */
	  memset(tileactions,0,sizeof(tileactions));
	  all_update();
	  break;
	case ALT:
	  /* Clear current settings */
	  memset(tileactions,0,sizeof(tileactions));
	  alt_update();
	  break;
	case EXPLICT:
	  /* Make widget active */
	  gtk_widget_set_sensitive(exp_call.r_label,TRUE);
	  gtk_widget_set_sensitive(exp_call.r_entry,TRUE);
	  gtk_widget_set_sensitive(exp_call.c_label,TRUE);
	  gtk_widget_set_sensitive(exp_call.c_entry,TRUE);
	  gtk_widget_set_sensitive(exp_call.applybut,TRUE);
	  explict_update(FALSE);
	  break;
	}
      exp_call.type = type;
    }
  else
    {
      switch(type)
	{
	case ALL:
	  break;
	case ALT:
	  break;
	case EXPLICT:
	  gtk_widget_set_sensitive(exp_call.r_label,FALSE);
	  gtk_widget_set_sensitive(exp_call.r_entry,FALSE);
	  gtk_widget_set_sensitive(exp_call.c_label,FALSE);
	  gtk_widget_set_sensitive(exp_call.c_entry,FALSE);
	  gtk_widget_set_sensitive(exp_call.applybut,FALSE);
	  break;
	}
    }

  dialog_update_preview();
}                  


static void
tileit_scale_update(GtkAdjustment *adjustment, gint *value)
{
  GtkWidget *entry;
  char       buf[256];
  
  if (*value != adjustment->value) {
    *value = adjustment->value;
    
    entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
    sprintf(buf,"%d",*value);
    
    gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
    gtk_entry_set_text(GTK_ENTRY(entry), buf);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);
    
    dialog_update_preview();
  }
} 


static void
tileit_reset(GtkWidget *widget, gpointer data)
{
  Reset_Call *r = (Reset_Call *)data;

  memset(tileactions,0,sizeof(tileactions));

  gtk_signal_handler_block_by_data(GTK_OBJECT(r->htoggle),&do_horz);
  gtk_signal_handler_block_by_data(GTK_OBJECT(r->vtoggle),&do_vert);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(r->htoggle),FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(r->vtoggle),FALSE);
  gtk_signal_handler_unblock_by_data(GTK_OBJECT(r->htoggle),&do_horz);
  gtk_signal_handler_unblock_by_data(GTK_OBJECT(r->vtoggle),&do_vert);
  do_horz = do_vert = FALSE; 

  dialog_update_preview();
} 


/* Could avoid almost dup. functions by using a field in the data 
 * passed.  Must still pass the data since used in sig blocking func.
 */

static void
tileit_exp_update(GtkWidget *widget, gpointer applied)
{
  explict_update(TRUE);
  dialog_update_preview();
} 


static void
tileit_exp_update_f(GtkWidget *widget, gpointer applied)
{
  explict_update(FALSE);
  dialog_update_preview();
} 

static void
tileit_entry_update(GtkWidget *widget, gint *value)
{
  GtkAdjustment *adjustment;
  gdouble        new_value;
  
  new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));
  
  if (*value != new_value) {
    adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));
    
    if ((new_value >= adjustment->lower) &&
	(new_value <= adjustment->upper)) {
      *value            = new_value;
      adjustment->value = new_value;
      
      gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
      
      dialog_update_preview();
    } 
  } 
} 


/* Cache the preview image - updates are a lot faster. */
/* The preview_cache will contain the small image */

static void
cache_preview()
{
  GPixelRgn src_rgn;
  int y,x;
  guchar *src_rows;
  guchar *p;
  int isgrey = 0;

  gimp_pixel_rgn_init(&src_rgn,tileitdrawable,sel_x1,sel_y1,sel_width,sel_height,FALSE,FALSE);

  src_rows = g_new(guchar ,sel_width*4); 
  p = tint.pv_cache = g_new(guchar ,preview_width*preview_height*4);

  img_width  = gimp_drawable_width(tileitdrawable->id);
  img_height = gimp_drawable_height(tileitdrawable->id);

  tint.img_bpp = gimp_drawable_bpp(tileitdrawable->id);   

  has_alpha = gimp_drawable_has_alpha(tileitdrawable->id);

  if(tint.img_bpp < 3)
    {
      tint.img_bpp = 3 + has_alpha;
    }

  switch ( gimp_drawable_type (tileitdrawable->id) )
    {
    case GRAYA_IMAGE:
    case GRAY_IMAGE:
      isgrey = 1;
      break;
    default:
      isgrey = 0;
      break;
    }

  for (y = 0; y < preview_height; y++) {
  
    gimp_pixel_rgn_get_row(&src_rgn,
			   src_rows,
			   sel_x1,
			   sel_y1 + (y*sel_height)/preview_height,
			   sel_width);
      
    for (x = 0; x < (preview_width); x ++) {
      /* Get the pixels of each col */
      int i;
      for (i = 0 ; i < 3; i++ )
	p[x*tint.img_bpp+i] = src_rows[((x*sel_width)/preview_width)*src_rgn.bpp +((isgrey)?0:i)]; 
      if(has_alpha)
	p[x*tint.img_bpp+3] = src_rows[((x*sel_width)/preview_width)*src_rgn.bpp + ((isgrey)?1:3)];
    }
    p += (preview_width*tint.img_bpp);
  }
  g_free(src_rows);
}


static void
tileit_get_pixel(int x, int y, guchar *pixel)
{
  static gint row  = -1;
  static gint col  = -1;
  
  gint    newcol, newrow;
  gint    newcoloff, newrowoff;
  guchar *p;
  int     i;
  
  if ((x < 0) || (x >= img_width) || (y < 0) || (y >= img_height)) {
    pixel[0] = 0;
    pixel[1] = 0;
    pixel[2] = 0;
    pixel[3] = 0;
    
    return;
  }
  
  newcol    = x / tile_width;
  newcoloff = x % tile_width;
  newrow    = y / tile_height;
  newrowoff = y % tile_height;
  
  if ((col != newcol) || (row != newrow) || (the_tile == NULL)) {
    if (the_tile != NULL)
      gimp_tile_unref(the_tile, FALSE);
    
    the_tile = gimp_drawable_get_tile(tileitdrawable, FALSE, newrow, newcol);
    gimp_tile_ref(the_tile);
    
    col = newcol;
    row = newrow;
  } 
  
  p = the_tile->data + the_tile->bpp * (the_tile->ewidth * newrowoff + newcoloff);
  
  for (i = img_bpp; i; i--)
    *pixel++ = *p++;
}


static void
do_tiles(void)
{
  GPixelRgn dest_rgn;
  gpointer  pr;
  gint      progress, max_progress;
  guchar   *dest_row;
  guchar   *dest;
  gint      row, col;
  guchar    pixel[4];
  int 	    nc,nr;
  int       i;
  
  /* Initialize pixel region */
  
  gimp_pixel_rgn_init(&dest_rgn, tileitdrawable, sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);
  
  progress     = 0;
  max_progress = sel_width * sel_height;
  
  img_bpp = gimp_drawable_bpp(tileitdrawable->id);
  
  for (pr = gimp_pixel_rgns_register(1, &dest_rgn);
       pr != NULL; pr = gimp_pixel_rgns_process(pr)) {
    dest_row = dest_rgn.data;
    
    for (row = dest_rgn.y; row < (dest_rgn.y + dest_rgn.h); row++) {
      dest = dest_row;
      
      for (col = dest_rgn.x; col < (dest_rgn.x + dest_rgn.w); col++)
	{
	  int an_action;
	  
	  an_action = 
	    tiles_xy(sel_width,
		     sel_height,
		     col-sel_x1,row-sel_y1,
		     &nc,&nr);
	  tileit_get_pixel(nc+sel_x1,nr+sel_y1,pixel);
	  for (i = 0; i < img_bpp; i++)
	    *dest++ = pixel[i];
	  
	  if(an_action && has_alpha)
	    {
	      dest--;
	      *dest = ((*dest)*opacity)/100;
	      dest++;
	    }
	}
      dest_row += dest_rgn.rowstride;
    } 
    
    progress += dest_rgn.w * dest_rgn.h;
    gimp_progress_update((double) progress / max_progress);
  }
  
  if (the_tile != NULL) {
    gimp_tile_unref(the_tile, FALSE);
    the_tile = NULL;
  }
  
  gimp_drawable_flush(tileitdrawable);
  gimp_drawable_merge_shadow(tileitdrawable->id, TRUE);
  gimp_drawable_update(tileitdrawable->id, sel_x1, sel_y1, sel_width, sel_height);
} 


/* Get the xy pos and any action */
static gint
tiles_xy(gint width,
	 gint height,
	 gint x,
	 gint y,
	 gint *nx,
	 gint *ny)
{
  gint px,py;
  gint rnum,cnum; 
  gint actiontype;
  gdouble rnd = 1 - (1.0/(gdouble)itvals.numtiles) +0.01;

  rnum = y*itvals.numtiles/height;

  py = (y*itvals.numtiles)%height;
  px = (x*itvals.numtiles)%width; 
  cnum = x*itvals.numtiles/width;
      
  if((actiontype = tileactions[cnum][rnum]))
    {
      if(actiontype & HORIZONTAL)
	{
	  gdouble pyr;
	  pyr =  height - y - 1 + rnd;
	  py = ((int)(pyr*(gdouble)itvals.numtiles))%height;
	}
      
      if(actiontype & VERTICAL)
	{
	  gdouble pxr;
	  pxr = width - x - 1 + rnd;
	  px = ((int)(pxr*(gdouble)itvals.numtiles))%width; 
	}
    }
  
  *nx = px;
  *ny = py;

  return(actiontype);
}


/* Given a row then srink it down a bit */
static void
do_tiles_preview(guchar *dest_row, 
	    guchar *src_rows,
	    gint width,
	    gint dh,
	    gint height,
	    gint bpp)
{
  gint x;
  gint i;
  gint px,py;
  gint rnum,cnum; 
  gint actiontype;
  gdouble rnd = 1 - (1.0/(gdouble)itvals.numtiles) +0.01;

  rnum = dh*itvals.numtiles/height;

  for (x = 0; x < width; x ++) 
    {
      
      py = (dh*itvals.numtiles)%height;
      
      px = (x*itvals.numtiles)%width; 
      cnum = x*itvals.numtiles/width;
      
      if((actiontype = tileactions[cnum][rnum]))
	{
	  if(actiontype & HORIZONTAL)
	    {
	      gdouble pyr;
	      pyr =  height - dh - 1 + rnd;
	      py = ((int)(pyr*(gdouble)itvals.numtiles))%height;
	    }
	  
	  if(actiontype & VERTICAL)
	    {
	      gdouble pxr;
	      pxr = width - x - 1 + rnd;
	      px = ((int)(pxr*(gdouble)itvals.numtiles))%width; 
	    }
	}

      for (i = 0 ; i < bpp; i++ )
	dest_row[x*tint.img_bpp+i] = 
	  src_rows[(px + (py*width))*bpp+i]; 

      if(has_alpha && actiontype)
	dest_row[x*tint.img_bpp + (bpp - 1)] = 
	  (dest_row[x*tint.img_bpp + (bpp - 1)]*opacity)/100;

    }
}

static void
dialog_update_preview(void)
{

  int     y;
  gint check,check_0,check_1;  

  for (y = 0; y < preview_height; y++) {
    
    if ((y / GIMP_CHECK_SIZE) & 1) {
      check_0 = GIMP_CHECK_DARK * 255;
      check_1 = GIMP_CHECK_LIGHT * 255;
    } else {
      check_0 = GIMP_CHECK_LIGHT * 255;
      check_1 = GIMP_CHECK_DARK * 255;
    }

    do_tiles_preview(tint.preview_row,
		tint.pv_cache,
		preview_width,
		y,
		preview_height,
		tint.img_bpp);

    if(tint.img_bpp > 3)
      {
	int i,j;
	for (i = 0, j = 0 ; i < sizeof(tint.preview_row); i += 4, j += 3 )
	  {
	    gint alphaval;
	    if (((i/4) / GIMP_CHECK_SIZE) & 1)
	      check = check_0;
	    else
	      check = check_1;
	    
	    alphaval = tint.preview_row[i + 3];
	    
	    tint.preview_row[j] = 
	      check + (((tint.preview_row[i] - check)*alphaval)/255);
	    tint.preview_row[j + 1] = 
	      check + (((tint.preview_row[i + 1] - check)*alphaval)/255);
	    tint.preview_row[j + 2] = 
	      check + (((tint.preview_row[i + 2] - check)*alphaval)/255);
	  }
      }
    
    gtk_preview_draw_row(GTK_PREVIEW(tint.preview), tint.preview_row, 0, y, preview_width);
  }

  draw_explict_sel();
  gtk_widget_draw(tint.preview, NULL);
  gdk_flush();
}


