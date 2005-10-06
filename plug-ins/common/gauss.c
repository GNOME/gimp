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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

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

static void      gauss             (GimpDrawable     *drawable,
                                    gdouble           horizontal,
                                    gdouble           vertical,
                                    BlurMethod        method,
                                    GtkWidget        *preview);

static void      update_preview    (GtkWidget        *preview,
                                    GtkWidget        *size);
/*
 * Gaussian blur interface
 */
static gboolean  gauss_dialog      (gint32            image_ID,
                                    GimpDrawable     *drawable);

/*
 * Gaussian blur helper functions
 */
static void      find_constants    (gdouble  n_p[],
                                    gdouble  n_m[],
                                    gdouble  d_p[],
                                    gdouble  d_m[],
                                    gdouble  bd_p[],
                                    gdouble  bd_m[],
                                    gdouble  std_dev);
static void      transfer_pixels   (gdouble *src1,
                                    gdouble *src2,
                                    guchar  *dest,
                                    gint     bytes,
                                    gint     width);

static gint *    make_curve        (gdouble    sigma,
                                    gint      *length);
static void      run_length_encode (guchar    *src,
                                    gint      *dest,
                                    gint       bytes,
                                    gint       width);


GimpPlugInInfo PLUG_IN_INFO =
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
  BLUR_IIR
};


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_FLOAT, "horizontal", "Horizontal radius of gaussian blur (in pixels, > 0.0)" },
    { GIMP_PDB_FLOAT, "vertical",   "Vertical radius of gaussian blur (in pixels, > 0.0)" },
    { GIMP_PDB_INT32, "method",   "IIR (0) or RLE (1)" }
  };

  static GimpParamDef args1[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_FLOAT, "radius", "Radius of gaussian blur (in pixels, > 0.0)" },
    { GIMP_PDB_INT32, "horizontal", "Blur in horizontal direction" },
    { GIMP_PDB_INT32, "vertical", "Blur in vertical direction" }
  };

  static GimpParamDef args2[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_FLOAT, "horizontal", "Horizontal radius of gaussian blur (in pixels, > 0.0)" },
    { GIMP_PDB_FLOAT, "vertical",   "Vertical radius of gaussian blur (in pixels, > 0.0)" }
  };

  gimp_install_procedure ("plug_in_gauss",
                          "Applies a gaussian blur to the specified drawable.",
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

  gimp_install_procedure ("plug_in_gauss_iir",
                          "Applies a gaussian blur to the specified drawable.",
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

  gimp_install_procedure ("plug_in_gauss_iir2",
                          "Applies a gaussian blur to the specified drawable.",
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

  gimp_install_procedure ("plug_in_gauss_rle",
                          "Applies a gaussian blur to the specified drawable.",
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

  gimp_install_procedure ("plug_in_gauss_rle2",
                          "Applies a gaussian blur to the specified drawable.",
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

  gimp_plugin_menu_register ("plug_in_gauss", "<Image>/Filters/Blur");
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
  gimp_tile_cache_ntiles (2 *
                          (MAX (drawable->width, drawable->height) /
                           gimp_tile_width () + 1));


  if (strcmp (name, "plug_in_gauss") == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data ("plug_in_gauss", &bvals);

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
          gimp_get_data ("plug_in_gauss", &bvals);
          break;

        default:
          break;
        }
    }
  else if (strcmp (name, "plug_in_gauss_iir") == 0)
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
  else if (strcmp (name, "plug_in_gauss_iir2") == 0)
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
  else if (strcmp (name, "plug_in_gauss_rle") == 0)
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
  else if (strcmp (name, "plug_in_gauss_rle2") == 0)
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
          gimp_progress_init (_("Gaussian Blur..."));

          /*  run the gaussian blur  */
          gauss (drawable,
                 bvals.horizontal, bvals.vertical,
                 bvals.method,
                 NULL);

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data ("plug_in_gauss",
                           &bvals, sizeof (BlurValues));

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
  GtkWidget *preview;

  GimpUnit   unit;
  gdouble    xres;
  gdouble    yres;
  gboolean   run;

  gimp_ui_init ("gaussian_blur", FALSE);

  dialog = gimp_dialog_new (_("Gaussian Blur"), "gaussian_blur",
                            NULL, 0,
                            gimp_standard_help_func, "plug-in-gauss",

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

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
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
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
  g_signal_connect_swapped (size, "value_changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (size, "refval_changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (update_preview),
                    size);

  frame = gimp_int_radio_group_new (TRUE, _("Blur Method"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &bvals.method, bvals.method,

                                    _("_IIR"), BLUR_IIR, NULL,
                                    _("_RLE"), BLUR_RLE, NULL,

                                    NULL);

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
  gint    i, j;
  gdouble alpha;

  for (i = 0; i < width * bytes; i += bytes)
    {
      alpha = buf[i + bytes - 1] * (1.0 / 255.0);
      for (j = 0; j < bytes - 1; j++)
        buf[i + j] *= alpha;
    }
}

/* Convert from premultiplied to separated alpha, on a single scan
   line. */
static void
separate_alpha (guchar *buf,
                gint    width,
                gint    bytes)
{
  gint   i, j;
  guchar alpha;
  gdouble recip_alpha;
  gint    new_val;

  for (i = 0; i < width * bytes; i += bytes)
    {
      alpha = buf[i + bytes - 1];
      if (alpha != 0 && alpha != 255)
        {
          recip_alpha = 255.0 / alpha;
          for (j = 0; j < bytes - 1; j++)
            {
              new_val = buf[i + j] * recip_alpha;
              buf[i + j] = MIN (255, new_val);
            }
        }
    }
}

static void
gauss (GimpDrawable *drawable,
       gdouble       horz,
       gdouble       vert,
       BlurMethod    method,
       GtkWidget    *preview)
{
  GimpPixelRgn  src_rgn, dest_rgn;
  gint          width, height;
  gint          bytes;
  gint          has_alpha;
  guchar       *dest, *dp;
  guchar       *src, *sp, *sp_p, *sp_m;
  gint         *buf = NULL;
  gint         *bb;
  gdouble       n_p[5], n_m[5];
  gdouble       d_p[5], d_m[5];
  gdouble       bd_p[5], bd_m[5];
  gdouble      *val_p = NULL;
  gdouble      *val_m = NULL;
  gdouble      *vp, *vm;
  gint          x1, y1, x2, y2;
  gint          i, j;
  gint          row, col, b;
  gint          terms;
  gdouble       progress, max_progress;
  gint          initial_p[4];
  gint          initial_m[4];
  gdouble       std_dev;
  gint          pixels;
  gint          total = 1;
  gint          start, end;
  gint         *curve;
  gint         *sum = NULL;
  gint          val;
  gint          length;
  gint          initial_pp, initial_mm;
  guchar       *preview_buffer1 = NULL;
  guchar       *preview_buffer2 = NULL;

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
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

      width  = (x2 - x1);
      height = (y2 - y1);
    }

  if (width < 1 || height < 1)
    return;

  bytes = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  switch (method)
    {
    case BLUR_IIR:
      val_p = g_new (gdouble, MAX (width, height) * bytes);
      val_m = g_new (gdouble, MAX (width, height) * bytes);
      break;

    case BLUR_RLE:
      buf = g_new (gint, MAX (width, height) * 2);
      break;
    }

  src =  g_new (guchar, MAX (width, height) * bytes);
  dest = g_new (guchar, MAX (width, height) * bytes);

  gimp_pixel_rgn_init (&src_rgn,
                       drawable, 0, 0, drawable->width, drawable->height,
                       FALSE, FALSE);
  if (preview)
    {
      preview_buffer1 = g_new (guchar, width * height * bytes);
      preview_buffer2 = g_new (guchar, width * height * bytes);
    }
  else
    {
      gimp_pixel_rgn_init (&dest_rgn,
                           drawable, 0, 0, drawable->width, drawable->height,
                           TRUE, TRUE);
    }

  progress = 0.0;
  max_progress  = (horz <= 0.0 ) ? 0 : width * height * horz;
  max_progress += (vert <= 0.0 ) ? 0 : width * height * vert;


  /*  First the vertical pass  */
  if (vert > 0.0)
    {
      vert = fabs (vert) + 1.0;
      std_dev = sqrt (-(vert * vert) / (2 * log (1.0 / 255.0)));

      switch (method)
        {
        case BLUR_IIR:
          /*  derive the constants for calculating the gaussian
           *  from the std dev
           */
          find_constants (n_p, n_m, d_p, d_m, bd_p, bd_m, std_dev);
          break;

        case BLUR_RLE:
          curve = make_curve (std_dev, &length);
          sum = g_new (gint, 2 * length + 1);

          sum[0] = 0;

          for (i = 1; i <= length*2; i++)
            sum[i] = curve[i-length-1] + sum[i-1];
          sum += length;

          total = sum[length] - sum[-length];
          break;
        }

      for (col = 0; col < width; col++)
        {
          switch (method)
            {
            case BLUR_IIR:
              memset (val_p, 0, height * bytes * sizeof (gdouble));
              memset (val_m, 0, height * bytes * sizeof (gdouble));
              break;

            case BLUR_RLE:
              break;
            }

          gimp_pixel_rgn_get_col (&src_rgn, src, col + x1, y1, height);

          if (has_alpha)
            multiply_alpha (src, height, bytes);

          switch (method)
            {
            case BLUR_IIR:
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

              transfer_pixels (val_p, val_m, dest, bytes, height);
              break;

            case BLUR_RLE:
              sp = src;
              dp = dest;

              for (b = 0; b < bytes; b++)
                {
                  initial_pp = sp[b];
                  initial_mm = sp[(height-1) * bytes + b];

                  /*  Determine a run-length encoded version of the row  */
                  run_length_encode (sp + b, buf, bytes, height);

                  for (row = 0; row < height; row++)
                    {
                      start = (row < length) ? -row : -length;
                      end = (height <= (row + length) ?
                             (height - row - 1) : length);

                      val = 0;
                      i = start;
                      bb = buf + (row + i) * 2;

                      if (start != -length)
                        val += initial_pp * (sum[start] - sum[-length]);

                      while (i < end)
                        {
                          pixels = bb[0];
                          i += pixels;
                          if (i > end)
                            i = end;
                          val += bb[1] * (sum[i] - sum[start]);
                          bb += (pixels * 2);
                          start = i;
                        }

                      if (end != length)
                        val += initial_mm * (sum[length] - sum[end]);

                      dp[row * bytes + b] = val / total;
                    }
                 }
              break;
            }

          if (has_alpha)
            separate_alpha (src, height, bytes);

          if (preview)
            {
              for (row = 0 ; row < height ; row++)
                memcpy (preview_buffer1 + (row * width + col) * bytes,
                        dest + row * bytes,
                        bytes);
            }
          else
            {
              gimp_pixel_rgn_set_col (&dest_rgn, dest, col + x1, y1, height);

              progress += height * vert;
              if ((col % 5) == 0)
                gimp_progress_update (progress / max_progress);
            }
        }

      /*  prepare for the horizontal pass  */
      gimp_pixel_rgn_init (&src_rgn,
                           drawable, 0, 0, drawable->width, drawable->height,
                           FALSE, TRUE);
    }
  else if (preview)
    {
      gimp_pixel_rgn_get_rect (&src_rgn,
                               preview_buffer1, x1, y1, width, height);
    }

  /*  Now the horizontal pass  */
  if (horz > 0.0)
    {
      horz = fabs (horz) + 1.0;

      if (horz != vert)
        {
          std_dev = sqrt (-(horz * horz) / (2 * log (1.0 / 255.0)));

          switch (method)
            {
            case BLUR_IIR:
              /*  derive the constants for calculating the gaussian
               *  from the std dev
               */
              find_constants (n_p, n_m, d_p, d_m, bd_p, bd_m, std_dev);
              break;

            case BLUR_RLE:
              curve = make_curve (std_dev, &length);
              sum = g_new (gint, 2 * length + 1);

              sum[0] = 0;

              for (i = 1; i <= length*2; i++)
                sum[i] = curve[i-length-1] + sum[i-1];
              sum += length;

              total = sum[length] - sum[-length];
              break;
            }
        }

      for (row = 0; row < height; row++)
        {
          switch (method)
            {
            case BLUR_IIR:
              memset (val_p, 0, width * bytes * sizeof (gdouble));
              memset (val_m, 0, width * bytes * sizeof (gdouble));
              break;

            case BLUR_RLE:
              break;
            }

          if (preview)
            {
              memcpy (src,
                      preview_buffer1 + row * width * bytes,
                      width * bytes);
            }
          else
            {
              gimp_pixel_rgn_get_row (&src_rgn, src, x1, row + y1, width);
            }

          if (has_alpha)
            multiply_alpha (dest, width, bytes);

          switch (method)
            {
            case BLUR_IIR:
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
              break;

            case BLUR_RLE:
              sp = src;
              dp = dest;

              for (b = 0; b < bytes; b++)
                {
                  initial_pp = sp[b];
                  initial_mm = sp[(width-1) * bytes + b];

                  /*  Determine a run-length encoded version of the row  */
                  run_length_encode (sp + b, buf, bytes, width);

                  for (col = 0; col < width; col++)
                    {
                      start = (col < length) ? -col : -length;
                      end = (width <= (col + length)) ? (width - col - 1) : length;

                      val = 0;
                      i = start;
                      bb = buf + (col + i) * 2;

                      if (start != -length)
                        val += initial_pp * (sum[start] - sum[-length]);

                      while (i < end)
                        {
                          pixels = bb[0];
                          i += pixels;
                          if (i > end)
                            i = end;
                          val += bb[1] * (sum[i] - sum[start]);
                          bb += (pixels * 2);
                          start = i;
                        }

                      if (end != length)
                        val += initial_mm * (sum[length] - sum[end]);

                      dp[col * bytes + b] = val / total;
                    }
                }
              break;
            }

          if (has_alpha)
            separate_alpha (dest, width, bytes);

          if (preview)
            {
              memcpy (preview_buffer2 + row * width * bytes,
                      dest,
                      width * bytes);
            }
          else
            {
              gimp_pixel_rgn_set_row (&dest_rgn, dest, x1, row + y1, width);

              progress += width * horz;
              if ((row % 5) == 0)
                gimp_progress_update (progress / max_progress);
            }
        }
    }
  else if (preview)
    {
      memcpy (preview_buffer2, preview_buffer1, width * height * bytes);
    }

  if (preview)
    {
      gimp_preview_draw_buffer (GIMP_PREVIEW (preview),
                                preview_buffer2, width * bytes);
      g_free (preview_buffer1);
      g_free (preview_buffer2);
    }
  else
    {
      /*  merge the shadow, update the drawable  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
    }

  /*  free up buffers  */
  switch (method)
    {
    case BLUR_IIR:
      g_free (val_p);
      g_free (val_m);
      break;

    case BLUR_RLE:
      g_free (buf);
      break;
    }

  g_free (src);
  g_free (dest);
}

static void
transfer_pixels (gdouble *src1,
                 gdouble *src2,
                 guchar  *dest,
                 gint     bytes,
                 gint     width)
{
  gint    b;
  gint    bend = bytes * width;
  gdouble sum;

  for(b = 0; b < bend; b++)
    {
      sum = *src1++ + *src2++;
      if (sum > 255) sum = 255;
      else if(sum < 0) sum = 0;

      *dest++ = (guchar) sum;
    }
}

static void
find_constants (gdouble n_p[],
                gdouble n_m[],
                gdouble d_p[],
                gdouble d_m[],
                gdouble bd_p[],
                gdouble bd_m[],
                gdouble std_dev)
{
  gint    i;
  gdouble constants [8];
  gdouble div;

  /*  The constants used in the implemenation of a casual sequence
   *  using a 4th order approximation of the gaussian operator
   */

  div = sqrt(2 * G_PI) * std_dev;
  constants [0] = -1.783 / std_dev;
  constants [1] = -1.723 / std_dev;
  constants [2] = 0.6318 / std_dev;
  constants [3] = 1.997  / std_dev;
  constants [4] = 1.6803 / div;
  constants [5] = 3.735 / div;
  constants [6] = -0.6803 / div;
  constants [7] = -0.2598 / div;

  n_p [0] = constants[4] + constants[6];
  n_p [1] = exp (constants[1]) *
    (constants[7] * sin (constants[3]) -
     (constants[6] + 2 * constants[4]) * cos (constants[3])) +
       exp (constants[0]) *
         (constants[5] * sin (constants[2]) -
          (2 * constants[6] + constants[4]) * cos (constants[2]));
  n_p [2] = 2 * exp (constants[0] + constants[1]) *
    ((constants[4] + constants[6]) * cos (constants[3]) * cos (constants[2]) -
     constants[5] * cos (constants[3]) * sin (constants[2]) -
     constants[7] * cos (constants[2]) * sin (constants[3])) +
       constants[6] * exp (2 * constants[0]) +
         constants[4] * exp (2 * constants[1]);
  n_p [3] = exp (constants[1] + 2 * constants[0]) *
    (constants[7] * sin (constants[3]) - constants[6] * cos (constants[3])) +
      exp (constants[0] + 2 * constants[1]) *
        (constants[5] * sin (constants[2]) - constants[4] * cos (constants[2]));
  n_p [4] = 0.0;

  d_p [0] = 0.0;
  d_p [1] = -2 * exp (constants[1]) * cos (constants[3]) -
    2 * exp (constants[0]) * cos (constants[2]);
  d_p [2] = 4 * cos (constants[3]) * cos (constants[2]) * exp (constants[0] + constants[1]) +
    exp (2 * constants[1]) + exp (2 * constants[0]);
  d_p [3] = -2 * cos (constants[2]) * exp (constants[0] + 2 * constants[1]) -
    2 * cos (constants[3]) * exp (constants[1] + 2 * constants[0]);
  d_p [4] = exp (2 * constants[0] + 2 * constants[1]);

  for (i = 0; i <= 4; i++)
    d_m [i] = d_p [i];

  n_m[0] = 0.0;
  for (i = 1; i <= 4; i++)
    n_m [i] = n_p[i] - d_p[i] * n_p[0];

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
 * The equations: g(r) = exp (- r^2 / (2 * sigma^2))
 *                   r = sqrt (x^2 + y ^2)
 */

static gint *
make_curve (gdouble  sigma,
            gint    *length)
{
  gint    *curve;
  gdouble  sigma2;
  gdouble  l;
  gint     temp;
  gint     i, n;

  sigma2 = 2 * sigma * sigma;
  l = sqrt (-sigma2 * log (1.0 / 255.0));

  n = ceil (l) * 2;
  if ((n % 2) == 0)
    n += 1;

  curve = g_new (gint, n);

  *length = n / 2;
  curve += *length;
  curve[0] = 255;

  for (i = 1; i <= *length; i++)
    {
      temp = (gint) (exp (- (i * i) / sigma2) * 255);
      curve[-i] = temp;
      curve[i] = temp;
    }

  return curve;
}

static void
run_length_encode (guchar *src,
                   gint   *dest,
                   gint    bytes,
                   gint    width)
{
  gint   start;
  gint   i;
  gint   j;
  guchar last;

  last = *src;
  src += bytes;
  start = 0;

  for (i = 1; i < width; i++)
    {
      if (*src != last)
        {
          for (j = start; j < i; j++)
            {
              *dest++ = (i - j);
              *dest++ = last;
            }
          start = i;
          last = *src;
        }
      src += bytes;
    }

  for (j = start; j < i; j++)
    {
      *dest++ = (i - j);
      *dest++ = last;
    }
}

