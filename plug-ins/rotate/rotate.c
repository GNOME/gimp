/*  
 *  Rotate plug-in v0.7 by Sven Neumann, neumanns@uni-duesseldorf.de  
 *  1998/05/28
 *
 *  Any suggestions, bug-reports or patches are very welcome.
 * 
 *  A lot of changes in version 0.3 were inspired by (or even simply 
 *  copied from) the similar rotators-plug-in by Adam D. Moss. 
 *
 *  As I still prefer my version I have not stopped developing it.
 *  Probably this will result in one plug-in that includes the advantages 
 *  of both approaches.
 */

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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Revision history
 *  (09/28/97)  v0.1   first development release 
 *  (09/29/97)  v0.2   nicer dialog,
 *                     changed the menu-location to Filters/Transforms
 *  (10/01/97)  v0.3   now handles layered images and undo
 *  (10/13/97)  v0.3a  small bugfix, no real changes
 *  (10/17/97)  v0.4   now handles selections
 *  (01/09/98)  v0.5   a few fixes to support portability
 *  (01/15/98)  v0.6   fixed a line that caused rotate to crash on some 
 *                     systems               
 *  (05/28/98)  v0.7   use the new gimp_message function for error output
 */

/* TODO List
 *  - handle channels and masks
 *  - rewrite the main function to make it work on tiles rather than
 *    process the image row by row. This should result in a significant
 *    speedup (thanks to quartic for this suggestion). 
 */  

#include <gtk/gtk.h>
#include <libgimp/gimp.h>

/* Defines */
#define PLUG_IN_NAME        "plug_in_rotate"
#define PLUG_IN_PRINT_NAME  "Rotate"
#define PLUG_IN_VERSION     "v0.6 (01/15/98)"
#define PLUG_IN_MENU_PATH   "<Image>/Image/Transforms/Rotate"
#define PLUG_IN_IMAGE_TYPES "RGB*, INDEXED*, GRAY*"
#define PLUG_IN_AUTHOR      "Sven Neumann (neumanns@uni-duesseldorf.de)"
#define PLUG_IN_COPYRIGHT   "Sven Neumann"
#define PLUG_IN_DESCRIBTION "Rotates a layer or the whole image by 90, 180 or 270 degrees"
#define PLUG_IN_HELP        "This plug-in does rotate the active layer or the whole image clockwise by multiples of 90 degrees. When the whole image is choosen, the image is resized if necessary."

#define NUMBER_IN_ARGS 5
#define IN_ARGS { PARAM_INT32,    "run_mode", "Interactive, non-interactive"},\
		{ PARAM_IMAGE,    "image", "Input image" },\
		{ PARAM_DRAWABLE, "drawable", "Input drawable"},\
                { PARAM_INT32,    "angle", "Angle { 90 (1), 180 (2), 270 (3) } degrees"},\
                { PARAM_INT32,    "everything", "Rotate the whole image? { TRUE, FALSE }"}

#define NUMBER_OUT_ARGS 0 
#define OUT_ARGS NULL

#define NUM_ANGLES 4

gchar *angle_label[NUM_ANGLES] = { "0", "90", "180", "270" };

typedef struct {
  gint angle;
  gint everything;
} RotateValues;

typedef struct {
  gint run;
} RotateInterface;

static RotateValues rotvals = 
{ 
  1,        /* default to 90 degrees */
  1         /* default to whole image */
};

static RotateInterface rotint =
{
  FALSE	    /* run */
};


static void  query (void);
static void  run (gchar *name,
		  gint nparams,	          /* number of parameters passed in */
		  GParam * param,	  /* parameters passed in */
		  gint *nreturn_vals,      /* number of parameters returned */
		  GParam ** return_vals); /* parameters to be returned */
static void  rotate (void);
static void  rotate_drawable (GDrawable *drawable);
static void  rotate_compute_offsets (gint *offsetx, 
				     gint *offsety, 
				     gint image_width, gint image_height,
				     gint width, gint height);
static gint  rotate_dialog (void);
static void  rotate_close_callback (GtkWidget *widget,
				    gpointer   data);
static void  rotate_ok_callback (GtkWidget *widget,
				 gpointer   data);
static void  rotate_toggle_update (GtkWidget *widget,
				   gpointer   data);
gint32 my_gimp_selection_float (gint32 image_ID, gint32 drawable_ID);
gint32 my_gimp_selection_is_empty (gint32 image_ID);

/* Global Variables */
GPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,	  /* quit_proc  */
  query,  /* query_proc */
  run     /* run_proc   */
};

/* the image and drawable that will be used later */
GDrawable *active_drawable = NULL;
gint32    image_ID = -1;

/* Functions */

MAIN ()

static void query (void)
{

static GParamDef args[] = { IN_ARGS };
static gint nargs = NUMBER_IN_ARGS;
static GParamDef *return_vals = OUT_ARGS;
static gint nreturn_vals = NUMBER_OUT_ARGS;

/* the actual installation of the plugin */
gimp_install_procedure (PLUG_IN_NAME,
			PLUG_IN_DESCRIBTION,
			PLUG_IN_HELP,
			PLUG_IN_AUTHOR,
			PLUG_IN_COPYRIGHT,
			PLUG_IN_VERSION,
			PLUG_IN_MENU_PATH,
			PLUG_IN_IMAGE_TYPES,
			PROC_PLUG_IN,		
			nargs,
			nreturn_vals,
			args,
			return_vals);
}

static void 
run (gchar *name,		/* name of plugin */
     gint nparams,		/* number of in-paramters */
     GParam * param,		/* in-parameters */
     gint *nreturn_vals,		/* number of out-parameters */
     GParam ** return_vals)	/* out-parameters */
{

  /* Get the runmode from the in-parameters */
  GRunModeType run_mode = param[0].data.d_int32;	
  
  /* status variable, use it to check for errors in invocation usualy only 
     during non-interactive calling */	
  GStatusType status = STATUS_SUCCESS;	 
  
  /*always return at least the status to the caller. */
  static GParam values[1];
  
  /* initialize the return of the status */ 	
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  *nreturn_vals = 1;
  *return_vals = values;

  /* get image and drawable */
  image_ID = param[1].data.d_int32;
  active_drawable = gimp_drawable_get (param[2].data.d_drawable);
  
  /*how are we running today? */
  switch (run_mode)
  {
    case RUN_INTERACTIVE:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, &rotvals);
      rotvals.angle = rotvals.angle % NUM_ANGLES;

      /* Get information from the dialog */
      if (!rotate_dialog())
		return;
      break;

    case RUN_NONINTERACTIVE:
      /* check to see if invoked with the correct number of parameters */
      if (nparams == NUMBER_IN_ARGS)
	{
	  rotvals.angle = (gint) param[3].data.d_int32;
	  rotvals.angle = rotvals.angle % NUM_ANGLES;
	  rotvals.everything = (gint) param[4].data.d_int32;
	}
      else
	status = STATUS_CALLING_ERROR;
      break;
      
    case RUN_WITH_LAST_VALS:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, &rotvals);
      rotvals.angle = rotvals.angle % NUM_ANGLES;
      break;
    
    default:
      break;
  } /* switch */

  if (status == STATUS_SUCCESS)
  {
    /* Set the number of tiles you want to cache */
    /* gimp_tile_cache_ntiles ( ); */

    /* Run the main function */
    rotate();
    
    /* If run mode is interactive, flush displays, else (script) don't 
       do it, as the screen updates would make the scripts slow */
    if (run_mode != RUN_NONINTERACTIVE)
      gimp_displays_flush ();
    
    /* Store variable states for next run */
    if (run_mode == RUN_INTERACTIVE)
      gimp_set_data (PLUG_IN_NAME, &rotvals, sizeof (RotateValues));
  }
  values[0].data.d_status = status; 
}


/* Some helper functions */

gint32
my_gimp_selection_is_empty (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 is_empty;

  /* initialize */

  is_empty = 0;
  
  return_vals = gimp_run_procedure ("gimp_selection_is_empty",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    is_empty = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return is_empty;
}

gint32
my_gimp_selection_float (gint32 image_ID, gint32 drawable_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 layer_ID;

  return_vals = gimp_run_procedure ("gimp_selection_float",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_DRAWABLE, drawable_ID, 
				    PARAM_INT32, 0, 
				    PARAM_INT32, 0,
				    PARAM_END);

  layer_ID= 0;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    layer_ID = return_vals[1].data.d_layer;

  gimp_destroy_params (return_vals, nreturn_vals);

  return layer_ID;
}

static void
rotate_compute_offsets (gint* offsetx,
			gint* offsety,
			gint image_width, gint image_height,
			gint width, gint height)
{
  gint buffer;

  if (rotvals.everything)  /* rotate around the image center */
    {
      switch ( rotvals.angle )
	{
	case 1:   /* 90° */
	  buffer   = *offsetx;
	  *offsetx = image_height - *offsety - height;
	  *offsety = buffer;
	  break;
	case 2:   /* 180° */
	  *offsetx = image_width - *offsetx - width;
	  *offsety = image_height - *offsety - height;
	  break;
	case 3:   /* 270° */
	  buffer   = *offsetx;
	  *offsetx = *offsety;
	  *offsety = image_width - buffer - width;
	}
    }
  else  /* rotate around the drawable center */
    {
      if (rotvals.angle != 2)
	{
	  *offsetx = *offsetx + (width-height)/2 ;
	  *offsety = *offsety + (height-width)/2 ;
	}
    }
  return;
}


static void
rotate_drawable (GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height, longside, bytes;
  gint row, col, byte;
  gint offsetx, offsety;
  gint was_preserve_transparency = FALSE;
  guchar *buffer, *src_row, *dest_row;

  /* initialize */

  row = 0;

  /* Get the size of the input drawable. */
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  if (gimp_layer_get_preserve_transparency(drawable->id))
    {
	was_preserve_transparency = TRUE;
	gimp_layer_set_preserve_transparency ( drawable->id,FALSE );
    }

  if (rotvals.angle == 2)  /* we're rotating by 180° */
    { 
      if ( !gimp_drawable_layer(drawable->id) ) exit;
      /* not a layer, probably a channel, abort operation */

      gimp_tile_cache_ntiles ( 2*((width/gimp_tile_width())+1) );

      gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, 
			   FALSE, FALSE);
      gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height,
			   TRUE, TRUE);
      
      src_row  = (guchar *) g_malloc (width * bytes);
      dest_row = (guchar *) g_malloc (width * bytes);
      
      for (row = 0; row < height; row++)
	{
	  gimp_pixel_rgn_get_row (&srcPR, src_row, 0, row, width); 
	  for (col = 0; col < width; col++) 
	    { 
	      for (byte = 0; byte < bytes; byte++)
		{
		  dest_row[col * bytes + byte] =  
		    src_row[(width - col - 1) * bytes + byte];
		}
	    } 	    
	  gimp_pixel_rgn_set_row (&destPR, dest_row, 0, (height - row - 1), 
				  width);
	  
	  if ((row % 5) == 0)
	    gimp_progress_update ((double) row / (double) height);
	}
 
      g_free (src_row);
      g_free (dest_row);
 
      gimp_drawable_flush (drawable); 
      gimp_drawable_merge_shadow (drawable->id, TRUE); 
      gimp_drawable_update (drawable->id, 0, 0, width, height); 

    }
  else                     /* we're rotating by 90° or 270° */
    {  
      (width > height) ? (longside = width) : (longside = height);

      if ( gimp_drawable_layer(drawable->id) )
	{
	  gimp_layer_resize(drawable->id, longside, longside, 0, 0);
	  gimp_drawable_flush (drawable);
	  drawable = gimp_drawable_get(drawable->id);
	}
      else
	/* not a layer... probably a channel... abort operation */
	exit;
      
      gimp_tile_cache_ntiles ( ((longside/gimp_tile_width())+1) + 
			       ((longside/gimp_tile_height())+1) );

      gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, longside, longside, 
			   FALSE, FALSE);
      gimp_pixel_rgn_init (&destPR, drawable, 0, 0, longside, longside,
			   TRUE, TRUE);
      
      buffer = g_malloc (longside * bytes);
      
      if (rotvals.angle == 1)     /* we're rotating by 90° */
	{
	  for (row = 0; row < height; row++)
	    {
	      gimp_pixel_rgn_get_row (&srcPR, buffer, 0, row, width); 
	      gimp_pixel_rgn_set_col (&destPR, buffer, (height - row - 1), 0,
				      width);

	      if ((row % 5) == 0)
		gimp_progress_update ((double) row / (double) height);
	    }
	}
      else                        /* we're rotating by 270° */
	{
	  for (col = 0; col < width; col++)
	    {
	      gimp_pixel_rgn_get_col (&srcPR, buffer, col, 0, height); 
	      gimp_pixel_rgn_set_row (&destPR, buffer, 0, (width - col - 1), 
				      height);

	      if ((row % 5) == 0)
		gimp_progress_update ((double) row / (double) height);
	    }
	}
 
      g_free (buffer);

      gimp_progress_update ( 1.0 );

      gimp_drawable_flush (drawable); 
      gimp_drawable_merge_shadow (drawable->id, TRUE);
      gimp_drawable_update (drawable->id, 0, 0, height, width); 

      gimp_layer_resize(drawable->id, height, width, 0, 0);
      drawable = gimp_drawable_get(drawable->id);

      gimp_drawable_flush (drawable);
      gimp_drawable_update (drawable->id, 0, 0, height, width);
    }

  gimp_drawable_offsets (drawable->id, &offsetx, &offsety);
  rotate_compute_offsets (&offsetx, &offsety, 
			  gimp_image_width(image_ID),
			  gimp_image_height(image_ID), 
			  width, height); 
  gimp_layer_set_offsets (drawable->id, offsetx, offsety);

  if (was_preserve_transparency)
    gimp_layer_set_preserve_transparency ( drawable->id, TRUE );

  return;
}


/* The main rotate function */
static void 
rotate (void)
{
  gint nreturn_vals;  
  GDrawable *drawable;
  gint32 *layers;
  gint i, nlayers;

  if (rotvals.angle == 0) return;  

  /* if there's a selection and we try to rotate the whole image */
  /* create an error message and exit */     
  if ( rotvals.everything )
    {
      if ( !my_gimp_selection_is_empty (image_ID) ) 
	{
	  gimp_message("You can not rotate the whole image if there's a selection.");
	  gimp_drawable_detach (active_drawable);
	  return;
	}
      if ( gimp_layer_is_floating_selection (active_drawable->id) ) 
	{
	  gimp_message("You can not rotate the whole image if there's a floating selection.");
	  gimp_drawable_detach (active_drawable);
	  return;
	}
    }
  
  gimp_progress_init ("Rotating...");

  gimp_run_procedure ("gimp_undo_push_group_start", &nreturn_vals,
		      PARAM_IMAGE, image_ID, PARAM_END);

  if (rotvals.everything)  /* rotate the whole image */ 
    {	
      gimp_drawable_detach (active_drawable);
      layers = gimp_image_get_layers (image_ID, &nlayers);
      for ( i=0; i<nlayers; i++ )
	{
	  drawable = gimp_drawable_get (layers[i]);
	  rotate_drawable (drawable);
	  gimp_drawable_detach (drawable);
	}
      g_free(layers);	  
      if (rotvals.angle != 2)  
	{
	  gimp_image_resize (image_ID,
			     gimp_image_height(image_ID),
			     gimp_image_width(image_ID),
			     0, 0);
	}
    }
  else  /* rotate only the active layer */
    {

      /* check for active selection and float it */
      if ( !my_gimp_selection_is_empty (image_ID) &&
	   !gimp_layer_is_floating_selection (active_drawable->id) )
	active_drawable = 
	  gimp_drawable_get (my_gimp_selection_float (image_ID, 
						      active_drawable->id) );
			    
      rotate_drawable (active_drawable);
      gimp_drawable_detach (active_drawable);
    }

  gimp_run_procedure ("gimp_undo_push_group_end", &nreturn_vals,
		      PARAM_IMAGE, image_ID, PARAM_END);
  return;
}



/* Rotate dialog */

static gint 
rotate_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *radio_label; 
  GtkWidget *unit_label; 
  GtkWidget *hbox;
  GtkWidget *radio_button;
  GtkWidget *check_button;
  GSList    *radio_group = NULL;
  gint radio_pressed[NUM_ANGLES];
  gint everything;
  gint argc = 1;
  gint i;
  gchar **argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("Rotate");

  for (i=0;i<NUM_ANGLES;i++)
    {
      radio_pressed[i] = (rotvals.angle == i);
    }
  everything = rotvals.everything;

  /* Init GTK  */
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gdk_set_use_xshm (gimp_use_xshm ());

  /* Main Dialog */
  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), PLUG_IN_PRINT_NAME);
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      (GtkSignalFunc) rotate_close_callback,
		      NULL);
  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) rotate_ok_callback,
                      dialog);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dialog));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new ("Rotate clockwise by");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
		      frame, TRUE, TRUE, 0);

  /* table for radio_buttons */
  table = gtk_table_new (6, 6, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 8);
  gtk_table_set_col_spacings (GTK_TABLE (table), 1);
  gtk_container_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /* radio buttons */

  /* 0 degrees */
  radio_label = gtk_label_new ( angle_label[0] );
  gtk_table_attach ( GTK_TABLE (table), radio_label, 2, 3, 0, 1, 0, 0, 0, 0);
  gtk_widget_show (radio_label);
  radio_button = gtk_radio_button_new ( radio_group );
  radio_group = gtk_radio_button_group ( GTK_RADIO_BUTTON (radio_button) );  
  gtk_table_attach ( GTK_TABLE (table), radio_button, 2, 3, 1, 2, 0, 0, 0, 0);
  gtk_signal_connect ( GTK_OBJECT (radio_button), "toggled",
		       (GtkSignalFunc) rotate_toggle_update,
		       &radio_pressed[0]);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (radio_button), 
				   radio_pressed[0]);
  gtk_widget_show (radio_button);

  /* 90 degrees */  
  radio_label = gtk_label_new ( angle_label[1] );
  gtk_table_attach ( GTK_TABLE (table), radio_label, 4, 5, 2, 3, 0, 0, 0, 0);
  gtk_widget_show (radio_label);
  radio_button = gtk_radio_button_new ( radio_group );
  radio_group = gtk_radio_button_group ( GTK_RADIO_BUTTON (radio_button) );  
  gtk_table_attach ( GTK_TABLE (table), radio_button, 3, 4, 2, 3, 0, 0, 0, 0);
  gtk_signal_connect ( GTK_OBJECT (radio_button), "toggled",
		       (GtkSignalFunc) rotate_toggle_update,
		       &radio_pressed[1]);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (radio_button), 
				   radio_pressed[1]);
  gtk_widget_show (radio_button);

  /* 180 degrees */
  radio_label = gtk_label_new ( angle_label[2] );
  gtk_table_attach ( GTK_TABLE (table), radio_label, 2, 3, 4, 5, 0, 0, 0, 0);
  gtk_widget_show (radio_label);
  radio_button = gtk_radio_button_new ( radio_group );
  radio_group = gtk_radio_button_group ( GTK_RADIO_BUTTON (radio_button) );  
  gtk_table_attach ( GTK_TABLE (table), radio_button, 2, 3, 3, 4, 0, 0, 0, 0);
  gtk_signal_connect ( GTK_OBJECT (radio_button), "toggled",
		       (GtkSignalFunc) rotate_toggle_update,
		       &radio_pressed[2]);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (radio_button), 
				   radio_pressed[2]);
  gtk_widget_show (radio_button);

  /* 270 degrees */ 
  radio_label = gtk_label_new ( angle_label[3] );
  gtk_table_attach ( GTK_TABLE (table), radio_label, 0, 1, 2, 3, 0, 0, 0, 0);
  gtk_widget_show (radio_label);
  radio_button = gtk_radio_button_new ( radio_group );
  radio_group = gtk_radio_button_group ( GTK_RADIO_BUTTON (radio_button) );  
  gtk_table_attach ( GTK_TABLE (table), radio_button, 1, 2, 2, 3, 0, 0, 0, 0);
  gtk_signal_connect ( GTK_OBJECT (radio_button), "toggled",
		       (GtkSignalFunc) rotate_toggle_update,
		       &radio_pressed[3]);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (radio_button), 
				   radio_pressed[3]);
  gtk_widget_show (radio_button);

  /* label: degrees */

  unit_label = gtk_label_new ( "degrees" );
  gtk_table_attach ( GTK_TABLE (table), unit_label, 5, 6, 5, 6, 0, 0, 0, 0);
  gtk_widget_show (unit_label);

 
  gtk_widget_show (table);
  gtk_widget_show (frame);

  /* hbox for check-button */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (hbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
		      hbox, TRUE, TRUE, 0);
  /* check button */
  check_button = gtk_check_button_new_with_label ("Rotate the whole image");
  gtk_box_pack_end (GTK_BOX (hbox), check_button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (check_button), "toggled",
		      (GtkSignalFunc) rotate_toggle_update,
		      &everything);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (check_button), 
			       rotvals.everything);
  gtk_widget_show (check_button);
  gtk_widget_show (hbox);

  gtk_widget_show (dialog);

  gtk_main ();
  gdk_flush ();

  rotvals.angle=0;
  for (i = 0; i < NUM_ANGLES; i++)
    {
      if (radio_pressed[i]==TRUE)
	{
	  rotvals.angle = i;
	  break;
	}
    }
  rotvals.everything = everything;

  return rotint.run;
}


/*  Rotate interface functions  */

static void
rotate_close_callback (GtkWidget *widget,
		       gpointer   data)
{
  gtk_main_quit ();
}

static void
rotate_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  rotint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
rotate_toggle_update (GtkWidget *widget,
		      gpointer   data)
{
  gint *toggle_val;

  toggle_val = (gint *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}






