/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Plugin to convert a selection to a path.
 *
 * Copyright (C) 1999 Andy Thomas  alt@gimp.org
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

/* Change log:-
 * 0.1 First version.
 */

#include "config.h"

#include <string.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "libgimpmath/gimpmath.h"

#include "global.h"
#include "types.h"
#include "pxl-outline.h"
#include "fit.h"
#include "spline.h"
#include "selection-to-path.h"

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-sel2path"
#define PLUG_IN_BINARY "selection-to-path"
#define PLUG_IN_ROLE   "gimp-selection-to-path"

#define RESPONSE_RESET 1
#define MID_POINT      127


typedef struct _Sel2path      Sel2path;
typedef struct _Sel2pathClass Sel2pathClass;

struct _Sel2path
{
  GimpPlugIn parent_instance;
};

struct _Sel2pathClass
{
  GimpPlugInClass parent_class;
};


#define SEL2PATH_TYPE  (sel2path_get_type ())
#define SEL2PATH (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SEL2PATH_TYPE, Sel2path))

GType                   sel2path_get_type         (void) G_GNUC_CONST;

static GList          * sel2path_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * sel2path_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * sel2path_run              (GimpProcedure        *procedure,
                                                   GimpRunMode           run_mode,
                                                   GimpImage            *image,
                                                   GimpDrawable        **drawables,
                                                   GimpProcedureConfig  *config,
                                                   gpointer              run_data);

static gint             sel2path_dialog           (GimpProcedure        *procedure,
                                                   GimpProcedureConfig  *config);
static gboolean         sel2path                  (GimpImage            *image);


G_DEFINE_TYPE (Sel2path, sel2path, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SEL2PATH_TYPE)
DEFINE_STD_SET_I18N


static gint         sel_x1, sel_y1, sel_x2, sel_y2;
static gint         has_sel, sel_width, sel_height;
static GeglSampler *sel_sampler;


static void
sel2path_class_init (Sel2pathClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = sel2path_query_procedures;
  plug_in_class->create_procedure = sel2path_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
sel2path_init (Sel2path *sel2path)
{
}

static GList *
sel2path_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
sel2path_create_procedure (GimpPlugIn  *plug_in,
                           const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            sel2path_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      gimp_procedure_set_documentation (procedure,
                                        _("Converts a selection to a path"),
                                        _("Converts a selection to a path"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Andy Thomas",
                                      "Andy Thomas",
                                      "1999");

      gimp_procedure_add_double_argument (procedure, "align-threshold",
                                          _("_Align Threshold"),
                                          _("If two endpoints are closer than this, "
                                            "they are made to be equal."),
                                          0.2, 2.0, 0.5,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "corner-always-threshold",
                                          _("Corner Al_ways Threshold"),
                                          _("If the angle defined by a point and its predecessors "
                                            "and successors is smaller than this, it's a corner, "
                                            "even if it's within 'corner_surround' pixels of a "
                                            "point with a smaller angle."),
                                          30, 180, 60.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "corner-surround",
                                       _("Corner _Surround"),
                                       _("Number of points to consider when determining if a "
                                         "point is a corner or not."),
                                       3, 8, 4,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "corner-threshold",
                                          _("Cor_ner Threshold"),
                                          _("If a point, its predecessors, and its successors "
                                            "define an angle smaller than this, it's a corner."),
                                          0, 180, 100.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "error-threshold",
                                          _("Error Thres_hold"),
                                          _("Amount of error at which a fitted spline is "
                                            "unacceptable. If any pixel is further away "
                                            "than this from the fitted curve, we try again."),
                                          0.2, 10, 0.4,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "filter-alternative-surround",
                                       _("_Filter Alternative Surround"),
                                       _("A second number of adjacent points to consider "
                                         "when filtering."),
                                       1, 10, 1,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "filter-epsilon",
                                          _("Filter E_psilon"),
                                          _("If the angles between the vectors produced by "
                                            "filter_surround and filter_alternative_surround "
                                            "points differ by more than this, use the one from "
                                            "filter_alternative_surround."),
                                          5, 40, 10.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "filter-iteration-count",
                                       _("Filter Iteration Co_unt"),
                                       _("Number of times to smooth original data points.  "
                                         "Increasing this number dramatically --- to 50 or "
                                         "so --- can produce vastly better results. But if "
                                         "any points that 'should' be corners aren't found, "
                                         "the curve goes to hell around that point."),
                                       4, 70, 4,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "filter-percent",
                                          _("Filt_er Percent"),
                                          _("To produce the new point, use the old point plus "
                                            "this times the neighbors."),
                                          0, 1, 0.33,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "filter-secondary-surround",
                                       _("Filter Secondar_y Surround"),
                                       _("Number of adjacent points to consider if "
                                         "'filter_surround' points defines a straight line."),
                                       3, 10, 3,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "filter-surround",
                                       _("Filter Surroun_d"),
                                       _("Number of adjacent points to consider when filtering."),
                                       2, 10, 2,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "keep-knees",
                                           _("_Keep Knees"),
                                           _("Says whether or not to remove 'knee' "
                                             "points after finding the outline."),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "line-reversion-threshold",
                                          _("_Line Reversion Threshold"),
                                          _("If a spline is closer to a straight line than this, "
                                            "it remains a straight line, even if it would otherwise "
                                            "be changed back to a curve. This is weighted by the "
                                            "square of the curve length, to make shorter curves "
                                            "more likely to be reverted."),
                                          0.01, 0.2, 0.01,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "line-threshold",
                                          _("L_ine Threshold"),
                                          _("How many pixels (on the average) a spline can "
                                            "diverge from the line determined by its endpoints "
                                            "before it is changed to a straight line."),
                                          0.2, 4, 0.5,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "reparametrize-improvement",
                                          _("Reparametri_ze Improvement"),
                                          _("If reparameterization doesn't improve the fit by this "
                                            "much percent, stop doing it. ""Amount of error at which "
                                            "it is pointless to reparameterize."),
                                          0, 1, 0.01,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "reparametrize-threshold",
                                          _("Repara_metrize Threshold"),
                                          _("Amount of error at which it is pointless to reparameterize.  "
                                            "This happens, for example, when we are trying to fit the "
                                            "outline of the outside of an 'O' with a single spline. "
                                            "The initial fit is not good enough for the Newton-Raphson "
                                            "iteration to improve it.  It may be that it would be better "
                                            "to detect the cases where we didn't find any corners."),
                                          1, 50, 1.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "subdivide-search",
                                          _("Subdi_vide Search"),
                                          _("Percentage of the curve away from the worst point "
                                           "to look for a better place to subdivide."),
                                          0.05, 1, 0.1,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "subdivide-surround",
                                       _("Su_bdivide Surround"),
                                       _("Number of points to consider when deciding whether "
                                         "a given point is a better place to subdivide."),
                                       2, 10, 4,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "subdivide-threshold",
                                          _("Subdivide Th_reshold"),
                                          _("How many pixels a point can diverge from a straight "
                                            "line and still be considered a better place to "
                                            "subdivide."),
                                          0.01, 1, 0.03,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "tangent-surround",
                                       _("_Tangent Surround"),
                                       _("Number of points to look at on either side of a "
                                         "point when computing the approximation to the "
                                         "tangent at that point."),
                                       2, 10, 3,
                                       G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
sel2path_run (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              GimpDrawable        **drawables,
              GimpProcedureConfig  *config,
              gpointer              run_data)
{
  gegl_init (NULL, NULL);

  if (gimp_selection_is_empty (image))
    {
      g_message (_("No selection to convert"));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  if (run_mode == GIMP_RUN_INTERACTIVE &&
      ! sel2path_dialog (procedure, config))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);

  fit_set_params (config);

  if (! sel2path (image))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             NULL);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

/* Build the dialog up. This was the hard part! */
static gint
sel2path_dialog (GimpProcedure       *procedure,
                 GimpProcedureConfig *config)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *scrolled_win;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Selection to Path Advanced Settings"));

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            RESPONSE_RESET,
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "align-threshold", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "corner-always-threshold", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "corner-surround", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "corner-threshold", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "error-threshold", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "filter-alternative-surround", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "filter-epsilon", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "filter-iteration-count", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "filter-percent", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "filter-secondary-surround", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "filter-surround", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "line-reversion-threshold", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "line-threshold", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "reparametrize-improvement", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "reparametrize-threshold", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "subdivide-search", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "subdivide-surround", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "subdivide-threshold", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "tangent-surround", 1.0);

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "selection-to-path-box",
                                         "align-threshold",
                                         "corner-always-threshold",
                                         "corner-surround",
                                         "corner-threshold",
                                         "error-threshold",
                                         "filter-alternative-surround",
                                         "filter-epsilon",
                                         "filter-iteration-count",
                                         "filter-percent",
                                         "filter-secondary-surround",
                                         "filter-surround",
                                         "keep-knees",
                                         "line-reversion-threshold",
                                         "line-threshold",
                                         "reparametrize-improvement",
                                         "reparametrize-threshold",
                                         "subdivide-search",
                                         "subdivide-surround",
                                         "subdivide-threshold",
                                         "tangent-surround",
                                          NULL);
  gtk_box_set_spacing (GTK_BOX (vbox), 12);

  scrolled_win = gimp_procedure_dialog_fill_scrolled_window (GIMP_PROCEDURE_DIALOG (dialog),
                                                             "scrollwin",
                                                             "selection-to-path-box");
  gtk_widget_set_size_request (scrolled_win, 400, 400);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (scrolled_win),
                                             FALSE);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "scrollwin",
                              NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

guchar
sel_pixel_value (gint row,
                 gint col)
{
  guchar ret;

  if (col > sel_width || row > sel_height)
    {
      g_warning ("sel_pixel_value [%d,%d] out of bounds", col, row);
      return 0;
    }

  gegl_sampler_get (sel_sampler,
                    col + sel_x1, row + sel_y1, NULL, &ret, GEGL_ABYSS_NONE);

  return ret;
}

gboolean
sel_pixel_is_white (gint row,
                    gint col)
{
  if (sel_pixel_value (row, col) < MID_POINT)
    return TRUE;
  else
    return FALSE;
}

gint
sel_get_width (void)
{
  return sel_width;
}

gint
sel_get_height (void)
{
  return sel_height;
}

gboolean
sel_valid_pixel (gint row,
                 gint col)
{
  return (0 <= (row) && (row) < sel_get_height ()
          && 0 <= (col) && (col) < sel_get_width ());
}


static void
do_points (spline_list_array_type  in_splines,
           GimpImage              *image)
{
  GimpPath    *path;
  gint32       stroke;
  gint         i, j;
  gboolean     have_points = FALSE;
  spline_list_type spline_list;

  /* check if there really is something to do... */
  for (i = 0; i < SPLINE_LIST_ARRAY_LENGTH (in_splines); i++)
    {
      spline_list = SPLINE_LIST_ARRAY_ELT (in_splines, i);
      /* Ignore single points that are on their own */
      if (SPLINE_LIST_LENGTH (spline_list) < 2)
        continue;
      have_points = TRUE;
      break;
    }

  if (! have_points)
    return;

  path = gimp_path_new (image, _("Selection"));

  for (j = 0; j < SPLINE_LIST_ARRAY_LENGTH (in_splines); j++)
    {
      spline_type seg;

      spline_list = SPLINE_LIST_ARRAY_ELT (in_splines, j);

      /* Ignore single points that are on their own */
      if (SPLINE_LIST_LENGTH (spline_list) < 2)
        continue;

      /*
       * we're constructing the path backwards
       * to have the result of least surprise for "Text along Path".
       */
      seg = SPLINE_LIST_ELT (spline_list, SPLINE_LIST_LENGTH (spline_list) - 1);
      stroke = gimp_path_bezier_stroke_new_moveto (path,
                                                   END_POINT (seg).x,
                                                   END_POINT (seg).y);

      for (i = SPLINE_LIST_LENGTH (spline_list); i > 0; i--)
        {
          seg = SPLINE_LIST_ELT (spline_list, i-1);

          if (SPLINE_DEGREE (seg) == LINEAR)
            gimp_path_bezier_stroke_lineto (path, stroke,
                                            START_POINT (seg).x,
                                            START_POINT (seg).y);
          else if (SPLINE_DEGREE (seg) == CUBIC)
            gimp_path_bezier_stroke_cubicto (path, stroke,
                                             CONTROL2 (seg).x,
                                             CONTROL2 (seg).y,
                                             CONTROL1 (seg).x,
                                             CONTROL1 (seg).y,
                                             START_POINT (seg).x,
                                             START_POINT (seg).y);
          else
            g_warning ("print_spline: strange degree (%d)",
                       SPLINE_DEGREE (seg));
        }

      gimp_path_stroke_close (path, stroke);

      /* transform to GIMPs coordinate system, taking the selections
       * bounding box into account  */
      gimp_path_stroke_scale (path, stroke, 1.0, -1.0);
      gimp_path_stroke_translate (path, stroke,
                                  sel_x1, sel_y1 + sel_height + 1);
    }

  gimp_image_insert_path (image, path, NULL, -1);
}


static gboolean
sel2path (GimpImage *image)
{
  GimpSelection           *selection;
  GeglBuffer              *sel_buffer;
  pixel_outline_list_type  olt;
  spline_list_array_type   splines;

  gimp_selection_bounds (image, &has_sel,
                         &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width  = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  /* Now get the selection channel */

  selection = gimp_image_get_selection (image);

  if (! selection)
    return FALSE;

  sel_buffer  = gimp_drawable_get_buffer (GIMP_DRAWABLE (selection));
  sel_sampler = gegl_buffer_sampler_new (sel_buffer,
                                         babl_format ("Y u8"),
                                         GEGL_SAMPLER_NEAREST);

  olt = find_outline_pixels ();

  splines = fitted_splines (olt);

  do_points (splines, image);

  g_object_unref (sel_sampler);
  g_object_unref (sel_buffer);

  gimp_displays_flush ();

  return TRUE;
}

void
safe_free (address *item)
{
  g_free (*item);
  *item = NULL;
}
