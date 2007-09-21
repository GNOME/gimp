/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Despeckle (adaptive median) filter
 *
 *   Copyright 1997-1998 Michael Sweet (mike@easysw.com)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/*
 * Constants...
 */

#define PLUG_IN_PROC     "plug-in-despeckle"
#define PLUG_IN_BINARY   "despeckle"
#define PLUG_IN_VERSION  "May 1998"
#define SCALE_WIDTH      100
#define ENTRY_WIDTH        3
#define MAX_RADIUS        20

#define FILTER_ADAPTIVE  0x01
#define FILTER_RECURSIVE 0x02

#define despeckle_radius (despeckle_vals[0])    /* diameter of filter */
#define filter_type      (despeckle_vals[1])    /* filter type */
#define black_level      (despeckle_vals[2])    /* Black level */
#define white_level      (despeckle_vals[3])    /* White level */

#define VALUE_SWAP(a,b)   \
  { register gdouble t = (a); (a) = (b); (b) = t; }
#define POINTER_SWAP(a,b) \
  { register const guchar *t = (a); (a) = (b); (b) = t; }



/*
 * Local functions...
 */

static void      query (void);
static void      run   (const gchar      *name,
                        gint              nparams,
                        const GimpParam  *param,
                        gint             *nreturn_vals,
                        GimpParam       **return_vals);

static void      despeckle                 (void);
static void      despeckle_median          (guchar        *src,
                                            guchar        *dst,
                                            gint           width,
                                            gint           height,
                                            gint           bpp,
                                            gint           radius,
                                            gboolean       preview);

static gboolean  despeckle_dialog          (void);

static void      dialog_adaptive_callback  (GtkWidget     *widget,
                                            gpointer       data);
static void      dialog_recursive_callback (GtkWidget     *widget,
                                            gpointer       data);

static void      preview_update            (GtkWidget     *preview);

static gint      quick_median_select       (const guchar **p,
                                            guchar        *i,
                                            gint           n);
static inline guchar  pixel_luminance      (const guchar  *p,
                                            gint           bpp);
static inline void    pixel_copy           (guchar        *dest,
                                            const guchar  *src,
                                            gint           bpp);

/*
 * Globals...
 */

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init  */
  NULL,  /* quit  */
  query, /* query */
  run    /* run   */
};

static GtkWidget    *preview;                 /* Preview widget   */
static GimpDrawable *drawable = NULL;         /* Current drawable */


static gint despeckle_vals[4] =
{
  3,                  /* Default value for the diameter */
  FILTER_ADAPTIVE,    /* Default value for the filter type */
  7,                  /* Default value for the black level */
  248                 /* Default value for the white level */
};


/*
 * 'main()' - Main entry - just call gimp_main()...
 */

MAIN ()

/*
 * 'query()' - Respond to a plug-in query...
 */

static void
query (void)
{
  static const GimpParamDef   args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT32,    "radius",   "Filter box radius (default = 3)" },
    { GIMP_PDB_INT32,    "type",     "Filter type (0 = median, 1 = adaptive, 2 = recursive-median, 3 = recursive-adaptive)" },
    { GIMP_PDB_INT32,    "black",    "Black level (-1 to 255)" },
    { GIMP_PDB_INT32,    "white",    "White level (0 to 256)" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Remove speckle noise from the image"),
                          "This plug-in selectively performs a median or "
                          "adaptive box filter on an image.",
                          "Michael Sweet <mike@easysw.com>",
                          "Copyright 1997-1998 by Michael Sweet",
                          PLUG_IN_VERSION,
                          N_("Des_peckle..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Enhance");
}


/*
 * 'run()' - Run the filter...
 */

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpRunMode        run_mode;
  GimpPDBStatusType  status;
  GimpParam          *values;

  INIT_I18N ();

  /*
   * Initialize parameter data...
   */

  status   = GIMP_PDB_SUCCESS;
  run_mode = param[0].data.d_int32;

  values = g_new (GimpParam, 1);

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  /*
   * Get drawable information...
   */

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*
   * See how we will run
   */

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE :
      /*
       * Possibly retrieve data...
       */

      gimp_get_data (PLUG_IN_PROC, &despeckle_radius);

      /*
       * Get information from the dialog...
       */
      if (gimp_drawable_is_rgb(drawable->drawable_id) ||
          gimp_drawable_is_gray(drawable->drawable_id))
       {
          if (! despeckle_dialog ())
          return;
       }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*
       * Make sure all the arguments are present...
       */

      if (nparams < 4 || nparams > 9)
        status = GIMP_PDB_CALLING_ERROR;
      else if (nparams == 4)
        {
          despeckle_radius = param[3].data.d_int32;
          filter_type      = FILTER_ADAPTIVE;
          black_level      = 7;
          white_level      = 248;
        }
      else if (nparams == 5)
        {
          despeckle_radius = param[3].data.d_int32;
          filter_type      = param[4].data.d_int32;
          black_level      = 7;
          white_level      = 248;
        }
      else if (nparams == 6)
        {
          despeckle_radius = param[3].data.d_int32;
          filter_type      = param[4].data.d_int32;
          black_level      = param[5].data.d_int32;
          white_level      = 248;
        }
      else
        {
          despeckle_radius = param[3].data.d_int32;
          filter_type      = param[4].data.d_int32;
          black_level      = param[5].data.d_int32;
          white_level      = param[6].data.d_int32;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*
       * Possibly retrieve data...
       */

      INIT_I18N();
      gimp_get_data (PLUG_IN_PROC, despeckle_vals);
        break;

    default:
      status = GIMP_PDB_CALLING_ERROR;
      break;
    }

  /*
   * Despeckle the image...
   */

  if (status == GIMP_PDB_SUCCESS)
    {
    	if (gimp_drawable_is_rgb(drawable->drawable_id) ||
            gimp_drawable_is_gray(drawable->drawable_id))
        {

          /*
           * Run!
           */

          despeckle ();

          /*
           * If run prevmode is interactive, flush displays...
           */

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*
           * Store data...
           */

          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC,
                           despeckle_vals, sizeof (despeckle_vals));
        }
      else
        status = GIMP_PDB_EXECUTION_ERROR;
    }

  /*
   * Reset the current run status...
   */

  values[0].data.d_status = status;

  /*
   * Detach from the drawable...
   */

  gimp_drawable_detach (drawable);
}


/*
 * 'despeckle()' - Despeckle an image using a median filter.
 *
 * A median filter basically collects pixel values in a region around the
 * target pixel, sorts them, and uses the median value. This code uses a
 * circular row buffer to improve performance.
 *
 * The adaptive filter is based on the median filter but analizes the histogram
 * of the region around the target pixel and adjusts the despeckle diameter
 * accordingly.
 */

static void
despeckle (void)
{
  GimpPixelRgn  src_rgn,        /* Source image region */
                dst_rgn;
  guchar       *src, *dst;
  gint          img_bpp;
  gint          width;
  gint          height;
  gint          x1, y1 ,x2 ,y2;

  img_bpp = gimp_drawable_bpp (drawable->drawable_id);
  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  width  = x2 - x1;
  height = y2 - y1;

  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dst_rgn, drawable, x1, y1, width, height, TRUE, TRUE);

  src = g_new (guchar, width * height * img_bpp);
  dst = g_new (guchar, width * height * img_bpp);

  gimp_pixel_rgn_get_rect (&src_rgn, src, x1, y1, width, height);

  despeckle_median (src, dst, width, height, img_bpp, despeckle_radius, FALSE);

  gimp_pixel_rgn_set_rect (&dst_rgn, dst, x1, y1, width, height);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);

  g_free (dst);
  g_free (src);
}



/*
 * 'despeckle_dialog()' - Popup a dialog window for the filter box size...
 */

static gint
despeckle_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *button;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Despeckle"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (preview_update),
                    NULL);

  frame = gimp_frame_new (_("Median"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  button = gtk_check_button_new_with_mnemonic (_("_Adaptive"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                filter_type & FILTER_ADAPTIVE);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_adaptive_callback),
                    NULL);

  button = gtk_check_button_new_with_mnemonic (_("R_ecursive"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                filter_type & FILTER_RECURSIVE);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_recursive_callback),
                    NULL);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   * Box size (diameter) control...
   */

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Radius:"), SCALE_WIDTH, ENTRY_WIDTH,
                              despeckle_radius, 1, MAX_RADIUS, 1, 5, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &despeckle_radius);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*
   * Black level control...
   */

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("_Black level:"), SCALE_WIDTH, ENTRY_WIDTH,
                              black_level, -1, 255, 1, 8, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &black_level);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*
   * White level control...
   */

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                              _("_White level:"), SCALE_WIDTH, ENTRY_WIDTH,
                              white_level, 0, 256, 1, 8, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &white_level);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*
   * Show it and wait for the user to do something...
   */

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  /*
   * Return ok/cancel...
   */

  return run;
}

/*
 * 'preview_update()' - Update the preview window.
 */

static void
preview_update (GtkWidget *widget)
{
  GimpPixelRgn  src_rgn;        /* Source image region */
  guchar       *dst;            /* Output image */
  GimpPreview  *preview;        /* The preview widget */
  guchar       *src;            /* Source pixel rows */
  gint          img_bpp;
  gint          x1,y1;
  gint          width, height;

  preview = GIMP_PREVIEW (widget);

  img_bpp = gimp_drawable_bpp (drawable->drawable_id);

  width  = preview->width;
  height = preview->height;

  gimp_preview_get_position (preview, &x1, &y1);

  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, width, height, FALSE, FALSE);

  dst = g_new (guchar, width * height * img_bpp);
  src = g_new (guchar, width * height * img_bpp);

  gimp_pixel_rgn_get_rect (&src_rgn, src, x1, y1, width, height);

  despeckle_median (src, dst, width, height, img_bpp, despeckle_radius, TRUE);

  gimp_preview_draw_buffer (preview, dst, width * img_bpp);

  g_free (src);
  g_free (dst);
}


static void
dialog_adaptive_callback (GtkWidget *widget,
                          gpointer   data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
    filter_type |= FILTER_ADAPTIVE;
  else
    filter_type &= ~FILTER_ADAPTIVE;

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void
dialog_recursive_callback (GtkWidget *widget,
                           gpointer   data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
    filter_type |= FILTER_RECURSIVE;
  else
    filter_type &= ~FILTER_RECURSIVE;

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void
despeckle_median (guchar   *src,
                  guchar   *dst,
                  gint      width,
                  gint      height,
                  gint      bpp,
                  gint      radius,
                  gboolean  preview)
{
  const guchar **buf;
  guchar        *ibuf;
  guint          progress;
  guint          max_progress;
  gint           x, y;
  gint           u, v;
  gint           diameter;
  gint           box;
  gint           pos;

  progress     = 0;
  max_progress = width * height;

  diameter = (2 * radius) + 1;
  box      = SQR (diameter);
  buf      = g_new (const guchar *, box);
  ibuf     = g_new (guchar, box);

  if (! preview)
    gimp_progress_init(_("Despeckle"));


  for (y = 0; y < height; y++)
    {
      gint ymin = MAX (0, y - radius);
      gint ymax = MIN (height - 1, y + radius);

      for (x = 0; x < width; x++)
        {
          gint xmin    = MAX (0, x - radius);
          gint xmax    = MIN (width - 1, x + radius);
          gint hist0   = 0;
          gint hist255 = 0;
          gint med     = -1;

          for (v = ymin; v <= ymax; v++)
            {
              for (u = xmin; u <= xmax; u++)
                {
                  gint value;

                  pos = (u + (v * width)) * bpp;
                  value = pixel_luminance (src + pos, bpp);

                  if (value > black_level && value < white_level)
                    {
                      med++;
                      buf[med]  = src + pos;
                      ibuf[med] = value;
                    }
                  else
                    {
                      if (value <= black_level)
                        hist0++;

                      if (value >= white_level)
                        hist255++;
                    }
                }
             }

           pos = (x + (y * width)) * bpp;

           if (med < 1)
             {
               pixel_copy (dst + pos, src + pos, bpp);
             }
           else
             {
               const guchar *pixel;

               pixel = buf[quick_median_select (buf, ibuf, med + 1)];

                if (filter_type & FILTER_RECURSIVE)
                  pixel_copy (src + pos, pixel, bpp);

               pixel_copy (dst + pos, pixel, bpp);
             }

          /*
           * Check the histogram and adjust the diameter accordingly...
           */
          if (filter_type & FILTER_ADAPTIVE)
            {
              if (hist0 >= radius || hist255 >= radius)
                {
                  if (radius < diameter / 2)
                    radius++;
                }
              else if (radius > 1)
                {
                  radius--;
                }
            }

        }

      progress += width;

      if (! preview && y % 32 == 0)
        gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
    }

  if (! preview)
    gimp_progress_update (1.0);

  g_free (ibuf);
  g_free (buf);
}

/*
 * This Quickselect routine is based on the algorithm described in
 * "Numerical recipes in C", Second Edition,
 * Cambridge University Press, 1992, Section 8.5, ISBN 0-521-43108-5
 * This code by Nicolas Devillard - 1998. Public domain.
 *
 * Modified to swap pointers: swap is done by comparing luminance
 * value for the pointer to RGB.
 */
static gint
quick_median_select (const guchar **p,
                     guchar        *i,
                     gint           n)
{
  gint low    = 0;
  gint high   = n - 1;
  gint median = (low + high) / 2;

  while (TRUE)
    {
      gint middle, ll, hh;

      if (high <= low) /* One element only */
        return median;

      if (high == low + 1)
        {
          /* Two elements only */
          if (i[low] > i[high])
            {
               VALUE_SWAP (i[low], i[high]);
               POINTER_SWAP (p[low], p[high]);
            }

          return median;
        }

      /* Find median of low, middle and high items; swap into position low */
      middle = (low + high) / 2;

      if (i[middle] > i[high])
        {
           VALUE_SWAP (i[middle], i[high]);
           POINTER_SWAP (p[middle], p[high]);
        }

      if (i[low] > i[high])
        {
           VALUE_SWAP (i[low], i[high]);
           POINTER_SWAP (p[low], p[high]);
        }

      if (i[middle] > i[low])
        {
          VALUE_SWAP (i[middle], i[low]);
          POINTER_SWAP (p[middle], p[low]);
        }

      /* Swap low item (now in position middle) into position (low+1) */
      VALUE_SWAP (i[middle], i[low+1]);
      POINTER_SWAP (p[middle], p[low+1]);

      /* Nibble from each end towards middle, swapping items when stuck */
      ll = low + 1;
      hh = high;

      while (TRUE)
        {
           do ll++;
           while (i[low] > i[ll]);

           do hh--;
           while (i[hh]  > i[low]);

           if (hh < ll)
             break;

           VALUE_SWAP (i[ll], i[hh]);
           POINTER_SWAP (p[ll], p[hh]);
        }

      /* Swap middle item (in position low) back into correct position */
      VALUE_SWAP (i[low], i[hh]);
      POINTER_SWAP (p[low], p[hh]);

      /* Re-set active partition */
      if (hh <= median)
        low = ll;

      if (hh >= median)
        high = hh - 1;
    }
}

static inline guchar
pixel_luminance (const guchar *p,
                 gint          bpp)
{
  switch (bpp)
    {
    case 1:
    case 2:
      return p[0];

    case 3:
    case 4:
      return GIMP_RGB_LUMINANCE (p[0], p[1], p[2]);

    default:
      return 0; /* should not be reached */
    }
}

static inline void
pixel_copy (guchar       *dest,
            const guchar *src,
            gint          bpp)
{
  switch (bpp)
    {
    case 4:
      *dest++ = *src++;
    case 3:
      *dest++ = *src++;
    case 2:
      *dest++ = *src++;
    case 1:
      *dest++ = *src++;
    }
}
