/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gtk/gtk.h>
#include "libgimp/gimp.h"
#include <unistd.h>

#include "gag.h"

char *gag_library_file;

static GParam	*gag_params;

static void query (void);
static void run (char *name, int nparams, GParam  *param,
                 int *nreturn_vals, GParam **return_vals);

static void gag_job(void);
static void gag_render_picture(GtkWidget *, gpointer);
static void gag_render( GDrawable *drawable, char *expr);

void (*gag_render_picture_ptr)(GtkWidget *, gpointer)= gag_render_picture;


GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};

MAIN()

static void
query (void)
{
  static GParamDef interface_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" }
  };

  static GParamDef render_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_STRING, "expression", "Expression to render" },
  };

  static GParamDef *return_vals = NULL;
  static gint i_nargs = sizeof (interface_args) / sizeof (interface_args[0]);
  static gint r_nargs = sizeof (render_args) / sizeof (interface_args[0]);
  static gint nreturn_vals = 0;

  gimp_install_procedure ("plug_in_gag_interface",
			  "Creates nice patterns using genetic algorithm",
			  "Interactively evolve expressions using crossing and mutating of population of expressions."
			  "Then expressions are rendered into patterns.",
			  "Daniel Skarda",
			  "Daniel Skarda",
			  "1997",
			  "<Image>/Filters/Artistic/GAG",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  i_nargs,
                          nreturn_vals,
			  interface_args,
                          return_vals);

  gimp_install_procedure ("plug_in_gag_render",
			  "Renders GAG-expression",
			  "Renders GAG-expression",
			  "Daniel Skarda",
			  "Daniel Skarda",
			  "1997",
			  "<Image>/Filters/Artistic/GAG",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  r_nargs,
                          nreturn_vals,
			  render_args,
                          return_vals);

}


static void
run (char *name, int nparams, GParam *param, int *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status;

  status = STATUS_SUCCESS;
  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  gag_params= param;

  /*  See how we will run  */

#ifdef GIMP_DEBUG
  printf("GAG pid: attach  %d\n", getpid());
  getchar();
#endif

  if (strcmp(name, "plug_in_gag_interface") == 0)
    {
      switch (run_mode) {
      case RUN_INTERACTIVE:
	/*  Possibly retrieve data  - for NOW NO DATA */
	/*  gimp_get_data("plug_in_gag", &snvals);  */

	/*  Get information from the dialog  */
	gag_job ();
	break;

      case RUN_NONINTERACTIVE:
	/*  Make sure all the arguments are there!  */
	status = STATUS_CALLING_ERROR;
	break;

      case RUN_WITH_LAST_VALS:
	/*  Possibly retrieve data  */
	/* gimp_get_data("plug_in_solid_noise", &snvals); */
	status = STATUS_CALLING_ERROR;
	break;

      default:
	break;
      }
    }
  else if (strcmp(name, "plug_in_gag_render")==0)
    {
      switch (run_mode) {
      case RUN_INTERACTIVE:
	status = STATUS_CALLING_ERROR;
	break;

      case RUN_NONINTERACTIVE:
	if (nparams == 4)
	  {
	    gag_render( 
              gimp_drawable_get (param[2].data.d_drawable),
	      param[3].data.d_string
	    );
	  }
	else status = STATUS_CALLING_ERROR;
	break;

      case RUN_WITH_LAST_VALS:
	/*  Possibly retrieve data  */
	/* gimp_get_data("plug_in_solid_noise", &snvals); */
	status = STATUS_CALLING_ERROR;
	break;

      default:
	break;
      }
    }
  values[0].data.d_status = status;
  gimp_drawable_detach(drawable);
}

static void gag_render_picture(GtkWidget *widget, gpointer data)
{
  char      temp[20000]="0.0";
  gint 	    nreturn_vals;
  INDIVIDUAL *iptr;

  iptr= *((INDIVIDUAL **) data);

  if (iptr->expression != NULL)
    expr_sprint(temp, iptr->expression);

  DPRINT("Launching new gag-plugin");
  gimp_run_procedure("plug_in_gag_render",
		     &nreturn_vals,
		     PARAM_INT32,   RUN_NONINTERACTIVE,
		     PARAM_IMAGE,   gag_params[1].data.d_image,
		     PARAM_DRAWABLE, gag_params[2].data.d_int32,
		     PARAM_STRING,  temp,
		     PARAM_END);
  DPRINT("Launching was successful");
  gimp_displays_flush ();
}

static void gag_job(void)
{
  GParam 	*return_vals;
  gint 		nreturn_vals;

  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("gag");   

  return_vals = gimp_run_procedure ("gimp_gimprc_query",
                                    &nreturn_vals,
                                    PARAM_STRING, "gag-library",
                                    PARAM_END);

  if (return_vals[0].data.d_status != STATUS_SUCCESS || 
      return_vals[1].data.d_string == NULL)
    {
      printf("No gag-library in gimprc: gag-library is set to /dev/null\n");
      gag_library_file= "/dev/null";
    }
  else
    gag_library_file = return_vals[1].data.d_string;


  gag_load_library(gag_library_file);

  expr_init();

  gtk_init (&argc, &argv); 

  gag_create_ui();

  gtk_main();

  g_free( argv[0] );
  g_free( argv );
  gimp_destroy_params (return_vals, nreturn_vals);
}

static void gag_render( GDrawable *drawable, char *expr)
{
  GPixelRgn 	dest_rgn;
  guchar    	*dest_row;
  guchar    	*dest;
  gint 		row, col;
  gint 		progress, max_progress;
  gint 		x1, y1, x2, y2, x, y;
  gpointer 	pr;
  
  DBL		result[4];
  DBL		fy, fdy, fx, fdx;
  int		imtype;

  NODE		*n;

  n= parse_prefix_expression(&expr);
  if (n == NULL) return;
  prepare_node( n );

  gimp_progress_init ("GAG Rendering...");

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  /* Get the foreground and background colors */

  fdy= 2.0 / ((DBL) drawable->height);
  fdx= 2.0 / ((DBL) drawable->width);

  imtype= gimp_drawable_type( drawable->id);

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn); pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      y = dest_rgn.y;

      dest_row = dest_rgn.data;
      fy= drawable->height;
      fy=  (- 2.0 * (DBL)y + fy) /  fy;

      for ( row = 0; row < dest_rgn.h; row++, y++, fy-= fdy)
	{
	  dest = dest_row;
	  x = dest_rgn.x;
	  fx= drawable->width;
	  fx=  (2.0 * (DBL) x - fx) / fx;

	  for (col = 0; col < dest_rgn.w; col++, x++, fx+=fdx)
	    {
	      eval_xy(result,fx,fy);

	      if (imtype < GRAY_IMAGE)
		{ /* RGB* images */
		  DBL  *p= result;

		  *dest= wrap_func(*p); dest++; p++;
		  *dest= wrap_func(*p); dest++; p++;
		  *dest= wrap_func(*p); dest++;
		  if (imtype==RGBA_IMAGE)
		    {
		      p++;
		      *dest= 255; dest++;
		    }
		}
	      else
		{  /* GRAY* images */
		  *dest= wrap_func(result[0]) * 0.299 + wrap_func(result[1]) * 0.587 + wrap_func(result[2]) * 0.114;
		  dest++;
		  if (imtype==GRAYA_IMAGE)
		    {
		      *dest= 255; dest++;
		    }		  
		}
	    }
	  dest_row += dest_rgn.rowstride;
	}
      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
    }

  destroy_node( n );

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

