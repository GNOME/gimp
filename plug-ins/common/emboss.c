/**************************************************
 * file: emboss/emboss.c
 *
 * Copyright (c) 1997 Eric L. Hernes (erich@rrnet.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-emboss"
#define PLUG_IN_BINARY "emboss"
#define PLUG_IN_ROLE   "gimp-emboss"


enum
{
  FUNCTION_BUMPMAP = 0,
  FUNCTION_EMBOSS  = 1
};

typedef struct
{
  gdouble  azimuth;
  gdouble  elevation;
  gint32   depth;
  gint32   embossp;
} piArgs;

static piArgs evals =
{
  30.0,    /* azimuth   */
  45.0,    /* elevation */
  20,      /* depth     */
  1        /* emboss    */
};

struct embossFilter
{
  gdouble Lx;
  gdouble Ly;
  gdouble Lz;
  gdouble Nz;
  gdouble Nz2;
  gdouble NzLz;
  gdouble bg;
} static Filter;

static void     query         (void);
static void     run           (const gchar      *name,
                               gint              nparam,
                               const GimpParam  *param,
                               gint             *nretvals,
                               GimpParam       **retvals);

static void     emboss        (GimpDrawable     *drawable,
                               GimpPreview      *preview);
static gboolean emboss_dialog (GimpDrawable     *drawable);

static void     emboss_init   (gdouble           azimuth,
                               gdouble           elevation,
                               gushort           width45);
static void     emboss_row    (const guchar     *src,
                               const guchar     *texture,
                               guchar           *dst,
                               guint             width,
                               guint             bypp,
                               gboolean          alpha);


#define DtoR(d) ((d)*(G_PI/(gdouble)180))


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init  */
  NULL,  /* quit  */
  query, /* query */
  run,   /* run   */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"  },
    { GIMP_PDB_IMAGE,    "image",     "The Image"                     },
    { GIMP_PDB_DRAWABLE, "drawable",  "The Drawable"                  },
    { GIMP_PDB_FLOAT,    "azimuth",   "The Light Angle (degrees)"     },
    { GIMP_PDB_FLOAT,    "elevation", "The Elevation Angle (degrees)" },
    { GIMP_PDB_INT32,    "depth",     "The Filter Width"              },
    { GIMP_PDB_INT32,    "emboss",    "Emboss or Bumpmap"             }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Simulate an image created by embossing"),
                          "Emboss or Bumpmap the given drawable, specifying "
                          "the angle and elevation for the light source.",
                          "Eric L. Hernes, John Schlag",
                          "Eric L. Hernes",
                          "1997",
                          N_("_Emboss..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Distorts");
}

static void
run (const gchar      *name,
     gint              nparam,
     const GimpParam  *param,
     gint             *nretvals,
     GimpParam       **retvals)
{
  static GimpParam  rvals[1];
  GimpDrawable     *drawable;

  *nretvals = 1;
  *retvals = rvals;

  INIT_I18N ();

  drawable = gimp_drawable_get (param[2].data.d_drawable);
  gimp_tile_cache_ntiles (drawable->ntile_cols);


  rvals[0].type = GIMP_PDB_STATUS;
  rvals[0].data.d_status = GIMP_PDB_SUCCESS;

  switch (param[0].data.d_int32)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &evals);

      if (! emboss_dialog (drawable))
        {
          rvals[0].data.d_status = GIMP_PDB_CANCEL;
        }
      else
        {
          gimp_set_data (PLUG_IN_PROC, &evals, sizeof (piArgs));
        }

      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparam != 7)
        {
          rvals[0].data.d_status = GIMP_PDB_CALLING_ERROR;
          break;
        }

      evals.azimuth   = param[3].data.d_float;
      evals.elevation = param[4].data.d_float;
      evals.depth     = param[5].data.d_int32;
      evals.embossp   = param[6].data.d_int32;

      emboss (drawable, NULL);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &evals);
      /* use this image and drawable, even with last args */
      emboss (drawable, NULL);
    break;
  }
}

#define pixelScale 255.9

static void
emboss_init (gdouble azimuth,
             gdouble elevation,
             gushort width45)
{
  /*
   * compute the light vector from the input parameters.
   * normalize the length to pixelScale for fast shading calculation.
   */
  Filter.Lx = cos (azimuth) * cos (elevation) * pixelScale;
  Filter.Ly = sin (azimuth) * cos (elevation) * pixelScale;
  Filter.Lz = sin (elevation) * pixelScale;

  /*
   * constant z component of image surface normal - this depends on the
   * image slope we wish to associate with an angle of 45 degrees, which
   * depends on the width of the filter used to produce the source image.
   */
  Filter.Nz = (6 * 255) / width45;
  Filter.Nz2 = Filter.Nz * Filter.Nz;
  Filter.NzLz = Filter.Nz * Filter.Lz;

  /* optimization for vertical normals: L.[0 0 1] */
  Filter.bg = Filter.Lz;
}


/*
 * ANSI C code from the article
 * "Fast Embossing Effects on Raster Image Data"
 * by John Schlag, jfs@kerner.com
 * in "Graphics Gems IV", Academic Press, 1994
 *
 *
 * Emboss - shade 24-bit pixels using a single distant light source.
 * Normals are obtained by differentiating a monochrome 'bump' image.
 * The unary case ('texture' == NULL) uses the shading result as output.
 * The binary case multiples the optional 'texture' image by the shade.
 * Images are in row major order with interleaved color components (rgbrgb...).
 * E.g., component c of pixel x,y of 'dst' is dst[3*(y*width + x) + c].
 *
 */

static void
emboss_row (const guchar *src,
            const guchar *texture,
            guchar       *dst,
            guint         width,
            guint         bypp,
            gboolean      alpha)
{
  const guchar *s[3];
  gdouble       M[3][3];
  gint          x, bytes;

  /* mung pixels, avoiding edge pixels */
  s[0] = src;
  s[1] = s[0] + (width * bypp);
  s[2] = s[1] + (width * bypp);
  dst += bypp;

  bytes = (alpha) ? bypp - 1 : bypp;

  if (texture)
    texture += (width + 1) * bypp;

  for (x = 1; x < width - 1; x++)
    {
      gdouble a;
      glong   Nx, Ny, NdotL;
      gint    shade, b;
      gint    i, j;

      for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
          M[i][j] = 0.0;

      for (b = 0; b < bytes; b++)
        {
          for (i = 0; i < 3; i++)
            for (j = 0; j < 3; j++)
              {
                if (alpha)
                  a = s[i][j * bypp + bytes] / 255.0;
                else
                  a = 1.0;

                M[i][j] += a * s[i][j * bypp + b];
              }
        }

      Nx = M[0][0] + M[1][0] + M[2][0] - M[0][2] - M[1][2] - M[2][2];
      Ny = M[2][0] + M[2][1] + M[2][2] - M[0][0] - M[0][1] - M[0][2];

      /* shade with distant light source */
      if ( Nx == 0 && Ny == 0 )
        shade = Filter.bg;
      else if ( (NdotL = Nx * Filter.Lx + Ny * Filter.Ly + Filter.NzLz) < 0 )
        shade = 0;
      else
        shade = NdotL / sqrt(Nx*Nx + Ny*Ny + Filter.Nz2);

      /* do something with the shading result */
      if (texture)
        {
          for (b = 0; b < bytes; b++)
            *dst++ = (*texture++ * shade) >> 8;

          if (alpha)
            {
              *dst++ = s[1][bypp + bytes]; /* preserve the alpha */
              texture++;
            }
        }
      else
        {
          for (b = 0; b < bytes; b++)
            *dst++ = shade;

          if (alpha)
            *dst++ = s[1][bypp + bytes]; /* preserve the alpha */
        }

      for (i = 0; i < 3; i++)
        s[i] += bypp;
    }

  if (texture)
    texture += bypp;
}

static void
emboss (GimpDrawable *drawable,
        GimpPreview  *preview)
{
  GimpPixelRgn  src, dst;
  gint          p_update;
  gint          y;
  gint          x1, y1, x2, y2;
  gint          width, height;
  gint          bypp, rowsize;
  gboolean      has_alpha;
  guchar       *srcbuf, *dstbuf;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
      x2 = x1 + width;
      y2 = y1 + height;
    }
  else
    {
      if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                          &x1, &y1, &width, &height))
	return;

      /* expand the bounds a little */
      x1 = MAX (0, x1 - evals.depth);
      y1 = MAX (0, y1 - evals.depth);
      x2 = MIN (drawable->width, x1 + width + evals.depth);
      y2 = MIN (drawable->height, y1 + height + evals.depth);

      width  = x2 - x1;
      height = y2 - y1;
    }

  bypp = drawable->bpp;
  p_update = MAX (1, height / 20);
  rowsize = width * bypp;
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  gimp_pixel_rgn_init (&src, drawable,
                       x1, y1, width, height,
                       FALSE, FALSE);
  gimp_pixel_rgn_init (&dst, drawable,
                       x1, y1, width, height,
                       preview == NULL, TRUE);

  srcbuf = g_new0 (guchar, rowsize * 3);
  dstbuf = g_new0 (guchar, rowsize);

  emboss_init (DtoR(evals.azimuth), DtoR(evals.elevation), evals.depth);
  if (!preview)
    gimp_progress_init (_("Emboss"));

  /* first row */
  gimp_pixel_rgn_get_rect (&src, srcbuf, x1, y1, width, 3);
  memcpy (srcbuf, srcbuf + rowsize, rowsize);
  emboss_row (srcbuf, evals.embossp ? NULL : srcbuf,
              dstbuf, width, bypp, has_alpha);
  gimp_pixel_rgn_set_row (&dst, dstbuf, 0, 0, width);

  /* middle rows */
  for (y = 0; y < height - 2; y++)
    {
      if (! preview && (y % p_update == 0))
        gimp_progress_update ((gdouble) y / (gdouble) height);

      gimp_pixel_rgn_get_rect (&src, srcbuf, x1, y1 + y, width, 3);
      emboss_row (srcbuf, evals.embossp ? NULL : srcbuf,
                  dstbuf, width, bypp, has_alpha);
      gimp_pixel_rgn_set_row (&dst, dstbuf, x1, y1 + y + 1, width);
    }

  /* last row */
  gimp_pixel_rgn_get_rect (&src, srcbuf, x1, y2 - 3, width, 3);
  memcpy (srcbuf + rowsize * 2, srcbuf + rowsize, rowsize);
  emboss_row (srcbuf, evals.embossp ? NULL : srcbuf,
              dstbuf, width, bypp, has_alpha);
  gimp_pixel_rgn_set_row (&dst, dstbuf, x1, y2 - 1, width);

  if (preview)
    {
      gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                         &dst);
    }
  else
    {
      gimp_progress_update (1.0);

      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
      gimp_displays_flush ();
    }

  g_free (srcbuf);
  g_free (dstbuf);
}

static gboolean
emboss_dialog (GimpDrawable *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *preview;
  GtkWidget     *radio1;
  GtkWidget     *radio2;
  GtkWidget     *frame;
  GtkWidget     *table;
  GtkAdjustment *adj;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Emboss"), PLUG_IN_ROLE,
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
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (emboss),
                            drawable);

  frame = gimp_int_radio_group_new (TRUE, _("Function"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &evals.embossp, evals.embossp,

                                    _("_Bumpmap"), FUNCTION_BUMPMAP, &radio1,
                                    _("_Emboss"),  FUNCTION_EMBOSS,  &radio2,

                                    NULL);

  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_signal_connect_swapped (radio1, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (radio2, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Azimuth:"), 100, 6,
                              evals.azimuth, 0.0, 360.0, 1.0, 10.0, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &evals.azimuth);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("E_levation:"), 100, 6,
                              evals.elevation, 0.0, 180.0, 1.0, 10.0, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &evals.elevation);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                              _("_Depth:"), 100, 6,
                              evals.depth, 1.0, 100.0, 1.0, 5.0, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &evals.depth);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (table);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  if (run)
    emboss (drawable, NULL);

  return run;
}
