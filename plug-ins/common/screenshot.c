/*  
 *  ScreenShot plug-in
 *  Copyright 1998-2000 Sven Neumann <sven@gimp.org>
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
  gboolean   root;
  gchar     *window_id;
  guint      delay;
  gboolean   decor;
} ScreenShotValues;

static ScreenShotValues shootvals = 
{ 
  FALSE,     /* root window */
  NULL,      /* window ID */
  0,         /* delay */
  TRUE,      /* decorations */
};


static void      query (void);
static void      run   (gchar      *name,
			gint        nparams,
			GimpParam  *param, 
			gint       *nreturn_vals,
			GimpParam **return_vals);

static void      shoot                (void);
static gboolean  shoot_dialog         (void);
static void      shoot_ok_callback    (GtkWidget *widget, 
				       gpointer   data);
static void      shoot_delay          (gint32     delay);
static gboolean  shoot_delay_callback (gpointer   data);


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

gboolean  run_flag = FALSE;


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

  static GimpParamDef return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

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
			  "1998 - 2000",
			  "v0.9.5 (2000/10/29)",
			  N_("<Toolbox>/File/Acquire/Screen Shot..."),
			  NULL,
			  GIMP_EXTENSION,		
			  G_N_ELEMENTS (args),
                          G_N_ELEMENTS (return_vals),
			  args, return_vals);
}

static void 
run (gchar      *name,
     gint        nparams,
     GimpParam  *param,
     gint       *nreturn_vals,
     GimpParam **return_vals)
{
  /* Get the runmode from the in-parameters */
  GimpRunMode run_mode = param[0].data.d_int32;	

  /* status variable, use it to check for errors in invocation usualy only 
   * during non-interactive calling
   */
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;	 

  /* always return at least the status to the caller. */
  static GimpParam values[2];

  /* initialize the return of the status */ 	
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  *nreturn_vals = 1;
  *return_vals  = values;

  INIT_I18N ();

  /* how are we running today? */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, &shootvals);
      shootvals.window_id = NULL;

     /* Get information from the dialog */
      if (!shoot_dialog ())
	status = GIMP_PDB_EXECUTION_ERROR;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams == 3) 
	{
	  shootvals.root      = param[1].data.d_int32;
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

      status = (image_ID != -1) ? GIMP_PDB_SUCCESS : GIMP_PDB_EXECUTION_ERROR;
    }

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
  GimpParam *params;
  gint       retvals;
  gchar     *tmpname;
  gchar     *xwdargv[7];    /*  need a maximum of 7 arguments to xwd  */
  gdouble    xres, yres;
  gint       pid;
  gint       wpid;
  gint       status;
  gint       i = 0;

  /*  get a temp name with the right extension  */
  tmpname = gimp_temp_name ("xwd");

  /*  construct the xwd arguments  */
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
  /*  fork off a xwd process  */
  if ((pid = fork ()) < 0)
    {
      g_message ("screenshot: fork failed: %s\n", g_strerror (errno));
      return;
    }
  else if (pid == 0)
    {
      execvp (XWD, xwdargv);
      /*  What are we doing here? exec must have failed  */
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
      status = -1;
      wpid = waitpid (pid, &status, 0);

      if ((wpid < 0) || !WIFEXITED (status))
	{
	  /*  the tmpfile may have been created even if xwd failed  */
	  unlink (tmpname);
	  g_free (tmpname);
	  g_message ("screenshot: xwd didn't work\n");
	  return;
	}
    }

  /*  now load the tmpfile using the xwd-plug-in  */
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

  /*  get rid of the tmpfile  */
  unlink (tmpname);
  g_free (tmpname);

  if (image_ID != -1)
    {
      GimpParasite *parasite;
      gchar *comment;
      
      /*  figure out the monitor resolution and set the image to it  */
      gimp_get_monitor_resolution (&xres, &yres);      
      gimp_image_set_resolution (image_ID, xres, yres);

      /*  unset the image filename  */
      gimp_image_set_filename (image_ID, "");

      
      /* Set the default comment parasite */
      comment = gimp_get_default_comment ();
      
      if (comment != NULL)
        {
          parasite = gimp_parasite_new ("gimp-comment", 
                                        GIMP_PARASITE_PERSISTENT,
                                        g_utf8_strlen (comment, -1) + 1,
                                        comment);

          gimp_image_parasite_attach (image_ID, parasite);
          gimp_parasite_free(parasite);
        }
    }
  
  return;
}


/*  ScreenShot dialog  */

static void
shoot_ok_callback (GtkWidget *widget, 
		   gpointer   data)
{
  run_flag = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static gboolean
shoot_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *decor_button;
  GtkWidget *spinner;
  GtkWidget *sep;
  GSList    *radio_group = NULL;
  GtkObject *adj;


  gimp_ui_init ("screenshot", FALSE);

  /*  main dialog  */
  dialog = gimp_dialog_new (_("Screen Shot"), "screenshot",
			    gimp_standard_help_func, "filters/screenshot.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    GTK_STOCK_CANCEL, gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,
			    GTK_STOCK_OK, shoot_ok_callback,
			    NULL, NULL, NULL, TRUE, FALSE,

			    NULL);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), main_vbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /*  single window  */
  frame = gtk_frame_new (_("Grab"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  button = gtk_radio_button_new_with_mnemonic (radio_group, 
					       _("_Single Window"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), ! shootvals.root);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (FALSE));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_radio_button_update),
                    &shootvals.root);

  /*  with decorations  */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  decor_button =
    gtk_check_button_new_with_mnemonic (_("With _Decorations"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (decor_button), 
				shootvals.decor);
  gtk_box_pack_end (GTK_BOX (hbox), decor_button, FALSE, FALSE, 0);
  gtk_widget_show (decor_button);

  g_object_set_data (G_OBJECT (button), "set_sensitive", decor_button);

  g_signal_connect (decor_button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &shootvals.decor);

  gtk_widget_show (hbox);

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

  /*  root window  */
  button = gtk_radio_button_new_with_mnemonic (radio_group,
					       _("_Whole Screen"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), shootvals.root);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (TRUE));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_radio_button_update),
                    &shootvals.root);

  gtk_widget_show (button);
  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /*  with delay  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new_with_mnemonic (_("_after"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
 
  adj = gtk_adjustment_new (shootvals.delay, 0.0, 100.0, 1.0, 5.0, 0.0);
  spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_widget_show (spinner);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update), 
                    &shootvals.delay);

  label = gtk_label_new (_("Seconds Delay"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox);
  gtk_widget_show (dialog);

  gtk_main ();

  return run_flag;
}


/*  delay functions  */
void
shoot_delay (gint delay)
{
  g_timeout_add (1000, shoot_delay_callback, &delay);
  gtk_main ();
}

gboolean
shoot_delay_callback (gpointer data)
{
  gint *seconds_left = (gint *)data;

  (*seconds_left)--;

  if (!*seconds_left) 
    gtk_main_quit();

  return *seconds_left;
}
