/* Page Curl 0.9 --- image filter plug-in for LIGMA
 * Copyright (C) 1996 Federico Mena Quintero
 * Ported to Ligma 1.0 1998 by Simon Budig <Simon.Budig@unix-ag.org>
 *
 * You can contact me at quartic@polloux.fciencias.unam.mx
 * You can contact the original LIGMA authors at ligma@xcf.berkeley.edu
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
 * Ported to the 3.0 plug-in API by Michael Natterer <mitch@ligma.org>
 */

/*
 * Version History
 * 0.5: (1996) Version for Ligma 0.54 by Federico Mena Quintero
 * 0.6: (Feb '98) First Version for Ligma 0.99.x, very buggy.
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-pagecurl"
#define PLUG_IN_BINARY  "pagecurl"
#define PLUG_IN_ROLE    "ligma-pagecurl"
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

typedef struct _Pagecurl      Pagecurl;
typedef struct _PagecurlClass PagecurlClass;

struct _Pagecurl
{
  LigmaPlugIn parent_instance;
};

struct _PagecurlClass
{
  LigmaPlugInClass parent_class;
};


#define PAGECURL_TYPE  (pagecurl_get_type ())
#define PAGECURL (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAGECURL_TYPE, Pagecurl))

GType                   pagecurl_get_type         (void) G_GNUC_CONST;

static GList          * pagecurl_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * pagecurl_create_procedure (LigmaPlugIn           *plug_in,
                                                   const gchar          *name);

static LigmaValueArray * pagecurl_run              (LigmaProcedure        *procedure,
                                                   LigmaRunMode           run_mode,
                                                   LigmaImage            *image,
                                                   gint                  n_drawables,
                                                   LigmaDrawable        **drawables,
                                                   const LigmaValueArray *args,
                                                   gpointer              run_data);

static void             dialog_scale_update       (LigmaLabelSpin        *spin,
                                                   gdouble              *value);

static gboolean         dialog                    (void);

static void             init_calculation          (LigmaDrawable     *drawable);
static LigmaLayer      * do_curl_effect            (LigmaDrawable     *drawable);
static void             clear_curled_region       (LigmaDrawable     *drawable);
static LigmaLayer      * page_curl                 (LigmaDrawable     *drawable);
static LigmaRGB        * get_gradient_samples      (LigmaDrawable     *drawable,
                                                   gboolean          reverse);


G_DEFINE_TYPE (Pagecurl, pagecurl, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (PAGECURL_TYPE)
DEFINE_STD_SET_I18N


/***** Variables *****/

static CurlParams curl;

/* Image parameters */

static LigmaImage *image      = NULL;

static GtkWidget *curl_image = NULL;

static gint   sel_x, sel_y;
static gint   true_sel_width, true_sel_height;
static gint   sel_width, sel_height;
static gint   drawable_position;

/* Center and radius of circle */

static LigmaVector2 center;
static double      radius;

/* Useful points to keep around */

static LigmaVector2 left_tangent;
static LigmaVector2 right_tangent;

/* Slopes --- these are *not* in the usual geometric sense! */

static gdouble diagl_slope;
static gdouble diagr_slope;
static gdouble diagb_slope;
static gdouble diagm_slope;

/* User-configured parameters */

static LigmaRGB fg_color;
static LigmaRGB bg_color;


static void
pagecurl_class_init (PagecurlClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pagecurl_query_procedures;
  plug_in_class->create_procedure = pagecurl_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
pagecurl_init (Pagecurl *pagecurl)
{
}

static GList *
pagecurl_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
pagecurl_create_procedure (LigmaPlugIn  *plug_in,
                           const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            pagecurl_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "RGB*, GRAY*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure, _("_Pagecurl..."));
      ligma_procedure_add_menu_path (procedure, "<Image>/Filters/Distorts");

      ligma_procedure_set_documentation (procedure,
                                        _("Curl up one of the image corners"),
                                        "This plug-in creates a pagecurl-effect.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Federico Mena Quintero and Simon Budig",
                                      "Federico Mena Quintero and Simon Budig",
                                      PLUG_IN_VERSION);

      LIGMA_PROC_ARG_INT (procedure, "colors",
                         "Colors",
                         "FG- and BG-Color (0), Current gradient (1), "
                         "Current gradient reversed (2)",
                         CURL_COLORS_FG_BG, CURL_COLORS_LAST, CURL_COLORS_FG_BG,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "edge",
                         "Edge",
                         "Edge to curl (1-4, clockwise, starting in the "
                         "lower right edge)",
                         CURL_EDGE_LOWER_RIGHT, CURL_EDGE_UPPER_RIGHT,
                         CURL_EDGE_LOWER_RIGHT,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "orientation",
                         "Orientation",
                         "Vertical (0), Horizontal (1)",
                         CURL_ORIENTATION_VERTICAL, CURL_ORIENTATION_HORIZONTAL,
                         CURL_ORIENTATION_VERTICAL,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "shade",
                             "Shade",
                             "Shade the region under the curl",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_VAL_LAYER (procedure, "curl-layer",
                           "Curl layer",
                           "The new layer with the curl.",
                           FALSE,
                           G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
pagecurl_run (LigmaProcedure        *procedure,
              LigmaRunMode           run_mode,
              LigmaImage            *_image,
              gint                  n_drawables,
              LigmaDrawable        **drawables,
              const LigmaValueArray *args,
              gpointer              run_data)
{
  LigmaDrawable   *drawable;
  LigmaValueArray *return_vals = NULL;

  gegl_init (NULL, NULL);

  image = _image;

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, LIGMA_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   ligma_procedure_get_name (procedure));

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  curl.colors      = LIGMA_VALUES_GET_INT     (args, 0);
  curl.edge        = LIGMA_VALUES_GET_INT     (args, 1);
  curl.orientation = LIGMA_VALUES_GET_INT     (args, 2);
  curl.shade       = LIGMA_VALUES_GET_BOOLEAN (args, 3);

  if (! ligma_drawable_mask_intersect (drawable, &sel_x, &sel_y,
                                      &true_sel_width, &true_sel_height))
    {
      return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS,
                                               NULL);
    }

  if (! (ligma_drawable_is_rgb (drawable) ||
         ligma_drawable_is_gray (drawable)))
    {
      /* Sorry - no indexed/noalpha images */
      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      ligma_get_data (PLUG_IN_PROC, &curl);

      /*  First acquire information with a dialog  */
      if (! dialog ())
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    case LIGMA_RUN_NONINTERACTIVE:
      break;

    case LIGMA_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      ligma_get_data (PLUG_IN_PROC, &curl);
      break;

    default:
      break;
    }

  return_vals = ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_LAYER (return_vals, 1, page_curl (drawable));

  if (run_mode != LIGMA_RUN_NONINTERACTIVE)
    ligma_displays_flush ();

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    ligma_set_data (PLUG_IN_PROC, &curl, sizeof (CurlParams));

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
dialog_scale_update (LigmaLabelSpin *scale,
                     gdouble       *value)
{
  *value = ((gdouble) ligma_label_spin_get_value (scale)) / 100.0;
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

  resource = g_strdup_printf ("/org/ligma/pagecurl-icons/curl%c.png",
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
  GtkWidget *grid;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *combo;
  GtkWidget *scale;
  gboolean   run;

  ligma_ui_init (PLUG_IN_BINARY);

  dialog = ligma_dialog_new (_("Pagecurl Effect"), PLUG_IN_ROLE,
                            NULL, 0,
                            ligma_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  ligma_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = ligma_frame_new ("LibLigma Widget Testing Ground");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  button = ligma_brush_select_button_new ("Brush Test", NULL,
                                         1.0, 20, LIGMA_LAYER_MODE_NORMAL);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            "Brush Select:", 0.0, 0.5,
                            button, 1);

  button = ligma_gradient_select_button_new ("Gradient Test", NULL);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            "Gradient Select:", 0.0, 0.5,
                            button, 1);

  button = ligma_palette_select_button_new ("Palette Test", NULL);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            "Palette Select:", 0.0, 0.5,
                            button, 1);

  button = ligma_font_select_button_new ("Font Test", NULL);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            "Font Select:", 0.0, 0.5,
                            button, 1);

  button = ligma_pattern_select_button_new ("Pattern Test", NULL);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 4,
                            "Pattern Select:", 0.0, 0.5,
                            button, 1);

  frame = ligma_frame_new (_("Curl Location"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  curl_image = gtk_image_new ();

  gtk_grid_attach (GTK_GRID (grid), curl_image, 0, 1, 2, 1);
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
                           "ligma-item-data", GINT_TO_POINTER (i));

        gtk_grid_attach (GTK_GRID (grid), button,
                         CURL_EDGE_LEFT  (i) ? 0 : 1,
                         CURL_EDGE_UPPER (i) ? 0 : 2,
                         1, 1);
                         // GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
        gtk_widget_show (button);

        g_signal_connect (button, "toggled",
                          G_CALLBACK (ligma_radio_button_update),
                          &curl.edge);

        g_signal_connect (button, "toggled",
                          G_CALLBACK (curl_pixbuf_update),
                           NULL);
      }
  }

  frame = ligma_frame_new (_("Curl Orientation"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

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
                           "ligma-item-data", GINT_TO_POINTER (i));

        gtk_box_pack_end (GTK_BOX (hbox), button, TRUE, TRUE, 0);
        gtk_widget_show (button);

        g_signal_connect (button, "toggled",
                          G_CALLBACK (ligma_radio_button_update),
                          &curl.orientation);

        g_signal_connect (button, "toggled",
                          G_CALLBACK (curl_pixbuf_update),
                          NULL);
      }
  }

  button = gtk_check_button_new_with_mnemonic (_("_Shade under curl"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), curl.shade);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &curl.shade);

  combo = g_object_new (LIGMA_TYPE_INT_COMBO_BOX, NULL);

  ligma_int_combo_box_prepend (LIGMA_INT_COMBO_BOX (combo),
                              LIGMA_INT_STORE_VALUE,     CURL_COLORS_GRADIENT_REVERSE,
                              LIGMA_INT_STORE_LABEL,     _("Current gradient (reversed)"),
                              LIGMA_INT_STORE_ICON_NAME, LIGMA_ICON_GRADIENT,
                              -1);
  ligma_int_combo_box_prepend (LIGMA_INT_COMBO_BOX (combo),
                              LIGMA_INT_STORE_VALUE,     CURL_COLORS_GRADIENT,
                              LIGMA_INT_STORE_LABEL,     _("Current gradient"),
                              LIGMA_INT_STORE_ICON_NAME, LIGMA_ICON_GRADIENT,
                              -1);
  ligma_int_combo_box_prepend (LIGMA_INT_COMBO_BOX (combo),
                              LIGMA_INT_STORE_VALUE,     CURL_COLORS_FG_BG,
                              LIGMA_INT_STORE_LABEL,     _("Foreground / background colors"),
                              LIGMA_INT_STORE_ICON_NAME, LIGMA_ICON_COLORS_DEFAULT,
                              -1);

  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                              curl.colors,
                              G_CALLBACK (ligma_int_combo_box_get_active),
                              &curl.colors, NULL);

  scale = ligma_scale_entry_new (_("_Opacity:"), curl.opacity * 100.0, 0.0, 100.0, 0.0);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (dialog_scale_update),
                    &curl.opacity);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 6);
  gtk_widget_show (scale);

  gtk_widget_show (dialog);

  run = (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
init_calculation (LigmaDrawable *drawable)
{
  gdouble      k;
  gdouble      alpha, beta;
  gdouble      angle;
  LigmaVector2  v1, v2;
  GList       *layers;

  ligma_layer_add_alpha (LIGMA_LAYER (drawable));

  /* Image parameters */

  /* Determine Position of original Layer in the Layer stack. */
  layers = ligma_image_list_layers (image);
  drawable_position = g_list_index (layers, drawable);
  g_list_free (layers);

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
  ligma_vector2_set (&center, k * cos (beta), k * sin (beta));
  radius = center.y;

  /* left_tangent  */

  ligma_vector2_set (&left_tangent, radius * -sin (alpha), radius * cos (alpha));
  ligma_vector2_add (&left_tangent, &left_tangent, &center);

  /* right_tangent */

  ligma_vector2_sub (&v1, &left_tangent, &center);
  ligma_vector2_set (&v2, sel_width - center.x, sel_height - center.y);
  angle = -2.0 * acos (ligma_vector2_inner_product (&v1, &v2) /
                       (ligma_vector2_length (&v1) *
                        ligma_vector2_length (&v2)));
  ligma_vector2_set (&right_tangent,
                    v1.x * cos (angle) + v1.y * -sin (angle),
                    v1.x * sin (angle) + v1.y * cos (angle));
  ligma_vector2_add (&right_tangent, &right_tangent, &center);

  /* Slopes */

  diagl_slope = (double) sel_width / sel_height;
  diagr_slope = (sel_width - right_tangent.x) / (sel_height - right_tangent.y);
  diagb_slope = ((right_tangent.y - left_tangent.y) /
                 (right_tangent.x - left_tangent.x));
  diagm_slope = (sel_width - center.x) / sel_height;

  /* Colors */

  ligma_context_get_foreground (&fg_color);
  ligma_context_get_background (&bg_color);
}

static LigmaLayer *
do_curl_effect (LigmaDrawable *drawable)
{
  gint          x = 0;
  gint          y = 0;
  gboolean      color_image;
  gint          x1, y1;
  guint         progress, max_progress;
  gdouble       intensity, alpha;
  LigmaVector2   v, dl, dr;
  gdouble       dl_mag, dr_mag, angle, factor;
  GeglBuffer   *curl_buffer;
  LigmaLayer    *curl_layer;
  LigmaRGB      *grad_samples = NULL;
  gint          width, height, n_ch;
  GeglRectangle *roi;
  GeglBufferIterator *iter;
  const Babl         *format;

  color_image = ligma_drawable_is_rgb (drawable);

  curl_layer = ligma_layer_new (image,
                               _("Curl Layer"),
                               true_sel_width,
                               true_sel_height,
                               color_image ?
                               LIGMA_RGBA_IMAGE : LIGMA_GRAYA_IMAGE,
                               100,
                               ligma_image_get_default_new_layer_mode (image));

  ligma_image_insert_layer (image, curl_layer,
                           LIGMA_LAYER (ligma_item_get_parent (LIGMA_ITEM (drawable))),
                           drawable_position);
  ligma_drawable_fill (LIGMA_DRAWABLE (curl_layer), LIGMA_FILL_TRANSPARENT);

  ligma_drawable_get_offsets (drawable, &x1, &y1);
  ligma_layer_set_offsets (curl_layer, sel_x + x1, sel_y + y1);

  curl_buffer = ligma_drawable_get_shadow_buffer (LIGMA_DRAWABLE (curl_layer));

  width  = gegl_buffer_get_width (curl_buffer);
  height = gegl_buffer_get_height (curl_buffer);
  format = babl_format ("R'G'B'A float");
  n_ch   = babl_format_get_n_components (format);

  iter = gegl_buffer_iterator_new (curl_buffer,
                                   GEGL_RECTANGLE (0, 0, width, height), 0,
                                   format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);

  /* Init shade_under */
  ligma_vector2_set (&dl, -sel_width, -sel_height);
  dl_mag = ligma_vector2_length (&dl);
  ligma_vector2_set (&dr,
                    -(sel_width - right_tangent.x),
                    -(sel_height - right_tangent.y));
  dr_mag = ligma_vector2_length (&dr);
  alpha = acos (ligma_vector2_inner_product (&dl, &dr) / (dl_mag * dr_mag));

  /* Gradient Samples */
  switch (curl.colors)
    {
    case CURL_COLORS_FG_BG:
      break;
    case CURL_COLORS_GRADIENT:
      grad_samples = get_gradient_samples (LIGMA_DRAWABLE (curl_layer), FALSE);
      break;
    case CURL_COLORS_GRADIENT_REVERSE:
      grad_samples = get_gradient_samples (LIGMA_DRAWABLE (curl_layer), TRUE);
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
          LigmaRGB color;

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
                  ligma_rgba_set (&color, 0, 0, 0, 0);
                }
              else if (right_of_diagr (x, y) ||
                       (right_of_diagm (x, y) &&
                        below_diagb (x, y) &&
                        !inside_circle (x, y)))
                {
                  /* curled region: transparent black */
                  ligma_rgba_set (&color, 0, 0, 0, 0);
                }
              else
                {
                  v.x = -(sel_width - x);
                  v.y = -(sel_height - y);
                  angle = acos (ligma_vector2_inner_product (&v, &dl) /
                                (ligma_vector2_length (&v) * dl_mag));

                  if (inside_circle (x, y) || below_diagb (x, y))
                    {
                      /* Below the curl. */
                      factor = angle / alpha;
                      ligma_rgba_set (&color, 0, 0, 0, curl.shade ? factor : 0);
                    }
                  else
                    {
                      LigmaRGB *gradrgb;

                      /* On the curl */
                      switch (curl.colors)
                        {
                        case CURL_COLORS_FG_BG:
                          intensity = pow (sin (G_PI * angle / alpha), 1.5);
                          ligma_rgba_set (&color,
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
      ligma_progress_update ((double) progress / (double) max_progress);
    }

  ligma_progress_update (0.5);
  gegl_buffer_flush (curl_buffer);
  ligma_drawable_merge_shadow (LIGMA_DRAWABLE (curl_layer), FALSE);
  ligma_drawable_update (LIGMA_DRAWABLE (curl_layer), 0, 0, width, height);

  g_free (grad_samples);

  return curl_layer;
}

/************************************************/

static void
clear_curled_region (LigmaDrawable *drawable)
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

  buf        = ligma_drawable_get_buffer (drawable);
  shadow_buf = ligma_drawable_get_shadow_buffer (drawable);
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
      ligma_progress_update ((double) progress / (double) max_progress);
    }

  ligma_progress_update (1.0);
  gegl_buffer_flush (shadow_buf);
  ligma_drawable_merge_shadow (drawable, TRUE);
  ligma_drawable_update (drawable,
                        sel_x, sel_y,
                        true_sel_width, true_sel_height);
}

static LigmaLayer *
page_curl (LigmaDrawable *drawable)
{
  LigmaLayer *curl_layer;

  ligma_image_undo_group_start (image);

  ligma_progress_init (_("Page Curl"));

  init_calculation (drawable);

  curl_layer = do_curl_effect (drawable);

  clear_curled_region (drawable);

  ligma_image_undo_group_end (image);

  return curl_layer;
}

/*
  Returns NGRADSAMPLES samples of active gradient.
  Each sample has (ligma_drawable_get_bpp (drawable)) bytes.
  "ripped" from gradmap.c.
 */
static LigmaRGB *
get_gradient_samples (LigmaDrawable *drawable,
                      gboolean      reverse)
{
  gchar   *gradient_name;
  gint     n_d_samples;
  gdouble *d_samples = NULL;
  LigmaRGB *rgba;
  gint     i;

  gradient_name = ligma_context_get_gradient ();

  ligma_gradient_get_uniform_samples (gradient_name, NGRADSAMPLES, reverse,
                                     &n_d_samples, &d_samples);

  rgba = g_new0 (LigmaRGB, NGRADSAMPLES);
  for (i = 0; i < NGRADSAMPLES; i++)
    {
      ligma_rgba_set (rgba + i,
                     d_samples[i*4 + 0],
                     d_samples[i*4 + 1],
                     d_samples[i*4 + 2],
                     d_samples[i*4 + 3]);
    }

  g_free (gradient_name);

  return rgba;
}
