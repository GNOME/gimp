/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * mapcolor plugin
 * Copyright (C) 1998 Peter Kirchgessner
 * email: peter@kirchgessner.net, WWW: http://www.kirchgessner.net)
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
 */

/* Event history:
 * V 1.00, PK, 26-Oct-98: Creation.
 * V 1.01, PK, 21-Nov-99: Fix problem with working on layered images
 *                        Internationalization
 */
#define VERSIO                                     1.01
static char dversio[] =                          "v1.01  21-Nov-99";
static char ident[] = "@(#) GIMP mapcolor plug-in v1.01  21-Nov-99";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <gtk/gtk.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "libgimp/stdplugins-intl.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define PRV_WIDTH  50
#define PRV_HEIGHT 20

typedef struct
{
 guchar colors[4][3];
 gint32 map_mode;
} PluginValues;

PluginValues plvals = 
{
  {
    { 0, 0, 0},
    { 255, 255, 255 },
    { 0, 0, 0 },
    { 255, 255, 255 }
  },
  0
};

gint run_flag = FALSE;

/* Declare some local functions.
 */
static void   query (void);
static void   run   (char    *name,
		     gint     nparams,
		     GParam  *param,
		     gint    *nreturn_vals,
		     GParam **return_vals);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static gint   dialog                  (void);
static void   mapcolor_ok_callback    (GtkWidget *widget,
				       gpointer   data);
static void   add_color_button        (int        csel_index,
				       int        left,
				       int        top,
				       GtkWidget *table);

static void   color_mapping           (GDrawable *drawable);


/* The run mode */
static GRunModeType l_run_mode;

static char *csel_title[4] = 
{ 
  N_("First source color"), 
  N_("Second source color"),
  N_("First destination color"), 
  N_("Second destination color") 
};


MAIN ()

static void
query (void)

{
  static GParamDef adjust_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (not used)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable to adjust" }
  };
  static int nadjust_args = sizeof (adjust_args) / sizeof (adjust_args[0]);

  static GParamDef map_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (not used)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable where colors are to map" },
    { PARAM_COLOR, "srccolor_1", "First source color" },
    { PARAM_COLOR, "srccolor_2", "Second source color" },
    { PARAM_COLOR, "dstcolor_1", "First destination color" },
    { PARAM_COLOR, "dstcolor_2", "Second destination color" },
    { PARAM_INT32, "map_mode", "Mapping mode (0: linear, others reserved)" }
  };
  static int nmap_args = sizeof (map_args) / sizeof (map_args[0]);

  INIT_I18N ();

  gimp_install_procedure ("plug_in_color_adjust",
                          _("Adjust current foreground/background color in the\
 drawable to black/white"),
                          _("The current foreground color is mapped to black, \
the current background color is mapped to white."),
                          "Peter Kirchgessner",
                          "Peter Kirchgessner",
                          dversio,
                          N_("<Image>/Filters/Colors/Map/Adjust FG-BG"),
                          "RGB*",
                          PROC_PLUG_IN,
                          nadjust_args, 0,
                          adjust_args, NULL);

  gimp_install_procedure ("plug_in_color_map",
                          _("Map two source colors to two destination colors. \
Other colors are mapped by interpolation."),
                          _("Map two source colors to two destination colors. \
Other colors are mapped by interpolation."),
                          "Peter Kirchgessner",
                          "Peter Kirchgessner",
                          dversio,
                          N_("<Image>/Filters/Colors/Map/Color Mapping..."),
                          "RGB*",
                          PROC_PLUG_IN,
                          nmap_args, 0,
                          map_args, NULL);
}


static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)

{
  static GParam values[1];
  GRunModeType run_mode;
  GDrawable *drawable = NULL;
  GStatusType status = STATUS_SUCCESS;
  guchar *c = (guchar *)ident;
  int j;

  INIT_I18N_UI();

  l_run_mode = run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  while (status == STATUS_SUCCESS)
    {
      if (nparams < 3)
	{
	  status = STATUS_CALLING_ERROR;
	  break;
	}

      /* Make sure the drawable is RGB color */
      drawable = gimp_drawable_get (param[2].data.d_drawable);
      if (!gimp_drawable_is_rgb (drawable->id))
	{
	  gimp_message (_("Color Mapping / Adjust FG/BG:\nCannot operate on grey/indexed images"));
	  status = STATUS_EXECUTION_ERROR;
	  break;
	}

      if (strcmp (name, "plug_in_color_adjust") == 0)
	{
	  if (nparams != 3)  /* Make sure all the arguments are there */
	    {
	      status = STATUS_CALLING_ERROR;
	      break;
	    }

	  c = &(plvals.colors[0][0]);      /* First source color */
	  gimp_palette_get_foreground (c, c+1, c+2);
	  c = &(plvals.colors[1][0]);      /* Second source color */
	  gimp_palette_get_background (c, c+1, c+2);
	  c = &(plvals.colors[2][0]);      /* First destination color */
	  c[0] = c[1] = c[2] = 0;          /* Foreground mapped to black */
	  c = &(plvals.colors[3][0]);      /* second destination color */
	  c[0] = c[1] = c[2] = 255;        /* Background mapped to white */
	  plvals.map_mode = 0;

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_progress_init (_("Adjusting Foreground/Background"));

	  color_mapping (drawable);
	  break;
	}

      if (strcmp (name, "plug_in_color_map") == 0)
	{
	  if (run_mode == RUN_NONINTERACTIVE)
	    {
	      if (nparams != 8)  /* Make sure all the arguments are there */
		{
		  status = STATUS_CALLING_ERROR;
		  break;
		}

	      for (j = 0; j < 4; j++)
		{
		  plvals.colors[j][0] = param[3+j].data.d_color.red;
		  plvals.colors[j][1] = param[3+j].data.d_color.green;
		  plvals.colors[j][2] = param[3+j].data.d_color.blue;
		}
	      plvals.map_mode = param[7].data.d_int32;
	    }
	  else if (run_mode == RUN_INTERACTIVE)
	    {
	      gimp_get_data (name, &plvals);

	      c = &(plvals.colors[0][0]);      /* First source color */
	      gimp_palette_get_foreground (c, c+1, c+2);
	      c = &(plvals.colors[1][0]);      /* Second source color */
	      gimp_palette_get_background (c, c+1, c+2);

	      if (!dialog ())
		break;
	    }
	  else if (run_mode == RUN_WITH_LAST_VALS)
	    {
	      gimp_get_data (name, &plvals);
	    }
	  else
	    {
	      status = STATUS_CALLING_ERROR;
	      break;
	    }

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_progress_init (_("Mapping colors"));

	  color_mapping (drawable);

	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data (name, &plvals, sizeof (plvals));

	  break;
	}

      status = STATUS_EXECUTION_ERROR;
    }

  if ((status == STATUS_SUCCESS) && (run_mode != RUN_NONINTERACTIVE))
    gimp_displays_flush ();

  if (drawable != NULL) gimp_drawable_detach (drawable);
  values[0].data.d_status = status;
}

static gint
dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *hbox;
  GtkWidget *table;
  guchar  *color_cube;
  gchar  **argv;
  gint     argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("mapcolor");

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

  dlg = gimp_dialog_new (_("Map Colors"), "mapcolor",
			 gimp_plugin_help_func, "filters/mapcolor.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), mapcolor_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /* The table keeps the color selections */
  table = gtk_table_new (2, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 6);
  gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  add_color_button (0, 0, 0, table);
  add_color_button (1, 2, 0, table);
  add_color_button (2, 0, 1, table);
  add_color_button (3, 2, 1, table);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return run_flag;
}

static void
add_color_button (gint       csel_index,
                  gint       left,
                  gint       top,
                  GtkWidget *table)

{
  GtkWidget *label;
  GtkWidget *button;

  label = gtk_label_new ((left == 0) ? _("From:") : _("To:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, left, left+1, top, top+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  button = gimp_color_button_new (gettext (csel_title[csel_index]),
				  PRV_WIDTH, PRV_HEIGHT,
				  plvals.colors[csel_index], 3);
  gtk_table_attach (GTK_TABLE (table), button, left+1, left+2, top, top+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);
}

static void
mapcolor_ok_callback (GtkWidget *widget,
                      gpointer   data)

{
  plvals.map_mode = 0;  /* Currently always linear mapping */

  run_flag = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
get_mapping (guchar *src_col1,
             guchar *src_col2,
             guchar *dst_col1,
             guchar *dst_col2,
             gint32  map_mode,
             guchar *redmap,
             guchar *greenmap,
             guchar *bluemap)

{
  int rgb, i, j, a, as, b, bs;
  guchar *colormap[3];

  /* Currently we always do a linear mapping */

  colormap[0] = redmap;
  colormap[1] = greenmap;
  colormap[2] = bluemap;

  switch (map_mode)
    {
    case 0:
    default:
      for (rgb = 0; rgb < 3; rgb++)
	{
	  a = src_col1[rgb];  as = dst_col1[rgb];
	  b = src_col2[rgb];  bs = dst_col2[rgb];
	  for (i = 0; i < 256; i++)
	    {
	      j = ((i - a) * (bs - as)) / (b - a) + as;
	      if (j > 255) j = 255; else if (j < 0) j = 0;
	      colormap[rgb][i] = j;
	    }
	}
      break;
    }
}

static void
color_mapping (GDrawable *drawable)

{
  int processed, total;
  gint x, y, xmin, xmax, ymin, ymax, bpp = (gint)drawable->bpp;
  unsigned char *src, *dest;
  GPixelRgn src_rgn, dest_rgn;
  gpointer pr;
  double progress;
  unsigned char redmap[256], greenmap[256], bluemap[256];
  unsigned char *src_col1 = plvals.colors[0];
  unsigned char *src_col2 = plvals.colors[1];
  unsigned char *dst_col1 = plvals.colors[2];
  unsigned char *dst_col2 = plvals.colors[3];

  if (   (src_col1[0] == src_col2[0])
      || (src_col1[1] == src_col2[1])
      || (src_col1[2] == src_col2[2])) return;

  gimp_drawable_mask_bounds (drawable->id, &xmin, &ymin, &xmax, &ymax);
  if ((ymin == ymax) || (xmin == xmax)) return;
  total = (xmax - xmin) * (ymax - ymin);

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
  gimp_pixel_rgn_init (&src_rgn, drawable, xmin, ymin,
                       (xmax - xmin), (ymax - ymin), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, xmin, ymin,
                       (xmax - xmin), (ymax - ymin), TRUE, TRUE);

  pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);

  get_mapping (src_col1, src_col2, dst_col1, dst_col2, plvals.map_mode,
               redmap, greenmap, bluemap);

  processed = 0;
  progress = 0.0;
  for (; pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      for (y = 0; y < src_rgn.h; y++)
	{
	  src = src_rgn.data + y * src_rgn.rowstride;
	  dest = dest_rgn.data + y * dest_rgn.rowstride;
	  for (x = 0; x < src_rgn.w; x++)
	    {
	      dest[0] = redmap[src[0]];
	      dest[1] = greenmap[src[1]];
	      dest[2] = bluemap[src[2]];
	      if (bpp > 3) dest[3] = src[3];
	      src += bpp;
	      dest += bpp;
	      processed++;
	    }
	}
      if (l_run_mode != RUN_NONINTERACTIVE)
	{
	  if ((double)processed/(double)total - progress > 0.1)
	    {
	      progress = (double)processed/(double)total;
	      gimp_progress_update (progress);
	    }
	}
    }
  if (l_run_mode != RUN_NONINTERACTIVE)
    gimp_progress_update (1.0);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, xmin, ymin, (xmax - xmin), (ymax - ymin));
}
