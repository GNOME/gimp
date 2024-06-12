/* Warp  --- image filter plug-in for GIMP
 * Copyright (C) 1997 John P. Beale
 * Much of the 'warp' is from the Displace plug-in: 1996 Stephen Robert Norris
 * Much of the 'displace' code taken in turn  from the pinch plug-in
 *   which is by 1996 Federico Mena Quintero
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
 * You can contact me (the warp author) at beale@best.com
 * Please send me any patches or enhancements to this code.
 * You can contact the original GIMP authors at gimp@xcf.berkeley.edu
 *
 * --------------------------------------------------------------------
 * Warp Program structure: after running the user interface and setting the
 * parameters, warp generates a brand-new image (later to be deleted
 * before the user ever sees it) which contains two grayscale layers,
 * representing the X and Y gradients of the "control" image. For this
 * purpose, all channels of the control image are summed for a scalar
 * value at each pixel coordinate for the gradient operation.
 *
 * The X,Y components of the calculated gradient are then used to
 * displace pixels from the source image into the destination
 * image. The displacement vector is rotated a user-specified amount
 * first. This displacement operation happens iteratively, generating
 * a new displaced image from each prior image.
 * -------------------------------------------------------------------
 *
 * Revision History:
 * Version 0.37  12/19/98 Fixed Tooltips and freeing memory
 * Version 0.36  11/9/97  Changed XY vector layers  back to own image
 *               fixed 'undo' problem (hopefully)
 *
 * Version 0.35  11/3/97  Added vector-map, mag-map, grad-map to
 *               diff vector instead of separate operation
 *               further futzing with drawable updates
 *               starting adding tooltips
 *
 * Version 0.34  10/30/97   'Fixed' drawable update problem
 *               Added 16-bit resolution to differential map
 *               Added substep increments for finer control
 *
 * Version 0.33  10/26/97   Added 'angle increment' to user interface
 *
 * Version 0.32  10/25/97   Added magnitude control map (secondary control)
 *               Changed undo behavior to be one undo-step per warp call.
 *
 * Version 0.31  10/25/97   Fixed src/dest pixregions so program works
 *               with multiple-layer images. Still don't know
 *               exactly what I did to fix it :-/  Also, added 'color' option
 *               for border pixels to use the current selected foreground color.
 *
 * Version 0.3   10/20/97  Initial release for Gimp 0.99.xx
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Some useful macros */

#define PLUG_IN_PROC    "plug-in-warp"
#define PLUG_IN_BINARY  "warp"
#define PLUG_IN_ROLE    "gimp-warp"
#define ENTRY_WIDTH     75
#define MIN_ARGS         6  /* minimum number of arguments required */

enum
{
  WRAP,
  SMEAR,
  BLACK,
  COLOR
};


typedef struct _Warp      Warp;
typedef struct _WarpClass WarpClass;

struct _Warp
{
  GimpPlugIn parent_instance;
};

struct _WarpClass
{
  GimpPlugInClass parent_class;
};


#define WARP_TYPE  (warp_get_type ())
#define WARP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WARP_TYPE, Warp))

GType                   warp_get_type         (void) G_GNUC_CONST;

static GList          * warp_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * warp_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * warp_run              (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               gint                  n_drawables,
                                               GimpDrawable        **drawables,
                                               GimpProcedureConfig  *config,
                                               gpointer              run_data);

static void             blur16                (GimpDrawable   *drawable);

static void             diff                  (GimpDrawable         *drawable,
                                               GimpProcedureConfig  *config,
                                               GimpDrawable        **xl,
                                               GimpDrawable        **yl);

static void             diff_prepare_row      (GeglBuffer     *buffer,
                                               const Babl     *format,
                                               guchar         *data,
                                               gint            x,
                                               gint            y,
                                               gint            w);

static void             warp_one              (GimpDrawable        *draw,
                                               GimpDrawable        *newid,
                                               GimpDrawable        *map_x,
                                               GimpDrawable        *map_y,
                                               GimpDrawable        *mag_draw,
                                               gboolean             first_time,
                                               gint                 step,
                                               GimpProcedureConfig *config);

static void             warp                  (GimpDrawable        *drawable,
                                               GimpProcedureConfig *config);

static gboolean         warp_dialog           (GimpDrawable        *drawable,
                                               GimpProcedure       *procedure,
                                               GimpProcedureConfig *config);
static void             warp_pixel            (GeglBuffer          *buffer,
                                               const Babl          *format,
                                               gint                 width,
                                               gint                 height,
                                               gint                 x1,
                                               gint                 y1,
                                               gint                 x2,
                                               gint                 y2,
                                               gint                 x,
                                               gint                 y,
                                               guchar              *pixel,
                                               GimpProcedureConfig *config);

static gdouble       warp_map_mag_give_value  (guchar         *pt,
                                               gint            alpha,
                                               gint            bytes);


G_DEFINE_TYPE (Warp, warp, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (WARP_TYPE)
DEFINE_STD_SET_I18N


static gint         progress = 0;              /* progress indicator bar      */
static GimpRunMode  run_mode;                  /* interactive, non-, etc.     */
static guchar       color_pixel[4] = {0, 0, 0, 255};  /* current fg color     */


static void
warp_class_init (WarpClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = warp_query_procedures;
  plug_in_class->create_procedure = warp_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
warp_init (Warp *warp)
{
}

static GList *
warp_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
warp_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            warp_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Warp..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Map");

      gimp_procedure_set_documentation (procedure,
                                        _("Twist or smear image in many "
                                          "different ways"),
                                        _("Smears an image along vector paths "
                                          "calculated as the gradient of a "
                                          "separate control matrix. The effect "
                                          "can look like brushstrokes of acrylic "
                                          "or watercolor paint, in some cases."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "John P. Beale",
                                      "John P. Beale",
                                      "1997");

      gimp_procedure_add_double_argument (procedure, "amount",
                                          _("Step si_ze"),
                                          _("Pixel displacement multiplier"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, 10.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_drawable_argument (procedure, "warp-map",
                                            _("Dis_placement Map"),
                                            _("Displacement control map"),
                                            TRUE,
                                            G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "iter",
                                       _("I_terations"),
                                       _("Iteration count"),
                                       1, 100, 5,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "dither",
                                          _("_Dither size"),
                                          _("Random dither amount"),
                                          0, 100, 0.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "angle",
                                          _("Rotatio_n angle"),
                                          _("Angle of gradient vector rotation"),
                                          0, 360, 90.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "wrap-type",
                                          _("On ed_ges"),
                                          _("Wrap type"),
                                          gimp_choice_new_with_values ("wrap",  WRAP,  _("Wrap"),             NULL,
                                                                       "smear", SMEAR, _("Smear"),            NULL,
                                                                       "black", BLACK, _("Black"),            NULL,
                                                                       "color", COLOR, _("Foreground Color"), NULL,
                                                                       NULL),
                                          "wrap",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_drawable_argument (procedure, "mag-map",
                                            _("_Magnitude Map"),
                                            _("Magnitude control map"),
                                            TRUE,
                                            G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "mag-use",
                                           _("_Use magnitude map"),
                                           _("Use magnitude map"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "substeps",
                                       _("Su_bsteps"),
                                       _("Substeps between image updates"),
                                       1, 100, 1,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_drawable_argument (procedure, "grad-map",
                                            _("Gradient Ma_p"),
                                            _("Gradient control map"),
                                            TRUE,
                                            G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "grad-scale",
                                          _("Gradient s_cale"),
                                          _("Scaling factor for gradient map (0=don't use)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_drawable_argument (procedure, "vector-map",
                                            _("_Vector Map"),
                                            _("Fixed vector control map"),
                                            TRUE,
                                            G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "vector-scale",
                                          _("Vector magn_itude"),
                                          _("Scaling factor for fixed vector map (0=don't use)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "vector-angle",
                                          _("Ang_le"),
                                          _("Angle for fixed vector map"),
                                          0, 360, 0.0,
                                          G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
warp_run (GimpProcedure        *procedure,
          GimpRunMode           _run_mode,
          GimpImage            *image,
          gint                  n_drawables,
          GimpDrawable        **drawables,
          GimpProcedureConfig  *config,
          gpointer              run_data)
{
  GimpDrawable *drawable;
  GeglColor    *color;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  /* get currently selected foreground pixel color */
  color = gimp_context_get_foreground ();
  gegl_color_get_pixel (color, babl_format ("R'G'B' u8"), color_pixel);
  g_object_unref (color);

  run_mode = _run_mode;

  /* Initialize maps for GUI */
  if (run_mode == GIMP_RUN_INTERACTIVE)
    g_object_set (config,
                  "warp-map",   drawable,
                  "mag-map",    drawable,
                  "grad-map",   drawable,
                  "vector-map", drawable,
                  NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE &&
      ! warp_dialog (drawable, procedure, config))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  warp (drawable, config);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
warp_dialog (GimpDrawable        *drawable,
             GimpProcedure       *procedure,
             GimpProcedureConfig *config)
{
  GtkWidget    *dialog;
  GtkWidget    *hbox;
  gboolean      run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Warp"));

  /* Basic Options */
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "basic-options-left",
                                  "amount", "iter", "wrap-type",
                                  NULL);

  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "basic-options-hbox",
                                         "basic-options-left",
                                         "warp-map",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (hbox), 12);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "basic-options-label", _("Basic Options"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "basic-options-frame",
                                    "basic-options-label", FALSE,
                                    "basic-options-hbox");

  /* Advanced Options */
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "advanced-options-left",
                                  "dither", "angle", "substeps",
                                  NULL);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "advanced-options-right",
                                  "mag-map", "mag-use",
                                  NULL);

  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "advanced-options-hbox",
                                         "advanced-options-left",
                                         "advanced-options-right",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (hbox), 12);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "advanced-options-label",
                                   _("Advanced Options"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "advanced-options-frame",
                                    "advanced-options-label", FALSE,
                                    "advanced-options-hbox");

  /* More Advanced Options */
  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "more-advanced-options-left",
                                  "grad-scale", "vector-scale",
                                  "vector-angle",
                                  NULL);

  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "more-advanced-options-hbox",
                                         "more-advanced-options-left",
                                         "grad-map", "vector-map",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (hbox), 12);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "more-advanced-options-label",
                                   _("More Advanced Options"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "more-advanced-options-frame",
                                    "more-advanced-options-label", FALSE,
                                    "more-advanced-options-hbox");

  /* Show dialog */
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "basic-options-frame",
                              "advanced-options-frame",
                              "more-advanced-options-frame",
                              NULL);

  gtk_widget_set_visible (dialog, TRUE);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

/* ---------------------------------------------------------------------- */

static const Babl *
get_u8_format (GimpDrawable *drawable)
{
  if (gimp_drawable_is_rgb (drawable))
    {
      if (gimp_drawable_has_alpha (drawable))
        return babl_format ("R'G'B'A u8");
      else
        return babl_format ("R'G'B' u8");
    }
  else
    {
      if (gimp_drawable_has_alpha (drawable))
        return babl_format ("Y'A u8");
      else
        return babl_format ("Y' u8");
    }
}

static void
blur16 (GimpDrawable *drawable)
{
  /*  blur a 2-or-more byte-per-pixel drawable,
   *  1st 2 bytes interpreted as a 16-bit height field.
   */
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *format;
  gint width, height;
  gint src_bytes;
  gint dest_bytes;
  gint dest_bytes_inc;
  gint offb, off1;

  guchar *dest, *d;  /* pointers to rows of X and Y diff. data */
  guchar *prev_row, *pr;
  guchar *cur_row, *cr;
  guchar *next_row, *nr;
  guchar *tmp;
  gint row, col;  /* relating to indexing into pixel row arrays */
  gint x1, y1, x2, y2;
  gdouble pval;          /* average pixel value of pixel & neighbors */

  /* --------------------------------------- */

  if (! gimp_drawable_mask_intersect (drawable,
                                      &x1, &y1, &width, &height))
    return;

  x2 = x1 + width;
  y2 = y1 + height;

  width  = gimp_drawable_get_width  (drawable);     /* size of input drawable*/
  height = gimp_drawable_get_height (drawable);

  format = get_u8_format (drawable);

  /* bytes per pixel in SOURCE drawable, must be 2 or more  */
  src_bytes = babl_format_get_bytes_per_pixel (format);

  dest_bytes = src_bytes;  /* bytes per pixel in SOURCE drawable, >= 2  */
  dest_bytes_inc = dest_bytes - 2;  /* this is most likely zero, but I guess it's more conservative... */

  /*  allocate row buffers for source & dest. data  */

  prev_row = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  cur_row  = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  next_row = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  dest     = g_new (guchar, (x2 - x1) * src_bytes);

  /* initialize the pixel regions (read from source, write into dest)  */
  src_buffer   = gimp_drawable_get_buffer (drawable);
  dest_buffer  = gimp_drawable_get_shadow_buffer (drawable);

  pr = prev_row + src_bytes;   /* row arrays are prepared for indexing to -1 (!) */
  cr = cur_row  + src_bytes;
  nr = next_row + src_bytes;

  diff_prepare_row (src_buffer, format, pr, x1, y1, (x2 - x1));
  diff_prepare_row (src_buffer, format, cr, x1, y1+1, (x2 - x1));

  /*  loop through the rows, applying the smoothing function  */
  for (row = y1; row < y2; row++)
    {
      /*  prepare the next row  */
      diff_prepare_row (src_buffer, format, nr, x1, row + 1, (x2 - x1));

      d = dest;
      for (col = 0; col < (x2 - x1); col++) /* over columns of pixels */
        {
          offb = col*src_bytes;    /* base of byte pointer offset */
          off1 = offb+1;                 /* offset into row arrays */

          pval = (256.0 * pr[offb - src_bytes] + pr[off1 - src_bytes] +
                  256.0 * pr[offb] + pr[off1] +
                  256.0 * pr[offb + src_bytes] + pr[off1 + src_bytes] +
                  256.0 * cr[offb - src_bytes] + cr[off1 - src_bytes] +
                  256.0 * cr[offb]  + cr[off1] +
                  256.0 * cr[offb + src_bytes] + cr[off1 + src_bytes] +
                  256.0 * nr[offb - src_bytes] + nr[off1 - src_bytes] +
                  256.0 * nr[offb] + nr[off1] +
                  256.0 * nr[offb + src_bytes]) + nr[off1 + src_bytes];

          pval /= 9.0;  /* take the average */
          *d++ = (guchar) (((gint) pval) >> 8);   /* high-order byte */
          *d++ = (guchar) (((gint) pval) % 256);  /* low-order byte  */
          d += dest_bytes_inc;       /* move data pointer on to next destination pixel */
       }

      /*  store the dest  */
      gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (x1, row, (x2 - x1), 1), 0,
                       format, dest,
                       GEGL_AUTO_ROWSTRIDE);

      /*  shuffle the row pointers  */
      tmp = pr;
      pr = cr;
      cr = nr;
      nr = tmp;

      if ((row % 8) == 0)
        gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  gimp_progress_update (1.0);

  gimp_drawable_merge_shadow (drawable, TRUE);
  gimp_drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));

  g_free (prev_row);  /* row buffers allocated at top of fn. */
  g_free (cur_row);
  g_free (next_row);
  g_free (dest);

}


/* ====================================================================== */
/* Get one row of pixels from the PixelRegion and put them in 'data'      */

static void
diff_prepare_row (GeglBuffer *buffer,
                  const Babl *format,
                  guchar     *data,
                  gint        x,
                  gint        y,
                  gint        w)
{
  gint bpp = babl_format_get_bytes_per_pixel (format);
  gint b;

  /* y = CLAMP (y, 0, pixel_rgn->h - 1); FIXME? */

  gegl_buffer_get (buffer, GEGL_RECTANGLE (x, y, w, 1), 1.0,
                   format, data,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  /*  Fill in edge pixels  */
  for (b = 0; b < bpp; b++)
    {
      data[b - (gint) bpp] = data[b];
      data[w * bpp + b] = data[(w - 1) * bpp + b];
    }
}

/* -------------------------------------------------------------------------- */
/*  'diff' combines the input drawables to prepare the two                    */
/*  16-bit (X,Y) vector displacement maps                                     */
/* -------------------------------------------------------------------------- */

static void
diff (GimpDrawable         *drawable,
      GimpProcedureConfig  *config,
      GimpDrawable        **xl,
      GimpDrawable        **yl)
{
  GimpDrawable *draw_xd;
  GimpDrawable *draw_yd; /* vector disp. drawables */
  GimpImage    *image;              /* image holding X and Y diff. arrays */
  GimpImage    *new_image;          /* image holding X and Y diff. layers */
  GList        *selected_layers;    /* currently selected layers          */
  GimpLayer    *xlayer;
  GimpLayer    *ylayer;  /* individual X and Y layer ID numbers */
  GeglBuffer   *src_buffer;
  GeglBuffer   *destx_buffer;
  const Babl   *destx_format;
  GeglBuffer   *desty_buffer;
  const Babl   *desty_format;
  GeglBuffer   *vec_buffer;
  GeglBuffer   *mag_buffer = NULL;
  GeglBuffer   *grad_buffer;
  gint          width, height;
  const Babl   *src_format;
  gint          src_bytes;
  const Babl   *mformat = NULL;
  gint          mbytes  = 0;
  const Babl   *vformat = NULL;
  gint          vbytes  = 0;
  const Babl   *gformat = NULL;
  gint          gbytes  = 0;   /* bytes-per-pixel of various source drawables */
  const Babl   *dest_format;
  gint          dest_bytes;
  gint          dest_bytes_inc;
  gint          do_gradmap = FALSE; /* whether to add in gradient of gradmap to final diff. map */
  gint          do_vecmap = FALSE; /* whether to add in a fixed vector scaled by the vector map */
  gint          do_magmap = FALSE; /* whether to multiply result by the magnitude map */

  guchar *destx, *dx, *desty, *dy;  /* pointers to rows of X and Y diff. data */
  guchar *tmp;
  guchar *prev_row, *pr;
  guchar *cur_row, *cr;
  guchar *next_row, *nr;
  guchar *prev_row_g, *prg = NULL;  /* pointers to gradient map data */
  guchar *cur_row_g, *crg = NULL;
  guchar *next_row_g, *nrg = NULL;
  guchar *cur_row_v, *crv = NULL;   /* pointers to vector map data */
  guchar *cur_row_m, *crm = NULL;   /* pointers to magnitude map data */
  gint row, col, offb, off, bytes;  /* relating to indexing into pixel row arrays */
  gint x1, y1, x2, y2;
  gint dvalx, dvaly;                /* differential value at particular pixel */
  gdouble tx, ty;                   /* temporary x,y differential value increments from gradmap, etc. */
  gdouble rdx, rdy;                 /* x,y differential values: real #s */
  gdouble rscalefac;                /* scaling factor for x,y differential of 'curl' map */
  gdouble gscalefac;                /* scaling factor for x,y differential of 'gradient' map */
  gdouble r, theta, dtheta;         /* rectangular<-> spherical coordinate transform for vector rotation */
  gdouble scale_vec_x, scale_vec_y; /* fixed vector X,Y component scaling factors */

  GimpDrawable *mag_map;
  GimpDrawable *grad_map;
  GimpDrawable *vector_map;
  gdouble       angle;
  gdouble       grad_scale;
  gdouble       vector_scale;
  gdouble       vector_angle;
  gboolean      use_mag;

  gimp_ui_init (PLUG_IN_BINARY);

  g_object_get (config,
                "angle",        &angle,
                "mag-use",      &use_mag,
                "grad-scale",   &grad_scale,
                "vector-scale", &vector_scale,
                "vector-angle", &vector_angle,
                NULL);

  /* ----------------------------------------------------------------------- */

  if (grad_scale != 0.0)
    do_gradmap = TRUE;              /* add in gradient of gradmap if scale != 0.000 */

  if (vector_scale != 0.0)    /* add in gradient of vectormap if scale != 0.000 */
    do_vecmap = TRUE;

  do_magmap = (use_mag == TRUE); /* multiply by magnitude map if so requested */

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  if (! gimp_drawable_mask_intersect (drawable,
                                      &x1, &y1, &width, &height))
    return;

  g_object_get (config,
                "mag-map",    &mag_map,
                "grad-map",   &grad_map,
                "vector-map", &vector_map,
                NULL);

  x2 = x1 + width;
  y2 = y1 + height;

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width  = gimp_drawable_get_width  (drawable);
  height = gimp_drawable_get_height (drawable);

  src_format = get_u8_format (drawable);
  src_bytes  = babl_format_get_bytes_per_pixel (src_format);

  /* -- Add two layers: X and Y Displacement vectors -- */
  /* -- I'm using a RGB  drawable and using the first two bytes for a
        16-bit pixel value. This is either clever, or a kluge,
        depending on your point of view.  */

  image           = gimp_item_get_image (GIMP_ITEM (drawable));
  selected_layers = gimp_image_list_selected_layers (image);

  /* create new image for X,Y diff */
  new_image = gimp_image_new (width, height, GIMP_RGB);

  xlayer = gimp_layer_new (new_image, "Warp_X_Vectors",
                           width, height,
                           GIMP_RGB_IMAGE,
                           100.0,
                           gimp_image_get_default_new_layer_mode (new_image));

  ylayer = gimp_layer_new (new_image, "Warp_Y_Vectors",
                           width, height,
                           GIMP_RGB_IMAGE,
                           100.0,
                           gimp_image_get_default_new_layer_mode (new_image));

  draw_yd = GIMP_DRAWABLE (ylayer);
  draw_xd = GIMP_DRAWABLE (xlayer);

  gimp_image_insert_layer (new_image, xlayer, NULL, 1);
  gimp_image_insert_layer (new_image, ylayer, NULL, 1);
  gimp_drawable_fill (GIMP_DRAWABLE (xlayer), GIMP_FILL_BACKGROUND);
  gimp_drawable_fill (GIMP_DRAWABLE (ylayer), GIMP_FILL_BACKGROUND);
  gimp_image_take_selected_layers (image, selected_layers);

  dest_format = get_u8_format (draw_xd);
  dest_bytes  = babl_format_get_bytes_per_pixel (dest_format);
  /* for a GRAYA drawable, I would expect this to be two bytes; any more would be excess */
  dest_bytes_inc = dest_bytes - 2;

  /*  allocate row buffers for source & dest. data  */

  prev_row = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  cur_row  = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  next_row = g_new (guchar, (x2 - x1 + 2) * src_bytes);

  prev_row_g = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  cur_row_g  = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  next_row_g = g_new (guchar, (x2 - x1 + 2) * src_bytes);

  cur_row_v = g_new (guchar, (x2 - x1 + 2) * src_bytes);  /* vector map */
  cur_row_m = g_new (guchar, (x2 - x1 + 2) * src_bytes);  /* magnitude map */

  destx = g_new (guchar, (x2 - x1) * dest_bytes);
  desty = g_new (guchar, (x2 - x1) * dest_bytes);

  /*  initialize the source and destination pixel regions  */

  /* 'curl' vector-rotation input */
  src_buffer = gimp_drawable_get_buffer (drawable);

  /* destination: X diff output */
  destx_buffer = gimp_drawable_get_buffer (draw_xd);
  destx_format = get_u8_format (draw_xd);

  /* Y diff output */
  desty_buffer = gimp_drawable_get_buffer (draw_yd);
  desty_format = get_u8_format (draw_yd);

  pr = prev_row + src_bytes;
  cr = cur_row  + src_bytes;
  nr = next_row + src_bytes;

  diff_prepare_row (src_buffer, src_format, pr, x1, y1, (x2 - x1));
  diff_prepare_row (src_buffer, src_format, cr, x1, y1+1, (x2 - x1));

 /* fixed-vector (x,y) component scale factors */
  scale_vec_x = (vector_scale *
                 cos ((90 - vector_angle) * G_PI / 180.0) * 256.0 / 10);
  scale_vec_y = (vector_scale *
                 sin ((90 - vector_angle) * G_PI / 180.0) * 256.0 / 10);

  if (do_vecmap)
    {
      /* bytes per pixel in SOURCE drawable */
      vformat = get_u8_format (vector_map);
      vbytes  = babl_format_get_bytes_per_pixel (vformat);

      /* fixed-vector scale-map */
      vec_buffer = gimp_drawable_get_buffer (vector_map);

      crv = cur_row_v + vbytes;
      diff_prepare_row (vec_buffer, vformat, crv, x1, y1, (x2 - x1));
    }

  if (do_gradmap)
    {
      gformat = get_u8_format (grad_map);
      gbytes  = babl_format_get_bytes_per_pixel (gformat);

      /* fixed-vector scale-map */
      grad_buffer = gimp_drawable_get_buffer (grad_map);

      prg = prev_row_g + gbytes;
      crg = cur_row_g + gbytes;
      nrg = next_row_g + gbytes;
      diff_prepare_row (grad_buffer, gformat, prg, x1, y1 - 1, (x2 - x1));
      diff_prepare_row (grad_buffer, gformat, crg, x1, y1, (x2 - x1));
    }

  if (do_magmap)
    {
      mformat = get_u8_format (mag_map);
      mbytes  = babl_format_get_bytes_per_pixel (mformat);

      /* fixed-vector scale-map */
      mag_buffer = gimp_drawable_get_buffer (mag_map);

      crm = cur_row_m + mbytes;
      diff_prepare_row (mag_buffer, mformat, crm, x1, y1, (x2 - x1));
    }

  dtheta = angle * G_PI / 180.0;
  /* note that '3' is rather arbitrary here. */
  rscalefac = 256.0 / (3 * src_bytes);
  /* scale factor for gradient map components */
  gscalefac = grad_scale * 256.0 / (3 * gbytes);

  /*  loop through the rows, applying the differential convolution  */
  for (row = y1; row < y2; row++)
    {
      /*  prepare the next row  */
      diff_prepare_row (src_buffer, src_format, nr, x1, row + 1, (x2 - x1));

      if (do_magmap)
        diff_prepare_row (mag_buffer, mformat, crm, x1, row + 1, (x2 - x1));
      if (do_vecmap)
        diff_prepare_row (vec_buffer, vformat, crv, x1, row + 1, (x2 - x1));
      if (do_gradmap)
        diff_prepare_row (grad_buffer, gformat, crg, x1, row + 1, (x2 - x1));

      dx = destx;
      dy = desty;

      for (col = 0; col < (x2 - x1); col++) /* over columns of pixels */
        {
          rdx = 0.0;
          rdy = 0.0;
          ty = 0.0;
          tx = 0.0;

          offb = col * src_bytes;    /* base of byte pointer offset */
          for (bytes=0; bytes < src_bytes; bytes++) /* add all channels together */
            {
              off = offb+bytes;                 /* offset into row arrays */
              rdx += ((gint) -pr[off - src_bytes]   + (gint) pr[off + src_bytes] +
                      (gint) -2*cr[off - src_bytes] + (gint) 2*cr[off + src_bytes] +
                      (gint) -nr[off - src_bytes]   + (gint) nr[off + src_bytes]);

              rdy += ((gint) -pr[off - src_bytes] - (gint)2*pr[off] - (gint) pr[off + src_bytes] +
                      (gint) nr[off - src_bytes] + (gint)2*nr[off] + (gint) nr[off + src_bytes]);
            }

          rdx *= rscalefac;   /* take average, then reduce. Assume max. rdx now 65535 */
          rdy *= rscalefac;   /* take average, then reduce */

          theta = atan2(rdy,rdx);          /* convert to polar, then back to rectang. coords */
          r = sqrt(rdy*rdy + rdx*rdx);
          theta += dtheta;              /* rotate gradient vector by this angle (radians) */
          rdx = r * cos(theta);
          rdy = r * sin(theta);

          if (do_gradmap)
            {
              offb = col*gbytes;     /* base of byte pointer offset into pixel values (R,G,B,Alpha, etc.) */
              for (bytes=0; bytes < src_bytes; bytes++) /* add all channels together */
                {
                  off = offb+bytes;                 /* offset into row arrays */
                  tx += ((gint) -prg[off - gbytes]   + (gint) prg[off + gbytes] +
                         (gint) -2*crg[off - gbytes] + (gint) 2*crg[off + gbytes] +
                         (gint) -nrg[off - gbytes]   + (gint) nrg[off + gbytes]);

                  ty += ((gint) -prg[off - gbytes] - (gint)2*prg[off] - (gint) prg[off + gbytes] +
                         (gint) nrg[off - gbytes] + (gint)2*nrg[off] + (gint) nrg[off + gbytes]);
                }
              tx *= gscalefac;
              ty *= gscalefac;

              rdx += tx;         /* add gradient component in to the other one */
              rdy += ty;

            } /* if (do_gradmap) */

          if (do_vecmap)
            {  /* add in fixed vector scaled by  vec. map data */
              tx = (gdouble) crv[col*vbytes];       /* use first byte only */
              rdx += scale_vec_x * tx;
              rdy += scale_vec_y * tx;
            } /* if (do_vecmap) */

          if (do_magmap)
            {  /* multiply result by mag. map data */
              tx = (gdouble) crm[col*mbytes];
              rdx = (rdx * tx)/(255.0);
              rdy = (rdy * tx)/(255.0);
            } /* if do_magmap */

          dvalx = rdx + (2<<14);         /* take zero point to be 2^15, since this is two bytes */
          dvaly = rdy + (2<<14);

          if (dvalx < 0)
            dvalx = 0;

          if (dvalx > 65535)
            dvalx = 65535;

          *dx++ = (guchar) (dvalx >> 8);    /* store high order byte in value channel */
          *dx++ = (guchar) (dvalx % 256);   /* store low order byte in alpha channel */
          dx += dest_bytes_inc;       /* move data pointer on to next destination pixel */

          if (dvaly < 0)
            dvaly = 0;

          if (dvaly > 65535)
            dvaly = 65535;

          *dy++ = (guchar) (dvaly >> 8);
          *dy++ = (guchar) (dvaly % 256);
          dy += dest_bytes_inc;

        } /* ------------------------------- for (col...) ----------------  */

      /*  store the dest  */
      gegl_buffer_set (destx_buffer,
                       GEGL_RECTANGLE (x1, row, (x2 - x1), 1), 0,
                       destx_format, destx,
                       GEGL_AUTO_ROWSTRIDE);

      gegl_buffer_set (desty_buffer,
                       GEGL_RECTANGLE (x1, row, (x2 - x1), 1), 0,
                       desty_format, desty,
                       GEGL_AUTO_ROWSTRIDE);

      /*  swap around the pointers to row buffers  */
      tmp = pr;
      pr = cr;
      cr = nr;
      nr = tmp;

      if (do_gradmap)
        {
          tmp = prg;
          prg = crg;
          crg = nrg;
          nrg = tmp;
        }

      if ((row % 8) == 0)
        gimp_progress_update ((gdouble) row / (gdouble) (y2 - y1));

    } /* for (row..) */

  gimp_progress_update (1.0);

  g_object_unref (src_buffer);
  g_object_unref (destx_buffer);
  g_object_unref (desty_buffer);

  g_clear_object (&mag_map);
  g_clear_object (&grad_map);
  g_clear_object (&vector_map);

  gimp_drawable_update (draw_xd, x1, y1, (x2 - x1), (y2 - y1));
  gimp_drawable_update (draw_yd, x1, y1, (x2 - x1), (y2 - y1));

  gimp_displays_flush ();  /* make sure layer is visible */

  gimp_progress_init (_("Smoothing X gradient"));
  blur16 (draw_xd);

  gimp_progress_init (_("Smoothing Y gradient"));
  blur16 (draw_yd);

  g_free (prev_row);  /* row buffers allocated at top of fn. */
  g_free (cur_row);
  g_free (next_row);
  g_free (prev_row_g);  /* row buffers allocated at top of fn. */
  g_free (cur_row_g);
  g_free (next_row_g);
  g_free (cur_row_v);
  g_free (cur_row_m);

  g_free (destx);
  g_free (desty);

  *xl = GIMP_DRAWABLE (xlayer);  /* pass back the X and Y layer ID numbers */
  *yl = GIMP_DRAWABLE (ylayer);
}

/* -------------------------------------------------------------------------- */
/*            The Warp displacement is done here.                             */
/* -------------------------------------------------------------------------- */

static void
warp (GimpDrawable        *orig_draw,
      GimpProcedureConfig *config)
{
  GimpDrawable *disp_map;    /* Displacement map, ie, control array */
  GimpDrawable *mag_draw;    /* Magnitude multiplier factor map */
  GimpDrawable *map_x = NULL;
  GimpDrawable *map_y = NULL;
  gboolean      first_time = TRUE;
  gint          iter_count;
  gint          width;
  gint          height;
  gint          x1, y1, x2, y2;
  GimpImage    *image;

  /* index var. over all "warp" Displacement iterations */
  gint          warp_iter;

  /* calculate new X,Y Displacement image maps */

  gimp_progress_init (_("Finding XY gradient"));

  /* Get selection area */
  if (! gimp_drawable_mask_intersect (orig_draw,
                                      &x1, &y1, &width, &height))
    return;

  g_object_get (config,
                "warp-map", &disp_map,
                "mag-map",  &mag_draw,
                "iter",     &iter_count,
                NULL);

  x2 = x1 + width;
  y2 = y1 + height;

  width  = gimp_drawable_get_width  (orig_draw);
  height = gimp_drawable_get_height (orig_draw);

  /* generate x,y differential images (arrays) */
  diff (disp_map, config, &map_x, &map_y);

  for (warp_iter = 0; warp_iter < iter_count; warp_iter++)
    {
      gimp_progress_init_printf (_("Flow step %d"), warp_iter+1);
      progress = 0;

      warp_one (orig_draw, orig_draw,
                map_x, map_y, mag_draw,
                first_time, warp_iter, config);

      gimp_drawable_update (orig_draw,
                            x1, y1, (x2 - x1), (y2 - y1));

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      first_time = FALSE;
    }

  image = gimp_item_get_image (GIMP_ITEM (map_x));

  gimp_image_delete (image);
  g_clear_object (&disp_map);
  g_clear_object (&mag_draw);
}

/* -------------------------------------------------------------------------- */

static void
warp_one (GimpDrawable        *draw,
          GimpDrawable        *new,
          GimpDrawable        *map_x,
          GimpDrawable        *map_y,
          GimpDrawable        *mag_draw,
          gboolean             first_time,
          gint                 step,
          GimpProcedureConfig *config)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  GeglBuffer *map_x_buffer;
  GeglBuffer *map_y_buffer;
  GeglBuffer *mag_buffer = NULL;

  GeglBufferIterator *iter;

  gint        width;
  gint        height;

  const Babl *src_format;
  gint        src_bytes;
  const Babl *dest_format;
  gint        dest_bytes;

  guchar  pixel[4][4];
  gint    x1, y1, x2, y2;
  gint    x, y;
  gint    max_progress;

  gdouble needx, needy;
  gdouble xval=0;      /* initialize to quiet compiler grumbles */
  gdouble yval=0;      /* interpolated vector displacement */
  gdouble scalefac;        /* multiplier for vector displacement scaling */
  gdouble dscalefac;       /* multiplier for incremental displacement vectors */
  gint    xi, yi;
  gint    substep;         /* loop variable counting displacement vector substeps */

  guchar  values[4];
  guint32 ivalues[4];
  guchar  val;

  gint k;

  gdouble dx, dy;           /* X and Y Displacement, integer from GRAY map */

  const Babl *map_x_format;
  gint        map_x_bytes;
  const Babl *map_y_format;
  gint        map_y_bytes;
  const Babl *mag_format;
  gint        mag_bytes = 1;
  gboolean    mag_alpha = FALSE;

  GRand      *gr;

  gdouble     amount;
  gdouble     dither;
  gboolean    use_mag;
  gint        substeps;

  gr = g_rand_new (); /* Seed Pseudo Random Number Generator */

  /* ================ Outer Loop calculation ================================ */

  /* Get selection area */

  if (! gimp_drawable_mask_intersect (draw,
                                      &x1, &y1, &width, &height))
    return;

  g_object_get (config,
                "amount",   &amount,
                "dither",   &dither,
                "mag-use",  &use_mag,
                "substeps", &substeps,
                NULL);

  x2 = x1 + width;
  y2 = y1 + height;

  width  = gimp_drawable_get_width  (draw);
  height = gimp_drawable_get_height (draw);


  max_progress = (x2 - x1) * (y2 - y1);


  /*  --------- Register the (many) pixel regions ----------  */

  src_buffer = gimp_drawable_get_buffer (draw);

  src_format = get_u8_format (draw);
  src_bytes  = babl_format_get_bytes_per_pixel (src_format);

  iter = gegl_buffer_iterator_new (src_buffer,
                                   GEGL_RECTANGLE (x1, y1, (x2 - x1), (y2 - y1)),
                                   0, src_format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 5);


  dest_buffer = gimp_drawable_get_shadow_buffer (new);

  dest_format = get_u8_format (new);
  dest_bytes  = babl_format_get_bytes_per_pixel (dest_format);

  gegl_buffer_iterator_add (iter, dest_buffer,
                            GEGL_RECTANGLE (x1, y1, (x2 - x1), (y2 - y1)),
                            0, dest_format,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);


  map_x_buffer = gimp_drawable_get_buffer (map_x);

  map_x_format = get_u8_format (map_x);
  map_x_bytes  = babl_format_get_bytes_per_pixel (map_x_format);

  gegl_buffer_iterator_add (iter, map_x_buffer,
                            GEGL_RECTANGLE (x1, y1, (x2 - x1), (y2 - y1)),
                            0, map_x_format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);


  map_y_buffer = gimp_drawable_get_buffer (map_y);

  map_y_format = get_u8_format (map_y);
  map_y_bytes  = babl_format_get_bytes_per_pixel (map_y_format);

  gegl_buffer_iterator_add (iter, map_y_buffer,
                            GEGL_RECTANGLE (x1, y1, (x2 - x1), (y2 - y1)),
                            0, map_y_format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);


  if (use_mag)
    {
      mag_buffer = gimp_drawable_get_buffer (mag_draw);

      mag_format = get_u8_format (mag_draw);
      mag_bytes  = babl_format_get_bytes_per_pixel (mag_format);

      mag_alpha = gimp_drawable_has_alpha (mag_draw);

      gegl_buffer_iterator_add (iter, mag_buffer,
                                GEGL_RECTANGLE (x1, y1, (x2 - x1), (y2 - y1)),
                                0, mag_format,
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
    }

  /* substep displacement vector scale factor */
  dscalefac = amount / (256 * 127.5 * substeps);

  while (gegl_buffer_iterator_next (iter))
    {
      GeglRectangle  roi     = iter->items[1].roi;
      guchar        *srcrow  = iter->items[0].data;
      guchar        *destrow = iter->items[1].data;
      guchar        *mxrow   = iter->items[2].data;
      guchar        *myrow   = iter->items[3].data;
      guchar        *mmagrow = NULL;

      if (use_mag)
        mmagrow = iter->items[4].data;

      /* loop over destination pixels */
      for (y = roi.y; y < (roi.y + roi.height); y++)
        {
          guchar *dest = destrow;
          guchar *mx   = mxrow;
          guchar *my   = myrow;
          guchar *mmag = NULL;

          if (use_mag == TRUE)
            mmag = mmagrow;

          for (x = roi.x; x < (roi.x + roi.width); x++)
            {
              /* ----- Find displacement vector (amnt_x, amnt_y) ------------ */

              dx = dscalefac * ((256.0 * mx[0]) + mx[1] -32768);  /* 16-bit values */
              dy = dscalefac * ((256.0 * my[0]) + my[1] -32768);

              if (use_mag)
                {
                  scalefac = warp_map_mag_give_value (mmag,
                                                      mag_alpha,
                                                      mag_bytes) / 255.0;
                  dx *= scalefac;
                  dy *= scalefac;
                }

              if (dither != 0.0)
                {       /* random dither is +/- dither pixels */
                  dx += g_rand_double_range (gr, -dither, dither);
                  dy += g_rand_double_range (gr, -dither, dither);
                }

              if (substeps != 1)
                {   /* trace (substeps) iterations of displacement vector */
                  for (substep = 1; substep < substeps; substep++)
                    {
                      /* In this (substep) loop, (x,y) remain fixed. (dx,dy) vary each step. */
                      needx = x + dx;
                      needy = y + dy;

                      if (needx >= 0.0)
                        xi = (gint) needx;
                      else
                        xi = -((gint) -needx + 1);

                      if (needy >= 0.0)
                        yi = (gint) needy;
                      else
                        yi = -((gint) -needy + 1);

                      /* get 4 neighboring DX values from DiffX drawable for linear interpolation */
                      warp_pixel (map_x_buffer, map_x_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi, yi,
                                  pixel[0], config);
                      warp_pixel (map_x_buffer, map_x_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi + 1, yi,
                                  pixel[1], config);
                      warp_pixel (map_x_buffer, map_x_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi, yi + 1,
                                  pixel[2], config);
                      warp_pixel (map_x_buffer, map_x_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi + 1, yi + 1,
                                  pixel[3], config);

                      ivalues[0] = 256 * pixel[0][0] + pixel[0][1];
                      ivalues[1] = 256 * pixel[1][0] + pixel[1][1];
                      ivalues[2] = 256 * pixel[2][0] + pixel[2][1];
                      ivalues[3] = 256 * pixel[3][0] + pixel[3][1];

                      xval = gimp_bilinear_32 (needx, needy, ivalues);

                      /* get 4 neighboring DY values from DiffY drawable for linear interpolation */
                      warp_pixel (map_y_buffer, map_y_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi, yi,
                                  pixel[0], config);
                      warp_pixel (map_y_buffer, map_y_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi + 1, yi,
                                  pixel[1], config);
                      warp_pixel (map_y_buffer, map_y_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi, yi + 1,
                                  pixel[2], config);
                      warp_pixel (map_y_buffer, map_y_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi + 1, yi + 1,
                                  pixel[3], config);

                      ivalues[0] = 256 * pixel[0][0] + pixel[0][1];
                      ivalues[1] = 256 * pixel[1][0] + pixel[1][1];
                      ivalues[2] = 256 * pixel[2][0] + pixel[2][1];
                      ivalues[3] = 256 * pixel[3][0] + pixel[3][1];

                      yval = gimp_bilinear_32 (needx, needy, ivalues);

                      /* move displacement vector to this new value */
                      dx += dscalefac * (xval - 32768);
                      dy += dscalefac * (yval - 32768);

                    } /* for (substep) */
                } /* if (substeps != 0) */

              /* --------------------------------------------------------- */

              needx = x + dx;
              needy = y + dy;

              mx += map_x_bytes;         /* pointers into x,y displacement maps */
              my += map_y_bytes;

              if (use_mag == TRUE)
                mmag += mag_bytes;

              /* Calculations complete; now copy the proper pixel */

              if (needx >= 0.0)
                xi = (gint) needx;
              else
                xi = -((gint) -needx + 1);

              if (needy >= 0.0)
                yi = (gint) needy;
              else
                yi = -((gint) -needy + 1);

              /* get 4 neighboring pixel values from source drawable
               * for linear interpolation
               */
              warp_pixel (src_buffer, src_format,
                          width, height,
                          x1, y1, x2, y2,
                          xi, yi,
                          pixel[0], config);
              warp_pixel (src_buffer, src_format,
                          width, height,
                          x1, y1, x2, y2,
                          xi + 1, yi,
                          pixel[1], config);
              warp_pixel (src_buffer, src_format,
                          width, height,
                          x1, y1, x2, y2,
                          xi, yi + 1,
                          pixel[2], config);
              warp_pixel (src_buffer, src_format,
                          width, height,
                          x1, y1, x2, y2,
                          xi + 1, yi + 1,
                          pixel[3], config);

              for (k = 0; k < dest_bytes; k++)
                {
                  values[0] = pixel[0][k];
                  values[1] = pixel[1][k];
                  values[2] = pixel[2][k];
                  values[3] = pixel[3][k];

                  val = gimp_bilinear_8 (needx, needy, values);

                  *dest++ = val;
                }
            }

          /*      srcrow += src_rgn.rowstride; */
          srcrow  += src_bytes   * roi.width;
          destrow += dest_bytes  * roi.width;
          mxrow   += map_x_bytes * roi.width;
          myrow   += map_y_bytes * roi.width;

          if (use_mag == TRUE)
            mmagrow += mag_bytes * roi.width;
        }

      progress += (roi.width * roi.height);
      gimp_progress_update ((double) progress / (double) max_progress);
    }

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);
  g_object_unref (map_x_buffer);
  g_object_unref (map_y_buffer);

  if (use_mag == TRUE)
    g_object_unref (mag_buffer);

  gimp_progress_update (1.0);

  gimp_drawable_merge_shadow (draw, first_time);

  g_rand_free (gr);
}

/* ------------------------------------------------------------------------- */

static gdouble
warp_map_mag_give_value (guchar *pt,
                         gint    alpha,
                         gint    bytes)
{
  gdouble ret, val_alpha;

  if (bytes >= 3)
    ret =  (pt[0] + pt[1] + pt[2])/3.0;
  else
    ret = (gdouble) *pt;

  if (alpha)
    {
      val_alpha = pt[bytes - 1];
      ret = (ret * val_alpha / 255.0);
    };

  return (ret);
}


static void
warp_pixel (GeglBuffer          *buffer,
            const Babl          *format,
            gint                 width,
            gint                 height,
            gint                 x1,
            gint                 y1,
            gint                 x2,
            gint                 y2,
            gint                 x,
            gint                 y,
            guchar              *pixel,
            GimpProcedureConfig *config)
{
  static guchar  empty_pixel[4] = { 0, 0, 0, 0 };
  guchar        *data;
  gint           wrap_type;

  wrap_type = gimp_procedure_config_get_choice_id (config, "wrap-type");

  /* Tile the image. */
  if (wrap_type == WRAP)
    {
      if (x < 0)
        x = width - (-x % width);
      else
        x %= width;

      if (y < 0)
        y = height - (-y % height);
      else
        y %= height;
    }
  /* Smear out the edges of the image by repeating pixels. */
  else if (wrap_type == SMEAR)
    {
      if (x < 0)
        x = 0;
      else if (x > width - 1)
        x = width - 1;

      if (y < 0)
        y = 0;
      else if (y > height - 1)
        y = height - 1;
    }

  if (x >= x1 && y >= y1 && x < x2 && y < y2)
    {
      gegl_buffer_sample (buffer, x, y, NULL, pixel, format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
    }
  else
    {
      gint bpp = babl_format_get_bytes_per_pixel (format);
      gint b;

      if (wrap_type == BLACK)
        data = empty_pixel;
      else
        data = color_pixel;      /* must have selected COLOR type */

      for (b = 0; b < bpp; b++)
        pixel[b] = data[b];
    }
}
