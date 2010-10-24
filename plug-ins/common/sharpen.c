/*
 * Sharpen filters for GIMP - The GNU Image Manipulation Program
 *
 * Copyright 1997-1998 Michael Sweet (mike@easysw.com)
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
 *
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/*
 * Constants...
 */

#define PLUG_IN_PROC    "plug-in-sharpen"
#define PLUG_IN_BINARY  "sharpen"
#define PLUG_IN_ROLE    "gimp-sharpen"
#define PLUG_IN_VERSION "1.4.2 - 3 June 1998"
#define SCALE_WIDTH     100

/*
 * Local functions...
 */

static void     query (void);
static void     run   (const gchar      *name,
                       gint              nparams,
                       const GimpParam  *param,
                       gint             *nreturn_vals,
                       GimpParam       **returm_vals);

static void     compute_luts   (void);
static void     sharpen        (GimpDrawable *drawable);

static gboolean sharpen_dialog (GimpDrawable *drawable);

static void     preview_update (GimpPreview  *preview,
                                GimpDrawable *drawable);

typedef gint32 intneg;
typedef gint32 intpos;

static void     gray_filter  (int width, guchar *src, guchar *dst, intneg *neg0,
                              intneg *neg1, intneg *neg2);
static void     graya_filter (int width, guchar *src, guchar *dst, intneg *neg0,
                              intneg *neg1, intneg *neg2);
static void     rgb_filter   (int width, guchar *src, guchar *dst, intneg *neg0,
                              intneg *neg1, intneg *neg2);
static void     rgba_filter  (int width, guchar *src, guchar *dst, intneg *neg0,
                              intneg *neg1, intneg *neg2);


/*
 * Globals...
 */

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

typedef struct
{
  gint  sharpen_percent;
} SharpenParams;

static SharpenParams sharpen_params =
{
  10
};

static intneg neg_lut[256];   /* Negative coefficient LUT */
static intpos pos_lut[256];   /* Positive coefficient LUT */


MAIN ()

static void
query (void)
{
  static const GimpParamDef   args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"      },
    { GIMP_PDB_IMAGE,    "image",    "Input image"                       },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"                    },
    { GIMP_PDB_INT32,    "percent",  "Percent sharpening (default = 10)" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Make image sharper "
                             "(less powerful than Unsharp Mask)"),
                          "This plug-in selectively performs a convolution "
                          "filter on an image.",
                          "Michael Sweet <mike@easysw.com>",
                          "Copyright 1997-1998 by Michael Sweet",
                          PLUG_IN_VERSION,
                          N_("_Sharpen..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1]; /* Return values */
  GimpRunMode        run_mode;  /* Current run mode */
  GimpPDBStatusType  status;    /* Return status */
  GimpDrawable      *drawable;  /* Current image */

  /*
   * Initialize parameter data...
   */

  status   = GIMP_PDB_SUCCESS;
  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*
   * Get drawable information...
   */

  drawable = gimp_drawable_get (param[2].data.d_drawable);
  gimp_tile_cache_ntiles (2 * drawable->ntile_cols);


  /*
   * See how we will run
   */

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_PROC, &sharpen_params);

      /*
       * Get information from the dialog...
       */
      if (!sharpen_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*
       * Make sure all the arguments are present...
       */
      if (nparams != 4)
        status = GIMP_PDB_CALLING_ERROR;
      else
        sharpen_params.sharpen_percent = param[3].data.d_int32;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_PROC, &sharpen_params);
      break;

    default:
      status = GIMP_PDB_CALLING_ERROR;
      break;
    }

  /*
   * Sharpen the image...
   */

  if (status == GIMP_PDB_SUCCESS)
    {
      if ((gimp_drawable_is_rgb (drawable->drawable_id) ||
           gimp_drawable_is_gray (drawable->drawable_id)))
        {
          /*
           * Run!
           */
          sharpen (drawable);

          /*
           * If run mode is interactive, flush displays...
           */
          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*
           * Store data...
           */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC,
                           &sharpen_params, sizeof (SharpenParams));
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


static void
compute_luts (void)
{
  gint i;       /* Looping var */
  gint fact;    /* 1 - sharpness */

  fact = 100 - sharpen_params.sharpen_percent;
  if (fact < 1)
    fact = 1;

  for (i = 0; i < 256; i ++)
    {
      pos_lut[i] = 800 * i / fact;
      neg_lut[i] = (4 + pos_lut[i] - (i << 3)) >> 3;
    }
}

/*
 * 'sharpen()' - Sharpen an image using a convolution filter.
 */

static void
sharpen (GimpDrawable *drawable)
{
  GimpPixelRgn  src_rgn;        /* Source image region */
  GimpPixelRgn  dst_rgn;        /* Destination image region */
  guchar       *src_rows[4];    /* Source pixel rows */
  guchar       *src_ptr;        /* Current source pixel */
  guchar       *dst_row;        /* Destination pixel row */
  intneg       *neg_rows[4];    /* Negative coefficient rows */
  intneg       *neg_ptr;        /* Current negative coefficient */
  gint          i;              /* Looping vars */
  gint          y;              /* Current location in image */
  gint          row;            /* Current row in src_rows */
  gint          count;          /* Current number of filled src_rows */
  gint          width;          /* Byte width of the image */
  gint          x1;             /* Selection bounds */
  gint          y1;
  gint          y2;
  gint          sel_width;      /* Selection width */
  gint          sel_height;     /* Selection height */
  gint          img_bpp;        /* Bytes-per-pixel in image */
  void          (*filter)(int, guchar *, guchar *, intneg *, intneg *, intneg *);

  filter = NULL;

  if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                      &x1, &y1, &sel_width, &sel_height))
    return;

  y2 = y1 + sel_height;

  img_bpp = gimp_drawable_bpp (drawable->drawable_id);

  /*
   * Let the user know what we're doing...
   */
  gimp_progress_init (_("Sharpening"));

  /*
   * Setup for filter...
   */

  gimp_pixel_rgn_init (&src_rgn, drawable,
                       x1, y1, sel_width, sel_height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dst_rgn, drawable,
                       x1, y1, sel_width, sel_height, TRUE, TRUE);

  compute_luts ();

  width = sel_width * img_bpp;

  for (row = 0; row < 4; row ++)
    {
      src_rows[row] = g_new (guchar, width);
      neg_rows[row] = g_new (intneg, width);
    }

  dst_row = g_new (guchar, width);

  /*
   * Pre-load the first row for the filter...
   */

  gimp_pixel_rgn_get_row (&src_rgn, src_rows[0], x1, y1, sel_width);

  for (i = width, src_ptr = src_rows[0], neg_ptr = neg_rows[0];
       i > 0;
       i --, src_ptr ++, neg_ptr ++)
    *neg_ptr = neg_lut[*src_ptr];

  row   = 1;
  count = 1;

  /*
   * Select the filter...
   */

  switch (img_bpp)
    {
    case 1 :
      filter = gray_filter;
      break;
    case 2 :
      filter = graya_filter;
      break;
    case 3 :
      filter = rgb_filter;
      break;
    case 4 :
      filter = rgba_filter;
      break;
    };

  /*
   * Sharpen...
   */

  for (y = y1; y < y2; y ++)
    {
      /*
       * Load the next pixel row...
       */

      if ((y + 1) < y2)
        {
          /*
           * Check to see if our src_rows[] array is overflowing yet...
           */

          if (count >= 3)
            count --;

          /*
           * Grab the next row...
           */

          gimp_pixel_rgn_get_row (&src_rgn, src_rows[row],
                                  x1, y + 1, sel_width);
          for (i = width, src_ptr = src_rows[row], neg_ptr = neg_rows[row];
               i > 0;
               i --, src_ptr ++, neg_ptr ++)
            *neg_ptr = neg_lut[*src_ptr];

          count ++;
          row = (row + 1) & 3;
        }
      else
        {
          /*
           * No more pixels at the bottom...  Drop the oldest samples...
           */

          count --;
        }

      /*
       * Now sharpen pixels and save the results...
       */

      if (count == 3)
        {
          (* filter) (sel_width, src_rows[(row + 2) & 3], dst_row,
                      neg_rows[(row + 1) & 3] + img_bpp,
                      neg_rows[(row + 2) & 3] + img_bpp,
                      neg_rows[(row + 3) & 3] + img_bpp);

          /*
           * Set the row...
           */

          gimp_pixel_rgn_set_row (&dst_rgn, dst_row, x1, y, sel_width);
        }
      else if (count == 2)
        {
          if (y == y1)      /* first row */
            gimp_pixel_rgn_set_row (&dst_rgn, src_rows[0],
                                    x1, y, sel_width);
          else                  /* last row  */
            gimp_pixel_rgn_set_row (&dst_rgn, src_rows[(sel_height - 1) & 3],
                                    x1, y, sel_width);
        }

      if ((y & 15) == 0)
        gimp_progress_update ((gdouble) (y - y1) / (gdouble) sel_height);
    }

  /*
   * OK, we're done.  Free all memory used...
   */

  for (row = 0; row < 4; row ++)
    {
      g_free (src_rows[row]);
      g_free (neg_rows[row]);
    }

  g_free (dst_row);

  /*
   * Update the screen...
   */

  gimp_progress_update (1.0);
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id,
                        x1, y1, sel_width, sel_height);
}


/*
 * 'sharpen_dialog()' - Popup a dialog window for the filter box size...
 */

static gboolean
sharpen_dialog (GimpDrawable *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *preview;
  GtkWidget     *table;
  GtkAdjustment *adj;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Sharpen"), PLUG_IN_ROLE,
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
                    drawable);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Sharpness:"), SCALE_WIDTH, 0,
                              sharpen_params.sharpen_percent,
                              1, 99, 1, 10, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &sharpen_params.sharpen_percent);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
preview_update (GimpPreview  *preview,
                GimpDrawable *drawable)
{
  GimpPixelRgn  src_rgn;        /* Source image region */
  guchar       *src_ptr;        /* Current source pixel */
  guchar       *dst_ptr;        /* Current destination pixel */
  intneg       *neg_ptr;        /* Current negative pixel */
  gint          i;              /* Looping var */
  gint          y;              /* Current location in image */
  gint          width;          /* Byte width of the image */
  gint          x1, y1;
  gint          preview_width, preview_height;
  guchar       *preview_src, *preview_dst;
  intneg       *preview_neg;
  gint          img_bpp;        /* Bytes-per-pixel in image */

  void          (*filter)(int, guchar *, guchar *, intneg *, intneg *, intneg *);

  filter = NULL;

  compute_luts();

  gimp_preview_get_position (preview, &x1, &y1);
  gimp_preview_get_size (preview, &preview_width, &preview_height);

  img_bpp = gimp_drawable_bpp (drawable->drawable_id);


  preview_src = g_new (guchar, preview_width * preview_height * img_bpp);
  preview_neg = g_new (intneg, preview_width * preview_height * img_bpp);
  preview_dst = g_new (guchar, preview_width * preview_height * img_bpp);

  gimp_pixel_rgn_init (&src_rgn, drawable,
                       x1, y1, preview_width, preview_height,
                       FALSE, FALSE);

  width = preview_width * img_bpp;

  /*
   * Load the preview area...
   */

  gimp_pixel_rgn_get_rect (&src_rgn, preview_src, x1, y1,
                           preview_width, preview_height);

  for (i = width * preview_height, src_ptr = preview_src, neg_ptr = preview_neg;
       i > 0;
       i --)
    *neg_ptr++ = neg_lut[*src_ptr++];

  /*
   * Select the filter...
   */

  switch (img_bpp)
    {
    case 1:
      filter = gray_filter;
      break;
    case 2:
      filter = graya_filter;
      break;
    case 3:
      filter = rgb_filter;
      break;
    case 4:
      filter = rgba_filter;
      break;
    default:
      g_error ("Programmer stupidity error: img_bpp is %d\n",
               img_bpp);
    }

  /*
   * Sharpen...
   */

  memcpy (preview_dst, preview_src, width);
  memcpy (preview_dst + width * (preview_height - 1),
          preview_src + width * (preview_height - 1),
          width);

  for (y = preview_height - 2, src_ptr = preview_src + width,
           neg_ptr = preview_neg + width + img_bpp,
           dst_ptr = preview_dst + width;
       y > 0;
       y --, src_ptr += width, neg_ptr += width, dst_ptr += width)
    (*filter)(preview_width, src_ptr, dst_ptr, neg_ptr - width,
              neg_ptr, neg_ptr + width);

  gimp_preview_draw_buffer (preview, preview_dst, preview_width * img_bpp);

  g_free (preview_src);
  g_free (preview_neg);
  g_free (preview_dst);
}

/*
 * 'gray_filter()' - Sharpen grayscale pixels.
 */

static void
gray_filter (gint    width,     /* I - Width of line in pixels */
             guchar *src,       /* I - Source line */
             guchar *dst,       /* O - Destination line */
             intneg *neg0,      /* I - Top negative coefficient line */
             intneg *neg1,      /* I - Middle negative coefficient line */
             intneg *neg2)      /* I - Bottom negative coefficient line */
{
  intpos pixel;         /* New pixel value */

  *dst++ = *src++;
  width -= 2;

  while (width > 0)
    {
      pixel = (pos_lut[*src++] - neg0[-1] - neg0[0] - neg0[1] -
               neg1[-1] - neg1[1] -
               neg2[-1] - neg2[0] - neg2[1]);
      pixel = (pixel + 4) >> 3;
      *dst++ = CLAMP0255 (pixel);

      neg0 ++;
      neg1 ++;
      neg2 ++;
      width --;
    }

  *dst++ = *src++;
}

/*
 * 'graya_filter()' - Sharpen grayscale+alpha pixels.
 */

static void
graya_filter (gint   width,     /* I - Width of line in pixels */
              guchar *src,      /* I - Source line */
              guchar *dst,      /* O - Destination line */
              intneg *neg0,     /* I - Top negative coefficient line */
              intneg *neg1,     /* I - Middle negative coefficient line */
              intneg *neg2)     /* I - Bottom negative coefficient line */
{
  intpos pixel;         /* New pixel value */

  *dst++ = *src++;
  *dst++ = *src++;
  width -= 2;

  while (width > 0)
    {
      pixel = (pos_lut[*src++] - neg0[-2] - neg0[0] - neg0[2] -
               neg1[-2] - neg1[2] -
               neg2[-2] - neg2[0] - neg2[2]);
      pixel = (pixel + 4) >> 3;
      *dst++ = CLAMP0255 (pixel);

      *dst++ = *src++;
      neg0 += 2;
      neg1 += 2;
      neg2 += 2;
      width --;
    }

  *dst++ = *src++;
  *dst++ = *src++;
}

/*
 * 'rgb_filter()' - Sharpen RGB pixels.
 */

static void
rgb_filter (gint    width,      /* I - Width of line in pixels */
            guchar *src,        /* I - Source line */
            guchar *dst,        /* O - Destination line */
            intneg *neg0,       /* I - Top negative coefficient line */
            intneg *neg1,       /* I - Middle negative coefficient line */
            intneg *neg2)       /* I - Bottom negative coefficient line */
{
  intpos pixel;         /* New pixel value */

  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
  width -= 2;

  while (width > 0)
    {
      pixel = (pos_lut[*src++] - neg0[-3] - neg0[0] - neg0[3] -
               neg1[-3] - neg1[3] -
               neg2[-3] - neg2[0] - neg2[3]);
      pixel = (pixel + 4) >> 3;
      *dst++ = CLAMP0255 (pixel);

      pixel = (pos_lut[*src++] - neg0[-2] - neg0[1] - neg0[4] -
               neg1[-2] - neg1[4] -
               neg2[-2] - neg2[1] - neg2[4]);
      pixel = (pixel + 4) >> 3;
      *dst++ = CLAMP0255 (pixel);

      pixel = (pos_lut[*src++] - neg0[-1] - neg0[2] - neg0[5] -
               neg1[-1] - neg1[5] -
               neg2[-1] - neg2[2] - neg2[5]);
      pixel = (pixel + 4) >> 3;
      *dst++ = CLAMP0255 (pixel);

      neg0 += 3;
      neg1 += 3;
      neg2 += 3;
      width --;
    }

  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
}

/*
 * 'rgba_filter()' - Sharpen RGBA pixels.
 */

static void
rgba_filter (gint   width,      /* I - Width of line in pixels */
             guchar *src,       /* I - Source line */
             guchar *dst,       /* O - Destination line */
             intneg *neg0,      /* I - Top negative coefficient line */
             intneg *neg1,      /* I - Middle negative coefficient line */
             intneg *neg2)      /* I - Bottom negative coefficient line */
{
  intpos pixel;         /* New pixel value */

  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
  width -= 2;

  while (width > 0)
    {
      pixel = (pos_lut[*src++] - neg0[-4] - neg0[0] - neg0[4] -
               neg1[-4] - neg1[4] -
               neg2[-4] - neg2[0] - neg2[4]);
      pixel = (pixel + 4) >> 3;
      *dst++ = CLAMP0255 (pixel);

      pixel = (pos_lut[*src++] - neg0[-3] - neg0[1] - neg0[5] -
               neg1[-3] - neg1[5] -
               neg2[-3] - neg2[1] - neg2[5]);
      pixel = (pixel + 4) >> 3;
      *dst++ = CLAMP0255 (pixel);

      pixel = (pos_lut[*src++] - neg0[-2] - neg0[2] - neg0[6] -
               neg1[-2] - neg1[6] -
               neg2[-2] - neg2[2] - neg2[6]);
      pixel = (pixel + 4) >> 3;
      *dst++ = CLAMP0255 (pixel);

      *dst++ = *src++;

      neg0 += 4;
      neg1 += 4;
      neg2 += 4;
      width --;
    }

  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
  *dst++ = *src++;
}
