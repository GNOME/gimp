/*
 *  ScreenShot plug-in
 *  Copyright 1998-2000 Sven Neumann <sven@gimp.org>
 *  Copyright 2003      Henrik Brix Andersen <brix@gimp.org>
 *
 *  Any suggestions, bug-reports or patches are very welcome.
 *
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

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#if defined(GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined(GDK_WINDOWING_WIN32)
#include <windows.h>
#endif

#include "libgimp/stdplugins-intl.h"


/* Defines */
#define PLUG_IN_NAME "plug_in_screenshot"

#ifdef __GNUC__
#ifdef GDK_NATIVE_WINDOW_POINTER
#if GLIB_SIZEOF_VOID_P != 4
#warning window_id does not fit in PDB_INT32
#endif
#endif
#endif

typedef struct
{
  gboolean         root;
  guint            window_id;
  guint            delay;
} ScreenShotValues;

static ScreenShotValues shootvals =
{
  FALSE,     /* root window */
  0,         /* window ID   */
  0,         /* delay       */
};


static void      query (void);
static void      run   (const gchar      *name,
			gint              nparams,
			const GimpParam  *param,
			gint             *nreturn_vals,
			GimpParam       **return_vals);

static GdkNativeWindow select_window  (const GdkScreen *screen);
static gint32          create_image   (const GdkPixbuf *pixbuf);

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
gint32     image_ID = -1;

gboolean   run_flag = FALSE;

/* the screen on which we are running */
GdkScreen *cur_screen = NULL;

/* the window the user selected */
GdkNativeWindow selected_native;

/* Functions */

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode",  "Interactive, non-interactive" },
    { GIMP_PDB_INT32, "root",      "Root window { TRUE, FALSE }" },
    { GIMP_PDB_INT32, "window_id", "Window id" }
  };

  static GimpParamDef return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  gimp_install_procedure (PLUG_IN_NAME,
			  "Creates a screenshot of a single window or the whole screen",
                          "After specifying some options the user selects a window and "
                          "a time out is started. At the end of the time out the window "
                          "is grabbed and the image is loaded into The GIMP. Alternatively "
                          "the whole screen can be grabbed. When called non-interactively "
                          "it may grab the root window or use the window-id passed as a parameter.",
			  "Sven Neumann <sven@gimp.org>, Henrik Brix Andersen <brix@gimp.org>",
			  "1998 - 2003",
			  "v0.9.6 (2003/08/28)",
			  N_("<Toolbox>/File/Acquire/_Screen Shot..."),
			  NULL,
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args),
                          G_N_ELEMENTS (return_vals),
			  args, return_vals);
}

static void
run (const gchar      *name,
     gint             nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  /* Get the runmode from the in-parameters */
  GimpRunMode run_mode = param[0].data.d_int32;

  /* status variable, use it to check for errors in invocation usually only
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
      shootvals.window_id = 0;

     /* Get information from the dialog */
      if (!shoot_dialog ())
	status = GIMP_PDB_EXECUTION_ERROR;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams == 3)
	{
	  shootvals.root      = param[1].data.d_int32;
	  shootvals.window_id = param[2].data.d_int32;
	  shootvals.delay     = 0;
	}
      else
	status = GIMP_PDB_CALLING_ERROR;

      if (!gdk_init_check (0, NULL))
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

/* Allow the user to select a window with the mouse */

static GdkNativeWindow
select_window (const GdkScreen *screen)
{
#if defined(GDK_WINDOWING_X11)
  /* X11 specific code */

#define MASK (ButtonPressMask | ButtonReleaseMask)

  Display    *x_dpy;
  Cursor      x_cursor;
  XEvent      x_event;
  Window      x_win;
  Window      x_root;
  gint        x_scr;
  gint        status;
  gint        buttons;

  x_dpy = GDK_SCREEN_XDISPLAY (GDK_SCREEN (screen));
  x_scr = GDK_SCREEN_XNUMBER (GDK_SCREEN (screen));

  x_win    = None;
  x_root   = RootWindow (x_dpy, x_scr);
  x_cursor = XCreateFontCursor (x_dpy, GDK_CROSSHAIR);
  buttons  = 0;

  status = XGrabPointer (x_dpy, x_root, False,
                         MASK, GrabModeSync, GrabModeAsync,
                         x_root, x_cursor, CurrentTime);

  if (status != GrabSuccess)
    {
      g_message (_("Error grabbing the pointer"));
      return 0;
    }

  while ((x_win == None) || (buttons != 0))
    {
      XAllowEvents (x_dpy, SyncPointer, CurrentTime);
      XWindowEvent (x_dpy, x_root, MASK, &x_event);

      switch (x_event.type)
        {
        case ButtonPress:
          if (x_win == None)
            {
              x_win = x_event.xbutton.subwindow;
              if (x_win == None)
                x_win = x_root;
            }
          buttons++;
          break;

        case ButtonRelease:
          if (buttons > 0)
            buttons--;
          break;

        default:
          g_assert_not_reached ();
        }
    }

  XUngrabPointer (x_dpy, CurrentTime);
  XFreeCursor (x_dpy, x_cursor);

  return x_win;
#elif defined(GDK_WINDOWING_WIN32)
  /* MS Windows specific code goes here (yet to be written) */

  /* basically the code should grab the pointer using a crosshair
     cursor, allow the user to click on a window and return the
     obtained HWND (as a GdkNativeWindow) - for more details consult
     the X11 specific code below */

  /* note to self: take a look at the winsnap plug-in for example
     code */

#warning Win32 screenshot window chooser not implemented yet
  return 0;
#else
#warning screenshot window chooser not implemented yet for this GDB backend
  return 0;
#endif /* GDK_WINDOWING_WIN32 */
}

/* Create a GimpImage from a GdkPixbuf */

static gint32
create_image (const GdkPixbuf *pixbuf)
{
  GimpPixelRgn	pr;
  GimpDrawable *drawable;
  GimpParasite *parasite;
  gint32        image;
  gint32        layer;
  gdouble       xres, yres;
  gchar        *comment;
  gint          width, height;
  gint          rowstride;
  gboolean      status;
  gchar        *buf;
  gint          i;

  width  = gdk_pixbuf_get_width (GDK_PIXBUF (pixbuf));
  height = gdk_pixbuf_get_height (GDK_PIXBUF (pixbuf));

  image = gimp_image_new (width, height, GIMP_RGB);
  layer = gimp_layer_new (image, _("Background"),
                          width, height,
                          GIMP_RGB_IMAGE, 100, GIMP_NORMAL_MODE);

  gimp_image_add_layer (image, layer, 0);

  drawable = gimp_drawable_get (layer);

  gimp_pixel_rgn_init (&pr, drawable,
                       0, 0, width, height,
                       TRUE, FALSE);

  /* copy the contents of the GdkPixbuf to the GimpDrawable */
  rowstride = gdk_pixbuf_get_rowstride (GDK_PIXBUF (pixbuf));
  buf       = gdk_pixbuf_get_pixels (GDK_PIXBUF (pixbuf));
  status    = gimp_progress_init (_("Loading Screen Shot..."));

  for (i = 0; i < height; i++)
    {
      gimp_pixel_rgn_set_row (&pr, buf, 0, i, width);
      buf += rowstride;
      /* update progress every 10 percent */
      if (status && ((i + 1) * 100 / height) % 10 == 0)
        status = gimp_progress_update ((i + 1.0) / height);
    }

  gimp_progress_update (1.0);

  /*  figure out the monitor resolution and set the image to it  */
  gimp_get_monitor_resolution (&xres, &yres);
  gimp_image_set_resolution (image, xres, yres);

  /* Set the default comment parasite */
  comment = gimp_get_default_comment ();

  if (comment)
    {
      parasite = gimp_parasite_new ("gimp-comment",
                                    GIMP_PARASITE_PERSISTENT,
                                    g_utf8_strlen (comment, -1) + 1,
                                    comment);

      gimp_image_parasite_attach (image_ID, parasite);
      gimp_parasite_free (parasite);
      g_free (comment);
    }

  return image;
}

/* The main ScreenShot function */

static void
shoot (void)
{
  GdkWindow    *window;
  GdkPixbuf    *screenshot;
  GdkRectangle  clip;
  GdkPoint      origin;
  gint          screen_w, screen_h;

  /* use default screen if we are running non-interactively */
  if (cur_screen == NULL)
    cur_screen = gdk_screen_get_default ();

  screen_w = gdk_screen_get_width (GDK_SCREEN (cur_screen));
  screen_h = gdk_screen_get_height (GDK_SCREEN (cur_screen));
  clip.x   = 0;
  clip.y   = 0;

  if (shootvals.root)
    {
      /* entire screen */
      window = gdk_screen_get_root_window (GDK_SCREEN (cur_screen));
    }
  else
    {
      /* single window */
      if (shootvals.window_id)
        {
          window = gdk_window_foreign_new (shootvals.window_id);
        }
      else
        {
          window = gdk_window_foreign_new (selected_native);
        }
    }

  if (!window)
    {
      g_message (_("Specified window not found"));
      return;
    }

  gdk_drawable_get_size (GDK_WINDOW (window), &clip.width, &clip.height);
  gdk_window_get_origin (GDK_WINDOW (window), &origin.x, &origin.y);

  /* do clipping */
  if (origin.x < 0)
    {
      clip.x = -origin.x;
      clip.width += origin.x;
    }
  if (origin.y < 0)
    {
      clip.y = -origin.y;
      clip.height += origin.y;
    }
  if (origin.x + clip.width > screen_w)
    clip.width -= origin.x + clip.width - screen_w;
  if (origin.y + clip.height > screen_h)
    clip.height -= origin.y + clip.height - screen_h;

  screenshot = gdk_pixbuf_get_from_drawable (NULL, GDK_WINDOW (window),
                                             NULL, clip.x, clip.y, 0, 0,
                                             clip.width, clip.height);

  gdk_display_beep (gdk_screen_get_display (GDK_SCREEN (cur_screen)));
  gdk_flush ();

  if (!screenshot)
    {
      g_message (_("Error obtaining screenshot"));
      return;
    }

  image_ID = create_image (GDK_PIXBUF (screenshot));
}

/*  ScreenShot dialog  */

static void
shoot_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  run_flag = TRUE;

  /* get the screen on which we are running */
  cur_screen = gtk_widget_get_screen (GTK_WIDGET (widget));

  gtk_widget_destroy (GTK_WIDGET (data));

  if (!shootvals.root && !shootvals.window_id)
    selected_native = select_window (GDK_SCREEN (cur_screen));
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
  GtkWidget *spinner;
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
					       _("a _Single Window"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), ! shootvals.root);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (FALSE));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_radio_button_update),
                    &shootvals.root);

  /*  root window  */
  button = gtk_radio_button_new_with_mnemonic (radio_group,
					       _("the _Whole Screen"));
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
  gint *seconds_left = data;

  (*seconds_left)--;

  if (!*seconds_left)
    gtk_main_quit ();

  return *seconds_left;
}
