/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Plugin to convert a selection to a path.
 *
 * Copyright (C) 1999 Andy Thomas  alt@gimp.org
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
#
#include "global.h"
#include "types.h"
#include "pxl-outline.h"
#include "fit.h"
#include "spline.h"
#include "sel2path.h"

#include "libgimp/stdplugins-intl.h"


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


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static gint    sel_x1, sel_y1, sel_x2, sel_y2;
static gint    has_sel, sel_width, sel_height;
static SELVALS selVals;
GimpPixelRgn   selection_rgn;
gboolean       retVal = TRUE;  /* Toggle if cancle button clicked */

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)" },
  };

  static GimpParamDef advanced_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",                    "Interactive, non-interactive" },
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

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

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

  gimp_ui_init ("sel2path", FALSE);

  dlg = gimp_dialog_new (_("Selection to Path Advanced Settings"), "sel2path",
                         NULL, 0,
                         gimp_standard_help_func, "plug-in-sel2path-advanced",

                         GIMP_STOCK_RESET, RESPONSE_RESET,
                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dlg, "response",
                    G_CALLBACK (sel2path_response),
                    NULL);
  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  table = dialog_create_selection_area (sels);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), table);
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

  gimp_pixel_rgn_get_pixel(&selection_rgn,&ret,col+sel_x1,row+sel_y1);

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
gen_anchor (gdouble  *p,
            gdouble   x,
            gdouble   y,
            gboolean  is_newcurve)
{
/*   printf("TYPE: %s X: %d Y: %d\n",  */
/*       (is_newcurve)?"3":"1",  */
/*       sel_x1+(int)RINT(x),  */
/*       sel_y1 + sel_height - (int)RINT(y)+1); */

  *p++ = (sel_x1 + (gint)RINT(x));
  *p++ = sel_y1 + sel_height - (gint)RINT(y) + 1;
  *p++ = (is_newcurve) ? 3.0 : 1.0;
}


static void
gen_control (gdouble *p,
             gdouble  x,
             gdouble  y)
{
/*   printf("TYPE: 2 X: %d Y: %d\n",  */
/*       sel_x1+(int)RINT(x),  */
/*       sel_y1 + sel_height - (int)RINT(y)+1);  */

  *p++ = sel_x1 + (gint)RINT(x);
  *p++ = sel_y1 + sel_height - (gint)RINT(y) + 1;
  *p++ = 2.0;

}

static void
do_points (spline_list_array_type in_splines,
           gint32                 image_ID)
{
  unsigned this_list;
  gint seg_count = 0;
  gint point_count = 0;
  gdouble last_x,last_y;
  gdouble *parray;
  gdouble *cur_parray;
  gint path_point_count;

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (in_splines);
       this_list++)
    {
      spline_list_type in_list = SPLINE_LIST_ARRAY_ELT (in_splines, this_list);
      /* Ignore single points that are on their own */
      if(SPLINE_LIST_LENGTH (in_list) < 2)
        continue;
      point_count += SPLINE_LIST_LENGTH (in_list);
    }

/*   printf("Name SEL2PATH\n"); */
/*   printf("#POINTS: %d\n",point_count*3); */
/*   printf("CLOSED: 1\n"); */
/*   printf("DRAW: 0\n"); */
/*   printf("STATE: 4\n"); */

  path_point_count = point_count * 9;
  cur_parray = g_new0 (gdouble, path_point_count);
  parray = cur_parray;

  point_count = 0;

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (in_splines);
       this_list++)
    {
      unsigned this_spline;
      spline_list_type in_list = SPLINE_LIST_ARRAY_ELT (in_splines, this_list);

/*       if(seg_count > 0 && point_count > 0)   */
/*      gen_anchor(last_x,last_y,0);   */

      point_count = 0;

      /* Ignore single points that are on their own */
      if(SPLINE_LIST_LENGTH (in_list) < 2)
          continue;

      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (in_list);
           this_spline++)
        {
          spline_type s = SPLINE_LIST_ELT (in_list, this_spline);

          if (SPLINE_DEGREE (s) == LINEAR)
            {
              gen_anchor (cur_parray,
                          START_POINT (s).x, START_POINT (s).y,
                          seg_count && !point_count);
              cur_parray += 3;
              gen_control (cur_parray, START_POINT (s).x, START_POINT (s).y);
              cur_parray += 3;
              gen_control (cur_parray,END_POINT (s).x, END_POINT (s).y);
              cur_parray += 3;
              last_x = END_POINT (s).x;
              last_y = END_POINT (s).y;
            }
          else if (SPLINE_DEGREE (s) == CUBIC)
            {
              gen_anchor (cur_parray,
                          START_POINT (s).x, START_POINT (s).y,
                          seg_count && !point_count);
              cur_parray += 3;
              gen_control (cur_parray,CONTROL1 (s).x, CONTROL1 (s).y);
              cur_parray += 3;
              gen_control (cur_parray,CONTROL2 (s).x, CONTROL2 (s).y);
              cur_parray += 3;
              last_x = END_POINT (s).x;
              last_y = END_POINT (s).y;
            }
          else
            g_warning ("print_spline: strange degree (%d)", SPLINE_DEGREE (s));

          point_count++;
        }
      seg_count++;
    }

   gimp_path_set_points (image_ID, _("Selection"), 1, path_point_count, parray);
}


static gboolean
sel2path (gint32 image_ID)
{
  gint32                   selection_ID;
  GimpDrawable            *sel_drawable;
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

  sel_drawable = gimp_drawable_get (selection_ID);

  if (gimp_drawable_bpp (selection_ID) != 1)
    {
      g_warning ("Internal error. Selection bpp > 1");
      return FALSE;
    }

  gimp_pixel_rgn_init (&selection_rgn, sel_drawable,
                       sel_x1, sel_y1, sel_width, sel_height,
                       FALSE, FALSE);

  gimp_tile_cache_ntiles (2 * (sel_drawable->width + gimp_tile_width () - 1) /
                          gimp_tile_width ());

  olt = find_outline_pixels ();

  splines = fitted_splines (olt);

  do_points (splines, image_ID);

  gimp_drawable_detach (sel_drawable);

  gimp_displays_flush ();

  return TRUE;
}

void
safe_free (address *item)
{
  g_free (*item);
  *item = NULL;
}
