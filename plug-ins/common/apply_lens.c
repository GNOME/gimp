/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Apply lens plug-in --- makes your selected part of the image look like it
 *                        is viewed under a solid lens.
 * Copyright (C) 1997 Morten Eriksen
 * mortene@pvv.ntnu.no
 * (If you do anything cool with this plug-in, or have ideas for
 * improvements (which aren't on my ToDo-list) - send me an email).
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

/*
  TO DO:
  - antialiasing
  - adjustable (R, G, B and A) filter
  - optimize for speed!
  - refraction index warning dialog box when value < 1.0
  - use "true" lens with specified thickness
  - option to apply inverted lens
  - adjustable "c" value in the ellipsoid formula
  - radiobuttons for "ellipsoid" or "only horiz" and "only vert" (like in the
    Ad*b* Ph*t*sh*p Spherify plug-in..)
  - clean up source code
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-applylens"
#define PLUG_IN_BINARY  "apply_lens"


/* Declare local functions.
 */
static void      query (void);
static void      run   (const gchar      *name,
                        gint              nparams,
                        const GimpParam  *param,
                        gint             *nreturn_vals,
                        GimpParam       **return_vals);

static void      drawlens    (GimpDrawable *drawable,
                              GimpPreview  *preview);
static gboolean  lens_dialog (GimpDrawable *drawable);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

typedef struct
{
  gdouble  refraction;
  gboolean keep_surr;
  gboolean use_bkgr;
  gboolean set_transparent;
} LensValues;

static LensValues lvals =
{
  /* Lens refraction value */
  1.7,
  /* Surroundings options */
  TRUE, FALSE, FALSE
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",          "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",             "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",          "Input drawable" },
    { GIMP_PDB_FLOAT,    "refraction",        "Lens refraction index" },
    { GIMP_PDB_INT32,    "keep-surroundings", "Keep lens surroundings" },
    { GIMP_PDB_INT32,    "set-background",    "Set lens surroundings to BG value" },
    { GIMP_PDB_INT32,    "set-transparent",   "Set lens surroundings transparent" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Simulate an elliptical lens over the image"),
                          "This plug-in uses Snell's law to draw "
                          "an ellipsoid lens over the image",
                          "Morten Eriksen",
                          "Morten Eriksen",
                          "1997",
                          N_("Apply _Lens..."),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC,
                             "<Image>/Filters/Light and Shadow/Glass");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch(run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &lvals);
      if (!lens_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 7)
        status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
        {
          lvals.refraction      = param[3].data.d_float;
          lvals.keep_surr       = param[4].data.d_int32;
          lvals.use_bkgr        = param[5].data.d_int32;
          lvals.set_transparent = param[6].data.d_int32;
        }

      if (status == GIMP_PDB_SUCCESS && (lvals.refraction < 1.0))
        status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &lvals);
      break;

    default:
      break;
    }

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
  gimp_progress_init (_("Applying lens"));
  drawlens (drawable, NULL);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();
  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_set_data (PLUG_IN_PROC, &lvals, sizeof (LensValues));

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/*
  Ellipsoid formula: x^2/a^2 + y^2/b^2 + z^2/c^2 = 1
 */
static void
find_projected_pos (gfloat  a2,
                    gfloat  b2,
                    gfloat  c2,
                    gfloat  x,
                    gfloat  y,
                    gfloat *projx,
                    gfloat *projy)
{
  gfloat z;
  gfloat nxangle, nyangle, theta1, theta2;
  gfloat ri1 = 1.0;
  gfloat ri2 = lvals.refraction;

  z = sqrt ((1 - x * x / a2 - y * y / b2) * c2);

  nxangle = acos (x / sqrt(x * x + z * z));
  theta1 = G_PI / 2 - nxangle;
  theta2 = asin (sin (theta1) * ri1 / ri2);
  theta2 = G_PI / 2 - nxangle - theta2;
  *projx = x - tan (theta2) * z;

  nyangle = acos (y / sqrt (y * y + z * z));
  theta1 = G_PI / 2 - nyangle;
  theta2 = asin (sin (theta1) * ri1 / ri2);
  theta2 = G_PI / 2 - nyangle - theta2;
  *projy = y - tan (theta2) * z;
}

static void
drawlens (GimpDrawable *drawable,
          GimpPreview  *preview)
{
  GimpImageType  drawtype = gimp_drawable_type (drawable->drawable_id);
  GimpPixelRgn   srcPR, destPR;
  gint           width, height;
  gint           bytes;
  gint           row;
  gint           x1, y1, x2, y2;
  guchar        *src, *dest;
  gint           i, col;
  gfloat         regionwidth, regionheight, dx, dy, xsqr, ysqr;
  gfloat         a, b, c, asqr, bsqr, csqr, x, y;
  glong          pixelpos, pos;
  GimpRGB        background;
  guchar         bgr_red, bgr_blue, bgr_green;
  guchar         alphaval;

  gimp_context_get_background (&background);
  gimp_rgb_get_uchar (&background,
                      &bgr_red, &bgr_green, &bgr_blue);

  bytes = drawable->bpp;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
      x2 = x1 + width;
      y2 = y1 + height;
      src = gimp_drawable_get_thumbnail_data (drawable->drawable_id,
                                              &width, &height, &bytes);
      regionwidth  = width;
      regionheight = height;
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
      regionwidth = x2 - x1;
      regionheight = y2 - y1;
      width = drawable->width;
      height = drawable->height;
      gimp_pixel_rgn_init (&srcPR, drawable,
                           0, 0, width, height, FALSE, FALSE);
      gimp_pixel_rgn_init (&destPR, drawable,
                           0, 0, width, height, TRUE, TRUE);

      src  = g_new (guchar, regionwidth * regionheight * bytes);
      gimp_pixel_rgn_get_rect (&srcPR, src,
                               x1, y1, regionwidth, regionheight);
    }

  dest = g_new (guchar, regionwidth * regionheight * bytes);

  a = regionwidth / 2;
  b = regionheight / 2;

  c = MIN (a, b);

  asqr = a * a;
  bsqr = b * b;
  csqr = c * c;

  for (col = 0; col < regionwidth; col++)
    {
      dx = (gfloat) col - a + 0.5;
      xsqr = dx * dx;
      for (row = 0; row < regionheight; row++)
        {
          pixelpos = (col + row * regionwidth) * bytes;
          dy = -((gfloat) row - b) - 0.5;
          ysqr = dy * dy;
          if (ysqr < (bsqr - (bsqr * xsqr) / asqr))
            {
              find_projected_pos (asqr, bsqr, csqr, dx, dy, &x, &y);
              y = -y;
              pos = ((gint) (y + b) * regionwidth + (gint) (x + a)) * bytes;

              for (i = 0; i < bytes; i++)
                {
                  dest[pixelpos + i] = src[pos + i];
                }
            }
          else
            {
              if (lvals.keep_surr)
                {
                  for (i = 0; i < bytes; i++)
                    {
                      dest[pixelpos + i] = src[pixelpos + i];
                    }
                }
              else
                {
                  if (lvals.set_transparent)
                    alphaval = 0;
                  else
                    alphaval = 255;

                  switch (drawtype)
                    {
                    case GIMP_INDEXEDA_IMAGE:
                      dest[pixelpos + 1] = alphaval;
                    case GIMP_INDEXED_IMAGE:
                      dest[pixelpos + 0] = 0;
                      break;

                    case GIMP_RGBA_IMAGE:
                      dest[pixelpos + 3] = alphaval;
                    case GIMP_RGB_IMAGE:
                      dest[pixelpos + 0] = bgr_red;
                      dest[pixelpos + 1] = bgr_green;
                      dest[pixelpos + 2] = bgr_blue;
                      break;

                    case GIMP_GRAYA_IMAGE:
                      dest[pixelpos + 1] = alphaval;
                    case GIMP_GRAY_IMAGE:
                      dest[pixelpos+0] = bgr_red;
                      break;
                    }
                }
            }
        }

      if (!preview)
        {
          if (((gint) (regionwidth-col) % 5) == 0)
            gimp_progress_update ((gdouble) col / (gdouble) regionwidth);
        }
    }

  if (preview)
    {
      gimp_preview_draw_buffer (preview, dest, bytes * regionwidth);
    }
  else
    {
      gimp_pixel_rgn_set_rect (&destPR, dest, x1, y1,
                               regionwidth, regionheight);

      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, x2 - x1, y2 - y1);
    }

  g_free (src);
  g_free (dest);
}

static gboolean
lens_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *label;
  GtkWidget *toggle;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *spinbutton;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Lens Effect"), PLUG_IN_BINARY,
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

  preview = gimp_aspect_preview_new (drawable, NULL);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (drawlens),
                            drawable);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  toggle = gtk_radio_button_new_with_mnemonic_from_widget
    (NULL, _("_Keep original surroundings"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), lvals.keep_surr);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &lvals.keep_surr);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  toggle = gtk_radio_button_new_with_mnemonic_from_widget
    (GTK_RADIO_BUTTON (toggle),
     gimp_drawable_is_indexed (drawable->drawable_id)
     ? _("_Set surroundings to index 0")
     : _("_Set surroundings to background color"));
  gtk_box_pack_start(GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), lvals.use_bkgr);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &lvals.use_bkgr);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  if (gimp_drawable_has_alpha (drawable->drawable_id))
    {
      toggle = gtk_radio_button_new_with_mnemonic_from_widget
        (GTK_RADIO_BUTTON (toggle), _("_Make surroundings transparent"));
      gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    lvals.set_transparent);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &lvals.set_transparent);
      g_signal_connect_swapped (toggle, "toggled",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);
  }

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new_with_mnemonic (_("_Lens refraction index:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&adj, lvals.refraction,
                                     1.0, 100.0, 0.1, 1.0, 0, 1, 2);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &lvals.refraction);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (hbox);
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
