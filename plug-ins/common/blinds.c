/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Blinds plug-in. Distort an image as though it was stuck to
 * window blinds and the blinds where opened/closed.
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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/***** Magic numbers *****/

#define SCALE_WIDTH  150

#define MAX_FANS      10

#define HORIZONTAL     0
#define VERTICAL       1

/* Variables set in dialog box */
typedef struct data
{
  gint angledsp;
  gint numsegs;
  gint orientation;
  gint bg_trans;
} BlindVals;

#define PREVIEW_SIZE 128
static GtkWidget *preview;
static gint       preview_width, preview_height, preview_bpp;
static guchar    *preview_cache;

/* Array to hold each size of fans. And no there are not each the
 * same size (rounding errors...)
 */

static gint fanwidths[MAX_FANS];

static GimpDrawable *blindsdrawable;

static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static gboolean  blinds_dialog         (void);

static void      blinds_scale_update   (GtkAdjustment *adjustment,
                                        gint          *size_val);
static void      blinds_radio_update   (GtkWidget     *widget,
                                        gpointer       data);
static void      blinds_button_update  (GtkWidget     *widget,
                                        gpointer       data);
static void      dialog_update_preview (void);
static void      apply_blinds          (void);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

/* Values when first invoked */
static BlindVals bvals =
{
  30,
  3,
  HORIZONTAL,
  FALSE
};

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT32, "angle_dsp", "Angle of Displacement " },
    { GIMP_PDB_INT32, "number_of_segments", "Number of segments in blinds" },
    { GIMP_PDB_INT32, "orientation", "orientation; 0 = Horizontal, 1 = Vertical" },
    { GIMP_PDB_INT32, "backgndg_trans", "background transparent; FALSE,TRUE" }
  };

  gimp_install_procedure ("plug_in_blinds",
                          "Adds a blinds effect to the image. Rather like "
                          "putting the image on a set of window blinds and "
                          "the closing or opening the blinds",
                          "More here later",
                          "Andy Thomas",
                          "Andy Thomas",
                          "1997",
                          N_("_Blinds..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register ("plug_in_blinds",
                             N_("<Image>/Filters/Distorts"));
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam values[1];
  GimpDrawable *drawable;
  GimpRunMode run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  blindsdrawable = drawable =
    gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data ("plug_in_blinds", &bvals);
      if (! blinds_dialog())
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 7)
        status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
        {
          bvals.angledsp = param[3].data.d_int32;
          bvals.numsegs = param[4].data.d_int32;
          bvals.orientation = param[5].data.d_int32;
          bvals.bg_trans = param[6].data.d_int32;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data ("plug_in_blinds", &bvals);
      break;

    default:
      break;
    }

  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      gimp_progress_init ( _("Adding Blinds..."));

      apply_blinds ();

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data ("plug_in_blinds", &bvals, sizeof (BlindVals));
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static gboolean
blinds_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *align;
  GtkWidget *frame;
  GtkWidget *table;
  GtkObject *size_data;
  GtkWidget *toggle;
  gboolean   run;

  gimp_ui_init ("blinds", TRUE);

  dlg = gimp_dialog_new (_("Blinds"), "blinds",
                         NULL, 0,
                         gimp_standard_help_func, "plug-in-blinds",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  align = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, FALSE, 0);
  gtk_widget_show (align);

  preview = gimp_preview_area_new ();
  preview_width = preview_height = PREVIEW_SIZE;
  preview_cache =
    gimp_drawable_get_thumbnail_data (blindsdrawable->drawable_id,
                                      &preview_width,
                                      &preview_height,
                                      &preview_bpp);
  gtk_widget_set_size_request (preview, preview_width, preview_height);
  gtk_container_add (GTK_CONTAINER (align), preview);
  gtk_widget_show (preview);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame =
    gimp_int_radio_group_new (TRUE, _("Orientation"),
                              G_CALLBACK (blinds_radio_update),
                              &bvals.orientation, bvals.orientation,

                              _("_Horizontal"), HORIZONTAL, NULL,
                              _("_Vertical"),   VERTICAL,   NULL,

                              NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame = gimp_frame_new (_("Background"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle = gtk_check_button_new_with_mnemonic (_("_Transparent"));
  gtk_container_add (GTK_CONTAINER (frame), toggle);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (blinds_button_update),
                    &bvals.bg_trans);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.bg_trans);

  if (!gimp_drawable_has_alpha (blindsdrawable->drawable_id))
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
  g_signal_connect (size_data, "value_changed",
                    G_CALLBACK (blinds_scale_update),
                    &bvals.angledsp);

  size_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                    _("_Number of segments:"), SCALE_WIDTH, 0,
                                    bvals.numsegs, 1, MAX_FANS, 1, 2, 0,
                                    TRUE, 0, 0,
                                    NULL, NULL);
  g_signal_connect (size_data, "value_changed",
                    G_CALLBACK (blinds_scale_update),
                    &bvals.numsegs);

  gtk_widget_show (dlg);

  dialog_update_preview ();

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}

static void
blinds_radio_update (GtkWidget *widget,
                     gpointer   data)
{
  gimp_radio_button_update (widget, data);

  if (GTK_TOGGLE_BUTTON (widget)->active)
    dialog_update_preview ();
}

static void
blinds_button_update (GtkWidget *widget,
                      gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  dialog_update_preview ();
}

static void
blinds_scale_update (GtkAdjustment *adjustment,
                     gint          *value)
{
  gimp_int_adjustment_update (adjustment, value);

  dialog_update_preview ();
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
  /* The bvals.numsegs determins how many segments to slip it in to */
  /* The angle is the conceptual 'rotation' of each of these segments */

  /* Note the row is considered to be made up of a two dim array actual
   * pixel locations and the RGB colour at these locations.
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
dialog_update_preview (void)
{
  gint     y;
  guchar  *p, *buffer;
  GimpRGB  background;
  guchar   bg[4];

  p = preview_cache;

  gimp_palette_get_background (&background);

  if (bvals.bg_trans)
    gimp_rgb_set_alpha (&background, 0.0);

  gimp_drawable_get_color_uchar (blindsdrawable->drawable_id, &background, bg);

  buffer = g_new (guchar, preview_width * preview_height * preview_bpp);

  if (bvals.orientation)
    {
      for (y = 0; y < preview_height; y++)
        {
          blindsapply (p,
                       buffer + y * preview_width * preview_bpp,
                       preview_width,
                       preview_bpp, bg);
          p += preview_width * preview_bpp;
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
      guchar *sr = g_new (guchar, preview_height * 4);
      guchar *dr = g_new0 (guchar, preview_height * 4);
      guchar dummybg[4] = {0, 0, 0, 0};

      /* Fill in with background color ? */
      for (i = 0 ; i < preview_width ; i++)
        {
          gint j;
          gint bd = preview_bpp;
          guchar *dst;
          dst = &buffer[i * bd];

          for (j = 0 ; j < bd; j++)
            {
              dst[j] = bg[j];
            }
        }

      for ( y = 0 ; y < preview_height; y++)
        {
          sr[y] = y+1;
        }

      /* Bit of a fiddle since blindsapply really works on an image
       * row not a set of bytes. - preview can't be > 255
       * or must make dr sr int rows.
       */
      blindsapply (sr, dr, preview_height, 1, dummybg);

      for (y = 0; y < preview_height; y++)
        {
          if (dr[y] == 0)
            {
              /* Draw background line */
              p = buffer;
            }
          else
            {
              /* Draw line from src */
              p = preview_cache +
                (preview_width * preview_bpp * (dr[y] - 1));
            }
          memcpy (buffer + y * preview_width * preview_bpp,
                  p,
                  preview_width * preview_bpp);
        }
      g_free (sr);
      g_free (dr);
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                          0, 0, preview_width, preview_height,
                          gimp_drawable_type (blindsdrawable->drawable_id),
                          buffer,
                          preview_width * preview_bpp);

  g_free (buffer);
}

/* STEP tells us how many rows/columns to gulp down in one go... */
/* Note all the "4" literals around here are to do with the depths
 * of the images. Makes it easier to deal with for my small brain.
 */

#define STEP 40

static void
apply_blinds (void)
{
  GimpPixelRgn  des_rgn;
  GimpPixelRgn  src_rgn;
  guchar       *src_rows, *des_rows;
  gint          x, y;
  GimpRGB       background;
  guchar        bg[4];
  gint          sel_x1, sel_y1, sel_x2, sel_y2;
  gint          sel_width, sel_height;

  gimp_palette_get_background (&background);

  if (bvals.bg_trans)
    gimp_rgb_set_alpha (&background, 0.0);

  gimp_drawable_get_color_uchar (blindsdrawable->drawable_id, &background, bg);

  gimp_drawable_mask_bounds (blindsdrawable->drawable_id,
                             &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width  = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  gimp_pixel_rgn_init (&src_rgn, blindsdrawable,
                       sel_x1, sel_y1, sel_width, sel_height, FALSE, FALSE);
  gimp_pixel_rgn_init (&des_rgn, blindsdrawable,
                       sel_x1, sel_y1, sel_width, sel_height, TRUE, TRUE);

  src_rows = g_new (guchar, MAX (sel_width, sel_height) * 4 * STEP);
  des_rows = g_new (guchar, MAX (sel_width, sel_height) * 4 * STEP);

  if (bvals.orientation)
    {
      for (y = 0; y < sel_height; y += STEP)
        {
          gint rr;
          gint step;

          if((y + STEP) > sel_height)
            step = sel_height - y;
          else
            step = STEP;

          gimp_pixel_rgn_get_rect (&src_rgn,
                                   src_rows,
                                   sel_x1,
                                   sel_y1 + y,
                                   sel_width,
                                   step);

          /* OK I could make this better */
          for (rr = 0; rr < STEP; rr++)
            blindsapply (src_rows + (sel_width * rr * src_rgn.bpp),
                         des_rows + (sel_width * rr * src_rgn.bpp),
                         sel_width, src_rgn.bpp, bg);

          gimp_pixel_rgn_set_rect (&des_rgn,
                                   des_rows,
                                   sel_x1,
                                   sel_y1 + y,
                                   sel_width,
                                   step);

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
      gint   *sr  = g_new (gint, sel_height * 4);
      gint   *dr  = g_new (gint, sel_height * 4);
      guchar *dst = g_new (guchar, STEP * 4);
      guchar  dummybg[4];

      memset (dummybg, 0, 4);
      memset (dr, 0, sel_height * 4); /* all dr rows are background rows */
      for (y = 0; y < sel_height; y++)
        {
          sr[y] = y+1;
        }

      /* Hmmm. does this work portably? */
      /* This "swaps the intergers around that are held in in the
       * sr & dr arrays.
       */
      blindsapply ((guchar *) sr, (guchar *) dr,
                   sel_height, sizeof (gint), dummybg);

      /* Fill in with background color ? */
      for (i = 0 ; i < STEP ; i++)
        {
          int     j;
          guchar *bgdst;
          bgdst = &dst[i * src_rgn.bpp];

          for (j = 0 ; j < src_rgn.bpp; j++)
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

          gimp_pixel_rgn_get_rect (&src_rgn,
                                   src_rows,
                                   sel_x1 + x,
                                   sel_y1,
                                   step,
                                   sel_height);

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
                  p = src_rows + (step * src_rgn.bpp * (dr[rr] - 1));
                }
              memcpy (des_rows + (rr * step * src_rgn.bpp), p,
                      step * src_rgn.bpp);
            }

          gimp_pixel_rgn_set_rect (&des_rgn,
                                   des_rows,
                                   sel_x1 + x,
                                   sel_y1,
                                   step,
                                   sel_height);

          gimp_progress_update ((double) x / (double) sel_width);
        }

      g_free (dst);
      g_free (sr);
      g_free (dr);
    }

  g_free (src_rows);
  g_free (des_rows);

  gimp_drawable_flush (blindsdrawable);
  gimp_drawable_merge_shadow (blindsdrawable->drawable_id, TRUE);
  gimp_drawable_update (blindsdrawable->drawable_id,
                        sel_x1, sel_y1, sel_width, sel_height);

}
