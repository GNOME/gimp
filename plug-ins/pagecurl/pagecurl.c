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

#include "pagecurl-icons.c"


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


/***** Types *****/

typedef struct
{
  CurlColors       colors;
  gdouble          opacity;
  gboolean         shade;
  CurlEdge         edge;
  CurlOrientation  orientation;
} CurlParams;


/***** Prototypes *****/

static void      query                 (void);
static void      run                   (const gchar      *name,
                                        gint              nparams,
                                        const GimpParam  *param,
                                        gint             *nreturn_vals,
                                        GimpParam       **return_vals);
static void       set_default_params   (void);

static void       dialog_scale_update  (GtkAdjustment    *adjustment,
                                        gdouble          *value);

static gboolean   dialog               (void);

static void       init_calculation     (gint32            drawable_id);
static gint32     do_curl_effect       (gint32            drawable_id);
static void       clear_curled_region  (gint32            drawable_id);
static gint32     page_curl            (gint32            drawable_id);
static GimpRGB  * get_gradient_samples (gint32            drawable_id,
                                        gboolean          reverse);


/***** Variables *****/

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

static CurlParams curl;

/* Image parameters */

static gint32        image_id;

static GtkWidget *curl_image = NULL;

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

static GimpRGB fg_color;
static GimpRGB bg_color;


/***** Functions *****/

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",    "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",       "Input image"                          },
    { GIMP_PDB_DRAWABLE, "drawable",    "Input drawable"                       },
    { GIMP_PDB_INT32,    "colors",      "FG- and BG-Color (0), Current gradient (1), Current gradient reversed (2)" },
    { GIMP_PDB_INT32,    "edge",
        "Edge to curl (1-4, clockwise, starting in the lower right edge)"   },
    { GIMP_PDB_INT32,    "orientation", "Vertical (0), Horizontal (1)"         },
    { GIMP_PDB_INT32,    "shade",
        "Shade the region under the curl (1) or not (0)"                    },
  };

  static const GimpParamDef return_vals[] =
  {
    { GIMP_PDB_LAYER, "curl-layer", "The new layer with the curl." }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Curl up one of the image corners"),
                          "This plug-in creates a pagecurl-effect.",
                          "Federico Mena Quintero and Simon Budig",
                          "Federico Mena Quintero and Simon Budig",
                          PLUG_IN_VERSION,
                          N_("_Pagecurl..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args),
                          G_N_ELEMENTS (return_vals),
                          args,
                          return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Distorts");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            drawable_id;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  set_default_params ();

  /*  Possibly retrieve data  */
  gimp_get_data (PLUG_IN_PROC, &curl);

  *nreturn_vals = 2;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  values[1].type = GIMP_PDB_LAYER;
  values[1].data.d_layer = -1;

  /*  Get the specified drawable  */
  drawable_id = param[2].data.d_drawable;
  image_id = param[1].data.d_image;

  if ((gimp_drawable_is_rgb (drawable_id) ||
       gimp_drawable_is_gray (drawable_id)) &&
      gimp_drawable_mask_intersect (drawable_id, &sel_x, &sel_y,
                                    &true_sel_width, &true_sel_height))
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /*  First acquire information with a dialog  */
          if (! dialog ())
            return;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams != 7)
            status = GIMP_PDB_CALLING_ERROR;

          if (status == GIMP_PDB_SUCCESS)
            {
              curl.colors      = CLAMP (param[3].data.d_int32,
                                        0, CURL_COLORS_LAST);
              curl.edge        = CLAMP (param[4].data.d_int32,
                                        CURL_EDGE_FIRST, CURL_EDGE_LAST);
              curl.orientation = CLAMP (param[5].data.d_int32,
                                        0, CURL_ORIENTATION_LAST);
              curl.shade       = param[6].data.d_int32 ? TRUE : FALSE;
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          values[1].data.d_layer = page_curl (drawable_id);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &curl, sizeof (CurlParams));
        }
    }
  else
    /* Sorry - no indexed/noalpha images */
    status = GIMP_PDB_EXECUTION_ERROR;

  values[0].data.d_status = status;
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

static void
set_default_params (void)
{
  curl.colors      = CURL_COLORS_FG_BG;
  curl.opacity     = 1.0;
  curl.shade       = TRUE;
  curl.edge        = CURL_EDGE_LOWER_RIGHT;
  curl.orientation = CURL_ORIENTATION_VERTICAL;
}

/********************************************************************/
/* dialog callbacks                                                 */
/********************************************************************/

static void
dialog_scale_update (GtkAdjustment *adjustment,
                     gdouble       *value)
{
  *value = ((gdouble) gtk_adjustment_get_value (adjustment)) / 100.0;
}

static void
curl_pixbuf_update (void)
{
  GdkPixbuf *pixbuf;
  gint       index;
  gchar     *resource;

  switch (curl.edge)
    {
    case CURL_EDGE_LOWER_RIGHT: index = 0; break;
    case CURL_EDGE_LOWER_LEFT:  index = 1; break;
    case CURL_EDGE_UPPER_RIGHT: index = 2; break;
    case CURL_EDGE_UPPER_LEFT:  index = 3; break;
    default:
      return;
    }

  index += curl.orientation * 4;

  resource = g_strdup_printf ("/org/gimp/pagecurl-icons/curl%c.png",
                              '0' + index);
  pixbuf = gdk_pixbuf_new_from_resource (resource, NULL);
  g_free (resource);

  gtk_image_set_from_pixbuf (GTK_IMAGE (curl_image), pixbuf);
  g_object_unref (pixbuf);
}

static gboolean
dialog (void)
{
  /* Missing options: Color-dialogs? / own curl layer ? / transparency
     to original drawable / Warp-curl (unsupported yet) */

  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *combo;
  GtkObject *adjustment;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Pagecurl Effect"), PLUG_IN_ROLE,
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

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gimp_frame_new (_("Curl Location"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  table = gtk_table_new (3, 2, TRUE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);

  curl_image = gtk_image_new ();

  gtk_table_attach (GTK_TABLE (table), curl_image, 0, 2, 1, 2,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (curl_image);

  curl_pixbuf_update ();

  {
    static const gchar *name[] =
    {
      N_("Lower right"),
      N_("Lower left"),
      N_("Upper left"),
      N_("Upper right")
    };
    gint i;

    button = NULL;
    for (i = CURL_EDGE_FIRST; i <= CURL_EDGE_LAST; i++)
      {
        button =
          gtk_radio_button_new_with_label (button == NULL ?
                                           NULL :
                                           gtk_radio_button_get_group (GTK_RADIO_BUTTON (button)),
                                           gettext (name[i - CURL_EDGE_FIRST]));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                      curl.edge == i);

        g_object_set_data (G_OBJECT (button),
                           "gimp-item-data", GINT_TO_POINTER (i));

        gtk_table_attach (GTK_TABLE (table), button,
                          CURL_EDGE_LEFT  (i) ? 0 : 1,
                          CURL_EDGE_LEFT  (i) ? 1 : 2,
                          CURL_EDGE_UPPER (i) ? 0 : 2,
                          CURL_EDGE_UPPER (i) ? 1 : 3,
                          GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
        gtk_widget_show (button);

        g_signal_connect (button, "toggled",
                          G_CALLBACK (gimp_radio_button_update),
                          &curl.edge);

        g_signal_connect (button, "toggled",
                          G_CALLBACK (curl_pixbuf_update),
                           NULL);
      }
  }

  gtk_widget_show (table);
  gtk_widget_show (frame);

  frame = gimp_frame_new (_("Curl Orientation"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  {
    static const gchar *name[] =
    {
      N_("_Vertical"),
      N_("_Horizontal")
    };
    gint i;

    button = NULL;
    for (i = 0; i <= CURL_ORIENTATION_LAST; i++)
      {
        button = gtk_radio_button_new_with_mnemonic (button == NULL ?
                                                     NULL :
                                                     gtk_radio_button_get_group (GTK_RADIO_BUTTON (button)),
                                                     gettext (name[i]));

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                      curl.orientation == i);

        g_object_set_data (G_OBJECT (button),
                           "gimp-item-data", GINT_TO_POINTER (i));

        gtk_box_pack_end (GTK_BOX (hbox), button, TRUE, TRUE, 0);
        gtk_widget_show (button);

        g_signal_connect (button, "toggled",
                          G_CALLBACK (gimp_radio_button_update),
                          &curl.orientation);

        g_signal_connect (button, "toggled",
                          G_CALLBACK (curl_pixbuf_update),
                          NULL);
      }
  }

  gtk_widget_show (hbox);
  gtk_widget_show (frame);

  button = gtk_check_button_new_with_mnemonic (_("_Shade under curl"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), curl.shade);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &curl.shade);

  combo = g_object_new (GIMP_TYPE_INT_COMBO_BOX, NULL);

  gimp_int_combo_box_prepend (GIMP_INT_COMBO_BOX (combo),
                              GIMP_INT_STORE_VALUE,     CURL_COLORS_GRADIENT_REVERSE,
                              GIMP_INT_STORE_LABEL,     _("Current gradient (reversed)"),
                              GIMP_INT_STORE_ICON_NAME, GIMP_ICON_GRADIENT,
                              -1);
  gimp_int_combo_box_prepend (GIMP_INT_COMBO_BOX (combo),
                              GIMP_INT_STORE_VALUE,     CURL_COLORS_GRADIENT,
                              GIMP_INT_STORE_LABEL,     _("Current gradient"),
                              GIMP_INT_STORE_ICON_NAME, GIMP_ICON_GRADIENT,
                              -1);
  gimp_int_combo_box_prepend (GIMP_INT_COMBO_BOX (combo),
                              GIMP_INT_STORE_VALUE,     CURL_COLORS_FG_BG,
                              GIMP_INT_STORE_LABEL,     _("Foreground / background colors"),
                              GIMP_INT_STORE_ICON_NAME, GIMP_ICON_COLORS_DEFAULT,
                              -1);

  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              curl.colors,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &curl.colors);

  gtk_widget_show (dialog);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adjustment = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                     _("_Opacity:"), 100, 0,
                                     curl.opacity * 100.0, 0.0, 100.0,
                                     1.0, 1.0, 0.0,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (dialog_scale_update),
                    &curl.opacity);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
init_calculation (gint32 drawable_id)
{
  gdouble      k;
  gdouble      alpha, beta;
  gdouble      angle;
  GimpVector2  v1, v2;
  gint32      *image_layers;
  gint32       nlayers;

  gimp_layer_add_alpha (drawable_id);

  /* Image parameters */

  /* Determine Position of original Layer in the Layer stack. */

  image_layers = gimp_image_get_layers (image_id, &nlayers);
  drawable_position = 0;
  while (drawable_position < nlayers &&
         image_layers[drawable_position] != drawable_id)
    drawable_position++;

  switch (curl.orientation)
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

  gimp_context_get_foreground (&fg_color);
  gimp_context_get_background (&bg_color);
}

static gint32
do_curl_effect (gint32 drawable_id)
{
  gint          x = 0;
  gint          y = 0;
  gboolean      color_image;
  gint          x1, y1;
  guint         progress, max_progress;
  gdouble       intensity, alpha;
  GimpVector2   v, dl, dr;
  gdouble       dl_mag, dr_mag, angle, factor;
  GeglBuffer   *curl_buffer;
  gint32        curl_layer_id;
  GimpRGB      *grad_samples = NULL;
  gint          width, height, n_ch;
  GeglRectangle *roi;
  GeglBufferIterator *iter;
  const Babl         *format;

  color_image = gimp_drawable_is_rgb (drawable_id);

  curl_layer_id = gimp_layer_new (image_id,
                                  _("Curl Layer"),
                                  true_sel_width,
                                  true_sel_height,
                                  color_image ?
                                  GIMP_RGBA_IMAGE : GIMP_GRAYA_IMAGE,
                                  100,
                                  gimp_image_get_default_new_layer_mode (image_id));

  gimp_image_insert_layer (image_id, curl_layer_id,
                           gimp_item_get_parent (drawable_id),
                           drawable_position);
  gimp_drawable_fill (curl_layer_id, GIMP_FILL_TRANSPARENT);

  gimp_drawable_offsets (drawable_id, &x1, &y1);
  gimp_layer_set_offsets (curl_layer_id, sel_x + x1, sel_y + y1);

  curl_buffer = gimp_drawable_get_shadow_buffer (curl_layer_id);

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
  switch (curl.colors)
    {
    case CURL_COLORS_FG_BG:
      break;
    case CURL_COLORS_GRADIENT:
      grad_samples = get_gradient_samples (curl_layer_id, FALSE);
      break;
    case CURL_COLORS_GRADIENT_REVERSE:
      grad_samples = get_gradient_samples (curl_layer_id, TRUE);
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
          GimpRGB color;

          for (x1 = roi->x; x1 < roi->x + roi->width; x1++)
            {
              /* Map coordinates to get the curl correct... */
              switch (curl.orientation)
                {
                case CURL_ORIENTATION_VERTICAL:
                  x = CURL_EDGE_RIGHT (curl.edge) ? x1 : sel_width  - 1 - x1;
                  y = CURL_EDGE_UPPER (curl.edge) ? y1 : sel_height - 1 - y1;
                  break;

                case CURL_ORIENTATION_HORIZONTAL:
                  x = CURL_EDGE_LOWER (curl.edge) ? y1 : sel_width  - 1 - y1;
                  y = CURL_EDGE_LEFT  (curl.edge) ? x1 : sel_height - 1 - x1;
                  break;
                }

              if (left_of_diagl (x, y))
                { /* uncurled region: transparent black */
                  gimp_rgba_set (&color, 0, 0, 0, 0);
                }
              else if (right_of_diagr (x, y) ||
                       (right_of_diagm (x, y) &&
                        below_diagb (x, y) &&
                        !inside_circle (x, y)))
                {
                  /* curled region: transparent black */
                  gimp_rgba_set (&color, 0, 0, 0, 0);
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
                      gimp_rgba_set (&color, 0, 0, 0, curl.shade ? factor : 0);
                    }
                  else
                    {
                      GimpRGB *gradrgb;

                      /* On the curl */
                      switch (curl.colors)
                        {
                        case CURL_COLORS_FG_BG:
                          intensity = pow (sin (G_PI * angle / alpha), 1.5);
                          gimp_rgba_set (&color,
                                         intensity * bg_color.r + (1.0 - intensity) * fg_color.r,
                                         intensity * bg_color.g + (1.0 - intensity) * fg_color.g,
                                         intensity * bg_color.b + (1.0 - intensity) * fg_color.b,
                                         (1.0 - intensity * (1.0 - curl.opacity)));
                          break;

                        case CURL_COLORS_GRADIENT:
                        case CURL_COLORS_GRADIENT_REVERSE:
                          /* Calculate position in Gradient */
                          intensity = (angle/alpha) + sin (G_PI*2 * angle/alpha) * 0.075;

                          gradrgb = grad_samples + ((guint) (intensity * NGRADSAMPLES));

                          /* Check boundaries */
                          intensity = CLAMP (intensity, 0.0, 1.0);
                          color = *gradrgb;
                          color.a = gradrgb->a * (1.0 - intensity * (1.0 - curl.opacity));
                          break;
                        }
                    }
                }

              dest[0] = color.r;
              dest[1] = color.g;
              dest[2] = color.b;
              dest[3] = color.a;

              dest += n_ch;
            }
        }
      progress += roi->width * roi->height;
      gimp_progress_update ((double) progress / (double) max_progress);
    }

  gimp_progress_update (0.5);
  gegl_buffer_flush (curl_buffer);
  gimp_drawable_merge_shadow (curl_layer_id, FALSE);
  gimp_drawable_update (curl_layer_id, 0, 0, width, height);

  g_free (grad_samples);

  return curl_layer_id;
}

/************************************************/

static void
clear_curled_region (gint32 drawable_id)
{
  gint          x = 0;
  gint          y = 0;
  guint         x1, y1;
  gfloat       *dest, *src;
  guint         progress, max_progress;
  GeglBuffer   *buf;
  GeglBuffer   *shadow_buf;
  GeglRectangle *roi;
  GeglBufferIterator *iter;
  const Babl   *format;
  gint          width, height, bpp, n_ch;
  gint          buf_index;

  max_progress = 2 * sel_width * sel_height;
  progress = max_progress / 2;

  buf        = gimp_drawable_get_buffer (drawable_id);
  shadow_buf = gimp_drawable_get_shadow_buffer (drawable_id);
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
              switch (curl.orientation)
                {
                case CURL_ORIENTATION_VERTICAL:
                  x = (CURL_EDGE_RIGHT (curl.edge) ?
                       x1 - sel_x : sel_width  - 1 - (x1 - sel_x));
                  y = (CURL_EDGE_UPPER (curl.edge) ?
                       y1 - sel_y : sel_height - 1 - (y1 - sel_y));
                  break;

                case CURL_ORIENTATION_HORIZONTAL:
                  x = (CURL_EDGE_LOWER (curl.edge) ?
                       y1 - sel_y : sel_width - 1 - (y1 - sel_y));
                  y = (CURL_EDGE_LEFT (curl.edge)  ?
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
  gimp_drawable_merge_shadow (drawable_id, TRUE);
  gimp_drawable_update (drawable_id,
                        sel_x, sel_y,
                        true_sel_width, true_sel_height);
}

static gint32
page_curl (gint32 drawable_id)
{
  gint curl_layer_id;

  gimp_image_undo_group_start (image_id);

  gimp_progress_init (_("Page Curl"));

  init_calculation (drawable_id);

  curl_layer_id = do_curl_effect (drawable_id);

  clear_curled_region (drawable_id);

  gimp_image_undo_group_end (image_id);

  return curl_layer_id;
}

/*
  Returns NGRADSAMPLES samples of active gradient.
  Each sample has (gimp_drawable_bpp (drawable_id)) bytes.
  "ripped" from gradmap.c.
 */
static GimpRGB *
get_gradient_samples (gint32    drawable_id,
                      gboolean  reverse)
{
  gchar   *gradient_name;
  gint     n_d_samples;
  gdouble *d_samples = NULL;
  GimpRGB *rgba;
  gint     i;

  gradient_name = gimp_context_get_gradient ();

  gimp_gradient_get_uniform_samples (gradient_name, NGRADSAMPLES, reverse,
                                     &n_d_samples, &d_samples);

  rgba = g_new0 (GimpRGB, NGRADSAMPLES);
  for (i = 0; i < NGRADSAMPLES; i++)
    {
      gimp_rgba_set (rgba + i,
                     d_samples[i*4 + 0],
                     d_samples[i*4 + 1],
                     d_samples[i*4 + 2],
                     d_samples[i*4 + 3]);
    }

  g_free (gradient_name);

  return rgba;
}
