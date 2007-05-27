/* GIMP - The GNU Image Manipulation Program
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

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define GAUSS_PROC      "plug-in-gauss"
#define GAUSS_IIR_PROC  "plug-in-gauss-iir"
#define GAUSS_IIR2_PROC "plug-in-gauss-iir2"
#define GAUSS_RLE_PROC  "plug-in-gauss-rle"
#define GAUSS_RLE2_PROC "plug-in-gauss-rle2"
#define PLUG_IN_BINARY  "gauss"

typedef enum
{
  BLUR_IIR,
  BLUR_RLE
} BlurMethod;

typedef struct
{
  gdouble     horizontal;
  gdouble     vertical;
  BlurMethod  method;
} BlurValues;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static void      gauss             (GimpDrawable *drawable,
                                    gdouble       horizontal,
                                    gdouble       vertical,
                                    BlurMethod    method,
                                    GtkWidget    *preview);

static void      update_preview    (GtkWidget    *preview,
                                    GtkWidget    *size);
/*
 * Gaussian blur interface
 */
static gboolean  gauss_dialog      (gint32        image_ID,
                                    GimpDrawable *drawable);

/*
 * Gaussian blur helper functions
 */
static void      find_iir_constants    (gdouble  n_p[],
                                        gdouble  n_m[],
                                        gdouble  d_p[],
                                        gdouble  d_m[],
                                        gdouble  bd_p[],
                                        gdouble  bd_m[],
                                        gdouble  std_dev);

static void      transfer_pixels   (const gdouble *src1,
                                    const gdouble *src2,
                                    guchar        *dest,
                                    gint           bytes,
                                    gint           width);

static void      make_rle_curve    (gdouble   sigma,
                                    gint    **p_curve,
                                    gint     *p_length,
                                    gint    **p_sum,
                                    gint     *p_total);

static void      free_rle_curve    (gint     *curve,
                                    gint      length,
                                    gint     *sum);

static inline gint run_length_encode (const guchar *src,
                                      gint         *repeat,
                                      gint         *dest,
                                      gint          bytes,
                                      gint          width,
                                      gint          border,
                                      gboolean      pack);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static BlurValues bvals =
{
  5.0,  /*  x radius  */
  5.0,  /*  y radius  */
  BLUR_RLE
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",   "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",      "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",   "Input drawable" },
    { GIMP_PDB_FLOAT,    "horizontal", "Horizontal radius of gaussian blur (in pixels, > 0.0)" },
    { GIMP_PDB_FLOAT,    "vertical",   "Vertical radius of gaussian blur (in pixels, > 0.0)" },
    { GIMP_PDB_INT32,    "method",     "IIR (0) or RLE (1)" }
  };

  static const GimpParamDef args1[] =
  {
    { GIMP_PDB_INT32,    "run-mode",   "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",      "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",   "Input drawable" },
    { GIMP_PDB_FLOAT,    "radius",     "Radius of gaussian blur (in pixels, > 0.0)" },
    { GIMP_PDB_INT32,    "horizontal", "Blur in horizontal direction" },
    { GIMP_PDB_INT32,    "vertical",   "Blur in vertical direction" }
  };

  static const GimpParamDef args2[] =
  {
    { GIMP_PDB_INT32,    "run-mode",   "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",      "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",   "Input drawable" },
    { GIMP_PDB_FLOAT,    "horizontal", "Horizontal radius of gaussian blur (in pixels, > 0.0)" },
    { GIMP_PDB_FLOAT,    "vertical",   "Vertical radius of gaussian blur (in pixels, > 0.0)" }
  };

  gimp_install_procedure (GAUSS_PROC,
                          N_("Simplest, most commonly used way of blurring"),
                          "Applies a gaussian blur to the drawable, with "
                          "specified radius of affect.  The standard deviation "
                          "of the normal distribution used to modify pixel "
                          "values is calculated based on the supplied radius.  "
                          "Horizontal and vertical blurring can be "
                          "independently invoked by specifying only one to "
                          "run.  The IIR gaussian blurring works best for "
                          "large radius values and for images which are not "
                          "computer-generated.",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1995-1996",
                          N_("_Gaussian Blur..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_install_procedure (GAUSS_IIR_PROC,
                          N_("Apply a gaussian blur"),
                          "Applies a gaussian blur to the drawable, with "
                          "specified radius of affect.  The standard deviation "
                          "of the normal distribution used to modify pixel "
                          "values is calculated based on the supplied radius.  "
                          "Horizontal and vertical blurring can be "
                          "independently invoked by specifying only one to "
                          "run.  The IIR gaussian blurring works best for "
                          "large radius values and for images which are not "
                          "computer-generated.",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1995-1996",
                          NULL,
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args1), 0,
                          args1, NULL);

  gimp_install_procedure (GAUSS_IIR2_PROC,
                          N_("Apply a gaussian blur"),
                          "Applies a gaussian blur to the drawable, with "
                          "specified radius of affect.  The standard deviation "
                          "of the normal distribution used to modify pixel "
                          "values is calculated based on the supplied radius.  "
                          "This radius can be specified indepently on for the "
                          "horizontal and the vertical direction. The IIR "
                          "gaussian blurring works best for large radius "
                          "values and for images which are not "
                          "computer-generated.",
                          "Spencer Kimball, Peter Mattis & Sven Neumann",
                          "Spencer Kimball, Peter Mattis & Sven Neumann",
                          "1995-2000",
                          NULL,
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args2), 0,
                          args2, NULL);

  gimp_install_procedure (GAUSS_RLE_PROC,
                          N_("Apply a gaussian blur"),
                          "Applies a gaussian blur to the drawable, with "
                          "specified radius of affect.  The standard deviation "
                          "of the normal distribution used to modify pixel "
                          "values is calculated based on the supplied radius.  "
                          "Horizontal and vertical blurring can be "
                          "independently invoked by specifying only one to "
                          "run.  The RLE gaussian blurring performs most "
                          "efficiently on computer-generated images or images "
                          "with large areas of constant intensity.",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1995-1996",
                          NULL,
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args1), 0,
                          args1, NULL);

  gimp_install_procedure (GAUSS_RLE2_PROC,
                          N_("Apply a gaussian blur"),
                          "Applies a gaussian blur to the drawable, with "
                          "specified radius of affect.  The standard deviation "
                          "of the normal distribution used to modify pixel "
                          "values is calculated based on the supplied radius.  "
                          "This radius can be specified indepently on for the "
                          "horizontal and the vertical direction. The RLE "
                          "gaussian blurring performs most efficiently on "
                          "computer-generated images or images with large "
                          "areas of constant intensity.",
                          "Spencer Kimball, Peter Mattis & Sven Neumann",
                          "Spencer Kimball, Peter Mattis & Sven Neumann",
                          "1995-2000",
                          NULL,
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args2), 0,
                          args2, NULL);

  gimp_plugin_menu_register (GAUSS_PROC, "<Image>/Filters/Blur");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  gint32             image_ID;
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gdouble            radius = 0.;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified image and drawable  */
  image_ID = param[1].data.d_image;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  set the tile cache size so that the gaussian blur works well  */
  gimp_tile_cache_ntiles (2*
                          (MAX (drawable->width, drawable->height) /
                           gimp_tile_width () + 1));


  if (strcmp (name, GAUSS_PROC) == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data (GAUSS_PROC, &bvals);

          /*  First acquire information with a dialog  */
          if (! gauss_dialog (image_ID, drawable))
            return;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 6)
            status = GIMP_PDB_CALLING_ERROR;

          if (status == GIMP_PDB_SUCCESS)
            {
              bvals.horizontal = param[3].data.d_float;
              bvals.vertical   = param[4].data.d_float;
              bvals.method     = param[5].data.d_int32;
            }
          if (status == GIMP_PDB_SUCCESS &&
              (bvals.horizontal <= 0.0 && bvals.vertical <= 0.0))
            status = GIMP_PDB_CALLING_ERROR;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /*  Possibly retrieve data  */
          gimp_get_data (GAUSS_PROC, &bvals);
          break;

        default:
          break;
        }
    }
  else if (strcmp (name, GAUSS_IIR_PROC) == 0)
    {
      if (nparams != 6)
        status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
        {
          radius           = param[3].data.d_float;
          bvals.horizontal = (param[4].data.d_int32) ? radius : 0.;
          bvals.vertical   = (param[5].data.d_int32) ? radius : 0.;
          bvals.method     = BLUR_IIR;
        }

      if (radius <= 0.0)
        status = GIMP_PDB_CALLING_ERROR;

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          if (! gauss_dialog (image_ID, drawable))
            return;
        }
    }
  else if (strcmp (name, GAUSS_IIR2_PROC) == 0)
    {
      if (nparams != 5)
        status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
        {
          bvals.horizontal = param[3].data.d_float;
          bvals.vertical   = param[4].data.d_float;
          bvals.method     = BLUR_IIR;
        }

      if (bvals.horizontal <= 0.0 && bvals.vertical <= 0.0)
        status = GIMP_PDB_CALLING_ERROR;

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          if (! gauss_dialog (image_ID, drawable))
            return;
        }
    }
  else if (strcmp (name, GAUSS_RLE_PROC) == 0)
    {
      if (nparams != 6)
        status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
        {
          radius           = param[3].data.d_float;
          bvals.horizontal = (param[4].data.d_int32) ? radius : 0.;
          bvals.vertical   = (param[5].data.d_int32) ? radius : 0.;
          bvals.method     = BLUR_RLE;
        }

      if (radius <= 0.0)
        status = GIMP_PDB_CALLING_ERROR;

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          if (! gauss_dialog (image_ID, drawable))
            return;
        }
    }
  else if (strcmp (name, GAUSS_RLE2_PROC) == 0)
    {
      if (nparams != 5)
        status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
        {
          bvals.horizontal = param[3].data.d_float;
          bvals.vertical   = param[4].data.d_float;
          bvals.method     = BLUR_RLE;
        }

      if (bvals.horizontal <= 0.0 && bvals.vertical <= 0.0)
        status = GIMP_PDB_CALLING_ERROR;

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          if (! gauss_dialog (image_ID, drawable))
            return;
        }
    }
  else
    status = GIMP_PDB_CALLING_ERROR;

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id))
        {
          gimp_progress_init (_("Gaussian Blur"));

          /*  run the gaussian blur  */
          gauss (drawable,
                 bvals.horizontal, bvals.vertical,
                 bvals.method,
                 NULL);

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (GAUSS_PROC, &bvals, sizeof (BlurValues));

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();
        }
      else
        {
          g_message (_("Cannot operate on indexed color images."));
          status = GIMP_PDB_EXECUTION_ERROR;
        }

      gimp_drawable_detach (drawable);
    }

  values[0].data.d_status = status;
}



static gboolean
gauss_dialog (gint32        image_ID,
              GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *size;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *preview;

  GimpUnit   unit;
  gdouble    xres;
  gdouble    yres;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Gaussian Blur"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, GAUSS_PROC,

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

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  parameter settings  */
  frame = gimp_frame_new (_("Blur Radius"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  unit = gimp_image_get_unit (image_ID);

  size = gimp_coordinates_new (unit, "%a", TRUE, FALSE, -1,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE,

                               (bvals.horizontal == bvals.vertical),
                               FALSE,

                               _("_Horizontal:"), bvals.horizontal, xres,
                               0, 8 * MAX (drawable->width, drawable->height),
                               0, 0,

                               _("_Vertical:"), bvals.vertical, yres,
                               0, 8 * MAX (drawable->width, drawable->height),
                               0, 0);

  gtk_container_set_border_width (GTK_CONTAINER (size), 6);
  gtk_container_add (GTK_CONTAINER (frame), size);
  gtk_widget_show (size);

  gimp_size_entry_set_pixel_digits (GIMP_SIZE_ENTRY (size), 1);

  /*  FIXME: Shouldn't need two signal connections here,
             gimp_coordinates_new() seems to be severily broken.  */
  g_signal_connect_swapped (size, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (size, "refval-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (update_preview),
                    size);

  frame = gimp_int_radio_group_new (TRUE, _("Blur Method"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &bvals.method, bvals.method,

                                    _("_IIR"), BLUR_IIR, &button,
                                    _("_RLE"), BLUR_RLE, NULL,

                                    NULL);

  g_signal_connect_swapped (button, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      bvals.horizontal = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size), 0);
      bvals.vertical   = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size), 1);
    }

  gtk_widget_destroy (dialog);

  return run;
}

static void
update_preview (GtkWidget *preview,
                GtkWidget *size)
{
  gauss (gimp_drawable_preview_get_drawable (GIMP_DRAWABLE_PREVIEW (preview)),
         gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size), 0),
         gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size), 1),
         bvals.method,
         preview);
}

/* Convert from separated to premultiplied alpha, on a single scan line. */
static void
multiply_alpha (guchar *buf,
                gint    width,
                gint    bytes)
{
  gint i, j;

  for (i = 0; i < width; i++, buf += bytes)
    {
      gdouble alpha = buf[bytes - 1] * (1.0 / 255.0);

      for (j = 0; j < bytes - 1; j++)
        buf[j] = ROUND (buf[j] * alpha);
    }
}

/* Convert from premultiplied to separated alpha, on a single scan line. */
static void
separate_alpha (guchar *buf,
                gint    width,
                gint    bytes)
{
  gint i, j;

  for (i = 0; i < width; i++, buf += bytes)
    {
      guchar alpha = buf[bytes - 1];

      switch (alpha)
        {
        case 0:
        case 255:
          break;

        default:
          {
            gdouble recip_alpha = 255.0 / alpha;

            for (j = 0; j < bytes - 1; j++)
              {
                gint new_val = ROUND (buf[j] * recip_alpha);

                buf[j] = MIN (255, new_val);
              }
          }
          break;
        }
    }
}

/*
 * run_length_encode (src, rle, pix, dist, width, border, pack);
 *
 * Copy 'width' 8bit pixels from 'src' to 'pix' and extend both sides
 * by 'border' pixels so 'pix[]' is filled from '-border' to 'width+border-1'.
 *
 * 'dist' is the distance between the pixels in 'src'.
 *
 * If 'pack' is TRUE, then 'rle' is filled with a run-length encoding
 * of the pixels. In plain english, that means that rle[i] gives the
 * number of times the same pixel is found pix[i], pix[i+1], ...  A
 * standalone pixel has a rle value of 1.
 *
 * The function returns the number of times 2 identical consecutive pixels
 * were found.
 *
 * Note: The function must be inlined to insure that all tests on
 *       'pack' are efficiently resolved by the compiler (they are in
 *       the critical loop).  As a consequence, the function should
 *       only be called with known constant value for 'pack'.  In the
 *       current implementation, 'pack' is always TRUE but it might be
 *       more efficient to have an 'adaptive' algorithm that switches
 *       to FALSE when the run-length is innefficient.
 */
static inline gint
run_length_encode (const guchar *src,
                   gint         *rle,
                   gint         *pix,
                   gint          dist,  /* distance between 2 src pixels */
                   gint          width,
                   gint          border,
                   gboolean      pack)
{
  gint last;
  gint count = 0;
  gint i     = width;
  gint same  = 0;

  src += dist * (width - 1);

  if (pack)
    rle += width + border - 1;

  pix += width + border - 1;

  last  = *src;
  count = 0;

  /* the 'end' border */
  for (i = 0; i < border; i++)
    {
      count++;
      *pix--  = last;

      if (pack)
        *rle-- = count;
  }

  /* the real pixels */
  for (i = 0; i < width; i++)
    {
      gint c = *src;
      src -= dist;

      if (pack && c==last)
        {
          count++;
          *pix-- = last;
          *rle-- = count;
          same++;
        }
      else
        {
          count   = 1;
          last    = c;
          *pix--  = last;

          if (pack)
            *rle-- = count;
        }
    }

  /* the start pixels */
  for (i = 0; i < border; i++)
    {
      count++;
      *pix-- = last;

      if (pack)
        *rle-- = count;
  }

  return same;
}


static void
do_encoded_lre (const gint *enc,
                const gint *src,
                guchar     *dest,
                gint        width,
                gint        length,
                gint        dist,
                const gint *curve,
                gint        ctotal,
                const gint *csum)
{
  gint col;

  for (col = 0; col < width; col++, dest += dist)
    {
      const gint *rpt;
      const gint *pix;
      gint        nb;
      gint        s1;
      gint        i;
      gint        val   = ctotal / 2;
      gint        start = - length;

      rpt = &enc[col + start];
      pix = &src[col + start];

      s1 = csum[start];
      nb = rpt[0];
      i  = start + nb;

      while (i <= length)
        {
          gint s2 = csum[i];

          val += pix[0] * (s2-s1);
          s1 = s2;
          rpt = &rpt[nb];
          pix = &pix[nb];
          nb = rpt[0];
          i += nb;
        }

      val += pix[0] * (csum[length] - s1);

      val = val / ctotal;
      *dest = MIN (val, 255);
    }
}

static void
do_full_lre (const gint *src,
             guchar     *dest,
             gint        width,
             gint        length,
             gint        dist,
             const gint *curve,
             gint        ctotal)
{
  gint col;

  for (col = 0; col < width; col++, dest += dist)
    {
      const gint *x1;
      const gint *x2;
      const gint *c   = &curve[0];
      gint        i;
      gint        val = ctotal / 2;

      x1 = x2 = &src[col];

      /* The central point is a special case since it should only be
       * processed ONCE
       */

      val += x1[0] * c[0];

      c  += 1;
      x1 += 1;
      x2 -= 1;
      i   = length;

      /* Processing multiple points in a single iteration should be
       * faster but is not strictly required.
       * Some precise benchmarking will be needed to figure out
       * if this is really interesting.
       */
      while (i >= 8)
        {
          val += (x1[0] + x2[-0]) * c[0];
          val += (x1[1] + x2[-1]) * c[1];
          val += (x1[2] + x2[-2]) * c[2];
          val += (x1[3] + x2[-3]) * c[3];
          val += (x1[4] + x2[-4]) * c[4];
          val += (x1[5] + x2[-5]) * c[5];
          val += (x1[6] + x2[-6]) * c[6];
          val += (x1[7] + x2[-7]) * c[7];

          c  += 8;
          x1 += 8;
          x2 -= 8;
          i  -= 8;
        }

      while (i >= 4)
        {
          val += (x1[0] + x2[-0]) * c[0];
          val += (x1[1] + x2[-1]) * c[1];
          val += (x1[2] + x2[-2]) * c[2];
          val += (x1[3] + x2[-3]) * c[3];
          c  += 4;
          x1 += 4;
          x2 -= 4;
          i  -= 4;
        }

      /* Only that final loop is strictly required */

      while (i >= 1)
        {
          /* process the pixels at the distance i before and after the
           * central point. They must have the same coefficient
           */
          val += (x1[0] + x2[-0]) * c[0];
          c  += 1;
          x1 += 1;
          x2 -= 1;
          i  -= 1;
        }

      val = val / ctotal;
      *dest = MIN (val, 255);
    }
}

static void
gauss_iir (GimpDrawable *drawable,
           gdouble       horz,
           gdouble       vert,
           BlurMethod    method,
           guchar       *preview_buffer,
           gint          x1,
           gint          y1,
           gint          width,
           gint          height)
{
  GimpPixelRgn  src_rgn, dest_rgn;
  gint          bytes;
  gint          has_alpha;
  guchar       *dest;
  guchar       *src,  *sp_p, *sp_m;
  gdouble       n_p[5], n_m[5];
  gdouble       d_p[5], d_m[5];
  gdouble       bd_p[5], bd_m[5];
  gdouble      *val_p = NULL;
  gdouble      *val_m = NULL;
  gdouble      *vp, *vm;
  gint          i, j;
  gint          row, col, b;
  gint          terms;
  gdouble       progress, max_progress;
  gint          initial_p[4];
  gint          initial_m[4];
  gdouble       std_dev;
  gboolean      direct;
  gint          progress_step;

  direct = (preview_buffer == NULL);

  bytes = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  val_p = g_new (gdouble, MAX (width, height) * bytes);
  val_m = g_new (gdouble, MAX (width, height) * bytes);

  src =  g_new (guchar, MAX (width, height) * bytes);
  dest = g_new (guchar, MAX (width, height) * bytes);

  gimp_pixel_rgn_init (&src_rgn,
                       drawable, 0, 0, drawable->width, drawable->height,
                       FALSE, FALSE);
  if (direct)
    {
      gimp_pixel_rgn_init (&dest_rgn,
                           drawable, 0, 0, drawable->width, drawable->height,
                           TRUE, TRUE);
    }


  progress = 0.0;
  max_progress  = (horz <= 0.0) ? 0 : width * height * horz;
  max_progress += (vert <= 0.0) ? 0 : width * height * vert;


  /*  First the vertical pass  */
  if (vert > 0.0)
    {
      vert = fabs (vert) + 1.0;
      std_dev = sqrt (-(vert * vert) / (2 * log (1.0 / 255.0)));

      /* We do not want too many progress updates because they
       * can slow down the processing significantly for very
       * large images
       */
      progress_step = width / 16;

      if (progress_step < 5)
        progress_step = 5;

      /*  derive the constants for calculating the gaussian
       *  from the std dev
       */

      find_iir_constants (n_p, n_m, d_p, d_m, bd_p, bd_m, std_dev);

      for (col = 0; col < width; col++)
        {
          memset (val_p, 0, height * bytes * sizeof (gdouble));
          memset (val_m, 0, height * bytes * sizeof (gdouble));

          gimp_pixel_rgn_get_col (&src_rgn, src, col + x1, y1, height);

          if (has_alpha)
            multiply_alpha (src, height, bytes);

          sp_p = src;
          sp_m = src + (height - 1) * bytes;
          vp = val_p;
          vm = val_m + (height - 1) * bytes;

          /*  Set up the first vals  */
          for (i = 0; i < bytes; i++)
            {
              initial_p[i] = sp_p[i];
              initial_m[i] = sp_m[i];
            }

          for (row = 0; row < height; row++)
            {
              gdouble *vpptr, *vmptr;
              terms = (row < 4) ? row : 4;

              for (b = 0; b < bytes; b++)
                {
                  vpptr = vp + b; vmptr = vm + b;
                  for (i = 0; i <= terms; i++)
                    {
                      *vpptr += n_p[i] * sp_p[(-i * bytes) + b] - d_p[i] * vp[(-i * bytes) + b];
                      *vmptr += n_m[i] * sp_m[(i * bytes) + b] - d_m[i] * vm[(i * bytes) + b];
                    }
                  for (j = i; j <= 4; j++)
                    {
                      *vpptr += (n_p[j] - bd_p[j]) * initial_p[b];
                      *vmptr += (n_m[j] - bd_m[j]) * initial_m[b];
                    }
                }

              sp_p += bytes;
              sp_m -= bytes;
              vp += bytes;
              vm -= bytes;
            }

          transfer_pixels (val_p, val_m, dest, bytes, height);


          if (has_alpha)
            separate_alpha (dest, height, bytes);

          if (direct)
            {
              gimp_pixel_rgn_set_col(&dest_rgn, dest, col + x1, y1, height);

              progress += height * vert;

              if ((col % progress_step) == 0)
                gimp_progress_update (progress / max_progress);
            }
          else
            {
              for (row = 0; row < height; row++)
                memcpy (preview_buffer + (row * width + col) * bytes,
                        dest + row * bytes,
                        bytes);
            }
        }

      /*  prepare for the horizontal pass  */
      gimp_pixel_rgn_init (&src_rgn,
                           drawable,
                           0, 0,
                           drawable->width, drawable->height,
                           FALSE, TRUE);
    }
  else if (!direct)
    {
      gimp_pixel_rgn_get_rect (&src_rgn,
                               preview_buffer,
                               x1, y1,
                               width, height);
    }

  /*  Now the horizontal pass  */
  if (horz > 0.0)
    {

      /* We do not want too many progress updates because they
       * can slow down the processing significantly for very
       * large images
       */
      progress_step = height / 16;

      if (progress_step < 5)
        progress_step = 5;

      horz = fabs (horz) + 1.0;

      if (horz != vert)
        {
          std_dev = sqrt (-(horz * horz) / (2 * log (1.0 / 255.0)));

          /*  derive the constants for calculating the gaussian
           *  from the std dev
           */
          find_iir_constants (n_p, n_m, d_p, d_m, bd_p, bd_m, std_dev);

        }


      for (row = 0; row < height; row++)
        {

          memset (val_p, 0, width * bytes * sizeof (gdouble));
          memset (val_m, 0, width * bytes * sizeof (gdouble));

          if (direct)
            {
              gimp_pixel_rgn_get_row (&src_rgn, src, x1, row + y1, width);
            }
          else
            {
              memcpy (src,
                      preview_buffer + row * width * bytes,
                      width * bytes);
            }


          if (has_alpha)
            multiply_alpha (src, width, bytes);


          sp_p = src;
          sp_m = src + (width - 1) * bytes;
          vp = val_p;
          vm = val_m + (width - 1) * bytes;

          /*  Set up the first vals  */
          for (i = 0; i < bytes; i++)
            {
              initial_p[i] = sp_p[i];
              initial_m[i] = sp_m[i];
            }

          for (col = 0; col < width; col++)
            {
              gdouble *vpptr, *vmptr;

              terms = (col < 4) ? col : 4;

              for (b = 0; b < bytes; b++)
                {
                  vpptr = vp + b; vmptr = vm + b;

                  for (i = 0; i <= terms; i++)
                    {
                      *vpptr += n_p[i] * sp_p[(-i * bytes) + b] -
                        d_p[i] * vp[(-i * bytes) + b];
                      *vmptr += n_m[i] * sp_m[(i * bytes) + b] -
                        d_m[i] * vm[(i * bytes) + b];
                    }
                  for (j = i; j <= 4; j++)
                    {
                      *vpptr += (n_p[j] - bd_p[j]) * initial_p[b];
                      *vmptr += (n_m[j] - bd_m[j]) * initial_m[b];
                    }
                }

              sp_p += bytes;
              sp_m -= bytes;
              vp += bytes;
              vm -= bytes;
            }

          transfer_pixels (val_p, val_m, dest, bytes, width);

          if (has_alpha)
            separate_alpha (dest, width, bytes);

          if (direct)
            {
              gimp_pixel_rgn_set_row (&dest_rgn, dest, x1, row + y1, width);

              progress += width * horz;

              if ((row % progress_step) == 0)
                gimp_progress_update (progress / max_progress);
            }
          else
            {
              memcpy (preview_buffer + row * width * bytes,
                      dest,
                      width * bytes);
            }
        }
    }

  /*  free up buffers  */

  g_free (val_p);
  g_free (val_m);

  g_free (src);
  g_free (dest);
}


static void
gauss_rle (GimpDrawable *drawable,
           gdouble       horz,
           gdouble       vert,
           BlurMethod    method,
           guchar       *preview_buffer,
           gint          x1,
           gint          y1,
           gint          width,
           gint          height)
{
  GimpPixelRgn  src_rgn, dest_rgn;
  gint          bytes;
  gboolean      has_alpha;
  guchar       *dest;
  guchar       *src;
  gint          row, col, b;
  gdouble       progress, max_progress;
  gdouble       std_dev;
  gint          total = 1;
  gint         *curve = NULL;
  gint         *sum   = NULL;
  gint          length;
  gboolean      direct;
  gint          progress_step;

  direct = (preview_buffer == NULL);

  bytes = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  src  = g_new (guchar, MAX (width, height) * bytes);
  dest = g_new (guchar, MAX (width, height) * bytes);

  gimp_pixel_rgn_init (&src_rgn,
                       drawable, 0, 0, drawable->width, drawable->height,
                       FALSE, FALSE);

  if (direct)
    gimp_pixel_rgn_init (&dest_rgn,
			 drawable, 0, 0, drawable->width, drawable->height,
			 TRUE, TRUE);

  progress = 0.0;
  max_progress  = (horz <= 0.0) ? 0 : width * height * horz;
  max_progress += (vert <= 0.0) ? 0 : width * height * vert;


  /*  First the vertical pass  */
  if (vert > 0.0)
    {
      gint   * rle = NULL;
      gint   * pix = NULL;

      vert = fabs (vert) + 1.0;
      std_dev = sqrt (-(vert * vert) / (2 * log (1.0 / 255.0)));

      /* Insure that we do not have too many progress updates for
       * images with a lot of rows or columns
       */
      progress_step = width / 16;

      if (progress_step < 5)
        progress_step = 5;

      make_rle_curve (std_dev, &curve, &length, &sum, &total);

      rle = g_new (gint, height + 2 * length);
      rle += length; /* rle[] extends from -length to height+length-1 */

      pix = g_new (gint, height + 2 * length);
      pix += length; /* pix[] extends from -length to height+length-1 */

      for (col = 0; col < width; col++)
        {

          gimp_pixel_rgn_get_col (&src_rgn, src, col + x1, y1, height);

          if (has_alpha)
            multiply_alpha (src, height, bytes);

          for (b = 0; b < bytes; b++)
            {
              gint same =  run_length_encode (src + b, rle, pix, bytes,
					      height, length, TRUE);

              if (same > (3 * height) / 4)
		{
		  /* encoded_rle is only fastest if there are a lot of
		   * repeating pixels
		   */
                  do_encoded_lre (rle, pix, dest + b, height, length, bytes,
                                  curve, total, sum);
		}
	      else
		{
		  /* else a full but more simple algorithm is better */
		  do_full_lre (pix, dest + b, height, length, bytes,
			       curve, total);
		}
            }

          if (has_alpha)
            separate_alpha (dest, height, bytes);

          if (direct)
            {
              gimp_pixel_rgn_set_col (&dest_rgn, dest, col + x1, y1, height);

              progress += height * vert;

              if ((col % progress_step) == 0)
                gimp_progress_update (progress / max_progress);
            }
          else
            {
              for (row = 0; row < height; row++)
                memcpy (preview_buffer + (row * width + col) * bytes,
                        dest + row * bytes,
                        bytes);
            }

        }


      g_free (rle - length);
      g_free (pix - length);

      /* prepare for the horizontal pass  */
      gimp_pixel_rgn_init (&src_rgn,
                           drawable, 0, 0, drawable->width, drawable->height,
                           FALSE, TRUE);
    }
  else if (!direct)
    {
      gimp_pixel_rgn_get_rect (&src_rgn,
                               preview_buffer, x1, y1, width, height);
    }

  /*  Now the horizontal pass  */
  if (horz > 0.0)
    {
      gint   * rle = NULL;
      gint   * pix = NULL;

      /* Insure that we do not have too many progress updates for
       * images with a lot of rows or columns
       */
      progress_step = height / 16;

      if (progress_step < 5) {
        progress_step = 5;
      }

      horz = fabs (horz) + 1.0;

      /* euse the same curve if possible else recompute a new one */
      if (horz != vert)
        {

          std_dev = sqrt (-(horz * horz) / (2 * log (1.0 / 255.0)));

          if (curve != NULL) {
            free_rle_curve(curve, length, sum);
          }

          make_rle_curve(std_dev, &curve, &length, &sum, &total);

        }


      rle = g_new (gint, width+2*length);
      rle += length; /* so rle[] extends from -width to width+length-1 */

      pix = g_new (gint, width+2*length);
      pix += length; /* so pix[] extends from -width to width+length-1 */

      for (row = 0; row < height; row++)
        {
          if (direct)
            {
              gimp_pixel_rgn_get_row (&src_rgn, src, x1, row + y1, width);
            }
          else
            {
              memcpy (src,
                      preview_buffer + row * width * bytes,
                      width * bytes);
            }

          if (has_alpha)
            multiply_alpha (src, width, bytes);

          for (b = 0; b < bytes; b++)
            {
              gint same = run_length_encode (src + b, rle, pix, bytes,
					     width, length, TRUE);

              if (same > (3 * width) / 4)
		{
		  /* encoded_rle is only fastest if there are a lot of
		   * repeating pixels
		   */
                  do_encoded_lre (rle, pix, dest + b, width, length, bytes,
                                  curve, total, sum);
		}
	      else
		{
		  /* else a full but more simple algorithm is better */
		  do_full_lre (pix, dest + b, width, length, bytes,
                               curve, total);
		}
            }


          if (has_alpha)
            separate_alpha (dest, width, bytes);

          if (direct)
            {
              gimp_pixel_rgn_set_row (&dest_rgn, dest, x1, row + y1, width);

              progress += width * horz;

              if ((row % progress_step) == 0)
                gimp_progress_update (progress / max_progress);
            }
          else
            {
              memcpy (preview_buffer + row * width * bytes,
                      dest,
                      width * bytes);
            }
        }

      g_free (rle - length);
      g_free (pix - length);
    }

  if (curve)
    free_rle_curve (curve, length, sum);

  g_free (src);
  g_free (dest);
}


static void
gauss (GimpDrawable *drawable,
       gdouble       horz,
       gdouble       vert,
       BlurMethod    method,
       GtkWidget    *preview)
{

  gint    x1, y1, x2, y2;
  gint    width, height;
  guchar *preview_buffer;

  /*
   * IIR goes wrong if the blur radius is less than 1, so we silently
   * switch to RLE in this case.  See bug #315953
   */
  if (horz <= 1.0 || vert <= 1.0)
    method = BLUR_RLE;

  if (horz <= 0.0 && vert <= 0.0)
    {
      if (preview)
        gimp_preview_draw (GIMP_PREVIEW (preview));
      return;
    }

  if (preview)
    {
      gimp_preview_get_position (GIMP_PREVIEW (preview), &x1, &y1);
      gimp_preview_get_size (GIMP_PREVIEW (preview), &width, &height);

      if (width < 1 || height < 1)
        return;

      preview_buffer = g_new (guchar, width * height * drawable->bpp);

    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

      width  = (x2 - x1);
      height = (y2 - y1);

      if (width < 1 || height < 1)
        return;

      preview_buffer = NULL;

    }


  if (method == BLUR_IIR)
    gauss_iir (drawable,
	       horz, vert, method, preview_buffer, x1, y1, width, height);
  else
    gauss_rle (drawable,
	       horz, vert, method, preview_buffer, x1, y1, width, height);

  if (preview)
    {
      gimp_preview_draw_buffer (GIMP_PREVIEW (preview),
				preview_buffer, width * drawable->bpp);
      g_free (preview_buffer);
    }
  else
    {
      /*  merge the shadow, update the drawable  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
    }
}

static void
transfer_pixels (const gdouble *src1,
                 const gdouble *src2,
                 guchar        *dest,
                 gint           bytes,
                 gint           width)
{
  gint    b;
  gint    bend = bytes * width;
  gdouble sum;

  for (b = 0; b < bend; b++)
    {
      sum = *src1++ + *src2++;

      if (sum > 255)
	sum = 255;
      else if (sum < 0)
	sum = 0;

      *dest++ = (guchar) sum;
    }
}

static void
find_iir_constants (gdouble *n_p,
                    gdouble *n_m,
                    gdouble *d_p,
                    gdouble *d_m,
                    gdouble *bd_p,
                    gdouble *bd_m,
                    gdouble  std_dev)
{
  /*  The constants used in the implemenation of a casual sequence
   *  using a 4th order approximation of the gaussian operator
   */

  const gdouble div = sqrt (2 * G_PI) * std_dev;
  const gdouble x0 = -1.783 / std_dev;
  const gdouble x1 = -1.723 / std_dev;
  const gdouble x2 = 0.6318 / std_dev;
  const gdouble x3 = 1.997  / std_dev;
  const gdouble x4 = 1.6803 / div;
  const gdouble x5 = 3.735 / div;
  const gdouble x6 = -0.6803 / div;
  const gdouble x7 = -0.2598 / div;
  gint          i;

  n_p [0] = x4 + x6;
  n_p [1] = (exp(x1)*(x7*sin(x3)-(x6+2*x4)*cos(x3)) +
	     exp(x0)*(x5*sin(x2) - (2*x6+x4)*cos (x2)));
  n_p [2] = (2 * exp(x0+x1) *
	     ((x4+x6)*cos(x3)*cos(x2) - x5*cos(x3)*sin(x2) -
	      x7*cos(x2)*sin(x3)) +
	     x6*exp(2*x0) + x4*exp(2*x1));
  n_p [3] = (exp(x1+2*x0) * (x7*sin(x3) - x6*cos(x3)) +
	     exp(x0+2*x1) * (x5*sin(x2) - x4*cos(x2)));
  n_p [4] = 0.0;

  d_p [0] = 0.0;
  d_p [1] = -2 * exp(x1) * cos(x3) -  2 * exp(x0) * cos (x2);
  d_p [2] = 4 * cos(x3) * cos(x2) * exp(x0 + x1) +  exp(2 * x1) + exp(2 * x0);
  d_p [3] = -2 * cos(x2) * exp(x0 + 2*x1) -  2*cos(x3) * exp(x1 + 2*x0);
  d_p [4] = exp(2*x0 + 2*x1);

  for (i = 0; i <= 4; i++)
    d_m[i] = d_p[i];

  n_m[0] = 0.0;

  for (i = 1; i <= 4; i++)
    n_m[i] = n_p[i] - d_p[i] * n_p[0];

  {
    gdouble sum_n_p, sum_n_m, sum_d;
    gdouble a, b;

    sum_n_p = 0.0;
    sum_n_m = 0.0;
    sum_d = 0.0;

    for (i = 0; i <= 4; i++)
      {
        sum_n_p += n_p[i];
        sum_n_m += n_m[i];
        sum_d += d_p[i];
      }

    a = sum_n_p / (1.0 + sum_d);
    b = sum_n_m / (1.0 + sum_d);

    for (i = 0; i <= 4; i++)
      {
        bd_p[i] = d_p[i] * a;
        bd_m[i] = d_m[i] * b;
      }
  }
}


/*
 * make_rle_curve(sigma, &curve, &length, &sum, &total)
 *
 *
 * Fill the Gauss curve.
 *
 *               g(r) = exp (- r^2 / (2 * sigma^2))
 *                  r = sqrt (x^2 + y ^2)
 *
 * o length is filled with the length the curve (in both directions)
 * o curve[-length .. length] is allocated and filled with the
 *   (scaled) gauss curve.
 * o sum[-length .. length]   is allocated and filled with the 'summed' curve.
 * o total is filled with the sum of all elements in the curve (for
 *   normalization).
 *
 *
 *
 */


static void
make_rle_curve (gdouble   sigma,
                gint    **p_curve,
                gint     *p_length,
                gint    **p_sum,
                gint     *p_total)
{
  const gdouble  sigma2 = 2 * sigma * sigma;
  const gdouble  l      = sqrt (-sigma2 * log (1.0 / 255.0));
  gint           temp;
  gint           i, n;
  gint           length;
  gint          *sum;
  gint          *curve;

  n = ceil (l) * 2;
  if ((n % 2) == 0)
    n += 1;

  curve = g_new (gint, n);

  length = n / 2;
  curve += length; /* 'center' the curve[] */
  curve[0] = 255;

  for (i = 1; i <= length; i++)
    {
      temp = (gint) (exp (- (i * i) / sigma2) * 255);
      curve[-i] = temp;
      curve[i] = temp;
    }

  sum   = g_new (gint, 2 * length + 1);

  sum[0] = 0;
  for (i = 1; i <= length*2; i++)
    {
      sum[i] = curve[i-length-1] + sum[i-1];
    }

  sum += length; /* 'center' the sum[] */

  *p_total  = sum[length] - sum[-length];
  *p_curve  = curve;
  *p_sum    = sum;
  *p_length = length;

}

/*
 * Free a curve previously allocated with make_rle_curve
 */
static void
free_rle_curve (gint *curve,
                gint  length,
                gint *sum)
{
  g_free (sum - length);
  g_free (curve - length);
}
