/*  
 *  ScreenShot plug-in v0.6 by Sven Neumann, neumanns@uni-duesseldorf.de  
 *  1998/04/18
 *
 *  Any suggestions, bug-reports or patches are very welcome.
 * 
 *  This plug-in uses the X-utility xwd to grab an image from the screen
 *  and the xwd-plug-in created by Peter Kirchgessner (pkirchg@aol.com)
 *  to load this image into the gimp.
 *  Hence its nothing but a simple frontend to those utilities.
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
 *  (98/02/18)  v0.1   first development release 
 *  (98/02/19)  v0.2   small bugfix 
 *  (98/03/09)  v0.3   another one
 *  (98/03/13)  v0.4   cosmetic changes to the dialog
 *  (98/04/02)  v0.5   it works non-interactively now and registers
 *                     itself correctly as extension
 *  (98/04/18)  v0.6   cosmetic change to the dialog
 */

#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/* Defines */
#define PLUG_IN_NAME        "extension_screenshot"
#define PLUG_IN_PRINT_NAME  "Screen Shot"
#define PLUG_IN_VERSION     "v0.6 (98/04/18)"
#define PLUG_IN_MENU_PATH   "<Toolbox>/Xtns/Screen Shot"
#define PLUG_IN_AUTHOR      "Sven Neumann (neumanns@uni-duesseldorf.de)"
#define PLUG_IN_COPYRIGHT   "Sven Neumann"
#define PLUG_IN_DESCRIBTION "Create a screenshot of a single window or the whole screen"
#define PLUG_IN_HELP        "This extension serves as a simple frontend to the X-window utility xwd and the xwd-file-plug-in. After specifying some options, xwd is called, the user selects a window, and the resulting image is loaded into the gimp. When called non-interactively it may grab the root window or use the window-id passed as a parameter."

#define NUMBER_IN_ARGS 3
#define IN_ARGS { PARAM_INT32,    "run_mode",  "Interactive, non-interactive" },\
                { PARAM_INT32,    "root",      "Root window { TRUE, FALSE }" },\
                { PARAM_STRING,   "window_id", "Window id" }

#define NUMBER_OUT_ARGS 1 
#define OUT_ARGS { PARAM_IMAGE,   "image",     "Output image" }

#ifndef XWD
#define XWD "xwd"
#endif

typedef struct {
  gint root;
  gchar *window_id;
  gint decor;
} ScreenShotValues;

typedef struct {
  GtkWidget *decor_button;
  GtkWidget *single_button;
  GtkWidget *root_button;
  gint run;
} ScreenShotInterface;

static ScreenShotValues shootvals = 
{ 
  FALSE,    /* default to window */
  NULL,      
  TRUE      /* default to decorations */
};

static ScreenShotInterface shootint =
{
  FALSE	    /* run */
};


static void  query (void);
static void  run (gchar *name,
		  gint nparams,	          /* number of parameters passed in */
		  GParam * param,	  /* parameters passed in */
		  gint *nreturn_vals,     /* number of parameters returned */
		  GParam ** return_vals); /* parameters to be returned */
static void  shoot (void);
static gint  shoot_dialog (void);
static void  shoot_close_callback (GtkWidget *widget,
				   gpointer   data);
static void  shoot_ok_callback (GtkWidget *widget,
				gpointer   data);
static void  shoot_toggle_update (GtkWidget *widget,
				  gpointer   radio_button);
static void  shoot_display_image (gint32 image);

/* Global Variables */
GPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,	  /* quit_proc  */
  query,  /* query_proc */
  run     /* run_proc   */
};

/* the image that will be returned */
gint32    image_ID = -1;

/* Functions */

MAIN ()

static void query (void)
{
  static GParamDef args[] = { IN_ARGS };
  static gint nargs = NUMBER_IN_ARGS;
  static GParamDef return_vals[] = { OUT_ARGS };
  static gint nreturn_vals = NUMBER_OUT_ARGS;

  /* the actual installation of the plugin */
  gimp_install_procedure (PLUG_IN_NAME,
			  PLUG_IN_DESCRIBTION,
			  PLUG_IN_HELP,
			  PLUG_IN_AUTHOR,
			  PLUG_IN_COPYRIGHT,
			  PLUG_IN_VERSION,
			  PLUG_IN_MENU_PATH,
			  NULL,
			  PROC_EXTENSION,		
			  nargs,
			  nreturn_vals,
			  args,
			  return_vals);
}

static void 
run (gchar *name,		/* name of plugin */
     gint nparams,		/* number of in-paramters */
     GParam * param,		/* in-parameters */
     gint *nreturn_vals,	/* number of out-parameters */
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
  
  /*how are we running today? */
  switch (run_mode)
  {
    case RUN_INTERACTIVE:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, &shootvals);
      shootvals.window_id = NULL;

      /* Get information from the dialog */
      if (!shoot_dialog())
	return;
      break;

    case RUN_NONINTERACTIVE:
      if (nparams == NUMBER_IN_ARGS)
	{
	  shootvals.root      = (gint) param[1].data.d_int32;
	  shootvals.window_id = (gchar*) param[2].data.d_string;
	  shootvals.decor     = FALSE;
	}
      else
	status = STATUS_CALLING_ERROR;
      break;
     
    case RUN_WITH_LAST_VALS:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, &shootvals);
      break;
    
    default:
      break;
  } /* switch */

  if (status == STATUS_SUCCESS)
  {
    /* Run the main function */
    shoot();
        
    /* Store variable states for next run */
    if (run_mode == RUN_INTERACTIVE)
      gimp_set_data (PLUG_IN_NAME, &shootvals, sizeof (ScreenShotValues));
  }

  status = (image_ID != -1) ? STATUS_SUCCESS : STATUS_EXECUTION_ERROR;

  if (status == STATUS_SUCCESS)
  {
    if (run_mode == RUN_INTERACTIVE)
      {
	/* display the image */
	shoot_display_image (image_ID);
      }
    /* set return values */
    *nreturn_vals = 2;
    values[1].type = PARAM_IMAGE;
    values[1].data.d_image = image_ID;
  }
  values[0].data.d_status = status; 
}


/* The main ScreenShot function */
static void 
shoot (void)
{
  GParam *params;
  gint retvals;
  char *tmpname;
  char *xwdargv[7]; /* only need a maximum of 7 arguments to xwd */
  gint pid;
  gint status;
  gint i = 0;

  /* get a temp name with the right extension and save into it. */
  params = gimp_run_procedure ("gimp_temp_name",
			       &retvals,
			       PARAM_STRING, "xwd",
			       PARAM_END);
  tmpname = params[1].data.d_string;

  /* construct the xwd arguments */
  xwdargv[i++] = XWD;
  xwdargv[i++] = "-out";
  xwdargv[i++] = tmpname;
  if ( shootvals.root == TRUE )
    xwdargv[i++] = "-root";
  else 
    {
      if (shootvals.decor == TRUE )
	xwdargv[i++] = "-frame";
      if (shootvals.window_id != NULL)
	{
	  xwdargv[i++] = "-id";
	  xwdargv[i++] = shootvals.window_id;
	}
    }
  xwdargv[i] = NULL;
  
  /* fork off a xwd process */
  if ((pid = fork ()) < 0)
    {
      g_warning ("screenshot: fork failed: %s\n", g_strerror (errno));
      return;
    }
  else if (pid == 0)
    {
      execvp (XWD, xwdargv);
      /* What are we doing here? exec must have failed */
      g_warning ("screenshot: exec failed: xwd: %s\n", g_strerror (errno));
      return;
    }
  else
    {
      waitpid (pid, &status, 0);
	      
      if (!WIFEXITED (status))
	{
	  g_warning ("screenshot: xwd didn't work\n");
	  return;
	}
    }

  /* now load the tmpfile using the xwd-plug-in */
  params = gimp_run_procedure ("file_xwd_load",
			       &retvals,
			       PARAM_INT32, 1,
			       PARAM_STRING, tmpname,
			       PARAM_STRING, tmpname,
			       PARAM_END);
  image_ID = params[1].data.d_image;

  /* get rid of the tmpfile */
  unlink (tmpname);

  return;
}


/* ScreenShot dialog */

static gint 
shoot_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *radio_label;
  GSList    *radio_group = NULL;
  gint radio_pressed[2];
  gint decorations;
  gint argc = 1;
  gchar **argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("ScreenShot");

  radio_pressed[0] = (shootvals.root == FALSE);
  radio_pressed[1] = (shootvals.root == TRUE);
  decorations = shootvals.decor;

  /* Init GTK  */
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gdk_set_use_xshm (gimp_use_xshm ());

  /* Main Dialog */
  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), PLUG_IN_PRINT_NAME);
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      (GtkSignalFunc) shoot_close_callback,
		      NULL);
  /*  Action area  */
  button = gtk_button_new_with_label ("Grab");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) shoot_ok_callback,
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

  /*  Single Window */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
		      frame, TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), 
		      hbox, TRUE, TRUE, 0);
  shootint.single_button = gtk_radio_button_new ( radio_group );
  radio_group = gtk_radio_button_group ( GTK_RADIO_BUTTON (shootint.single_button) );  
  gtk_box_pack_start (GTK_BOX (hbox),
		      shootint.single_button, TRUE, TRUE, 0);
  gtk_signal_connect ( GTK_OBJECT (shootint.single_button), "toggled",
		       (GtkSignalFunc) shoot_toggle_update,
		       &radio_pressed[0]); 
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (shootint.single_button), 
				   radio_pressed[0]);
  gtk_widget_show (shootint.single_button);
  radio_label = gtk_label_new ( "Grab a single window" );
  gtk_box_pack_start (GTK_BOX (hbox),
		      radio_label, TRUE, TRUE, 0);
  gtk_widget_show (radio_label);
  gtk_widget_show (hbox);

  /* with decorations */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  shootint.decor_button = gtk_check_button_new_with_label ("Include decorations");
  gtk_signal_connect (GTK_OBJECT (shootint.decor_button), "toggled",
                      (GtkSignalFunc) shoot_toggle_update,
                      &decorations);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (shootint.decor_button), 
			       decorations);
  gtk_box_pack_end (GTK_BOX (hbox), shootint.decor_button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (shootint.decor_button, radio_pressed[0]);
  gtk_widget_show (shootint.decor_button);
  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* Root Window */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
		      frame, TRUE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  shootint.root_button = gtk_radio_button_new ( radio_group );
  radio_group = gtk_radio_button_group ( GTK_RADIO_BUTTON (shootint.root_button) );  
  gtk_box_pack_start (GTK_BOX (hbox),  shootint.root_button, TRUE, TRUE, 0);
  gtk_signal_connect ( GTK_OBJECT (shootint.root_button), "toggled",
		       (GtkSignalFunc) shoot_toggle_update,
		       &radio_pressed[1]);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (shootint.root_button), 
				   radio_pressed[1]);
  gtk_widget_show (shootint.root_button);

  radio_label = gtk_label_new ( "Grab the whole screen" );
  gtk_box_pack_start (GTK_BOX (hbox), radio_label, TRUE, TRUE, 0);
  gtk_widget_show (radio_label);

  gtk_widget_show (hbox);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  gtk_main ();
  gdk_flush ();

  shootvals.root = radio_pressed[1];
  shootvals.decor = decorations;

  return shootint.run;
}


/*  ScreenShot interface functions  */

static void
shoot_close_callback (GtkWidget *widget,
		       gpointer   data)
{
  gtk_main_quit ();
}

static void
shoot_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  shootint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
shoot_toggle_update (GtkWidget *widget,
		     gpointer   radio_button)
{
  gint *toggle_val;

  toggle_val = (gint *) radio_button;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;    
  else
    *toggle_val = FALSE;

  if (widget == shootint.single_button)
      gtk_widget_set_sensitive (shootint.decor_button, *toggle_val);
}

/* Display function */

void
shoot_display_image (gint32 image)
{
  GParam *params;
  gint retvals;
 
  params = gimp_run_procedure ("gimp_display_new",
			       &retvals,
			       PARAM_IMAGE, image,
			       PARAM_END);
}





