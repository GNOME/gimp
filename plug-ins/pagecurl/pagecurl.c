/* Page Curl 0.9 --- image filter plug-in for GIMP
 * Copyright (C) 1996 Federico Mena Quintero
 * Ported to Gimp 1.0 1998 by Simon Budig <Simon.Budig@unix-ag.org>
 *
 * You can contact me at quartic@polloux.fciencias.unam.mx
 * You can contact the original GIMP authors at gimp@xcf.berkeley.edu
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
 */

/*
 * Ported to the 0.99.x architecture by Simon Budig, Simon.Budig@unix-ag.org
 * Ported to the 3.0 plug-in API by Michael Natterer <mitch@gimp.org>
 */

/*
 * Version History
 * 0.5: (1996) Version for Gimp 0.54 by Federico Mena Quintero
 * 0.6: (Feb '98) First Version for Gimp 0.99.x, very buggy.
 * 0.8: (Mar '98) First "stable" version
 * 0.9: (May '98)
 *      - Added support for Gradients. It is now possible to map
 *        a gradient to the back of the curl.
 *      - This implies a changed PDB-Interface: New "mode" parameter.
 *      - Pagecurl now returns the ID of the new layer.
 *      - Exchanged the meaning of FG/BG Color, because mostly the FG
 *        color is darker.
 * 1.0: (July '04)
 *      - Code cleanup, added reverse gradient option.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-pagecurl"
#define PLUG_IN_BINARY  "pagecurl"
#define PLUG_IN_ROLE    "gimp-pagecurl"
#define PLUG_IN_VERSION "July 2004, 1.0"
#define NGRADSAMPLES    256


typedef enum
{
  CURL_COLORS_FG_BG,
  CURL_COLORS_GRADIENT,
  CURL_COLORS_GRADIENT_REVERSE,
  CURL_COLORS_LAST = CURL_COLORS_GRADIENT_REVERSE
} CurlColors;

typedef enum
{
  CURL_ORIENTATION_VERTICAL,
  CURL_ORIENTATION_HORIZONTAL,
  CURL_ORIENTATION_LAST = CURL_ORIENTATION_HORIZONTAL
} CurlOrientation;

typedef enum
{
  CURL_EDGE_LOWER_RIGHT = 1,
  CURL_EDGE_LOWER_LEFT  = 2,
  CURL_EDGE_UPPER_LEFT  = 3,
  CURL_EDGE_UPPER_RIGHT = 4,
  CURL_EDGE_FIRST       = CURL_EDGE_LOWER_RIGHT,
  CURL_EDGE_LAST        = CURL_EDGE_UPPER_RIGHT
} CurlEdge;


#define CURL_EDGE_LEFT(e)  ((e) == CURL_EDGE_LOWER_LEFT  || \
                            (e) == CURL_EDGE_UPPER_LEFT)
#define CURL_EDGE_RIGHT(e) ((e) == CURL_EDGE_LOWER_RIGHT || \
                            (e) == CURL_EDGE_UPPER_RIGHT)
#define CURL_EDGE_LOWER(e) ((e) == CURL_EDGE_LOWER_RIGHT || \
                            (e) == CURL_EDGE_LOWER_LEFT)
#define CURL_EDGE_UPPER(e) ((e) == CURL_EDGE_UPPER_LEFT  || \
                            (e) == CURL_EDGE_UPPER_RIGHT)


typedef struct _Pagecurl      Pagecurl;
typedef struct _PagecurlClass PagecurlClass;

struct _Pagecurl
{
  GimpPlugIn parent_instance;
};

struct _PagecurlClass
{
  GimpPlugInClass parent_class;
};


#define PAGECURL_TYPE  (pagecurl_get_type ())
#define PAGECURL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAGECURL_TYPE, Pagecurl))

GType                   pagecurl_get_type         (void) G_GNUC_CONST;

static GList          * pagecurl_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * pagecurl_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * pagecurl_run              (GimpProcedure        *procedure,
                                                   GimpRunMode           run_mode,
                                                   GimpImage            *image,
                                                   GimpDrawable        **drawables,
                                                   GimpProcedureConfig  *config,
                                                   gpointer              run_data);

static gboolean         dialog                    (GimpProcedure        *procedure,
                                                   GimpProcedureConfig  *config);

static void             init_calculation          (GimpDrawable         *drawable,
                                                   GimpProcedureConfig  *config);
static GimpLayer      * do_curl_effect            (GimpDrawable         *drawable,
                                                   GimpProcedureConfig  *config);
static void             clear_curled_region       (GimpDrawable         *drawable,
                                                   GimpProcedureConfig  *config);
static GimpLayer      * page_curl                 (GimpDrawable         *drawable,
                                                   GimpProcedureConfig  *config);
static gdouble        * get_gradient_samples      (GimpDrawable         *drawable,
                                                   gboolean              reverse);


G_DEFINE_TYPE (Pagecurl, pagecurl, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PAGECURL_TYPE)
DEFINE_STD_SET_I18N


/* Image parameters */

static GimpImage *image      = NULL;

static gint   sel_x, sel_y;
static gint   true_sel_width, true_sel_height;
static gint   sel_width, sel_height;
static gint   drawable_position;

/* Center and radius of circle */

static GimpVector2 center;
static double      radius;

/* Useful points to keep around */

static GimpVector2 left_tangent;
static GimpVector2 right_tangent;

/* Slopes --- these are *not* in the usual geometric sense! */

static gdouble diagl_slope;
static gdouble diagr_slope;
static gdouble diagb_slope;
static gdouble diagm_slope;

/* User-configured parameters */

static gdouble fg_color[4];
static gdouble bg_color[4];


static void
pagecurl_class_init (PagecurlClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pagecurl_query_procedures;
  plug_in_class->create_procedure = pagecurl_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
pagecurl_init (Pagecurl *pagecurl)
{
}

static GList *
pagecurl_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
pagecurl_create_procedure (GimpPlugIn  *plug_in,
                           const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            pagecurl_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Pagecurl..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Distorts");

      gimp_procedure_set_documentation (procedure,
                                        _("Curl up one of the image corners"),
                                        "This plug-in creates a pagecurl-effect.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Federico Mena Quintero and Simon Budig",
                                      "Federico Mena Quintero and Simon Budig",
                                      PLUG_IN_VERSION);

      gimp_procedure_add_choice_argument (procedure, "colors",
                                          _("Colors"), NULL,
                                          gimp_choice_new_with_values ("fg-bg",                     CURL_COLORS_FG_BG,            _("Foreground / background colors"), NULL,
                                                                       "current-gradient",          CURL_COLORS_GRADIENT,         _("Current gradient"),               NULL,
                                                                       "current-gradient-reversed", CURL_COLORS_GRADIENT_REVERSE, _("Current gradient (reversed)"),    NULL,
                                                                       NULL),
                                          "fg-bg",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "edge",
                                          _("Locatio_n"),
                                          _("Corner which is curled"),
                                          gimp_choice_new_with_values ("upper-left",  CURL_EDGE_UPPER_LEFT,  _("Upper left"),  NULL,
                                                                       "upper-right", CURL_EDGE_UPPER_RIGHT, _("Upper right"), NULL,
                                                                       "lower-left",  CURL_EDGE_LOWER_LEFT,  _("Lower left"),  NULL,
                                                                       "lower-right", CURL_EDGE_LOWER_RIGHT, _("Lower right"), NULL,
                                                                       NULL),
                                          "lower-right",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "orientation",
                                          _("Or_ientation"), NULL,
                                          gimp_choice_new_with_values ("vertical",   CURL_ORIENTATION_VERTICAL,   _("Vertical"),   NULL,
                                                                       "horizontal", CURL_ORIENTATION_HORIZONTAL, _("Horizontal"), NULL,
                                                                       NULL),
                                          "vertical",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "shade",
                                           _("Sh_ade"),
                                           _("Shade the region under the curl"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "opacity",
                                          _("Opaci_ty"),
                                          _("Opacity"),
                                          0.0, 1.0, 0.0,
                                          GIMP_PARAM_READWRITE);

      gimp_procedure_add_layer_return_value (procedure, "curl-layer",
                                         "Curl layer",
                                         "The new layer with the curl.",
                                         FALSE,
                                         G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
pagecurl_run (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *_image,
              GimpDrawable        **drawables,
              GimpProcedureConfig  *config,
              gpointer              run_data)
{
  GimpDrawable   *drawable;
  GimpValueArray *return_vals = NULL;

  gegl_init (NULL, NULL);

  image = _image;

  if (gimp_core_object_array_get_length ((GObject **) drawables) != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   gimp_procedure_get_name (procedure));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  if (! gimp_drawable_mask_intersect (drawable, &sel_x, &sel_y,
                                      &true_sel_width, &true_sel_height))
    {
      return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS,
                                               NULL);
    }

  if (! (gimp_drawable_is_rgb (drawable) ||
         gimp_drawable_is_gray (drawable)))
    {
      /* Sorry - no indexed/noalpha images */
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  if (run_mode == GIMP_RUN_INTERACTIVE && ! dialog (procedure, config))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  return_vals = gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_LAYER (return_vals, 1, page_curl (drawable, config));

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  return return_vals;
}

/*****************************************************
 * Functions to locate the current point in the curl.
 *  The following functions assume an curl in the
 *  lower right corner.
 *  diagb crosses the two tangential points from the
 *  circle with diagl and diagr.
 *
 *   +------------------------------------------------+
 *   |                                          __-- /|
 *   |                                      __--   _/ |
 *   |                                  __--      /   |
 *   |                              __--        _/    |
 *   |                          __--           /      |
 *   |                      __--____         _/       |
 *   |                  __----~~    \      _/         |
 *   |              __--             |   _/           |
 *   |    diagl __--               _/| _/diagr        |
 *   |      __--            diagm_/  |/               |
 *   |  __--                    /___/                 |
 *   +-------------------------+----------------------+
 */

static inline gboolean
left_of_diagl (gdouble x,
               gdouble y)
{
  return (x < (sel_width + (y - sel_height) * diagl_slope));
}

static inline gboolean
right_of_diagr (gdouble x,
                gdouble y)
{
  return (x > (sel_width + (y - sel_height) * diagr_slope));
}

static inline gboolean
below_diagb (gdouble x,
             gdouble y)
{
  return (y < (right_tangent.y + (x - right_tangent.x) * diagb_slope));
}

static inline gboolean
right_of_diagm (gdouble x,
                gdouble y)
{
  return (x > (sel_width + (y - sel_height) * diagm_slope));
}

static inline gboolean
inside_circle (gdouble x,
               gdouble y)
{
  x -= center.x;
  y -= center.y;

  return x * x + y * y <= radius * radius;
}


/********************************************************************/
/* dialog callbacks                                                 */
/********************************************************************/

static void
curl_pixbuf_update (GimpProcedureConfig *config,
                    const GParamSpec    *pspec,
                    GtkWidget           *curl_image)
{
  GdkPixbuf       *pixbuf;
  gint             index;
  gchar           *resource;
  CurlEdge         edge;
  CurlOrientation  orientation;

  edge = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config), "edge");
  switch (edge)
    {
    case CURL_EDGE_LOWER_RIGHT: index = 0; break;
    case CURL_EDGE_LOWER_LEFT:  index = 1; break;
    case CURL_EDGE_UPPER_RIGHT: index = 2; break;
    case CURL_EDGE_UPPER_LEFT:  index = 3; break;
    default:
      return;
    }

  orientation = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config), "orientation");
  index += orientation * 4;

  resource = g_strdup_printf ("/org/gimp/pagecurl-icons/curl%c.png",
                              '0' + index);
  pixbuf = gdk_pixbuf_new_from_resource (resource, NULL);
  g_free (resource);

  gtk_image_set_from_pixbuf (GTK_IMAGE (curl_image), pixbuf);
  g_object_unref (pixbuf);
}

static gboolean
dialog (GimpProcedure       *procedure,
        GimpProcedureConfig *config)
{
  /* Missing options: Color-dialogs? / own curl layer ? / transparency
     to original drawable / Warp-curl (unsupported yet) */
  GtkWidget *dialog;
  GtkWidget *curl_image;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Pagecurl Effect"));

  gimp_procedure_dialog_get_spin_scale (GIMP_PROCEDURE_DIALOG (dialog), "opacity", 100.0);
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "edge", "orientation", "shade", "colors", "opacity", NULL);

  curl_image = gtk_image_new ();

  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      curl_image, FALSE, FALSE, 0);
  gtk_widget_show (curl_image);

  g_signal_connect (config, "notify",
                    G_CALLBACK (curl_pixbuf_update),
                    curl_image);
  curl_pixbuf_update (config, NULL, curl_image);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static void
init_calculation (GimpDrawable        *drawable,
                  GimpProcedureConfig *config)
{
  GeglColor       *color;
  gdouble          k;
  gdouble          alpha, beta;
  gdouble          angle;
  GimpVector2      v1, v2;
  GList           *layers;
  CurlOrientation  orientation;

  orientation = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config), "orientation");

  gimp_layer_add_alpha (GIMP_LAYER (drawable));

  /* Image parameters */

  /* Determine Position of original Layer in the Layer stack. */
  layers = gimp_image_list_layers (image);
  drawable_position = g_list_index (layers, drawable);
  g_list_free (layers);

  switch (orientation)
    {
    case CURL_ORIENTATION_VERTICAL:
      sel_width  = true_sel_width;
      sel_height = true_sel_height;
      break;

    case CURL_ORIENTATION_HORIZONTAL:
      sel_width  = true_sel_height;
      sel_height = true_sel_width;
      break;
    }

  /* Circle parameters */

  alpha = atan ((double) sel_height / sel_width);
  beta = alpha / 2.0;
  k = sel_width / ((G_PI + alpha) * sin (beta) + cos (beta));
  gimp_vector2_set (&center, k * cos (beta), k * sin (beta));
  radius = center.y;

  /* left_tangent  */

  gimp_vector2_set (&left_tangent, radius * -sin (alpha), radius * cos (alpha));
  gimp_vector2_add (&left_tangent, &left_tangent, &center);

  /* right_tangent */

  gimp_vector2_sub (&v1, &left_tangent, &center);
  gimp_vector2_set (&v2, sel_width - center.x, sel_height - center.y);
  angle = -2.0 * acos (gimp_vector2_inner_product (&v1, &v2) /
                       (gimp_vector2_length (&v1) *
                        gimp_vector2_length (&v2)));
  gimp_vector2_set (&right_tangent,
                    v1.x * cos (angle) + v1.y * -sin (angle),
                    v1.x * sin (angle) + v1.y * cos (angle));
  gimp_vector2_add (&right_tangent, &right_tangent, &center);

  /* Slopes */

  diagl_slope = (double) sel_width / sel_height;
  diagr_slope = (sel_width - right_tangent.x) / (sel_height - right_tangent.y);
  diagb_slope = ((right_tangent.y - left_tangent.y) /
                 (right_tangent.x - left_tangent.x));
  diagm_slope = (sel_width - center.x) / sel_height;

  /* Colors */

  color = gimp_context_get_foreground ();
  gegl_color_get_pixel (color, babl_format ("R'G'B'A double"), fg_color);
  g_object_unref (color);
  color = gimp_context_get_background ();
  gegl_color_get_pixel (color, babl_format ("R'G'B'A double"), bg_color);
  g_object_unref (color);
}

static GimpLayer *
do_curl_effect (GimpDrawable        *drawable,
                GimpProcedureConfig *config)
{
  gint                x = 0;
  gint                y = 0;
  gboolean            color_image;
  gint                x1, y1;
  guint               progress, max_progress;
  gdouble             intensity, alpha;
  GimpVector2         v, dl, dr;
  gdouble             dl_mag, dr_mag, angle, factor;
  GeglBuffer         *curl_buffer;
  GimpLayer          *curl_layer;
  gdouble            *grad_samples = NULL;
  gint                width, height, n_ch;
  GeglRectangle      *roi;
  GeglBufferIterator *iter;
  const Babl         *format;
  CurlOrientation     orientation;
  CurlColors          colors;
  CurlEdge            edge;
  gboolean            shade;
  gdouble             opacity;

  orientation = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config), "orientation");
  colors      = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config), "colors");
  edge        = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config), "edge");

  g_object_get (config,
                "shade",   &shade,
                "opacity", &opacity,
                NULL);

  color_image = gimp_drawable_is_rgb (drawable);

  curl_layer = gimp_layer_new (image,
                               _("Curl Layer"),
                               true_sel_width,
                               true_sel_height,
                               color_image ?
                               GIMP_RGBA_IMAGE : GIMP_GRAYA_IMAGE,
                               100,
                               gimp_image_get_default_new_layer_mode (image));

  gimp_image_insert_layer (image, curl_layer,
                           GIMP_LAYER (gimp_item_get_parent (GIMP_ITEM (drawable))),
                           drawable_position);
  gimp_drawable_fill (GIMP_DRAWABLE (curl_layer), GIMP_FILL_TRANSPARENT);

  gimp_drawable_get_offsets (drawable, &x1, &y1);
  gimp_layer_set_offsets (curl_layer, sel_x + x1, sel_y + y1);

  curl_buffer = gimp_drawable_get_shadow_buffer (GIMP_DRAWABLE (curl_layer));

  width  = gegl_buffer_get_width (curl_buffer);
  height = gegl_buffer_get_height (curl_buffer);
  format = babl_format ("R'G'B'A float");
  n_ch   = babl_format_get_n_components (format);

  iter = gegl_buffer_iterator_new (curl_buffer,
                                   GEGL_RECTANGLE (0, 0, width, height), 0,
                                   format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);

  /* Init shade_under */
  gimp_vector2_set (&dl, -sel_width, -sel_height);
  dl_mag = gimp_vector2_length (&dl);
  gimp_vector2_set (&dr,
                    -(sel_width - right_tangent.x),
                    -(sel_height - right_tangent.y));
  dr_mag = gimp_vector2_length (&dr);
  alpha = acos (gimp_vector2_inner_product (&dl, &dr) / (dl_mag * dr_mag));

  /* Gradient Samples */
  switch (colors)
    {
    case CURL_COLORS_FG_BG:
      break;
    case CURL_COLORS_GRADIENT:
      grad_samples = get_gradient_samples (GIMP_DRAWABLE (curl_layer), FALSE);
      break;
    case CURL_COLORS_GRADIENT_REVERSE:
      grad_samples = get_gradient_samples (GIMP_DRAWABLE (curl_layer), TRUE);
      break;
    }

  max_progress = 2 * sel_width * sel_height;
  progress = 0;

  /* Main loop */
  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *dest;

      roi = &iter->items[0].roi;
      dest = (gfloat *) iter->items[0].data;

      for (y1 = roi->y; y1 < roi->y + roi->height; y1++)
        {
          gdouble color[4] = { 0, 0, 0, 0 };

          for (x1 = roi->x; x1 < roi->x + roi->width; x1++)
            {
              /* Map coordinates to get the curl correct... */
              switch (orientation)
                {
                case CURL_ORIENTATION_VERTICAL:
                  x = CURL_EDGE_RIGHT (edge) ? x1 : sel_width  - 1 - x1;
                  y = CURL_EDGE_UPPER (edge) ? y1 : sel_height - 1 - y1;
                  break;

                case CURL_ORIENTATION_HORIZONTAL:
                  x = CURL_EDGE_LOWER (edge) ? y1 : sel_width  - 1 - y1;
                  y = CURL_EDGE_LEFT  (edge) ? x1 : sel_height - 1 - x1;
                  break;
                }

              if (left_of_diagl (x, y))
                { /* uncurled region: transparent black */
                  for (gint i = 0; i < 4; i++)
                    color[i] = 0;
                }
              else if (right_of_diagr (x, y) ||
                       (right_of_diagm (x, y) &&
                        below_diagb (x, y) &&
                        !inside_circle (x, y)))
                {
                  /* curled region: transparent black */
                  for (gint i = 0; i < 4; i++)
                    color[i] = 0;
                }
              else
                {
                  v.x = -(sel_width - x);
                  v.y = -(sel_height - y);
                  angle = acos (gimp_vector2_inner_product (&v, &dl) /
                                (gimp_vector2_length (&v) * dl_mag));

                  if (inside_circle (x, y) || below_diagb (x, y))
                    {
                      /* Below the curl. */
                      factor = angle / alpha;

                      for (gint i = 0; i < 3; i++)
                        color[i] = 0;
                      color[3] = shade ? factor : 0;
                    }
                  else
                    {
                      guint rgb_index;

                      /* On the curl */
                      switch (colors)
                        {
                        case CURL_COLORS_FG_BG:
                          intensity = pow (sin (G_PI * angle / alpha), 1.5);

                          for (gint i = 0; i < 3; i++)
                            color[i] = intensity * bg_color[i] + (1.0 - intensity) * fg_color[i];
                          color[3] = 1.0 - intensity * (1.0 - opacity);
                          break;

                        case CURL_COLORS_GRADIENT:
                        case CURL_COLORS_GRADIENT_REVERSE:
                          /* Calculate position in Gradient */
                          intensity = (angle/alpha) + sin (G_PI*2 * angle/alpha) * 0.075;

                          rgb_index = ((guint) (intensity * NGRADSAMPLES));

                          /* Check boundaries */
                          intensity = CLAMP (intensity, 0.0, 1.0);
                          for (gint i = 0; i < 4; i++)
                            color[i] = grad_samples[(rgb_index * 4) + i];
                          color[3] = grad_samples[(rgb_index * 4) + 3] * (1.0 - intensity * (1.0 - opacity));
                          break;
                        }
                    }
                }

              for (gint i = 0; i < 4; i++)
                dest[i] = color[i];

              dest += n_ch;
            }
        }
      progress += roi->width * roi->height;
      gimp_progress_update ((double) progress / (double) max_progress);
    }

  gimp_progress_update (0.5);
  gegl_buffer_flush (curl_buffer);
  gimp_drawable_merge_shadow (GIMP_DRAWABLE (curl_layer), FALSE);
  gimp_drawable_update (GIMP_DRAWABLE (curl_layer), 0, 0, width, height);

  g_free (grad_samples);

  return curl_layer;
}

/************************************************/

static void
clear_curled_region (GimpDrawable        *drawable,
                     GimpProcedureConfig *config)
{
  gint                x = 0;
  gint                y = 0;
  guint               x1, y1;
  gfloat             *dest, *src;
  guint               progress, max_progress;
  GeglBuffer         *buf;
  GeglBuffer         *shadow_buf;
  GeglRectangle      *roi;
  GeglBufferIterator *iter;
  const Babl         *format;
  gint                width, height, bpp, n_ch;
  gint                buf_index;
  CurlOrientation     orientation;
  CurlEdge            edge;

  orientation = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config), "orientation");
  edge        = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config), "edge");

  max_progress = 2 * sel_width * sel_height;
  progress = max_progress / 2;

  buf        = gimp_drawable_get_buffer (drawable);
  shadow_buf = gimp_drawable_get_shadow_buffer (drawable);
  width  = gegl_buffer_get_width (buf);
  height = gegl_buffer_get_height (buf);

  format = babl_format ("R'G'B'A float");
  bpp    = babl_format_get_bytes_per_pixel (format);
  n_ch   = babl_format_get_n_components (format);

  iter = gegl_buffer_iterator_new (shadow_buf,
                                   GEGL_RECTANGLE (0, 0, width, height), 0,
                                   format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 2);
  buf_index = gegl_buffer_iterator_add (iter, buf, NULL, 0,
                                        format,
                                        GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      roi  = &iter->items[0].roi;
      dest = iter->items[0].data;
      src  = iter->items[buf_index].data;

      memcpy (dest, src, roi->width * roi->height * bpp);

      for (y1 = roi->y; y1 < roi->y + roi->height; y1++)
        {
          for (x1 = roi->x; x1 < roi->x + roi->width; x1++)
            {
              /* Map coordinates to get the curl correct... */
              switch (orientation)
                {
                case CURL_ORIENTATION_VERTICAL:
                  x = (CURL_EDGE_RIGHT (edge) ?
                       x1 - sel_x : sel_width  - 1 - (x1 - sel_x));
                  y = (CURL_EDGE_UPPER (edge) ?
                       y1 - sel_y : sel_height - 1 - (y1 - sel_y));
                  break;

                case CURL_ORIENTATION_HORIZONTAL:
                  x = (CURL_EDGE_LOWER (edge) ?
                       y1 - sel_y : sel_width - 1 - (y1 - sel_y));
                  y = (CURL_EDGE_LEFT (edge)  ?
                       x1 - sel_x : sel_height - 1 - (x1 - sel_x));
                  break;
                }

              if (right_of_diagr (x, y) ||
                  (right_of_diagm (x, y) &&
                   below_diagb (x, y) &&
                   !inside_circle (x, y)))
                {
                  /* Right of the curl: Alpha = 0 */
                  dest[0] = src[0];
                  dest[1] = src[1];
                  dest[2] = src[2];
                  dest[3] = 0.0;
                }

              dest += n_ch;
              src += n_ch;
            }
        }

      progress += roi->width * roi->height;
      gimp_progress_update ((double) progress / (double) max_progress);
    }

  gimp_progress_update (1.0);
  gegl_buffer_flush (shadow_buf);
  gimp_drawable_merge_shadow (drawable, TRUE);
  gimp_drawable_update (drawable,
                        sel_x, sel_y,
                        true_sel_width, true_sel_height);
}

static GimpLayer *
page_curl (GimpDrawable        *drawable,
           GimpProcedureConfig *config)
{
  GimpLayer *curl_layer;

  gimp_image_undo_group_start (image);

  gimp_progress_init (_("Page Curl"));

  init_calculation (drawable, config);

  curl_layer = do_curl_effect (drawable, config);

  clear_curled_region (drawable, config);

  gimp_image_undo_group_end (image);

  return curl_layer;
}

/*
  Returns NGRADSAMPLES samples of active gradient.
  Each sample has (gimp_drawable_get_bpp (drawable)) bytes.
  "ripped" from gradmap.c.
 */
static gdouble *
get_gradient_samples (GimpDrawable *drawable,
                      gboolean      reverse)
{
  GimpGradient  *gradient;
  GeglColor    **colors;
  const Babl    *format = babl_format ("R'G'B'A double");
  gdouble       *d_samples;

  gradient = gimp_context_get_gradient ();

  colors = gimp_gradient_get_uniform_samples (gradient, NGRADSAMPLES, reverse);

  d_samples = g_new0 (gdouble, NGRADSAMPLES * 4);
  for (gint i = 0; i < NGRADSAMPLES; i++)
    gegl_color_get_pixel (colors[i], format, &d_samples[i * 4]);

  gimp_color_array_free (colors);

  return d_samples;
}
