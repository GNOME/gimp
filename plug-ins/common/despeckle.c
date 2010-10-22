/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Despeckle (adaptive median) filter
 *
 * Copyright 1997-1998 Michael Sweet (mike@easysw.com)
 * optimized in 2010 by Przemyslaw Zych (kermidt.zed@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#define PLUG_IN_ROLE     "gimp-despeckle"
#define PLUG_IN_VERSION  "May 2010"
#define SCALE_WIDTH      100
#define ENTRY_WIDTH        3
#define MAX_RADIUS        30

#define FILTER_ADAPTIVE  0x01
#define FILTER_RECURSIVE 0x02

#define despeckle_radius (despeckle_vals[0])    /* diameter of filter */
#define filter_type      (despeckle_vals[1])    /* filter type */
#define black_level      (despeckle_vals[2])    /* Black level */
#define white_level      (despeckle_vals[3])    /* White level */

/* List that stores pixels falling in to the same luma bucket */
#define MAX_LIST_ELEMS SQR(2 * MAX_RADIUS + 1)

typedef struct
{
  const guchar *elems[MAX_LIST_ELEMS];
  gint          start;
  gint          count;
} PixelsList;

typedef struct
{
  gint       elems[256]; /* Number of pixels that fall into each luma bucket */
  PixelsList origs[256]; /* Original pixels */
  gint       xmin;
  gint       ymin;
  gint       xmax;
  gint       ymax; /* Source rect */
} DespeckleHistogram;

/* Number of pixels in actual histogram falling into each category */
static gint                hist0;    /* Less than min treshold */
static gint                hist255;  /* More than max treshold */
static gint                histrest; /* From min to max        */

static DespeckleHistogram  histogram;


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
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT32,    "radius",   "Filter box radius (default = 3)" },
    { GIMP_PDB_INT32,    "type",     "Filter type { MEDIAN (0), ADAPTIVE (1), RECURSIVE-MEDIAN (2), RECURSIVE-ADAPTIVE (3) }" },
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
  static GimpParam   values[1];

  INIT_I18N ();

  /*
   * Initialize parameter data...
   */

  status   = GIMP_PDB_SUCCESS;
  run_mode = param[0].data.d_int32;

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
  GimpPixelRgn  src_rgn;        /* Source image region */
  GimpPixelRgn  dst_rgn;
  guchar       *src;
  guchar       *dst;
  gint          img_bpp;
  gint          x, y;
  gint          width, height;

  img_bpp = gimp_drawable_bpp (drawable->drawable_id);

  if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                      &x, &y, &width, &height))
    return;

  gimp_pixel_rgn_init (&src_rgn, drawable, x, y, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dst_rgn, drawable, x, y, width, height, TRUE, TRUE);

  src = g_new (guchar, width * height * img_bpp);
  dst = g_new (guchar, width * height * img_bpp);

  gimp_pixel_rgn_get_rect (&src_rgn, src, x, y, width, height);

  despeckle_median (src, dst, width, height, img_bpp, despeckle_radius, FALSE);

  gimp_pixel_rgn_set_rect (&dst_rgn, dst, x, y, width, height);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x, y, width, height);

  g_free (dst);
  g_free (src);
}



/*
 * 'despeckle_dialog()' - Popup a dialog window for the filter box size...
 */

static gint
despeckle_dialog (void)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *vbox;
  GtkWidget     *table;
  GtkWidget     *frame;
  GtkWidget     *button;
  GtkAdjustment *adj;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Despeckle"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new_from_drawable_id (drawable->drawable_id);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (preview_update),
                    NULL);

  frame = gimp_frame_new (_("Median"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
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
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    filter_type |= FILTER_ADAPTIVE;
  else
    filter_type &= ~FILTER_ADAPTIVE;

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void
dialog_recursive_callback (GtkWidget *widget,
                           gpointer   data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    filter_type |= FILTER_RECURSIVE;
  else
    filter_type &= ~FILTER_RECURSIVE;

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}



static inline void
list_add_elem (PixelsList   *list,
               const guchar *elem)
{
  const gint pos = list->start + list->count++;

  list->elems[pos >= MAX_LIST_ELEMS ? pos - MAX_LIST_ELEMS : pos] = elem;
}

static inline void
list_del_elem (PixelsList* list)
{
  list->count--;
  list->start++;

  if (list->start >= MAX_LIST_ELEMS)
    list->start = 0;
}

static inline const guchar *
list_get_random_elem (PixelsList *list)
{
  const gint pos = list->start + rand () % list->count;

  if (pos >= MAX_LIST_ELEMS)
    return list->elems[pos - MAX_LIST_ELEMS];

  return list->elems[pos];
}

static inline void
histogram_add (DespeckleHistogram *hist,
               guchar              val,
               const guchar       *orig)
{
  hist->elems[val]++;
  list_add_elem (&hist->origs[val], orig);
}

static inline void
histogram_remove (DespeckleHistogram *hist,
                  guchar              val)
{
  hist->elems[val]--;
  list_del_elem (&hist->origs[val]);
}

static inline void
histogram_clean (DespeckleHistogram *hist)
{
  gint i;

  for (i = 0; i < 256; i++)
    {
      hist->elems[i] = 0;
      hist->origs[i].count = 0;
    }
}

static inline const guchar *
histogram_get_median (DespeckleHistogram *hist,
                      const guchar       *_default)
{
  gint count = histrest;
  gint i;
  gint sum = 0;

  if (! count)
    return _default;

  count = (count + 1) / 2;

  i = 0;
  while ((sum += hist->elems[i]) < count)
    i++;

  return list_get_random_elem (&hist->origs[i]);
}

static inline void
add_val (DespeckleHistogram *hist,
         const guchar       *src,
         gint                width,
         gint                bpp,
         gint                x,
         gint                y)
{
  const gint pos   = (x + (y * width)) * bpp;
  const gint value = pixel_luminance (src + pos, bpp);

  if (value > black_level && value < white_level)
  {
    histogram_add (hist, value, src + pos);
    histrest++;
  }
  else
  {
    if (value <= black_level)
      hist0++;

    if (value >= white_level)
      hist255++;
  }
}

static inline void
del_val (DespeckleHistogram *hist,
         const guchar       *src,
         gint                width,
         gint                bpp,
         gint                x,
         gint                y)
{
  const gint pos   = (x + (y * width)) * bpp;
  const gint value = pixel_luminance (src + pos, bpp);

  if (value > black_level && value < white_level)
  {
    histogram_remove (hist, value);
    histrest--;
  }
  else
  {
    if (value <= black_level)
      hist0--;

    if (value >= white_level)
      hist255--;
  }
}

static inline void
add_vals (DespeckleHistogram *hist,
          const guchar       *src,
          gint                width,
          gint                bpp,
          gint                xmin,
          gint                ymin,
          gint                xmax,
          gint                ymax)
{
  gint x;
  gint y;

  if (xmin > xmax)
    return;

  for (y = ymin; y <= ymax; y++)
    {
      for (x = xmin; x <= xmax; x++)
        {
          add_val (hist, src, width, bpp, x, y);
        }
    }
}

static inline void
del_vals (DespeckleHistogram *hist,
          const guchar       *src,
          gint                width,
          gint                bpp,
          gint                xmin,
          gint                ymin,
          gint                xmax,
          gint                ymax)
{
  gint x;
  gint y;

  if (xmin > xmax)
    return;

  for (y = ymin; y <= ymax; y++)
    {
      for (x = xmin; x <= xmax; x++)
        {
          del_val (hist, src, width, bpp, x, y);
        }
    }
}

static inline void
update_histogram (DespeckleHistogram *hist,
                  const guchar       *src,
                  gint                width,
                  gint                bpp,
                  gint                xmin,
                  gint                ymin,
                  gint                xmax,
                  gint                ymax)
{
  /* assuming that radious of the box can change no more than one
     pixel in each call */
  /* assuming that box is moving either right or down */

  del_vals (hist,
            src, width, bpp, hist->xmin, hist->ymin, xmin - 1, hist->ymax);
  del_vals (hist, src, width, bpp, xmin, hist->ymin, xmax, ymin - 1);
  del_vals (hist, src, width, bpp, xmin, ymax + 1, xmax, hist->ymax);

  add_vals (hist, src, width, bpp, hist->xmax + 1, ymin, xmax, ymax);
  add_vals (hist, src, width, bpp, xmin, ymin, hist->xmax, hist->ymin - 1);
  add_vals (hist,
            src, width, bpp, hist->xmin, hist->ymax + 1, hist->xmax, ymax);

  hist->xmin = xmin;
  hist->ymin = ymin;
  hist->xmax = xmax;
  hist->ymax = ymax;
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
  guint  progress;
  guint  max_progress;
  gint   x, y;
  gint   adapt_radius;
  gint   pos;
  gint   ymin;
  gint   ymax;
  gint   xmin;
  gint   xmax;

  memset (&histogram, 0, sizeof(histogram));
  progress     = 0;
  max_progress = width * height;

  if (! preview)
    gimp_progress_init(_("Despeckle"));

  adapt_radius = radius;
  for (y = 0; y < height; y++)
    {
      x = 0;
      ymin = MAX (0, y - adapt_radius);
      ymax = MIN (height - 1, y + adapt_radius);
      xmin = MAX (0, x - adapt_radius);
      xmax = MIN (width - 1, x + adapt_radius);
      hist0   = 0;
      histrest = 0;
      hist255 = 0;
      histogram_clean (&histogram);
      histogram.xmin = xmin;
      histogram.ymin = ymin;
      histogram.xmax = xmax;
      histogram.ymax = ymax;
      add_vals (&histogram,
                src, width, bpp,
                histogram.xmin, histogram.ymin, histogram.xmax, histogram.ymax);

      for (x = 0; x < width; x++)
        {
          const guchar *pixel;

          ymin = MAX (0, y - adapt_radius); /* update ymin, ymax when adapt_radius changed (FILTER_ADAPTIVE) */
          ymax = MIN (height - 1, y + adapt_radius);
          xmin = MAX (0, x - adapt_radius);
          xmax = MIN (width - 1, x + adapt_radius);

          update_histogram (&histogram,
                            src, width, bpp, xmin, ymin, xmax, ymax);

          pos = (x + (y * width)) * bpp;
          pixel = histogram_get_median (&histogram, src + pos);

          if (filter_type & FILTER_RECURSIVE)
            {
              del_val (&histogram, src, width, bpp, x, y);
              pixel_copy (src + pos, pixel, bpp);
              add_val (&histogram, src, width, bpp, x, y);
            }

          pixel_copy (dst + pos, pixel, bpp);

          /*
           * Check the histogram and adjust the diameter accordingly...
           */
          if (filter_type & FILTER_ADAPTIVE)
            {
              if (hist0 >= adapt_radius || hist255 >= adapt_radius)
                {
                  if (adapt_radius < radius)
                    adapt_radius++;
                }
              else if (adapt_radius > 1)
                {
                  adapt_radius--;
                }
            }
        }

      progress += width;

      if (! preview && y % 32 == 0)
        gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
    }

  if (! preview)
    gimp_progress_update (1.0);
}
