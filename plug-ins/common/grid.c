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

/* Original plug-in coded by Tim Newsome. 
 * 
 * Changed to make use of real-life units by Sven Neumann <sven@gimp.org>.
 * 
 * The interface code is heavily commented in the hope that it will
 * help other plug-in developers to adapt their plug-ins to make use
 * of the gimp_size_entry functionality.
 *
 * For more info see libgimp/gimpsizeentry.h
 */

#include <stdlib.h>
#include <stdio.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "libgimp/stdplugins-intl.h"


#define  SPIN_BUTTON_WIDTH   75
#define COLOR_BUTTON_WIDTH   55

/* Declare local functions. */
static void query  (void);
static void run    (char    *name,
		    int      nparams,
		    GParam  *param,
		    int     *nreturn_vals,
		    GParam **return_vals);

static gint dialog (gint32 image_ID, GDrawable *drawable);
static void doit   (GDrawable *drawable);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

gint sx1, sy1, sx2, sy2;
gint run_flag = FALSE;

typedef struct
{
  gint   hwidth;
  gint   hspace;
  gint   hoffset;
  guint8 hcolor[4];
  gint   vwidth;
  gint   vspace;
  gint   voffset;
  guint8 vcolor[4];
  gint   iwidth;
  gint   ispace;
  gint   ioffset;
  guint8 icolor[4];
}
Config;

Config grid_cfg =
{
  1, 16, 8, { 0, 0, 128, 255 },    /* horizontal   */
  1, 16, 8, { 0, 0, 128, 255 },    /* vertical     */
  0,  2, 6, { 0, 0, 255, 255 },    /* intersection */
};


MAIN ()

static 
void query (void)
{
  static GParamDef args[] =
  {
    {PARAM_INT32,    "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE,    "image",    "Input image"},
    {PARAM_DRAWABLE, "drawable", "Input drawable"},

    {PARAM_INT32,    "hwidth",   "Horizontal Width"},
    {PARAM_INT32,    "hspace",   "Horizontal Spacing"},
    {PARAM_INT32,    "hoffset",  "Horizontal Offset"},
    {PARAM_COLOR,    "hcolor",   "Horizontal Colour"},
    {PARAM_INT8,     "hopacity", "Horizontal Opacity (0...255)"},

    {PARAM_INT32,    "vwidth",   "Vertical Width"},
    {PARAM_INT32,    "vspace",   "Vertical Spacing"},
    {PARAM_INT32,    "voffset",  "Vertical Offset"},
    {PARAM_COLOR,    "vcolor",   "Vertical Colour"},
    {PARAM_INT8,     "vopacity", "Vertical Opacity (0...255)"},

    {PARAM_INT32,    "iwidth",   "Intersection Width"},
    {PARAM_INT32,    "ispace",   "Intersection Spacing"},
    {PARAM_INT32,    "ioffset",  "Intersection Offset"},
    {PARAM_COLOR,    "icolor",   "Intersection Colour"},
    {PARAM_INT8,     "iopacity", "Intersection Opacity (0...255)"},
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_grid",
			  "Draws a grid.",
			  "",
			  "Tim Newsome",
			  "Tim Newsome, Sven Neumann, Tom Rathborne",
			  "1997, 1999",
			  "<Image>/Filters/Render/Grid",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char    *name, 
     int      n_params, 
     GParam  *param, 
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  gint32 image_ID;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  INIT_I18N_UI(); 

  run_mode = param[0].data.d_int32;
  image_ID = param[1].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  if (run_mode == RUN_NONINTERACTIVE)
    {
      if (n_params != 18)
	status = STATUS_CALLING_ERROR;

      if( status == STATUS_SUCCESS)
	{
	  grid_cfg.hwidth    = param[3].data.d_int32;
	  grid_cfg.hspace    = param[4].data.d_int32;
	  grid_cfg.hoffset   = param[5].data.d_int32;
	  grid_cfg.hcolor[0] = param[6].data.d_color.red;
	  grid_cfg.hcolor[1] = param[6].data.d_color.green;
	  grid_cfg.hcolor[2] = param[6].data.d_color.blue;
	  grid_cfg.hcolor[3] = param[7].data.d_int8;

	  grid_cfg.vwidth    = param[8].data.d_int32;
	  grid_cfg.vspace    = param[9].data.d_int32;
	  grid_cfg.voffset   = param[10].data.d_int32;
	  grid_cfg.vcolor[0] = param[11].data.d_color.red;
	  grid_cfg.vcolor[1] = param[11].data.d_color.green;
	  grid_cfg.vcolor[2] = param[11].data.d_color.blue;
	  grid_cfg.vcolor[3] = param[12].data.d_int8;

	  grid_cfg.iwidth    = param[13].data.d_int32;
	  grid_cfg.ispace    = param[14].data.d_int32;
	  grid_cfg.ioffset   = param[15].data.d_int32;
	  grid_cfg.icolor[0] = param[16].data.d_color.red;
	  grid_cfg.icolor[1] = param[16].data.d_color.green;
	  grid_cfg.icolor[2] = param[16].data.d_color.blue;
	  grid_cfg.icolor[3] = param[17].data.d_int8;
	}
    }
  else
    {
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_grid", &grid_cfg);
    }

  if (run_mode == RUN_INTERACTIVE)
    {
      if (!dialog (image_ID, drawable))
	{
	  /* The dialog was closed, or something similarly evil happened. */
	  status = STATUS_EXECUTION_ERROR;
	}
    }

  if (grid_cfg.hspace <= 0 || grid_cfg.vspace <= 0)
    {
      status = STATUS_EXECUTION_ERROR;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id) || gimp_drawable_is_gray (drawable->id))
	{
	  gimp_progress_init (_("Drawing Grid..."));
	  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

	  doit (drawable);

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_grid", &grid_cfg, sizeof (grid_cfg));
	}
      else
	{
	  status = STATUS_EXECUTION_ERROR;
	}
      gimp_drawable_detach (drawable);
    }

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}


G_INLINE_FUNC void
pix_composite (guchar *p1, 
	       guchar  p2[4], 
	       int     bytes, 
	       int     alpha)
{
 int b;

 if (alpha)
   {
     bytes--;
   }

 for (b = 0; b < bytes; b++)
   {
     *p1 = *p1 * (1.0 - p2[3]/255.0) + p2[b] * p2[3]/255.0;
     p1++;
   }

 if (alpha && *p1 < 255)
   {
     b = *p1 + 255.0 * ((double)p2[3] / (255.0 - *p1));
     *p1 = b > 255 ? 255 : b;
   }
}

static void
doit (GDrawable * drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height, bytes;
  guchar *dest;
  int x, y, alpha;
  
  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  gimp_drawable_mask_bounds (drawable->id, &sx1, &sy1, &sx2, &sy2);

  /* Get the size of the input image. 
   *  (This will/must be the same as the size of the output image.)
   */
  width  = drawable->width;
  height = drawable->height;
  bytes  = drawable->bpp;
  alpha  = gimp_drawable_has_alpha (drawable->id);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  dest = malloc (width * bytes);
  for (y = sy1; y < sy2; y++)
    {
      gimp_pixel_rgn_get_row (&srcPR, dest, sx1, y, (sx2-sx1));

      if (((y - grid_cfg.voffset + height) % grid_cfg.vspace) < grid_cfg.vwidth )
	{ /* Draw row */
	  for (x = sx1; x < sx2; x++)
	    {
	      pix_composite ( &dest[(x-sx1) * bytes], grid_cfg.hcolor, bytes, alpha);
	    }
	}

      if (((y - grid_cfg.voffset + height + ((grid_cfg.iwidth - grid_cfg.vwidth) / 2)) % grid_cfg.vspace) < grid_cfg.iwidth )
        { /* Draw irow */
	  for (x = sx1; x < sx2; x++)
	    {
              if ((((x - grid_cfg.hoffset + width - (grid_cfg.hwidth /2)) % grid_cfg.hspace) >= grid_cfg.ispace
                && ((x - grid_cfg.hoffset + width - (grid_cfg.hwidth /2)) % grid_cfg.hspace) < grid_cfg.ioffset)
                || (abs(((x - grid_cfg.hoffset + width - (grid_cfg.hwidth /2)) % grid_cfg.hspace) - grid_cfg.hspace) >= grid_cfg.ispace
                && (abs(((x - grid_cfg.hoffset + width - (grid_cfg.hwidth /2)) % grid_cfg.hspace) - grid_cfg.hspace) < grid_cfg.ioffset))) {
	          pix_composite ( &dest[(x-sx1) * bytes], grid_cfg.icolor, bytes, alpha);
                }
	    }
        }

      for (x = sx1; x < sx2; x++)
        {
          if (((x - grid_cfg.hoffset + width) % grid_cfg.hspace) < grid_cfg.hwidth )
            {
	      pix_composite ( &dest[(x-sx1) * bytes], grid_cfg.vcolor, bytes, alpha);
            }

          if ((((x - grid_cfg.hoffset + width + ((grid_cfg.iwidth - grid_cfg.hwidth)/ 2)) % grid_cfg.hspace) < grid_cfg.iwidth)
            && (( (((y - grid_cfg.voffset + height - (grid_cfg.vwidth / 2)) % grid_cfg.vspace) >= grid_cfg.ispace)
               && (((y - grid_cfg.voffset + height - (grid_cfg.vwidth / 2)) % grid_cfg.vspace) < grid_cfg.ioffset))
            || (( (abs(((y - grid_cfg.voffset + height - (grid_cfg.vwidth / 2)) % grid_cfg.vspace) - grid_cfg.vspace) >= grid_cfg.ispace)
               && (abs(((y - grid_cfg.voffset + height - (grid_cfg.vwidth / 2)) % grid_cfg.vspace) - grid_cfg.vspace) < grid_cfg.ioffset))
            ))
          )
            {
	      pix_composite ( &dest[(x-sx1) * bytes], grid_cfg.icolor, bytes, alpha);
            }
        }

      gimp_pixel_rgn_set_row (&destPR, dest, sx1, y, (sx2-sx1) );
      gimp_progress_update ((double) y / (double) (sy2 - sy1));
    }
  free (dest);

  /*  update the timred region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, sx1, sy1, sx2 - sx1, sy2 - sy1);
}

/***************************************************
 * GUI stuff
 */

static void
close_callback (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

static void
ok_callback (GtkWidget * widget, gpointer data)
{
  GtkWidget *entry;

  run_flag = TRUE;

  entry = gtk_object_get_data (GTK_OBJECT (data), "width");
  grid_cfg.hwidth = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 0) + 0.5);
  grid_cfg.vwidth = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 1) + 0.5);
  grid_cfg.iwidth = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 2) + 0.5);
  
  entry = gtk_object_get_data (GTK_OBJECT (data), "space");
  grid_cfg.hspace = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 0) + 0.5);
  grid_cfg.vspace = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 1) + 0.5);
  grid_cfg.ispace = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 2) + 0.5);
 
  entry = gtk_object_get_data (GTK_OBJECT (data), "offset");
  grid_cfg.hoffset = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 0) + 0.5);
  grid_cfg.voffset = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 1) + 0.5);
  grid_cfg.ioffset = (int)(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (entry), 2) + 0.5);

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
entry_callback (GtkWidget * widget, gpointer data)
{
  static gdouble x = -1.0;
  static gdouble y = -1.0;
  gdouble new_x;
  gdouble new_y;

  new_x = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  new_y = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (data)))
    {
      if (new_x != x)
	{
	  y = new_y = x = new_x;
	  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 1, y);
	}
      if (new_y != y)
	{
	  x = new_x = y = new_y;
	  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 0, x);
	}
    }
  else
    {
      if (new_x != x)
	x = new_x;
      if (new_y != y)
	y = new_y;
    }     
}


static gint
dialog (gint32     image_ID,
	GDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *hbbox;
  GtkWidget *hbox;
  GtkWidget *width;
  GtkWidget *width_button;
  GtkWidget *space;
  GtkWidget *space_button;
  GtkWidget *offset;
  GtkWidget *align;
  GUnit      unit;
  gdouble    xres;
  gdouble    yres;
  gchar    **argv;
  gint       argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("grid");

  gtk_init (&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), _("Grid"));
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) close_callback, NULL);

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  unit   = gimp_image_get_unit (image_ID);

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
 
  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  The width entries  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, FALSE, 4);

  width = gimp_size_entry_new (3,                            /*  number_of_fields  */ 
			       UNIT_PIXEL,                   /*  FIXME - use unit  */
			       "%a",                         /*  unit_format       */
			       TRUE,                         /*  menu_show_pixels  */
			       TRUE,                         /*  menu_show_percent */
			       FALSE,                        /*  show_refval       */
			       SPIN_BUTTON_WIDTH,            /*  spinbutton_usize  */
			       GIMP_SIZE_ENTRY_UPDATE_SIZE); /*  update_policy     */

  /*  set the resolution to the image resolution  */
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (width), 0, xres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (width), 1, yres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (width), 2, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (width), 0, 0.0, (gdouble)(drawable->width));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (width), 1, 0.0, (gdouble)(drawable->height));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (width), 2, 0.0, (gdouble)(drawable->width));

  /*  set upper and lower limits (in pixels)  */
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (width), 0, 0.0, (gdouble)(drawable->width));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (width), 1, 0.0, (gdouble)(drawable->height));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (width), 2, 0.0, (gdouble)(drawable->width));

  /*  initialize the values  */
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (width), 0, (gdouble)grid_cfg.hwidth);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (width), 1, (gdouble)grid_cfg.vwidth);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (width), 2, (gdouble)grid_cfg.iwidth);

  /*  attach labels  */
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (width), _("Horizontal"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (width), _("Vertical"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (width), _("Intersection"), 0, 3, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (width), _("Width: "), 1, 0, 0.0); 

  /*  put a chain_button under the size_entries  */
  width_button = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);
  if (grid_cfg.hwidth == grid_cfg.vwidth)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (width_button), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (width), width_button, 1, 3, 2, 3);
  gtk_widget_show (width_button);
 
  /*  connect to the 'value_changed' and "unit_changed" signals because we have to 
      take care of keeping the entries in sync when the chainbutton is active        */
  gtk_signal_connect (GTK_OBJECT (width), "value_changed", 
		      (GtkSignalFunc) entry_callback, width_button);
  gtk_signal_connect (GTK_OBJECT (width), "unit_changed", 
		      (GtkSignalFunc) entry_callback, width_button);

  gtk_box_pack_end (GTK_BOX (hbox), width, FALSE, FALSE, 4);
  gtk_widget_show (width);
  gtk_widget_show (hbox);

  /*  The space entries  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, FALSE, 4);

  space = gimp_size_entry_new (3,                            /*  number_of_fields  */ 
			       UNIT_PIXEL,                   /*  FIXME - use unit  */
			       "%a",                         /*  unit_format       */
			       TRUE,                         /*  menu_show_pixels  */
			       TRUE,                         /*  menu_show_percent */
			       FALSE,                        /*  show_refval       */
			       SPIN_BUTTON_WIDTH,            /*  spinbutton_usize  */
			       GIMP_SIZE_ENTRY_UPDATE_SIZE); /*  update_policy     */

  /*  set the resolution to the image resolution  */
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (space), 0, xres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (space), 1, yres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (space), 2, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (space), 0, 0.0, (gdouble)(drawable->width));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (space), 1, 0.0, (gdouble)(drawable->height));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (space), 2, 0.0, (gdouble)(drawable->width));

  /*  set upper and lower limits (in pixels)  */
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (space), 0, 0.0, (gdouble)(drawable->width));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (space), 1, 0.0, (gdouble)(drawable->height));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (space), 2, 0.0, (gdouble)(drawable->width));

  /*  initialize the values  */
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (space), 0, (gdouble)grid_cfg.hspace);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (space), 1, (gdouble)grid_cfg.vspace);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (space), 2, (gdouble)grid_cfg.ispace);

  /*  attach labels  */
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (space), _("Spacing: "), 1, 0, 0.0); 

  /*  put a chain_button under the size_entries  */
  space_button = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);
  if (grid_cfg.hspace == grid_cfg.vspace)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (space_button), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (space), space_button, 1, 3, 2, 3);
  gtk_widget_show (space_button);
 
  /*  connect to the 'value_changed' and "unit_changed" signals because we have to 
      take care of keeping the entries in sync when the chainbutton is active        */
  gtk_signal_connect (GTK_OBJECT (space), "value_changed", 
		      (GtkSignalFunc) entry_callback, space_button);
  gtk_signal_connect (GTK_OBJECT (space), "unit_changed", 
		      (GtkSignalFunc) entry_callback, space_button);

  gtk_box_pack_end (GTK_BOX (hbox), space, FALSE, FALSE, 4);
  gtk_widget_show (space);
  gtk_widget_show (hbox);

  /*  The offset entries  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, FALSE, 4);

  offset = gimp_size_entry_new (3,                            /*  number_of_fields  */ 
				UNIT_PIXEL,                   /*  FIXME - use unit  */
				"%a",                         /*  unit_format       */
				TRUE,                         /*  menu_show_pixels  */
				TRUE,                         /*  menu_show_percent */
				FALSE,                        /*  show_refval       */
				SPIN_BUTTON_WIDTH,            /*  spinbutton_usize  */
				GIMP_SIZE_ENTRY_UPDATE_SIZE); /*  update_policy     */

  /*  set the resolution to the image resolution  */
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (offset), 0, xres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (offset), 1, yres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (offset), 2, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (offset), 0, 0.0, (gdouble)(drawable->width));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (offset), 1, 0.0, (gdouble)(drawable->height));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (offset), 2, 0.0, (gdouble)(drawable->width));

  /*  set upper and lower limits (in pixels)  */
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (offset), 0, 0.0, (gdouble)(drawable->width));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (offset), 1, 0.0, (gdouble)(drawable->height));
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (offset), 2, 0.0, (gdouble)(drawable->width));

  /*  initialize the values  */
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (offset), 0, (gdouble)grid_cfg.hoffset);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (offset), 1, (gdouble)grid_cfg.voffset);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (offset), 2, (gdouble)grid_cfg.ioffset);

  /*  attach labels  */
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (offset), _("Offset: "), 1, 0, 0.0); 

  /*  attach color selectors  */
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (offset), _("Color: "), 2, 0, 0.0); 

  button = gimp_color_button_new (_("Horizontal Color"), COLOR_BUTTON_WIDTH, 16, 
				  grid_cfg.hcolor, 4);
  align = gtk_alignment_new (0.0, 0.5, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), button);
  gtk_table_attach (GTK_TABLE (offset), align, 1, 2, 2, 3, 
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 10);
  gtk_widget_show (button);
  gtk_widget_show (align);

  button = gimp_color_button_new (_("Vertical Color"), COLOR_BUTTON_WIDTH, 16, 
				  grid_cfg.vcolor, 4);
  align = gtk_alignment_new (0.0, 0.5, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), button);
  gtk_table_attach (GTK_TABLE (offset), align, 2, 3, 2, 3, 
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 10);
  gtk_widget_show (button);
  gtk_widget_show (align);

  button = gimp_color_button_new (_("Intersection Color"), COLOR_BUTTON_WIDTH, 16, 
				  grid_cfg.icolor, 4);
  align = gtk_alignment_new (0.0, 0.5, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), button);
  gtk_table_attach (GTK_TABLE (offset), align, 3, 4, 2, 3, 
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 10);
  gtk_widget_show (button);
  gtk_widget_show (align);

  gtk_box_pack_end (GTK_BOX (hbox), offset, FALSE, FALSE, 4);
  gtk_widget_show (offset);
  gtk_widget_show (hbox);

  gtk_widget_show (dlg);

  gtk_object_set_data (GTK_OBJECT (dlg), "width",  width);
  gtk_object_set_data (GTK_OBJECT (dlg), "space",  space);  
  gtk_object_set_data (GTK_OBJECT (dlg), "offset", offset);  

  gtk_main ();
  gdk_flush ();

  return run_flag;
}


