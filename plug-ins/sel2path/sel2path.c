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

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "global.h"
#include "types.h"
#include "pxl-outline.h"
#include "fit.h"
#include "spline.h"
#include "sel2path.h"

#include "libgimp/stdplugins-intl.h"


#define MID_POINT 127

/***** Magic numbers *****/

/* Variables set in dialog box */

static void      query  (void);
static void      run    (gchar    *name,
			 gint      nparams,
			 GimpParam   *param,
			 gint     *nreturn_vals,
			 GimpParam  **return_vals);

static gint      sel2path_dialog         (SELVALS   *sels);
static void      sel2path_ok_callback    (GtkWidget *widget, 
					  gpointer   data);
static void      sel2path_reset_callback (GtkWidget *widget,
					  gpointer   data);
static void      dialog_print_selVals    (SELVALS   *sels);
gboolean         do_sel2path             (gint32     drawable_ID, 
					  gint32     image_ID);


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
GimpPixelRgn      selection_rgn;
gboolean       retVal = TRUE;  /* Toggle if cancle button clicked */

MAIN ()

static void
query_2 (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",                    "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",                       "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",                    "Input drawable" }, 
    { GIMP_PDB_FLOAT,    "align_threshold",             "align_threshold"},
    { GIMP_PDB_FLOAT,    "corner_always_threshold",     "corner_always_threshold"},
    { GIMP_PDB_INT8,     "corner_surround",             "corner_surround"},
    { GIMP_PDB_FLOAT,    "corner_threshold",            "corner_threshold"},
    { GIMP_PDB_FLOAT,    "error_threshold",             "error_threshold"},
    { GIMP_PDB_INT8,     "filter_alternative_surround", "filter_alternative_surround"},
    { GIMP_PDB_FLOAT,    "filter_epsilon",              "filter_epsilon"},
    { GIMP_PDB_INT8,     "filter_iteration_count",      "filter_iteration_count"},
    { GIMP_PDB_FLOAT,    "filter_percent",              "filter_percent"},
    { GIMP_PDB_INT8,     "filter_secondary_surround",   "filter_secondary_surround"},
    { GIMP_PDB_INT8,     "filter_surround",             "filter_surround"},
    { GIMP_PDB_INT8,     "keep_knees",                  "{1-Yes, 0-No}"},
    { GIMP_PDB_FLOAT,    "line_reversion_threshold",    "line_reversion_threshold"},
    { GIMP_PDB_FLOAT,    "line_threshold",              "line_threshold"},
    { GIMP_PDB_FLOAT,    "reparameterize_improvement",  "reparameterize_improvement"},
    { GIMP_PDB_FLOAT,    "reparameterize_threshold",    "reparameterize_threshold"},
    { GIMP_PDB_FLOAT,    "subdivide_search",            "subdivide_search"},
    { GIMP_PDB_INT8,     "subdivide_surround",          "subdivide_surround"},
    { GIMP_PDB_FLOAT,    "subdivide_threshold",         "subdivide_threshold"},
    { GIMP_PDB_INT8,     "tangent_surround",            "tangent_surround"},
  };
  static GimpParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_sel2path_advanced",
			  "Converts a selection to a path (with advanced user menu)",
			  "Converts a selection to a path (with advanced user menu)",
			  "Andy Thomas",
			  "Andy Thomas",
			  "1999",
			  NULL,
			  "RGB*, INDEXED*, GRAY*",
			  GIMP_PLUGIN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
  };
  static GimpParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_sel2path",
			  "Converts a selection to a path",
			  "Converts a selection to a path",
			  "Andy Thomas",
			  "Andy Thomas",
			  "1999",
			  N_("<Image>/Select/To Path"),
			  "RGB*, INDEXED*, GRAY*",
			  GIMP_PLUGIN,
			  nargs, nreturn_vals,
			  args, return_vals);

  query_2 ();
}

static void
run (gchar    *name,
     gint      nparams,
     GimpParam   *param,
     gint     *nreturn_vals,
     GimpParam  **return_vals)
{
  static GimpParam values[1];
  GimpDrawable *   drawable;
  gint32        drawable_ID;
  gint32        image_ID;
  GimpRunModeType  run_mode;
  GimpPDBStatusType   status = GIMP_PDB_SUCCESS;
  gboolean      no_dialog = FALSE;

  run_mode = param[0].data.d_int32;

  if(!strcmp(name,"plug_in_sel2path")) 
    {
      no_dialog = TRUE;
      INIT_I18N();
    } 
  else
    INIT_I18N_UI();

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  drawable_ID = param[2].data.d_drawable;
  drawable = gimp_drawable_get (drawable_ID);

  image_ID = gimp_drawable_image_id(drawable_ID);

  if(gimp_selection_is_empty(image_ID))
    {
      g_message(_("No selection to convert"));
      gimp_drawable_detach (drawable);
      return;
    }

  fit_set_default_params(&selVals);
  
  if(!no_dialog)
    {
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  if (gimp_get_data_size ("plug_in_sel2path_advanced") > 0)
	    {
	      gimp_get_data ("plug_in_sel2path_advanced", &selVals);
	    }

	  if (!sel2path_dialog (&selVals))
	    {
	      gimp_drawable_detach (drawable);
	      return;
	    }
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
	      fit_set_params(&selVals);
	  }

	  break;
	  
	case GIMP_RUN_WITH_LAST_VALS:
	  if(gimp_get_data_size ("plug_in_sel2path_advanced") > 0)
	    {
	      gimp_get_data ("plug_in_sel2path_advanced", &selVals);
	      
	      /* Set up the last values */
	      fit_set_params (&selVals);
	    }

	  break;
	  
	default:
	  break;
	}
    }

  do_sel2path (drawable_ID,image_ID);
  values[0].data.d_status = status;

  if (status == GIMP_PDB_SUCCESS)
    {
      dialog_print_selVals(&selVals);
      if (run_mode == GIMP_RUN_INTERACTIVE && !no_dialog)
	gimp_set_data ("plug_in_sel2path_advanced", &selVals, sizeof(SELVALS));
    }

  gimp_drawable_detach (drawable);
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

  dlg = gimp_dialog_new (_("Sel2Path Advanced Settings"), "sel2path",
			 gimp_standard_help_func, "filters/sel2path.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), sel2path_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,

			 _("Reset"), sel2path_reset_callback,
			 NULL, NULL, NULL, FALSE, FALSE,

			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);
  
  table = dialog_create_selection_area(sels);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), table);
  gtk_widget_show (table);

  gtk_widget_show (dlg);

  gtk_main ();

  return retVal;
}

static void
sel2path_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
  retVal = TRUE;
}

static void
sel2path_reset_callback (GtkWidget *widget,
			 gpointer   data)
{
  reset_adv_dialog();
  fit_set_params(&selVals);
}

guchar
sel_pixel_value (gint row, 
		 gint col)
{
  guchar ret;

  if(col > sel_width ||
     row > sel_height)
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

gint 
sel_valid_pixel (gint row, 
		 gint col)
{
  return (0 <= (row) && (row) < sel_get_height ()
	  && 0 <= (col) && (col) < sel_get_width ());
}

void
gen_anchor (gdouble  *p,
	    gdouble   x,
	    gdouble   y,
	    gboolean  is_newcurve)
{
/*   printf("TYPE: %s X: %d Y: %d\n",  */
/* 	 (is_newcurve)?"3":"1",  */
/* 	 sel_x1+(int)RINT(x),  */
/* 	 sel_y1 + sel_height - (int)RINT(y)+1); */

  *p++ = (sel_x1 + (gint)RINT(x));
  *p++ = sel_y1 + sel_height - (gint)RINT(y) + 1;
  *p++ = (is_newcurve) ? 3.0 : 1.0;
}
  

void  
gen_control (gdouble *p,
	     gdouble  x,
	     gdouble  y)
{
/*   printf("TYPE: 2 X: %d Y: %d\n",  */
/*  	 sel_x1+(int)RINT(x),  */
/*  	 sel_y1 + sel_height - (int)RINT(y)+1);  */

  *p++ = sel_x1 + (gint)RINT(x);
  *p++ = sel_y1 + sel_height - (gint)RINT(y) + 1;
  *p++ = 2.0;

}

void
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

  path_point_count = point_count*9;
  cur_parray = (gdouble *)g_new0(gdouble ,point_count*9);
  parray = cur_parray;

  point_count = 0;

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (in_splines);
       this_list++)
    {
      unsigned this_spline;
      spline_list_type in_list = SPLINE_LIST_ARRAY_ELT (in_splines, this_list);
      
/*       if(seg_count > 0 && point_count > 0)   */
/* 	gen_anchor(last_x,last_y,0);   */
      
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
	    g_message ( _("print_spline: strange degree (%d)"), 
			SPLINE_DEGREE (s));
	  
	  point_count++;
	}
      seg_count++;
    }

   gimp_path_set_points (image_ID,
 			_("selection_to_path"), 
 			1, 
 			path_point_count,
 			parray);
}


gboolean
do_sel2path (gint32 drawable_ID,
	     gint32 image_ID)
{
  gint32      selection_ID;
  GimpDrawable  *sel_drawable;
  pixel_outline_list_type olt;
  spline_list_array_type splines;

  gimp_selection_bounds (image_ID, &has_sel, 
			 &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width  = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  /* Now get the selection channel */

  selection_ID = gimp_image_get_selection(image_ID);

  if(selection_ID < 0)
    {
      g_message( _("gimp_image_get_selection failed"));
      return FALSE;
    }

  sel_drawable = gimp_drawable_get (selection_ID);

  if(gimp_drawable_bpp(selection_ID) != 1)
    {
      g_message( _("Internal error. Selection bpp > 1"));
      return FALSE;
    }

  gimp_pixel_rgn_init (&selection_rgn, sel_drawable,
		       sel_x1, sel_y1, sel_width, sel_height,
		       FALSE, FALSE);

  gimp_tile_cache_ntiles (2 * (sel_drawable->width + gimp_tile_width () - 1) / 
			  gimp_tile_width ());

  olt = find_outline_pixels ();

  splines = fitted_splines (olt);

  gimp_selection_none (image_ID);
  gimp_displays_flush ();

  do_points (splines, image_ID);

  return TRUE;
}

void
safe_free (address *item)
{
  if (item == NULL || *item == NULL)
    {
      g_warning ("safe_free: Attempt to free a null item.");
    }
  
  g_free (*item);
  
  *item = NULL;
}
