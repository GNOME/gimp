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

#include "gtk/gtk.h"
#include "libgimp/gimp.h"
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
			 GParam   *param,
			 gint     *nreturn_vals,
			 GParam  **return_vals);

static gint      sel2path_dialog (SELVALS *);
static void      sel2path_ok_callback (GtkWidget *,gpointer);
static void      sel2path_close_callback (GtkWidget *,gpointer);
static void      sel2path_reset_callback (GtkWidget *,gpointer);
static void      dialog_print_selVals(SELVALS *);

gboolean         do_sel2path (gint32,gint32);
static gint      gimp_selection_bounds (gint32,gint *,gint *,gint *,gint *,gint *);
static gint      gimp_selection_is_empty (gint32);
static gint      gimp_selection_none (gint32);
static gint      gimp_path_set_points (gint32,gchar *,gint,gint,gdouble *);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static gint    sel_x1, sel_y1, sel_x2, sel_y2;
static gint    has_sel, sel_width, sel_height;
static SELVALS selVals;
GPixelRgn      selection_rgn;
gboolean       retVal = TRUE;  /* Toggle if cancle button clicked */

MAIN ()

static void
query_2()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" }, 
    { PARAM_FLOAT, "align_threshold","align_threshold"},
    { PARAM_FLOAT, "corner_always_threshold","corner_always_threshold"},
    { PARAM_INT8,  "corner_surround","corner_surround"},
    { PARAM_FLOAT, "corner_threshold","corner_threshold"},
    { PARAM_FLOAT, "error_threshold","error_threshold"},
    { PARAM_INT8,  "filter_alternative_surround","filter_alternative_surround"},
    { PARAM_FLOAT, "filter_epsilon","filter_epsilon"},
    { PARAM_INT8,  "filter_iteration_count","filter_iteration_count"},
    { PARAM_FLOAT, "filter_percent","filter_percent"},
    { PARAM_INT8,  "filter_secondary_surround","filter_secondary_surround"},
    { PARAM_INT8,  "filter_surround","filter_surround"},
    { PARAM_INT8,  "keep_knees","{1-Yes, 0-No}"},
    { PARAM_FLOAT, "line_reversion_threshold","line_reversion_threshold"},
    { PARAM_FLOAT,"line_threshold","line_threshold"},
    { PARAM_FLOAT,"reparameterize_improvement","reparameterize_improvement"},
    { PARAM_FLOAT,"reparameterize_threshold","reparameterize_threshold"},
    { PARAM_FLOAT,"subdivide_search","subdivide_search"},
    { PARAM_INT8,  "subdivide_surround","subdivide_surround"},
    { PARAM_FLOAT,"subdivide_threshold","subdivide_threshold"},
    { PARAM_INT8,  "tangent_surround","tangent_surround"},
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_sel2path_advanced",
			  _("Converts a selection to a path (with advanced user menu)"),
			  _("Converts a selection to a path (with advanced user menu)"),
			  "Andy Thomas",
			  "Andy Thomas",
			  "1999",
			  NULL,
			  "RGB*, INDEXED*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_sel2path",
			  _("Converts a selection to a path"),
			  _("Converts a selection to a path"),
			  "Andy Thomas",
			  "Andy Thomas",
			  "1999",
			  N_("<Image>/Select/To Path"),
			  "RGB*, INDEXED*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);

  query_2();
}

static void
run    (gchar    *name,
	gint      nparams,
	GParam   *param,
	gint     *nreturn_vals,
	GParam  **return_vals)
{
  static GParam values[1];
  GDrawable *   drawable;
  gint32        drawable_ID;
  gint32        image_ID;
  GRunModeType  run_mode;
  GStatusType   status = STATUS_SUCCESS;
  gboolean      no_dialog = FALSE;

  run_mode = param[0].data.d_int32;

  if(!strcmp(name,"plug_in_sel2path")) {
    no_dialog = TRUE;
    INIT_I18N();
  } else
    INIT_I18N_UI();

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
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
	case RUN_INTERACTIVE:
	  if(gimp_get_data_size("plug_in_sel2path_advanced") > 0)
	    {
	      gimp_get_data("plug_in_sel2path_advanced", &selVals);
	    }

	  if (!sel2path_dialog(&selVals))
	    {
	      gimp_drawable_detach (drawable);
	      return;
	    }
	  /* Get the current settings */
	  fit_set_params(&selVals);
	  break;
	  
	case RUN_NONINTERACTIVE:
	  if (nparams != 23)
	    status = STATUS_CALLING_ERROR;
	  
	  if (status == STATUS_SUCCESS) {
	    selVals.align_threshold = param[3].data.d_float;
	    selVals.corner_always_threshold = param[4].data.d_float;
	    selVals.corner_surround = param[5].data.d_int8;
	    selVals.corner_threshold = param[6].data.d_float;
	    selVals.error_threshold = param[7].data.d_float;
	    selVals.filter_alternative_surround = param[8].data.d_int8;
	    selVals.filter_epsilon = param[9].data.d_float;
	    selVals.filter_iteration_count = param[10].data.d_int8;
	    selVals.filter_percent =  param[11].data.d_float;
	    selVals.filter_secondary_surround = param[12].data.d_int8;
	    selVals.filter_surround = param[13].data.d_int8;
	    selVals.keep_knees = param[14].data.d_int8;
	    selVals.line_reversion_threshold = param[15].data.d_float;
	    selVals.line_threshold = param[16].data.d_float;
	    selVals.reparameterize_improvement = param[17].data.d_float;
	    selVals.reparameterize_threshold = param[18].data.d_float;
	    selVals.subdivide_search = param[19].data.d_float;
	    selVals.subdivide_surround = param[20].data.d_int8;
	    selVals.subdivide_threshold =  param[21].data.d_float;
	    selVals.tangent_surround = param[22].data.d_int8;
	    fit_set_params(&selVals);
	  }

	  break;
	  
	case RUN_WITH_LAST_VALS:
	  if(gimp_get_data_size("plug_in_sel2path_advanced") > 0)
	    {
	      gimp_get_data("plug_in_sel2path_advanced", &selVals);
	      
	      /* Set up the last values */
	      fit_set_params(&selVals);
	    }

	  break;
	  
	default:
	  break;
	}
    }

  do_sel2path(drawable_ID,image_ID);
  values[0].data.d_status = status;

  if(status == STATUS_SUCCESS)
    {
      dialog_print_selVals(&selVals);
      if (run_mode == RUN_INTERACTIVE && !no_dialog)
	gimp_set_data("plug_in_sel2path_advanced", &selVals, sizeof(SELVALS));
    }

  gimp_drawable_detach (drawable);
}

static void
dialog_print_selVals(SELVALS *sels)
{
#if 0
  printf("selVals.align_threshold %g\n",selVals.align_threshold);
  printf("selVals.corner_always_threshol %g\n",selVals.corner_always_threshold);
  printf("selVals.corner_surround %g\n",selVals.corner_surround);
  printf("selVals.corner_threshold %g\n",selVals.corner_threshold);
  printf("selVals.error_threshold %g\n",selVals.error_threshold);
  printf("selVals.filter_alternative_surround %g\n",selVals.filter_alternative_surround);
  printf("selVals.filter_epsilon %g\n",selVals.filter_epsilon);
  printf("selVals.filter_iteration_count %g\n",selVals.filter_iteration_count);
  printf("selVals.filter_percent %g\n",selVals.filter_percent);
  printf("selVals.filter_secondary_surround %g\n",selVals.filter_secondary_surround);
  printf("selVals.filter_surround %g\n",selVals.filter_surround);
  printf("selVals.keep_knees %d\n",selVals.keep_knees);
  printf("selVals.line_reversion_threshold %g\n",selVals.line_reversion_threshold);
  printf("selVals.line_threshold %g\n",selVals.line_threshold);
  printf("selVals.reparameterize_improvement %g\n",selVals.reparameterize_improvement);
  printf("selVals.reparameterize_threshold %g\n",selVals.reparameterize_threshold);
  printf("selVals.subdivide_search %g\n",selVals.subdivide_search);
  printf("selVals.subdivide_surround %g\n",selVals.subdivide_surround);
  printf("selVals.subdivide_threshold %g\n",selVals.subdivide_threshold);
  printf("selVals.tangent_surround %g\n",selVals.tangent_surround);
#endif /* 0 */
}

/* Build the dialog up. This was the hard part! */
static gint
sel2path_dialog (SELVALS *sels)
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *vbox;
  GtkWidget *table;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("sel2path");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Sel2path");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) sel2path_close_callback,
		      NULL);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), vbox,
		     FALSE, FALSE, 0);
  gtk_widget_show(vbox);
  
  table = dialog_create_selection_area(sels);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  /*  Action area  */
  button = gtk_button_new_with_label ( _("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) sel2path_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( _("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( ("Default Values"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) sel2path_reset_callback,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return retVal;
}

static void
sel2path_close_callback (GtkWidget *widget,
			 gpointer   data)
{
  retVal = FALSE;
  gtk_main_quit ();
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
sel_pixel_value(gint row, gint col)
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

gint
sel_pixel_is_white(gint row, gint col)
{
  if(sel_pixel_value(row,col) < MID_POINT)
    return 1;
  else
    return 0;
}

gint 
sel_get_width()
{
  return sel_width;
}

gint
sel_get_height()
{
  return sel_height;
}

gint 
sel_valid_pixel(gint row, gint col)
{
  return (0 <= (row) && (row) < sel_get_height()
	  && 0 <= (col) && (col) < sel_get_width());
}

void
gen_anchor(gdouble *p,double x,double y,int is_newcurve)
{
/*   printf("TYPE: %s X: %d Y: %d\n",  */
/* 	 (is_newcurve)?"3":"1",  */
/* 	 sel_x1+(int)RINT(x),  */
/* 	 sel_y1 + sel_height - (int)RINT(y)+1); */

  *p++ = (sel_x1+(int)RINT(x));
  *p++ = sel_y1 + sel_height - (int)RINT(y)+1;
  *p++ = (is_newcurve)?3.0:1.0;

}
  

void  
gen_control(gdouble *p,double x,double y)
{
/*   printf("TYPE: 2 X: %d Y: %d\n",  */
/*  	 sel_x1+(int)RINT(x),  */
/*  	 sel_y1 + sel_height - (int)RINT(y)+1);  */

  *p++ = sel_x1+(int)RINT(x);
  *p++ = sel_y1 + sel_height - (int)RINT(y)+1;
  *p++ = 2.0;

}

void
do_points(spline_list_array_type in_splines,gint32 image_ID)
{
  unsigned this_list;
  int seg_count = 0;
  int point_count = 0;
  double last_x,last_y;
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
	      gen_anchor(cur_parray,START_POINT (s).x, START_POINT (s).y,seg_count && !point_count);
	      cur_parray += 3;
	      gen_control(cur_parray,START_POINT (s).x, START_POINT (s).y);
	      cur_parray += 3;
	      gen_control(cur_parray,END_POINT (s).x, END_POINT (s).y);
	      cur_parray += 3;
	      last_x = END_POINT (s).x;
	      last_y = END_POINT (s).y;
	    }
	  else if (SPLINE_DEGREE (s) == CUBIC)
	    {
	      gen_anchor(cur_parray,START_POINT (s).x, START_POINT (s).y,seg_count && !point_count);
	      cur_parray += 3;
	      gen_control(cur_parray,CONTROL1 (s).x, CONTROL1 (s).y);
	      cur_parray += 3;
	      gen_control(cur_parray,CONTROL2 (s).x, CONTROL2 (s).y);
	      cur_parray += 3;
	      last_x = END_POINT (s).x;
	      last_y = END_POINT (s).y;
	    }
	  else
	    g_message ( _("print_spline: strange degree (%d)"), SPLINE_DEGREE (s));
	  
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
do_sel2path(gint32 drawable_ID,gint32 image_ID )
{
  gint32      selection_ID;
  GDrawable  *sel_drawable;
  pixel_outline_list_type olt;
  spline_list_array_type splines;

  gimp_selection_bounds(image_ID,&has_sel,&sel_x1, &sel_y1, &sel_x2, &sel_y2);

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

  gimp_pixel_rgn_init(&selection_rgn,sel_drawable,sel_x1,sel_y1,sel_width,sel_height,FALSE,FALSE);

  gimp_tile_cache_ntiles(2 * (sel_drawable->width + gimp_tile_width() - 1) / gimp_tile_width());

  olt = find_outline_pixels();

  splines = fitted_splines (olt);

  gimp_selection_none(image_ID);

  gimp_displays_flush();

  do_points(splines,image_ID);

  return TRUE;
}

static gint
gimp_selection_bounds (gint32  image_ID,
		       gint   *has_sel,
		       gint   *x1,
		       gint   *y1,
		       gint   *x2,
		       gint   *y2)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

  return_vals = gimp_run_procedure ("gimp_selection_bounds",
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_ID,
                                    PARAM_END);
  result = FALSE;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      result = TRUE;
      *has_sel = return_vals[1].data.d_int32; 
      *x1 = return_vals[2].data.d_int32;
      *y1 = return_vals[3].data.d_int32;
      *x2 = return_vals[4].data.d_int32;
      *y2 = return_vals[5].data.d_int32;
    }
  
  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

static gint
gimp_path_set_points (gint32  image_ID,
		      gchar  *name,
		      gint    ptype,
		      gint    num_path_points,
		      gdouble *point_pairs)
{
  GParam *return_vals;
  int nreturn_vals;
  int result;

#if 0
  int count;

  for(count = 0; count < num_path_points; count++)
    {
      printf("[%d] %g\n",count,point_pairs[count]);
    }
#endif /* 0 */

  return_vals = gimp_run_procedure ("gimp_path_set_points",
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_ID,
				    PARAM_STRING, name,
				    PARAM_INT32, ptype,
				    PARAM_INT32, num_path_points,
				    PARAM_FLOATARRAY, point_pairs,
                                    PARAM_END);
  result = FALSE;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      result = TRUE;
    }
  
  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

static gint
gimp_selection_is_empty (gint32  image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint result;

  return_vals = gimp_run_procedure ("gimp_selection_is_empty",
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_ID,
                                    PARAM_END);
  result = FALSE;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      result = return_vals[1].data.d_int32; 
    }
  
  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

static gint
gimp_selection_none (gint32  image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint result;

  return_vals = gimp_run_procedure ("gimp_selection_none",
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_ID,
                                    PARAM_END);
  result = FALSE;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      result = return_vals[1].data.d_int32; 
    }
  
  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

void
safe_free (address *item)
{
  if (item == NULL || *item == NULL)
    {
      g_warning ("safe_free: Attempt to free a null item.");
      abort ();
    }
  
  free (*item);
  
  *item = NULL;
}
