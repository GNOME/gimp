/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Blinds plug-in. Distort an image as though it was stuck to
 * window blinds and the blinds where opened/closed.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * A fair proprotion of this code was taken from the Whirl plug-in
 * which was copyrighted by Federico Mena Quintero (as below).
 *
 * Whirl plug-in --- distort an image into a whirlpool
 * Copyright (C) 1997 Federico Mena Quintero
 *
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/***** Magic numbers *****/

#define PLUG_IN_PROC   "plug-in-blinds"
#define PLUG_IN_BINARY "blinds"
#define PLUG_IN_ROLE   "gimp-blinds"

#define SCALE_WIDTH    150

#define MAX_FANS       100

/* Variables set in dialog box */
typedef struct data
{
  gint                 angledsp;
  gint                 numsegs;
  GimpOrientationType  orientation;
  gboolean bg_trans;
} BlindVals;


static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static gboolean  blinds_dialog         (gint32         drawable_id);

static void      dialog_update_preview (gpointer       drawable_id,
                                        GimpPreview   *preview);
static void      apply_blinds          (gint32         drawable_id);


/* Array to hold each size of fans. And no there are not each the
 * same size (rounding errors...)
 */

static gint fanwidths[MAX_FANS];

/* Values when first invoked */
static BlindVals bvals =
{
  30,
  3,
  GIMP_ORIENTATION_HORIZONTAL,
  FALSE
};

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()


static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",       "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",          "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",       "Input drawable" },
    { GIMP_PDB_INT32,    "angle-dsp",      "Angle of Displacement" },
    { GIMP_PDB_INT32,    "num-segments",   "Number of segments in blinds" },
    { GIMP_PDB_INT32,    "orientation",    "The orientation { ORIENTATION-HORIZONTAL (0), ORIENTATION-VERTICAL (1) }" },
    { GIMP_PDB_INT32,    "bg-transparent", "Background transparent { FALSE, TRUE }" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Simulate an image painted on window blinds"),
                          "More here later",
                          "Andy Thomas",
                          "Andy Thomas",
                          "1997",
                          N_("_Blinds..."),
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
  static GimpParam  values[1];
  gint32            drawable_id;
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  run_mode    = param[0].data.d_int32;
  drawable_id = param[2].data.d_drawable;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &bvals);
      if (! blinds_dialog (drawable_id))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 7)
        status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
        {
          bvals.angledsp    = param[3].data.d_int32;
          bvals.numsegs     = param[4].data.d_int32;
          bvals.orientation = param[5].data.d_int32;
          bvals.bg_trans    = param[6].data.d_int32;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &bvals);
      break;

    default:
      break;
    }

  if (gimp_drawable_is_rgb  (drawable_id) ||
      gimp_drawable_is_gray (drawable_id))
    {
      gimp_progress_init (_("Adding blinds"));

      apply_blinds (drawable_id);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &bvals, sizeof (BlindVals));
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;
}


static gboolean
blinds_dialog (gint32 drawable_id)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkObject *size_data;
  GtkWidget *toggle;
  GtkWidget *horizontal;
  GtkWidget *vertical;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Blinds"), PLUG_IN_ROLE,
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

  preview = gimp_aspect_preview_new_from_drawable_id (drawable_id);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (dialog_update_preview),
                            GINT_TO_POINTER (drawable_id));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame =
    gimp_int_radio_group_new (TRUE, _("Orientation"),
                              G_CALLBACK (gimp_radio_button_update),
                              &bvals.orientation, bvals.orientation,

                              _("_Horizontal"), GIMP_ORIENTATION_HORIZONTAL,
                              &horizontal,

                              _("_Vertical"),   GIMP_ORIENTATION_VERTICAL,
                              &vertical,

                              NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_signal_connect_swapped (horizontal, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (vertical, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  frame = gimp_frame_new (_("Background"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle = gtk_check_button_new_with_mnemonic (_("_Transparent"));
  gtk_container_add (GTK_CONTAINER (frame), toggle);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &bvals.bg_trans);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.bg_trans);

  if (! gimp_drawable_has_alpha (drawable_id))
    {
      gtk_widget_set_sensitive (toggle, FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), FALSE);
    }

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  size_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                    _("_Displacement:"), SCALE_WIDTH, 0,
                                    bvals.angledsp, 1, 90, 1, 15, 0,
                                    TRUE, 0, 0,
                                    NULL, NULL);
  g_signal_connect (size_data, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &bvals.angledsp);
  g_signal_connect_swapped (size_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  size_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                    _("_Number of segments:"), SCALE_WIDTH, 0,
                                    bvals.numsegs, 1, MAX_FANS, 1, 2, 0,
                                    TRUE, 0, 0,
                                    NULL, NULL);
  g_signal_connect (size_data, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &bvals.numsegs);
  g_signal_connect_swapped (size_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
blindsapply (guchar *srow,
             guchar *drow,
             gint    width,
             gint    bpp,
             guchar *bg)
{
  guchar  *src;
  guchar  *dst;
  gint     i,j,k;
  gdouble  ang;
  gint     available;

  /* Make the row 'shrink' around points along its length */
  /* The bvals.numsegs determines how many segments to slip it in to */
  /* The angle is the conceptual 'rotation' of each of these segments */

  /* Note the row is considered to be made up of a two dim array actual
   * pixel locations and the RGB color at these locations.
   */

  /* In the process copy the src row to the destination row */

  /* Fill in with background color ? */
  for (i = 0 ; i < width ; i++)
    {
      dst = &drow[i*bpp];

      for (j = 0 ; j < bpp; j++)
        {
          dst[j] = bg[j];
        }
    }

  /* Apply it */

  available = width;
  for (i = 0; i < bvals.numsegs; i++)
    {
      /* Width of segs are variable */
      fanwidths[i] = available / (bvals.numsegs - i);
      available -= fanwidths[i];
    }

  /* do center points  first - just for fun...*/
  available = 0;
  for (k = 1; k <= bvals.numsegs; k++)
    {
      int point;

      point = available + fanwidths[k - 1] / 2;

      available += fanwidths[k - 1];

      src = &srow[point * bpp];
      dst = &drow[point * bpp];

      /* Copy pixels across */
      for (j = 0 ; j < bpp; j++)
        {
          dst[j] = src[j];
        }
    }

  /* Disp for each point */
  ang = (bvals.angledsp * 2 * G_PI) / 360; /* Angle in rads */
  ang = (1 - fabs (cos (ang)));

  available = 0;
  for (k = 0 ; k < bvals.numsegs; k++)
    {
      int dx; /* Amount to move by */
      int fw;

      for (i = 0 ; i < (fanwidths[k]/2) ; i++)
        {
          /* Copy pixels across of left half of fan */
          fw = fanwidths[k] / 2;
          dx = (int) (ang * ((double) (fw - (double)(i % fw))));

          src = &srow[(available + i) * bpp];
          dst = &drow[(available + i + dx) * bpp];

          for (j = 0; j < bpp; j++)
            {
              dst[j] = src[j];
            }

          /* Right side */
          j = i + 1;
          src = &srow[(available + fanwidths[k] - j
                       - (fanwidths[k] % 2)) * bpp];
          dst = &drow[(available + fanwidths[k] - j
                       - (fanwidths[k] % 2) - dx) * bpp];

          for (j = 0; j < bpp; j++)
            {
              dst[j] = src[j];
            }
        }

      available += fanwidths[k];
    }
}

static void
dialog_update_preview (gpointer     drawable_ptr,
                       GimpPreview *preview)
{
  gint32   drawable_id = GPOINTER_TO_INT (drawable_ptr);
  gint     y;
  guchar  *p, *buffer, *cache;
  GimpRGB  background;
  guchar   bg[4];
  gint     width, height, bpp;

  gimp_preview_get_size (preview, &width, &height);
  cache = gimp_drawable_get_thumbnail_data (drawable_id,
                                            &width, &height, &bpp);
  p = cache;

  gimp_context_get_background (&background);

  if (bvals.bg_trans)
    gimp_rgb_set_alpha (&background, 0.0);

  if (gimp_drawable_is_gray (drawable_id))
    {
      bg[0] = gimp_rgb_luminance_uchar (&background);
      gimp_rgba_get_uchar (&background, NULL, NULL, NULL, bg + 3);
    }
  else
    {
      gimp_rgba_get_uchar (&background, bg, bg + 1, bg + 2, bg + 3);
    }

  buffer = g_new (guchar, width * height * bpp);

  if (bvals.orientation == GIMP_ORIENTATION_VERTICAL)
    {
      for (y = 0; y < height; y++)
        {
          blindsapply (p,
                       buffer + y * width * bpp,
                       width,
                       bpp, bg);
          p += width * bpp;
        }
    }
  else
    {
      /* Horizontal blinds */
      /* Apply the blinds algo to a single column -
       * this act as a transfomation matrix for the
       * rows. Make row 0 invalid so we can find it again!
       */
      gint i;
      guchar *sr = g_new (guchar, height * 4);
      guchar *dr = g_new0 (guchar, height * 4);
      guchar dummybg[4] = {0, 0, 0, 0};

      /* Fill in with background color ? */
      for (i = 0 ; i < width ; i++)
        {
          gint j;
          gint bd = bpp;
          guchar *dst;
          dst = &buffer[i * bd];

          for (j = 0 ; j < bd; j++)
            {
              dst[j] = bg[j];
            }
        }

      for ( y = 0 ; y < height; y++)
        {
          sr[y] = y+1;
        }

      /* Bit of a fiddle since blindsapply really works on an image
       * row not a set of bytes. - preview can't be > 255
       * or must make dr sr int rows.
       */
      blindsapply (sr, dr, height, 1, dummybg);

      for (y = 0; y < height; y++)
        {
          if (dr[y] == 0)
            {
              /* Draw background line */
              p = buffer;
            }
          else
            {
              /* Draw line from src */
              p = cache +
                (width * bpp * (dr[y] - 1));
            }
          memcpy (buffer + y * width * bpp,
                  p,
                  width * bpp);
        }
      g_free (sr);
      g_free (dr);
    }

  gimp_preview_draw_buffer (preview, buffer, width * bpp);

  g_free (buffer);
  g_free (cache);
}

/* STEP tells us how many rows/columns to gulp down in one go... */
/* Note all the "4" literals around here are to do with the depths
 * of the images. Makes it easier to deal with for my small brain.
 */

#define STEP 40

static void
apply_blinds (gint32 drawable_id)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *format;
  guchar     *src_rows, *des_rows;
  gint        bytes;
  gint        x, y;
  GimpRGB     background;
  guchar      bg[4];
  gint        sel_x1, sel_y1;
  gint        sel_width, sel_height;

  gimp_context_get_background (&background);

  if (bvals.bg_trans)
    gimp_rgb_set_alpha (&background, 0.0);

  gimp_rgba_get_uchar (&background, bg, bg + 1, bg + 2, bg + 3);

  if (! gimp_drawable_mask_intersect (drawable_id,
                                      &sel_x1, &sel_y1,
                                      &sel_width, &sel_height))
    return;

  if (gimp_drawable_has_alpha (drawable_id))
    format = babl_format ("R'G'B'A u8");
  else
    format = babl_format ("R'G'B' u8");

  bytes = babl_format_get_bytes_per_pixel (format);

  src_buffer  = gimp_drawable_get_buffer (drawable_id);
  dest_buffer = gimp_drawable_get_shadow_buffer (drawable_id);

  src_rows = g_new (guchar, MAX (sel_width, sel_height) * bytes * STEP);
  des_rows = g_new (guchar, MAX (sel_width, sel_height) * bytes * STEP);

  if (bvals.orientation == GIMP_ORIENTATION_VERTICAL)
    {
      for (y = 0; y < sel_height; y += STEP)
        {
          gint rr;
          gint step;

          if ((y + STEP) > sel_height)
            step = sel_height - y;
          else
            step = STEP;

          gegl_buffer_get (src_buffer,
                           GEGL_RECTANGLE (sel_x1, sel_y1 + y,
                                           sel_width, step), 1.0,
                           format, src_rows,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          /* OK I could make this better */
          for (rr = 0; rr < STEP; rr++)
            blindsapply (src_rows + (sel_width * rr * bytes),
                         des_rows + (sel_width * rr * bytes),
                         sel_width, bytes, bg);

          gegl_buffer_set (dest_buffer,
                           GEGL_RECTANGLE (sel_x1, sel_y1 + y,
                                           sel_width, step), 0,
                           format, des_rows,
                           GEGL_AUTO_ROWSTRIDE);

          gimp_progress_update ((double) y / (double) sel_height);
        }
    }
  else
    {
      /* Horizontal blinds */
      /* Apply the blinds algo to a single column -
       * this act as a transfomation matrix for the
       * rows. Make row 0 invalid so we can find it again!
       */
      gint    i;
      gint   *sr  = g_new (gint, sel_height * bytes);
      gint   *dr  = g_new (gint, sel_height * bytes);
      guchar *dst = g_new (guchar, STEP * bytes);
      guchar  dummybg[4];

      memset (dummybg, 0, 4);
      memset (dr, 0, sel_height * bytes); /* all dr rows are background rows */
      for (y = 0; y < sel_height; y++)
        {
          sr[y] = y+1;
        }

      /* Hmmm. does this work portably? */
      /* This "swaps" the integers around that are held in in the
       * sr & dr arrays.
       */
      blindsapply ((guchar *) sr, (guchar *) dr,
                   sel_height, sizeof (gint), dummybg);

      /* Fill in with background color ? */
      for (i = 0 ; i < STEP ; i++)
        {
          int     j;
          guchar *bgdst;
          bgdst = &dst[i * bytes];

          for (j = 0 ; j < bytes; j++)
            {
              bgdst[j] = bg[j];
            }
        }

      for (x = 0; x < sel_width; x += STEP)
        {
          int     rr;
          int     step;
          guchar *p;

          if((x + STEP) > sel_width)
            step = sel_width - x;
          else
            step = STEP;

          gegl_buffer_get (src_buffer,
                           GEGL_RECTANGLE (sel_x1 + x, sel_y1,
                                           step, sel_height), 1.0,
                           format, src_rows,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          /* OK I could make this better */
          for (rr = 0; rr < sel_height; rr++)
            {
              if(dr[rr] == 0)
                {
                  /* Draw background line */
                  p = dst;
                }
              else
                {
                  /* Draw line from src */
                  p = src_rows + (step * bytes * (dr[rr] - 1));
                }
              memcpy (des_rows + (rr * step * bytes), p,
                      step * bytes);
            }

          gegl_buffer_set (dest_buffer,
                           GEGL_RECTANGLE (sel_x1 + x, sel_y1,
                                           step, sel_height), 0,
                           format, des_rows,
                           GEGL_AUTO_ROWSTRIDE);

          gimp_progress_update ((double) x / (double) sel_width);
        }

      g_free (dst);
      g_free (sr);
      g_free (dr);
    }

  g_free (src_rows);
  g_free (des_rows);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  gimp_progress_update (1.0);

  gimp_drawable_merge_shadow (drawable_id, TRUE);
  gimp_drawable_update (drawable_id,
                        sel_x1, sel_y1, sel_width, sel_height);
}
