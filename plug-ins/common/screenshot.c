/*  
 *  ScreenShot plug-in
 *  Copyright 1998-1999 Sven Neumann <sven@gimp.org>
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

#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __EMX__
#include <process.h>
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Defines */
#define PLUG_IN_NAME        "extension_screenshot"

#ifndef XWD
#define XWD "xwd"
#endif

typedef struct
{
  gint   root;
  gchar *window_id;
  guint  delay;
  gint   decor;
} ScreenShotValues;

typedef struct
{
  GtkWidget *decor_button;
  GtkWidget *delay_spinner;
  GtkWidget *single_button;
  GtkWidget *root_button;
  gint       run;
} ScreenShotInterface;

static ScreenShotValues shootvals = 
{ 
  FALSE,     /* root window */
  NULL,      /* window ID */
  0,         /* delay */
  TRUE,      /* decorations */
};

static ScreenShotInterface shootint =
{
  FALSE	    /* run */
};


static void  query (void);
static void  run   (gchar   *name,
		    gint     nparams,      /* number of parameters passed in */
		    GimpParam  *param,        /* parameters passed in */
		    gint    *nreturn_vals, /* number of parameters returned */
		    GimpParam **return_vals); /* parameters to be returned */

static void  shoot                (void);
static gint  shoot_dialog         (void);
static void  shoot_ok_callback    (GtkWidget *widget,
				   gpointer   data);
static void  shoot_toggle_update  (GtkWidget *widget,
				   gpointer   radio_button);
static void  shoot_delay          (gint32     delay);
static gint  shoot_delay_callback (gpointer   data);

/* Global Variables */
GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,	 /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

/* the image that will be returned */
gint32    image_ID = -1;

/* Functions */

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,  "run_mode",  "Interactive, non-interactive" },
    { GIMP_PDB_INT32,  "root",      "Root window { TRUE, FALSE }" },
    { GIMP_PDB_STRING, "window_id", "Window id" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  static GimpParamDef return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };
  static gint nreturn_vals = sizeof (return_vals) / sizeof (return_vals[0]);

  gimp_install_procedure (PLUG_IN_NAME,
			  "Creates a screenshot of a single window or the whole screen",
			  "This extension serves as a simple frontend to the "
			  "X-window utility xwd and the xwd-file-plug-in. "
			  "After specifying some options, xwd is called, the "
			  "user selects a window, and the resulting image is "
			  "loaded into the gimp. Alternatively the whole "
			  "screen can be grabbed. When called non-interactively "
			  "it may grab the root window or use the window-id "
			  "passed as a parameter.",
			  "Sven Neumann <sven@gimp.org>",
			  "1998, 1999",
			  "v0.9.4 (99/12/28)",
			  N_("<Toolbox>/File/Acquire/Screen Shot..."),
			  NULL,
			  GIMP_EXTENSION,		
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void 
run (gchar   *name,
     gint     nparams,
     GimpParam  *param,
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  /* Get the runmode from the in-parameters */
  GimpRunModeType run_mode = param[0].data.d_int32;	

  /* status variable, use it to check for errors in invocation usualy only 
   * during non-interactive calling
   */
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;	 

  /* always return at least the status to the caller. */
  static GimpParam values[1];

  /* initialize the return of the status */ 	
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  *nreturn_vals = 1;
  *return_vals = values;

  /* how are we running today? */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, &shootvals);
      shootvals.window_id = NULL;

      INIT_I18N_UI ();

     /* Get information from the dialog */
      if (!shoot_dialog ())
	return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams == 3) 
	{
	  shootvals.root      = (gint) param[1].data.d_int32;
	  shootvals.window_id = (gchar*) param[2].data.d_string;
	  shootvals.delay     = 0;
	  shootvals.decor     = FALSE;
	}
      else
	status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, &shootvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (shootvals.delay > 0)
	shoot_delay (shootvals.delay);
      /* Run the main function */
      shoot ();
    }

  status = (image_ID != -1) ? GIMP_PDB_SUCCESS : GIMP_PDB_EXECUTION_ERROR;

  if (status == GIMP_PDB_SUCCESS)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
	{
	  /* Store variable states for next run */
	  gimp_set_data (PLUG_IN_NAME, &shootvals, sizeof (ScreenShotValues));
	  /* display the image */
	  gimp_display_new (image_ID);
	}
      /* set return values */
      *nreturn_vals = 2;
      values[1].type = GIMP_PDB_IMAGE;
      values[1].data.d_image = image_ID;
    }
  values[0].data.d_status = status; 
}


/* The main ScreenShot function */
static void 
shoot (void)
{
  GimpParam  *params;
  gint     retvals;
  gchar   *tmpname;
  gchar   *xwdargv[7]; /* only need a maximum of 7 arguments to xwd */
  gdouble  xres, yres;
  gint     pid;
  gint     status;
  gint     i = 0;

  /* get a temp name with the right extension and save into it. */
  tmpname = gimp_temp_name ("xwd");

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
  
#ifndef __EMX__
  /* fork off a xwd process */
  if ((pid = fork ()) < 0)
    {
      g_message ("screenshot: fork failed: %s\n", g_strerror (errno));
      return;
    }
  else if (pid == 0)
    {
      execvp (XWD, xwdargv);
      /* What are we doing here? exec must have failed */
      g_message ("screenshot: exec failed: xwd: %s\n", g_strerror (errno));
      return;
    }
  else
#else /* __EMX__ */
  pid = spawnvp (P_NOWAIT, XWD, xwdargv);
  if (pid == -1)
    {
      g_message ("screenshot: spawn failed: %s\n", g_strerror (errno));
      return;
    }
#endif
    {
      waitpid (pid, &status, 0);
	      
      if (!WIFEXITED (status))
	{
	  g_message ("screenshot: xwd didn't work\n");
	  return;
	}
    }

  /* now load the tmpfile using the xwd-plug-in */
  params = gimp_run_procedure ("file_xwd_load",
			       &retvals,
			       GIMP_PDB_INT32, 1,
			       GIMP_PDB_STRING, tmpname,
			       GIMP_PDB_STRING, tmpname,
			       GIMP_PDB_END);
  if (params[0].data.d_status == GIMP_PDB_SUCCESS)
    {
      image_ID = params[1].data.d_image;
    }
  gimp_destroy_params (params, retvals);

  /* get rid of the tmpfile */
  unlink (tmpname);
  g_free (tmpname);

  if (image_ID != -1)
    {
      /* figure out the monitor resolution and set the image to it */
      gimp_get_monitor_resolution (&xres, &yres);      
      gimp_image_set_resolution (image_ID, xres, yres);

      /* unset the image filename */
      gimp_image_set_filename (image_ID, "");
    }
  
  return;
}


/* ScreenShot dialog */

static gint 
shoot_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GSList    *radio_group = NULL;
  GtkAdjustment *adj;
  gint  radio_pressed[2];
  gint  decorations;
  guint delay;

  radio_pressed[0] = (shootvals.root == FALSE);
  radio_pressed[1] = (shootvals.root == TRUE);
  decorations = shootvals.decor;
  delay = shootvals.delay;

  /* Init GTK  */
  gimp_ui_init ("screenshot", FALSE);

  /* Main Dialog */
  dialog = gimp_dialog_new (_("Screen Shot"), "screenshot",
			    gimp_standard_help_func, "filters/screenshot.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    _("Grab"), shoot_ok_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), main_vbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /*  Single Window */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  shootint.single_button =
    gtk_radio_button_new_with_label (radio_group, _("Grab a Single Window"));
  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (shootint.single_button) );  
  gtk_box_pack_start (GTK_BOX (vbox), shootint.single_button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (shootint.single_button), "toggled",
		      (GtkSignalFunc) shoot_toggle_update,
		      &radio_pressed[0]); 
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shootint.single_button), 
				radio_pressed[0]);
  gtk_widget_show (shootint.single_button);

  /* with decorations */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  shootint.decor_button =
    gtk_check_button_new_with_label (_("Include Decorations"));
  gtk_signal_connect (GTK_OBJECT (shootint.decor_button), "toggled",
                      (GtkSignalFunc) shoot_toggle_update,
                      &decorations);
  gtk_box_pack_end (GTK_BOX (hbox), shootint.decor_button, FALSE, FALSE, 0);
  gtk_widget_show (shootint.decor_button);
  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* Root Window */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  shootint.root_button =
    gtk_radio_button_new_with_label (radio_group, _("Grab the Whole Screen"));
  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (shootint.root_button));  
  gtk_box_pack_start (GTK_BOX (vbox), shootint.root_button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (shootint.root_button), "toggled",
		      (GtkSignalFunc) shoot_toggle_update,
		      &radio_pressed[1]);
  gtk_widget_show (shootint.root_button);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* with delay */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("after"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
 
  adj = (GtkAdjustment *) gtk_adjustment_new ((gfloat)delay, 0.0, 100.0, 1.0, 5.0, 0.0);
  shootint.delay_spinner = gtk_spin_button_new (adj, 0, 0);
  gtk_box_pack_start (GTK_BOX(hbox), shootint.delay_spinner, FALSE, FALSE, 0);
  gtk_widget_show (shootint.delay_spinner);

  label = gtk_label_new (_("Seconds Delay"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shootint.decor_button), 
			       decorations);
  gtk_widget_set_sensitive (shootint.decor_button, radio_pressed[0]);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shootint.root_button), 
				radio_pressed[1]);

  gtk_widget_show (dialog);

  gtk_main ();
  gdk_flush ();

  shootvals.root = radio_pressed[1];
  shootvals.decor = decorations;

  return shootint.run;
}


/*  ScreenShot interface functions  */

static void
shoot_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  shootint.run = TRUE;
  shootvals.delay = 
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON (shootint.delay_spinner));
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
    {
      gtk_widget_set_sensitive (shootint.decor_button, *toggle_val);
    }
  if (widget == shootint.root_button)
    {
      gtk_widget_set_sensitive (shootint.decor_button, !*toggle_val);
    }    
}

/* Delay functions */

void
shoot_delay (gint delay)
{
  gint timeout;

  timeout = gtk_timeout_add (1000, shoot_delay_callback, &delay);
  gtk_main ();
}

gint 
shoot_delay_callback (gpointer data)
{
  gint *seconds_left;
  
  seconds_left = (gint *)data;
  (*seconds_left)--;
  if (!*seconds_left) 
    gtk_main_quit();
  return (*seconds_left);
}

