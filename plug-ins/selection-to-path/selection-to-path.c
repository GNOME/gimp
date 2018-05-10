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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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


#define PLUG_IN_BINARY "selection-to-path"
#define PLUG_IN_ROLE   "gimp-selection-to-path"

#define RESPONSE_RESET 1
#define MID_POINT      127

/***** Magic numbers *****/

/* Variables set in dialog box */

static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static gint      sel2path_dialog         (SELVALS   *sels);
static void      sel2path_response       (GtkWidget *widget,
                                          gint       response_id,
                                          gpointer   data);
static void      dialog_print_selVals    (SELVALS   *sels);
static gboolean  sel2path                (gint32     image_ID);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static gint         sel_x1, sel_y1, sel_x2, sel_y2;
static gint         has_sel, sel_width, sel_height;
static SELVALS      selVals;
static GeglSampler *sel_sampler;
static gboolean     retVal = TRUE;  /* Toggle if cancle button clicked */

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)" },
  };

  static const GimpParamDef advanced_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",                    "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",                       "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",                    "Input drawable (unused)" },
    { GIMP_PDB_FLOAT,    "align-threshold",             "align_threshold"},
    { GIMP_PDB_FLOAT,    "corner-always-threshold",     "corner_always_threshold"},
    { GIMP_PDB_INT8,     "corner-surround",             "corner_surround"},
    { GIMP_PDB_FLOAT,    "corner-threshold",            "corner_threshold"},
    { GIMP_PDB_FLOAT,    "error-threshold",             "error_threshold"},
    { GIMP_PDB_INT8,     "filter-alternative-surround", "filter_alternative_surround"},
    { GIMP_PDB_FLOAT,    "filter-epsilon",              "filter_epsilon"},
    { GIMP_PDB_INT8,     "filter-iteration-count",      "filter_iteration_count"},
    { GIMP_PDB_FLOAT,    "filter-percent",              "filter_percent"},
    { GIMP_PDB_INT8,     "filter-secondary-surround",   "filter_secondary_surround"},
    { GIMP_PDB_INT8,     "filter-surround",             "filter_surround"},
    { GIMP_PDB_INT8,     "keep-knees",                  "{1-Yes, 0-No}"},
    { GIMP_PDB_FLOAT,    "line-reversion-threshold",    "line_reversion_threshold"},
    { GIMP_PDB_FLOAT,    "line-threshold",              "line_threshold"},
    { GIMP_PDB_FLOAT,    "reparameterize-improvement",  "reparameterize_improvement"},
    { GIMP_PDB_FLOAT,    "reparameterize-threshold",    "reparameterize_threshold"},
    { GIMP_PDB_FLOAT,    "subdivide-search",            "subdivide_search"},
    { GIMP_PDB_INT8,     "subdivide-surround",          "subdivide_surround"},
    { GIMP_PDB_FLOAT,    "subdivide-threshold",         "subdivide_threshold"},
    { GIMP_PDB_INT8,     "tangent-surround",            "tangent_surround"},
  };

  gimp_install_procedure ("plug-in-sel2path",
                          "Converts a selection to a path",
                          "Converts a selection to a path",
                          "Andy Thomas",
                          "Andy Thomas",
                          "1999",
                          NULL,
                          "RGB*, INDEXED*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_install_procedure ("plug-in-sel2path-advanced",
                          "Converts a selection to a path (with advanced user menu)",
                          "Converts a selection to a path (with advanced user menu)",
                          "Andy Thomas",
                          "Andy Thomas",
                          "1999",
                          NULL,
                          "RGB*, INDEXED*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (advanced_args), 0,
                          advanced_args, NULL);
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
  GimpRunMode        run_mode;
  GimpPDBStatusType  status    = GIMP_PDB_SUCCESS;
  gboolean           no_dialog;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  no_dialog = (strcmp (name, "plug-in-sel2path") == 0);

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  image_ID = param[1].data.d_image;
  if (image_ID < 0)
    {
      g_warning ("plug-in-sel2path needs a valid image ID");
      return;
    }

  if (gimp_selection_is_empty (image_ID))
    {
      g_message (_("No selection to convert"));
      return;
    }

  fit_set_default_params (&selVals);

  if (!no_dialog)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          if (gimp_get_data_size ("plug-in-sel2path-advanced") > 0)
            {
              gimp_get_data ("plug-in-sel2path-advanced", &selVals);
            }

          if (!sel2path_dialog (&selVals))
            return;

          /* Get the current settings */
          fit_set_params (&selVals);
          break;

        case GIMP_RUN_NONINTERACTIVE:
          if (nparams != 23)
            status = GIMP_PDB_CALLING_ERROR;

          if (status == GIMP_PDB_SUCCESS)
            {
              selVals.align_threshold             =  param[3].data.d_float;
              selVals.corner_always_threshold     =  param[4].data.d_float;
              selVals.corner_surround             =  param[5].data.d_int8;
              selVals.corner_threshold            =  param[6].data.d_float;
              selVals.error_threshold             =  param[7].data.d_float;
              selVals.filter_alternative_surround =  param[8].data.d_int8;
              selVals.filter_epsilon              =  param[9].data.d_float;
              selVals.filter_iteration_count      = param[10].data.d_int8;
              selVals.filter_percent              = param[11].data.d_float;
              selVals.filter_secondary_surround   = param[12].data.d_int8;
              selVals.filter_surround             = param[13].data.d_int8;
              selVals.keep_knees                  = param[14].data.d_int8;
              selVals.line_reversion_threshold    = param[15].data.d_float;
              selVals.line_threshold              = param[16].data.d_float;
              selVals.reparameterize_improvement  = param[17].data.d_float;
              selVals.reparameterize_threshold    = param[18].data.d_float;
              selVals.subdivide_search            = param[19].data.d_float;
              selVals.subdivide_surround          = param[20].data.d_int8;
              selVals.subdivide_threshold         = param[21].data.d_float;
              selVals.tangent_surround            = param[22].data.d_int8;
              fit_set_params (&selVals);
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          if(gimp_get_data_size ("plug-in-sel2path-advanced") > 0)
            {
              gimp_get_data ("plug-in-sel2path-advanced", &selVals);

              /* Set up the last values */
              fit_set_params (&selVals);
            }
          break;

        default:
          break;
        }
    }

  sel2path (image_ID);
  values[0].data.d_status = status;

  if (status == GIMP_PDB_SUCCESS)
    {
      dialog_print_selVals(&selVals);
      if (run_mode == GIMP_RUN_INTERACTIVE && !no_dialog)
        gimp_set_data ("plug-in-sel2path-advanced", &selVals, sizeof(SELVALS));
    }
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

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dlg = gimp_dialog_new (_("Selection to Path Advanced Settings"),
                         PLUG_IN_ROLE,
                         NULL, 0,
                         gimp_standard_help_func, "plug-in-sel2path-advanced",

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
do_points (spline_list_array_type in_splines,
           gint32                 image_ID)
{
  gint32   vectors;
  gint32   stroke;
  gint     i, j;
  gboolean have_points = FALSE;
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

  if (!have_points)
    return;

  vectors = gimp_vectors_new (image_ID, _("Selection"));

  for (j = 0; j < SPLINE_LIST_ARRAY_LENGTH (in_splines); j++)
    {
      spline_type seg;

      spline_list = SPLINE_LIST_ARRAY_ELT (in_splines, j);

      /* Ignore single points that are on their own */
      if (SPLINE_LIST_LENGTH (spline_list) < 2)
        continue;

      seg = SPLINE_LIST_ELT (spline_list, 0);
      stroke = gimp_vectors_bezier_stroke_new_moveto (vectors,
                                                      START_POINT (seg).x,
                                                      START_POINT (seg).y);

      for (i = 0; i < SPLINE_LIST_LENGTH (spline_list); i++)
        {
          seg = SPLINE_LIST_ELT (spline_list, i);

          if (SPLINE_DEGREE (seg) == LINEAR)
            gimp_vectors_bezier_stroke_lineto (vectors, stroke,
                                               END_POINT (seg).x,
                                               END_POINT (seg).y);
          else if (SPLINE_DEGREE (seg) == CUBIC)
            gimp_vectors_bezier_stroke_cubicto (vectors, stroke,
                                                CONTROL1 (seg).x,
                                                CONTROL1 (seg).y,
                                                CONTROL2 (seg).x,
                                                CONTROL2 (seg).y,
                                                END_POINT (seg).x,
                                                END_POINT (seg).y);
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

  gimp_image_insert_vectors (image_ID, vectors, -1, -1);
}


static gboolean
sel2path (gint32 image_ID)
{
  gint32                   selection_ID;
  GeglBuffer              *sel_buffer;
  pixel_outline_list_type  olt;
  spline_list_array_type   splines;

  gimp_selection_bounds (image_ID, &has_sel,
                         &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width  = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  /* Now get the selection channel */

  selection_ID = gimp_image_get_selection (image_ID);

  if (selection_ID < 0)
    return FALSE;

  sel_buffer  = gimp_drawable_get_buffer (selection_ID);
  sel_sampler = gegl_buffer_sampler_new (sel_buffer,
                                         babl_format ("Y u8"),
                                         GEGL_SAMPLER_NEAREST);

  olt = find_outline_pixels ();

  splines = fitted_splines (olt);

  do_points (splines, image_ID);

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
