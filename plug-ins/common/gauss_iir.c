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


typedef struct
{
  gdouble  radius;
  gboolean horizontal;
  gboolean vertical;
} BlurValues;

typedef struct
{
  gdouble horizontal;
  gdouble vertical;
} Blur2Values;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static void      gauss_iir         (GimpDrawable *drawable,
                                    gdouble       horizontal,
                                    gdouble       vertical);

/*
 * Gaussian blur interface
 */
static gint      gauss_iir_dialog  (void);
static gint      gauss_iir2_dialog (gint32        image_ID,
                                    GimpDrawable *drawable);

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


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static BlurValues bvals =
{
  5.0,  /*  radius           */
  TRUE, /*  horizontal blur  */
  TRUE  /*  vertical blur    */
};

static Blur2Values b2vals =
{
  5.0,  /*  x radius  */
  5.0   /*  y radius  */
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
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

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
                          N_("<Image>/Filters/Blur/Gaussian Blur (_IIR)..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args2), 0,
                          args2, NULL);
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

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified image and drawable  */
  image_ID = param[1].data.d_image;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  if (strcmp (name, "plug_in_gauss_iir") == 0)   /* the old-fashioned way of calling it */
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data ("plug_in_gauss_iir", &bvals);

          /*  First acquire information with a dialog  */
          if (! gauss_iir_dialog ())
            return;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 6)
            status = GIMP_PDB_CALLING_ERROR;
          if (status == GIMP_PDB_SUCCESS)
            {
              bvals.radius     = param[3].data.d_float;
              bvals.horizontal = (param[4].data.d_int32) ? TRUE : FALSE;
              bvals.vertical   = (param[5].data.d_int32) ? TRUE : FALSE;
            }
          if (status == GIMP_PDB_SUCCESS && (bvals.radius <= 0.0))
            status = GIMP_PDB_CALLING_ERROR;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /*  Possibly retrieve data  */
          gimp_get_data ("plug_in_gauss_iir", &bvals);
          break;

        default:
          break;
        }

      if (!(bvals.horizontal || bvals.vertical))
        {
          g_message (_("You must specify either horizontal "
                       "or vertical (or both)"));
          status = GIMP_PDB_CALLING_ERROR;
        }
    }
  else if (strcmp (name, "plug_in_gauss_iir2") == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data ("plug_in_gauss_iir2", &b2vals);

          /*  First acquire information with a dialog  */
          if (! gauss_iir2_dialog (image_ID, drawable))
            return;
          break;
        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 5)
            status = GIMP_PDB_CALLING_ERROR;
          if (status == GIMP_PDB_SUCCESS)
            {
              b2vals.horizontal = param[3].data.d_float;
              b2vals.vertical   = param[4].data.d_float;
            }
          if (status == GIMP_PDB_SUCCESS &&
              (b2vals.horizontal <= 0.0 && b2vals.vertical <= 0.0))
            status = GIMP_PDB_CALLING_ERROR;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /*  Possibly retrieve data  */
          gimp_get_data ("plug_in_gauss_iir2", &b2vals);
          break;

        default:
          break;
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
          gimp_progress_init (_("IIR Gaussian Blur"));

          /*  set the tile cache size so that the gaussian blur works well  */
          gimp_tile_cache_ntiles (2 *
                                  (MAX (drawable->width, drawable->height) /
                                   gimp_tile_width () + 1));

          /*  run the gaussian blur  */
          if (strcmp (name, "plug_in_gauss_iir") == 0)
            {
              gauss_iir (drawable, (bvals.horizontal ? bvals.radius : 0.0),
                                   (bvals.vertical ? bvals.radius : 0.0));

              /*  Store data  */
              if (run_mode == GIMP_RUN_INTERACTIVE)
                gimp_set_data ("plug_in_gauss_iir",
                               &bvals, sizeof (BlurValues));
            }
          else
            {
              gauss_iir (drawable, b2vals.horizontal, b2vals.vertical);

              /*  Store data  */
              if (run_mode == GIMP_RUN_INTERACTIVE)
                gimp_set_data ("plug_in_gauss_iir2",
                               &b2vals, sizeof (Blur2Values));
            }

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

static gint
gauss_iir_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  gboolean   run;

  gimp_ui_init ("gauss_iir", FALSE);

  dlg = gimp_dialog_new (_("IIR Gaussian Blur"), "gauss_iir",
                         NULL, 0,
                         gimp_standard_help_func, "plug-in-gauss-iir",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  toggle = gtk_check_button_new_with_label (_("Blur Horizontally"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.horizontal);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    (GtkSignalFunc) gimp_toggle_button_update,
                    &bvals.horizontal);

  toggle = gtk_check_button_new_with_label (_("Blur Vertically"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.vertical);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    (GtkSignalFunc) gimp_toggle_button_update,
                    &bvals.vertical);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new (_("Blur Radius:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&adj,
                                     bvals.radius, 0.0, GIMP_MAX_IMAGE_SIZE,
                                     0.0, 5.0, 0, 1, 2);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, TRUE, TRUE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &bvals.radius);

  gtk_widget_show (hbox);
  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}


static gint
gauss_iir2_dialog (gint32        image_ID,
                   GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *size;
  GimpUnit   unit;
  gdouble    xres;
  gdouble    yres;
  gboolean   run;

  gimp_ui_init ("gauss_iir2", FALSE);

  dlg = gimp_dialog_new (_("IIR Gaussian Blur"), "gauss_iir",
                         NULL, 0,
                         gimp_standard_help_func, "plug-in-gauss-iir2",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Blur Radius"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  unit = gimp_image_get_unit (image_ID);

  size = gimp_coordinates_new (unit, "%a", TRUE, FALSE, -1,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE,

                               b2vals.horizontal == b2vals.vertical,
                               FALSE,

                               _("_Horizontal:"), b2vals.horizontal, xres,
                               0, 8 * MAX (drawable->width, drawable->height),
                               0, 0,

                               _("_Vertical:"), b2vals.vertical, yres,
                               0, 8 * MAX (drawable->width, drawable->height),
                               0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (size), 4);
  gtk_container_add (GTK_CONTAINER (frame), size);

  gimp_size_entry_set_pixel_digits (GIMP_SIZE_ENTRY (size), 1);

  gtk_widget_show (size);
  gtk_widget_show (frame);
  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  if (run)
    {
      b2vals.horizontal = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size), 0);
      b2vals.vertical   = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size), 1);
    }

  gtk_widget_destroy (dlg);

  return run;
}


/* Convert from separated to premultiplied alpha, on a single scan line. */
static void
multiply_alpha (guchar *buf,
                gint    width,
                gint    bytes)
{
  gint i, j;
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
  gint i, j;
  guchar alpha;
  gdouble recip_alpha;
  gint new_val;

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
gauss_iir (GimpDrawable *drawable,
           gdouble       horz,
           gdouble       vert)
{
  GimpPixelRgn src_rgn, dest_rgn;
  gint width, height;
  gint bytes;
  gint has_alpha;
  guchar *dest;
  guchar *src, *sp_p, *sp_m;
  gdouble n_p[5], n_m[5];
  gdouble d_p[5], d_m[5];
  gdouble bd_p[5], bd_m[5];
  gdouble *val_p, *val_m, *vp, *vm;
  gint x1, y1, x2, y2;
  gint i, j;
  gint row, col, b;
  gint terms;
  gdouble progress, max_progress;
  gint initial_p[4];
  gint initial_m[4];
  gdouble std_dev;

  if (horz <= 0.0 && vert <= 0.0)
    return;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  width  = (x2 - x1);
  height = (y2 - y1);

  if (width < 1 || height < 1)
    return;

  bytes = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha(drawable->drawable_id);

  val_p = g_new (gdouble, MAX (width, height) * bytes);
  val_m = g_new (gdouble, MAX (width, height) * bytes);

  src =  g_new (guchar, MAX (width, height) * bytes);
  dest = g_new (guchar, MAX (width, height) * bytes);

  gimp_pixel_rgn_init (&src_rgn,
                       drawable, 0, 0, drawable->width, drawable->height,
                       FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn,
                       drawable, 0, 0, drawable->width, drawable->height,
                       TRUE, TRUE);

  progress = 0.0;
  max_progress  = (horz <= 0.0) ? 0 : width * height * horz;
  max_progress += (vert <= 0.0) ? 0 : width * height * vert;


  /*  First the vertical pass  */
  if (vert > 0.0)
    {
      vert = fabs (vert) + 1.0;
      std_dev = sqrt (-(vert * vert) / (2 * log (1.0 / 255.0)));

      /*  derive the constants for calculating the gaussian from the std dev  */
      find_constants (n_p, n_m, d_p, d_m, bd_p, bd_m, std_dev);

      for (col = 0; col < width; col++)
        {
          memset(val_p, 0, height * bytes * sizeof (gdouble));
          memset(val_m, 0, height * bytes * sizeof (gdouble));

          gimp_pixel_rgn_get_col (&src_rgn, src, col + x1, y1, (y2 - y1));
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
          if (has_alpha)
            separate_alpha (src, height, bytes);

          gimp_pixel_rgn_set_col (&dest_rgn, dest, col + x1, y1, (y2 - y1));

          progress += height * vert;
          if ((col % 5) == 0)
            gimp_progress_update (progress / max_progress);
        }

      /*  prepare for the horizontal pass  */
      gimp_pixel_rgn_init (&src_rgn,
                           drawable, 0, 0, drawable->width, drawable->height,
                           FALSE, TRUE);
    }

  /*  Now the horizontal pass  */
  if (horz > 0.0)
    {
      horz = fabs (horz) + 1.0;

      if (horz != vert)
        {
          std_dev = sqrt (-(horz * horz) / (2 * log (1.0 / 255.0)));

          /*  derive the constants for calculating the gaussian from the std dev  */
          find_constants (n_p, n_m, d_p, d_m, bd_p, bd_m, std_dev);
        }

      for (row = 0; row < height; row++)
        {
          memset(val_p, 0, width * bytes * sizeof (gdouble));
          memset(val_m, 0, width * bytes * sizeof (gdouble));

          gimp_pixel_rgn_get_row (&src_rgn, src, x1, row + y1, (x2 - x1));
          if (has_alpha)
            multiply_alpha (dest, width, bytes);

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

          gimp_pixel_rgn_set_row (&dest_rgn, dest, x1, row + y1, (x2 - x1));

          progress += width * horz;
          if ((row % 5) == 0)
            gimp_progress_update (progress / max_progress);
        }
    }

  /*  merge the shadow, update the drawable  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));

  /*  free up buffers  */
  g_free (val_p);
  g_free (val_m);
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
  gint b;
  gint bend = bytes * width;
  gdouble sum;

  for(b = 0; b < bend; b++)
    {
      sum = *src1++ + *src2++;
      *dest++ = (guchar) CLAMP (sum, 0.0, 255.0);
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
  gint i;
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
