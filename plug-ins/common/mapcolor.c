/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * mapcolor plugin
 * Copyright (C) 1998 Peter Kirchgessner
 * (email: pkirchg@aol.com, WWW: http://members.aol.com/pkirchg)
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
 */
#define VERSIO                                     1.00
static char dversio[] =                          "v1.00  26-Oct-98";
static char ident[] = "@(#) GIMP mapcolor plug-in v1.00  26-Oct-98";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define PRV_WIDTH  50
#define PRV_HEIGHT 20

typedef struct
{
 guchar colors[4][3];
 gint32 map_mode;
} PLVALS;

typedef struct
{
  GtkWidget *activate;   /* The button that activates the color sel. dialog */
  GtkWidget *colselect;  /* The colour selection dialog itself */
  GtkWidget *preview;    /* The colour preview */
  unsigned char color[3];/* The selected colour */
} CSEL;


typedef struct
{
  GtkWidget *dialog;
  CSEL       csel[4];
  PLVALS    *plvals;
  gint  run;  /*  run  */
} RunInterface;

static RunInterface runint =
{
  NULL,     /* dialog */
  {
    { NULL, NULL, NULL, { 0 } },  /* Color selection dialogs */
    { NULL, NULL, NULL, { 0 } },
    { NULL, NULL, NULL, { 0 } },
    { NULL, NULL, NULL, { 0 } }
  },
  NULL,     /* plug-in values */
  FALSE     /* run */
};


/* Declare some local functions.
 */
static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static gint   dialog (PLVALS *plvals);
static void   mapcolor_close_callback (GtkWidget *widget,
                                gpointer data);
static void   mapcolor_ok_callback (GtkWidget *widget,
                                gpointer data);
static void   add_color_button (int csel_index, int left, int top,
                                GtkWidget *table, PLVALS *plvals);
static void   color_select_ok_callback (GtkWidget *widget,
                                gpointer data);
static void   color_select_cancel_callback (GtkWidget *widget,
                                gpointer data);
static void   color_preview_show (GtkWidget *widget,
                                 unsigned char *color);

static void color_mapping (GDrawable *drawable, PLVALS *plvals);


/* The run mode */
static GRunModeType l_run_mode;


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

  gimp_install_procedure ("plug_in_color_adjust",
                          "Adjust current foreground/background color in the\
 drawable to black/white",
                          "The current foreground color is mapped to black, \
the current background color is mapped to white.",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner",
                          dversio,
                          "<Image>/Filters/Colors/Adjust Fgrd.-Bkgrd.",
                          "RGB*",
                          PROC_PLUG_IN,
                          nadjust_args, 0,
                          adjust_args, NULL);

  gimp_install_procedure ("plug_in_color_map",
                          "Map two source colors to two destination colors. \
Other colors are mapped by interpolation.",
                          "Map two source colors to two destination colors. \
Other colors are mapped by interpolation.",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner",
                          dversio,
                          "<Image>/Filters/Colors/Color Mapping",
                          "RGB*",
                          PROC_PLUG_IN,
                          nmap_args, 0,
                          map_args, NULL);
}


static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)

{
  static GParam values[1];
  GRunModeType run_mode;
  GDrawable *drawable = NULL;
  GStatusType status = STATUS_SUCCESS;
  guchar *c = (guchar *)ident;
  int j;
  static PLVALS plvals =
  {
    { { 0, 0, 0}, { 255, 255, 255 }, { 0, 0, 0 }, { 255, 255, 255 } },
    0
  };

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
    if (!gimp_drawable_color (drawable->id))
    {
      gimp_message ("color_adjust/color_map: cannot operate on grey/indexed\
 images");
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
        gimp_progress_init ("Adjust Foreground/Background");

      color_mapping (drawable, &plvals);
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

        if (!dialog (&plvals)) break;
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
        gimp_progress_init ("Mapping colors");

      color_mapping (drawable, &plvals);

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
dialog (PLVALS *plvals)

{
  GtkWidget *button;
  GtkWidget *dlg;
  GtkWidget *hbox;
  GtkWidget *table;
  guchar *color_cube;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("Map colors");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gdk_set_use_xshm(gimp_use_xshm());

  gtk_preview_set_gamma(gimp_gamma());
  gtk_preview_set_install_cmap(gimp_install_cmap());
  color_cube = gimp_color_cube();
  gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2],
                             color_cube[3]);
  gtk_widget_set_default_visual(gtk_preview_get_visual());
  gtk_widget_set_default_colormap(gtk_preview_get_cmap());

  runint.dialog = dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Map colors");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      (GtkSignalFunc) mapcolor_close_callback,
                      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) mapcolor_ok_callback, dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             (GtkSignalFunc) gtk_widget_destroy,
                             GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_show (button);

  /* parameter settings */
  runint.plvals = plvals;

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /* The table keeps the color selections */
  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  add_color_button (0, 0, 1, table, plvals);
  add_color_button (1, 0, 2, table, plvals);
  add_color_button (2, 2, 1, table, plvals);
  add_color_button (3, 2, 2, table, plvals);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return runint.run;
}


static void
color_preview_show (GtkWidget *widget,
                    unsigned char *rgb)

{guchar *buf, *bp;
 int j, width, height;

 width = PRV_WIDTH;
 height = PRV_HEIGHT;

 bp = buf = g_malloc (width*3);
 if (buf == NULL) return;

 for (j = 0; j < width; j++)
 {
   *(bp++) = rgb[0];
   *(bp++) = rgb[1];
   *(bp++) = rgb[2];
 }
 for (j = 0; j < height; j++)
   gtk_preview_draw_row (GTK_PREVIEW (widget), buf, 0, j, width);

 gtk_widget_draw (widget, NULL);

 g_free (buf);
}


static void
color_select_ok_callback (GtkWidget *widget,
                          gpointer data)

{gdouble color[3];
 int idx, j;
 GtkWidget *dialog;

 idx = (int)data;
 if ((dialog = runint.csel[idx].colselect) == NULL) return;

 gtk_color_selection_get_color (
   GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel),
   color);

 for (j = 0; j < 3; j++)
   runint.csel[idx].color[j] = (unsigned char)(color[j]*255.0);

 color_preview_show (runint.csel[idx].preview, &(runint.csel[idx].color[0]));

 runint.csel[idx].colselect = NULL;
 gtk_widget_destroy (dialog);
}


static void
color_select_cancel_callback (GtkWidget *widget,
                              gpointer data)

{int idx;
 GtkWidget *dialog;

 idx = (int)data;
 if ((dialog = runint.csel[idx].colselect) == NULL) return;

 runint.csel[idx].colselect = NULL;
 gtk_widget_destroy (dialog);
}


static void
mapcolor_color_button_callback (GtkWidget *widget,
                                gpointer data)

{int idx, j;
 GtkColorSelectionDialog *csd;
 GtkWidget *dialog;
 gdouble colour[3];
 static char *label[4] = { "First source color", "Second source color",
                 "First destination color", "Second destination color" };

 idx = (int)data;

 /* Is the colour selection dialog already running ? */
 if (runint.csel[idx].colselect != NULL) return;

 for (j = 0; j < 3; j++)
   colour[j] = runint.csel[idx].color[j] / 255.0;

 dialog = runint.csel[idx].colselect = gtk_color_selection_dialog_new (
            label[idx]);

 csd = GTK_COLOR_SELECTION_DIALOG (dialog);

 gtk_widget_destroy (csd->help_button);

 gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
                      (GtkSignalFunc) color_select_cancel_callback, data);
 gtk_signal_connect (GTK_OBJECT (csd->ok_button), "clicked",
                     (GtkSignalFunc) color_select_ok_callback, data);
 gtk_signal_connect (GTK_OBJECT (csd->cancel_button), "clicked",
                     (GtkSignalFunc) color_select_cancel_callback, data);

 gtk_color_selection_set_color (GTK_COLOR_SELECTION (csd->colorsel), colour);

 gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
 gtk_widget_show (dialog);
}


static void
add_color_button (int csel_index,
                  int left,
                  int top,
                  GtkWidget *table,
                  PLVALS *plvals)

{
 GtkWidget *label;
 GtkWidget *button;
 GtkWidget *preview;
 GtkWidget *hbox;

 hbox = gtk_hbox_new (FALSE, 0);
 gtk_container_border_width (GTK_CONTAINER (hbox), 5);
 gtk_table_attach (GTK_TABLE (table), hbox, left, left+1, top, top+1,
                   GTK_FILL, GTK_FILL, 0, 0);

 label = gtk_label_new ((left == 0) ? "From:" : "To:");
 gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
 gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
 gtk_widget_show (label);
 gtk_widget_show (hbox);

 hbox = gtk_hbox_new (FALSE, 0);
 gtk_container_border_width (GTK_CONTAINER (hbox), 5);
 gtk_table_attach (GTK_TABLE (table), hbox, left+1, left+2, top, top+1,
                   GTK_FILL, GTK_FILL, 0, 0);

 button = runint.csel[csel_index].activate = gtk_button_new ();

 memcpy (&(runint.csel[csel_index].color[0]),
         &(plvals->colors[csel_index][0]), 3);
 preview = runint.csel[csel_index].preview = gtk_preview_new(GTK_PREVIEW_COLOR);
 gtk_preview_size (GTK_PREVIEW (preview), PRV_WIDTH, PRV_HEIGHT);
 gtk_container_add (GTK_CONTAINER (button), preview);
 gtk_widget_show (preview);

 color_preview_show (preview, &(runint.csel[csel_index].color[0]));

 gtk_signal_connect (GTK_OBJECT (button), "clicked",
                     (GtkSignalFunc) mapcolor_color_button_callback,
                     (gpointer)csel_index);
 gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
 gtk_widget_show (button);
 gtk_widget_show (hbox);
}


static void
mapcolor_close_callback (GtkWidget *widget,
                         gpointer data)

{
  gtk_main_quit ();
}


static void
mapcolor_ok_callback (GtkWidget *widget,
                      gpointer data)

{int j;
 GtkWidget *dialog;
 PLVALS *plvals = runint.plvals;

 for (j = 0; j < 4; j++)
   memcpy (&(plvals->colors[j][0]), &(runint.csel[j].color[0]), 3);

 /* Destroy color selection dialogs if still running */
 for (j = 0; j < 4; j++)
 {
   dialog = runint.csel[j].colselect;
   if (dialog != NULL)
   {
     runint.csel[j].colselect = NULL;
     gtk_widget_destroy (GTK_WIDGET (dialog));
   }
 }
 plvals->map_mode = 0;  /* Currently always linear mapping */

 runint.run = TRUE;
 gtk_widget_destroy (GTK_WIDGET (data));
}


static void
get_mapping (guchar *src_col1,
             guchar *src_col2,
             guchar *dst_col1,
             guchar *dst_col2,
             gint32 map_mode,
             guchar *redmap,
             guchar *greenmap,
             guchar *bluemap)

{int rgb, i, j, a, as, b, bs;
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
color_mapping (GDrawable *drawable,
               PLVALS *plvals)

{
  int processed, total;
  gint x, y, xmin, xmax, ymin, ymax;
  unsigned char *src, *dest;
  GPixelRgn src_rgn, dest_rgn;
  gpointer pr;
  double progress;
  unsigned char redmap[256], greenmap[256], bluemap[256];
  unsigned char *src_col1 = plvals->colors[0];
  unsigned char *src_col2 = plvals->colors[1];
  unsigned char *dst_col1 = plvals->colors[2];
  unsigned char *dst_col2 = plvals->colors[3];

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

  get_mapping (src_col1, src_col2, dst_col1, dst_col2, plvals->map_mode,
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
        src += drawable->bpp;
        dest += drawable->bpp;
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
