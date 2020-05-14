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
                                                   GimpDrawable         *drawable,
                                                   const GimpValueArray *args,
                                                   gpointer              run_data);

static gint             sel2path_dialog           (SELVALS              *sels);
static void             sel2path_response         (GtkWidget            *widget,
                                                   gint                  response_id,
                                                   gpointer              data);
static void             dialog_print_selVals      (SELVALS              *sels);
static gboolean         sel2path                  (GimpImage            *image);


G_DEFINE_TYPE (Sel2path, sel2path, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SEL2PATH_TYPE)


static gint         sel_x1, sel_y1, sel_x2, sel_y2;
static gint         has_sel, sel_width, sel_height;
static SELVALS      selVals;
static GeglSampler *sel_sampler;
static gboolean     retVal = TRUE;  /* Toggle if cancel button clicked */


static void
sel2path_class_init (Sel2pathClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = sel2path_query_procedures;
  plug_in_class->create_procedure = sel2path_create_procedure;
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

      gimp_procedure_set_documentation (procedure,
                                        "Converts a selection to a path",
                                        "Converts a selection to a path",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Andy Thomas",
                                      "Andy Thomas",
                                      "1999");

      GIMP_PROC_ARG_DOUBLE (procedure, "align-threshold",
                            "Align threshold",
                            "Align threshold",
                            0.2, 2.0, 0.5,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "corner-always-threshold",
                            "Corner always threshold",
                            "Corner always threshold",
                            30, 180, 60.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "corner-surround",
                         "Corner surround",
                         "Corner surround",
                         3, 8, 4,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "corner-threshold",
                            "Corner threshold",
                            "Corner threshold",
                            0, 180, 100.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "error-threshold",
                            "Error threshold",
                            "Error threshold",
                            0.2, 10, 0.4,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "filter-alternative-surround",
                         "Filter alternative surround",
                         "Filter alternative surround",
                         1, 10, 1,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "filter-epsilon",
                            "Filter epsilon",
                            "Filter epsilon",
                            5, 40, 10.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "filter-iteration-count",
                         "Filter iteration count",
                         "Filter iteration count",
                         4, 70, 4,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "filter-percent",
                            "Filter percent",
                            "Filter percent",
                            0, 1, 0.33,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "filter-secondary-surround",
                         "Filter secondary surround",
                         "Filter secondary surround",
                         3, 10, 3,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "filter-surround",
                         "Filter surround",
                         "Filter surround",
                         2, 10, 2,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "keep-knees",
                             "Keep knees",
                             "Keep knees",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "line-reversion-threshold",
                            "Line reversion threshold",
                            "Line reversion threshold",
                            0.01, 0.2, 0.01,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "line-threshold",
                            "Line threshold",
                            "Line threshold",
                            0.2, 4, 0.5,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "reparametrize-improvement",
                            "Reparametrize improvement",
                            "Reparametrize improvement",
                            0, 1, 0.01,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "reparametrize-threshold",
                            "Reparametrize threshold",
                            "Reparametrize threshold",
                            1, 50, 1.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "subdivide-search",
                            "Subdivide search",
                            "Subdivide search",
                            0.05, 1, 0.1,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "subdivide-surround",
                         "Subdivide surround",
                         "Subdivide surround",
                         2, 10, 4,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "subdivide-threshold",
                            "Subdivide threshold",
                            "Subdivide threshold",
                            0.01, 1, 0.03,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "tangent-surround",
                         "Tangent surround",
                         "Tangent surround",
                         2, 10, 3,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
sel2path_run (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              GimpDrawable         *drawable,
              const GimpValueArray *args,
              gpointer              run_data)
{
  INIT_I18N ();
  gegl_init (NULL, NULL);

  if (gimp_selection_is_empty (image))
    {
      g_message (_("No selection to convert"));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_SUCCESS,
                                               NULL);
    }

  fit_set_default_params (&selVals);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      if (gimp_get_data_size (PLUG_IN_PROC) > 0)
        gimp_get_data (PLUG_IN_PROC, &selVals);

      if (! sel2path_dialog (&selVals))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);

      /* Get the current settings */
      fit_set_params (&selVals);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      selVals.align_threshold             = GIMP_VALUES_GET_DOUBLE  (args, 0);
      selVals.corner_always_threshold     = GIMP_VALUES_GET_DOUBLE  (args, 1);
      selVals.corner_surround             = GIMP_VALUES_GET_INT     (args, 2);
      selVals.corner_threshold            = GIMP_VALUES_GET_DOUBLE  (args, 3);
      selVals.error_threshold             = GIMP_VALUES_GET_DOUBLE  (args, 4);
      selVals.filter_alternative_surround = GIMP_VALUES_GET_INT     (args, 5);
      selVals.filter_epsilon              = GIMP_VALUES_GET_DOUBLE  (args, 6);
      selVals.filter_iteration_count      = GIMP_VALUES_GET_INT     (args, 7);
      selVals.filter_percent              = GIMP_VALUES_GET_DOUBLE  (args, 8);
      selVals.filter_secondary_surround   = GIMP_VALUES_GET_INT     (args, 9);
      selVals.filter_surround             = GIMP_VALUES_GET_INT     (args, 10);
      selVals.keep_knees                  = GIMP_VALUES_GET_BOOLEAN (args, 11);
      selVals.line_reversion_threshold    = GIMP_VALUES_GET_DOUBLE  (args, 12);
      selVals.line_threshold              = GIMP_VALUES_GET_DOUBLE  (args, 13);
      selVals.reparameterize_improvement  = GIMP_VALUES_GET_DOUBLE  (args, 14);
      selVals.reparameterize_threshold    = GIMP_VALUES_GET_DOUBLE  (args, 15);
      selVals.subdivide_search            = GIMP_VALUES_GET_DOUBLE  (args, 16);
      selVals.subdivide_surround          = GIMP_VALUES_GET_INT     (args, 17);
      selVals.subdivide_threshold         = GIMP_VALUES_GET_DOUBLE  (args, 18);
      selVals.tangent_surround            = GIMP_VALUES_GET_INT     (args, 19);

      fit_set_params (&selVals);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      if (gimp_get_data_size (PLUG_IN_PROC) > 0)
        {
          gimp_get_data (PLUG_IN_PROC, &selVals);

          /* Set up the last values */
          fit_set_params (&selVals);
        }
      break;

    default:
      break;
    }

  if (! sel2path (image))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             NULL);

  dialog_print_selVals (&selVals);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_set_data (PLUG_IN_PROC, &selVals, sizeof(SELVALS));

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static void
dialog_print_selVals (SELVALS *sels)
{
#if 0
  printf ("selVals.align_threshold %g\n",             selVals.align_threshold);
  printf ("selVals.corner_always_threshol %g\n",      selVals.corner_always_threshold);
  printf ("selVals.corner_surround %g\n",             selVals.corner_surround);
  printf ("selVals.corner_threshold %g\n",            selVals.corner_threshold);
  printf ("selVals.error_threshold %g\n",             selVals.error_threshold);
  printf ("selVals.filter_alternative_surround %g\n", selVals.filter_alternative_surround);
  printf ("selVals.filter_epsilon %g\n",              selVals.filter_epsilon);
  printf ("selVals.filter_iteration_count %g\n",      selVals.filter_iteration_count);
  printf ("selVals.filter_percent %g\n",              selVals.filter_percent);
  printf ("selVals.filter_secondary_surround %g\n",   selVals.filter_secondary_surround);
  printf ("selVals.filter_surround %g\n",             selVals.filter_surround);
  printf ("selVals.keep_knees %d\n",                  selVals.keep_knees);
  printf ("selVals.line_reversion_threshold %g\n",    selVals.line_reversion_threshold);
  printf ("selVals.line_threshold %g\n",              selVals.line_threshold);
  printf ("selVals.reparameterize_improvement %g\n",  selVals.reparameterize_improvement);
  printf ("selVals.reparameterize_threshold %g\n",    selVals.reparameterize_threshold);
  printf ("selVals.subdivide_search %g\n"             selVals.subdivide_search);
  printf ("selVals.subdivide_surround %g\n",          selVals.subdivide_surround);
  printf ("selVals.subdivide_threshold %g\n",         selVals.subdivide_threshold);
  printf ("selVals.tangent_surround %g\n",            selVals.tangent_surround);
#endif /* 0 */
}

/* Build the dialog up. This was the hard part! */
static gint
sel2path_dialog (SELVALS *sels)
{
  GtkWidget *dlg;
  GtkWidget *table;

  retVal = FALSE;

  gimp_ui_init (PLUG_IN_BINARY);

  dlg = gimp_dialog_new (_("Selection to Path Advanced Settings"),
                         PLUG_IN_ROLE,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Reset"), RESPONSE_RESET,
                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                            RESPONSE_RESET,
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  g_signal_connect (dlg, "response",
                    G_CALLBACK (sel2path_response),
                    NULL);
  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  table = dialog_create_selection_area (sels);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  gtk_widget_show (dlg);

  gtk_main ();

  return retVal;
}

static void
sel2path_response (GtkWidget *widget,
                   gint       response_id,
                   gpointer   data)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      reset_adv_dialog ();
      fit_set_params (&selVals);
      break;

    case GTK_RESPONSE_OK:
      retVal = TRUE;

    default:
      gtk_widget_destroy (widget);
      break;
    }
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
  GimpVectors *vectors;
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

  vectors = gimp_vectors_new (image, _("Selection"));

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
      stroke = gimp_vectors_bezier_stroke_new_moveto (vectors,
                                                      END_POINT (seg).x,
                                                      END_POINT (seg).y);

      for (i = SPLINE_LIST_LENGTH (spline_list); i > 0; i--)
        {
          seg = SPLINE_LIST_ELT (spline_list, i-1);

          if (SPLINE_DEGREE (seg) == LINEAR)
            gimp_vectors_bezier_stroke_lineto (vectors, stroke,
                                               START_POINT (seg).x,
                                               START_POINT (seg).y);
          else if (SPLINE_DEGREE (seg) == CUBIC)
            gimp_vectors_bezier_stroke_cubicto (vectors, stroke,
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

      gimp_vectors_stroke_close (vectors, stroke);

      /* transform to GIMPs coordinate system, taking the selections
       * bounding box into account  */
      gimp_vectors_stroke_scale (vectors, stroke, 1.0, -1.0);
      gimp_vectors_stroke_translate (vectors, stroke,
                                     sel_x1, sel_y1 + sel_height + 1);
    }

  gimp_image_insert_vectors (image, vectors, NULL, -1);
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
