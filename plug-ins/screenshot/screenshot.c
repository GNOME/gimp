/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Screenshot plug-in
 * Copyright 1998-2007 Sven Neumann <sven@gimp.org>
 * Copyright 2003      Henrik Brix Andersen <brix@gimp.org>
 * Copyright 2012      Simone Karin Lehmann - OS X patches
 * Copyright 2016      Michael Natterer <mitch@gimp.org>
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
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "screenshot.h"
#include "screenshot-gnome-shell.h"
#include "screenshot-icon.h"
#include "screenshot-osx.h"
#include "screenshot-x11.h"
#include "screenshot-win32.h"

#include "libgimp/stdplugins-intl.h"


/* Defines */

#define PLUG_IN_PROC   "plug-in-screenshot"
#define PLUG_IN_BINARY "screenshot"
#define PLUG_IN_ROLE   "gimp-screenshot"

#ifdef __GNUC__
#ifdef GDK_NATIVE_WINDOW_POINTER
#if GLIB_SIZEOF_VOID_P != 4
#warning window_id does not fit in PDB_INT32
#endif
#endif
#endif


static void                query               (void);
static void                run                 (const gchar      *name,
                                                gint              nparams,
                                                const GimpParam  *param,
                                                gint             *nreturn_vals,
                                                GimpParam       **return_vals);

static GimpPDBStatusType   shoot               (GdkScreen        *screen,
                                                gint32           *image_ID,
                                                GError          **error);

static gboolean            shoot_dialog        (GdkScreen       **screen);
static gboolean            shoot_quit_timeout  (gpointer          data);
static gboolean            shoot_delay_timeout (gpointer          data);


/* Global Variables */

static ScreenshotBackend      backend      = SCREENSHOT_BACKEND_NONE;
static ScreenshotCapabilities capabilities = 0;

static ScreenshotValues shootvals =
{
  SHOOT_WINDOW, /* root window  */
  TRUE,         /* include WM decorations */
  0,            /* window ID    */
  0,            /* select delay */
  0,            /* coords of region dragged out by pointer */
  0,
  0,
  0,
  FALSE         /* show cursor */
};

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};


/* Functions */

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run-mode",  "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"     },
    { GIMP_PDB_INT32, "root",      "Root window { TRUE, FALSE }"      },
    { GIMP_PDB_INT32, "window-id", "Window id"                        },
    { GIMP_PDB_INT32, "x1",        "(optional) Region left x coord"   },
    { GIMP_PDB_INT32, "y1",        "(optional) Region top y coord"    },
    { GIMP_PDB_INT32, "x2",        "(optional) Region right x coord"  },
    { GIMP_PDB_INT32, "y2",        "(optional) Region bottom y coord" }
  };

  static const GimpParamDef return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Create an image from an area of the screen"),
                          "The plug-in takes screenshots of an "
                          "interactively selected window or of the desktop, "
                          "either the whole desktop or an interactively "
                          "selected region. When called non-interactively, it "
                          "may grab the root window or use the window-id "
                          "passed as a parameter.  The last four parameters "
                          "are optional and can be used to specify the corners "
                          "of the region to be grabbed."
                          "On Mac OS X or on gnome-shell, "
                          "when called non-interactively, the plugin"
                          "only can take screenshots of the entire root window."
                          "Grabbing a window or a region is not supported"
                          "non-interactively. To grab a region or a particular"
                          "window, you need to use the interactive mode."
                          ,
                          "Sven Neumann <sven@gimp.org>, "
                          "Henrik Brix Andersen <brix@gimp.org>,"
                          "Simone Karin Lehmann",
                          "1998 - 2008",
                          "v1.1 (2008/04)",
                          N_("_Screenshot..."),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args),
                          G_N_ELEMENTS (return_vals),
                          args, return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/File/Create/Acquire");
  gimp_plugin_icon_register (PLUG_IN_PROC, GIMP_ICON_TYPE_INLINE_PIXBUF,
                             screenshot_icon);
}

static void
run (const gchar      *name,
     gint             nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  GdkScreen         *screen = NULL;
  gint32             image_ID;
  GError            *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

#ifdef PLATFORM_OSX
  if (! backend && screenshot_osx_available ())
    {
      backend      = SCREENSHOT_BACKEND_OSX;
      capabilities = screenshot_osx_get_capabilities ();

      /* on OS X, this just means shoot the shadow, default to nope */
      shootvals.decorate = FALSE;
    }
#endif

#ifdef G_OS_WIN32
  if (! backend && screenshot_win32_available ())
    {
      backend      = SCREENSHOT_BACKEND_WIN32;
      capabilities = screenshot_win32_get_capabilities ();
    }
#endif

  if (! backend && screenshot_gnome_shell_available ())
    {
      backend      = SCREENSHOT_BACKEND_GNOME_SHELL;
      capabilities = screenshot_gnome_shell_get_capabilities ();
    }

#ifdef GDK_WINDOWING_X11
  if (! backend && screenshot_x11_available ())
    {
      backend      = SCREENSHOT_BACKEND_X11;
      capabilities = screenshot_x11_get_capabilities ();
    }
#endif

  /* how are we running today? */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_PROC, &shootvals);
      shootvals.window_id = 0;

     /* Get information from the dialog */
      if (! shoot_dialog (&screen))
        status = GIMP_PDB_CANCEL;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams == 3)
        {
          gboolean do_root = param[1].data.d_int32;

          shootvals.shoot_type   = do_root ? SHOOT_ROOT : SHOOT_WINDOW;
          shootvals.window_id    = param[2].data.d_int32;
          shootvals.select_delay = 0;
        }
      else if (nparams == 7)
        {
          shootvals.shoot_type   = SHOOT_REGION;
          shootvals.window_id    = param[2].data.d_int32;
          shootvals.select_delay = 0;
          shootvals.x1           = param[3].data.d_int32;
          shootvals.y1           = param[4].data.d_int32;
          shootvals.x2           = param[5].data.d_int32;
          shootvals.y2           = param[6].data.d_int32;
        }
      else
        {
          status = GIMP_PDB_CALLING_ERROR;
        }

      if (! gdk_init_check (NULL, NULL))
        status = GIMP_PDB_CALLING_ERROR;

      if (! (capabilities & SCREENSHOT_CAN_PICK_NONINTERACTIVELY))
        {
          if (shootvals.shoot_type == SHOOT_WINDOW ||
              shootvals.shoot_type == SHOOT_REGION)
            status = GIMP_PDB_CALLING_ERROR;
        }
        break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_PROC, &shootvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      status = shoot (screen, &image_ID, &error);
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          /* Store variable states for next run */
          gimp_set_data (PLUG_IN_PROC, &shootvals, sizeof (ScreenshotValues));

          gimp_display_new (image_ID);

          /* Give some sort of feedback that the shot is done */
          if (shootvals.select_delay > 0)
            {
              gdk_display_beep (gdk_screen_get_display (screen));
              gdk_flush (); /* flush so the beep makes it to the server */
            }
        }

      *nreturn_vals = 2;

      values[1].type         = GIMP_PDB_IMAGE;
      values[1].data.d_image = image_ID;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}


/* The main Screenshot function */

static GimpPDBStatusType
shoot (GdkScreen  *screen,
       gint32     *image_ID,
       GError    **error)
{
#ifdef PLATFORM_OSX
  if (backend == SCREENSHOT_BACKEND_OSX)
    return screenshot_osx_shoot (&shootvals, screen, image_ID, error);
#endif

#ifdef G_OS_WIN32
  if (backend == SCREENSHOT_BACKEND_WIN32)
    return screenshot_win32_shoot (&shootvals, screen, image_ID, error);
#endif

  if (backend == SCREENSHOT_BACKEND_GNOME_SHELL)
    return screenshot_gnome_shell_shoot (&shootvals, screen, image_ID, error);

#ifdef GDK_WINDOWING_X11
  if (backend == SCREENSHOT_BACKEND_X11)
    return screenshot_x11_shoot (&shootvals, screen, image_ID, error);
#endif

  return GIMP_PDB_CALLING_ERROR; /* silence compiler */
}


/*  Screenshot dialog  */

static void
shoot_dialog_add_hint (GtkNotebook *notebook,
                       ShootType    type,
                       const gchar *hint)
{
  GtkWidget *label;

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   hint,
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.0,
                        NULL);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);

  gtk_notebook_insert_page (notebook, label, NULL, type);
  gtk_widget_show (label);
}

static void
shoot_radio_button_toggled (GtkWidget *widget,
                            GtkWidget *notebook)
{
  gimp_radio_button_update (widget, &shootvals.shoot_type);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), shootvals.shoot_type);
}

static gboolean
shoot_dialog (GdkScreen **screen)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *notebook;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *label;
  GtkWidget     *button;
  GtkWidget     *toggle;
  GtkWidget     *spinner;
  GdkPixbuf     *pixbuf;
  GSList        *radio_group = NULL;
  GtkAdjustment *adj;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Screenshot"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,

                            NULL);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  _("S_nap"), GTK_RESPONSE_OK);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  pixbuf = gdk_pixbuf_new_from_inline (-1, screenshot_icon, FALSE, NULL);
  if (pixbuf)
    {
      gtk_button_set_image (GTK_BUTTON (button),
                            gtk_image_new_from_pixbuf (pixbuf));
      g_object_unref (pixbuf);
    }

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  /*  Hints  */
  notebook = g_object_new (GTK_TYPE_NOTEBOOK,
                           "show-border", FALSE,
                           "show-tabs",   FALSE,
                           NULL);
  gtk_box_pack_end (GTK_BOX (main_vbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  shoot_dialog_add_hint (GTK_NOTEBOOK (notebook), SHOOT_ROOT,
                         _("After the delay, the screenshot is taken."));
  shoot_dialog_add_hint (GTK_NOTEBOOK (notebook), SHOOT_REGION,
                         _("After the delay, drag your mouse to select "
                           "the region for the screenshot."));
  shoot_dialog_add_hint (GTK_NOTEBOOK (notebook), SHOOT_WINDOW,
                         _("At the end of the delay, click in a window "
                           "to snap it."));

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), shootvals.shoot_type);

  /*  Area  */
  frame = gimp_frame_new (_("Area"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);


  /*  single window  */
  button = gtk_radio_button_new_with_mnemonic (radio_group,
                                               _("Take a screenshot of "
                                                 "a single _window"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (SHOOT_WINDOW));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (shoot_radio_button_toggled),
                    notebook);

  /*  window decorations  */
  if (capabilities & SCREENSHOT_CAN_SHOOT_DECORATIONS)
    {
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      toggle = gtk_check_button_new_with_mnemonic (_("Include window _decoration"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    shootvals.decorate);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 24);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &shootvals.decorate);

      g_object_bind_property (button, "active",
                              toggle, "sensitive",
                              G_BINDING_SYNC_CREATE);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                shootvals.shoot_type == SHOOT_WINDOW);

  /*  whole screen  */
  button = gtk_radio_button_new_with_mnemonic (radio_group,
                                               _("Take a screenshot of "
                                                 "the entire _screen"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (SHOOT_ROOT));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (shoot_radio_button_toggled),
                    notebook);

  /*  mouse pointer  */
  if (capabilities & SCREENSHOT_CAN_SHOOT_POINTER)
    {
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      toggle = gtk_check_button_new_with_mnemonic (_("Include _mouse pointer"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    shootvals.show_cursor);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 24);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &shootvals.show_cursor);

      g_object_bind_property (button, "active",
                              toggle, "sensitive",
                              G_BINDING_SYNC_CREATE);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                shootvals.shoot_type == SHOOT_ROOT);

  /*  dragged region  */
  button = gtk_radio_button_new_with_mnemonic (radio_group,
                                               _("Select a _region to grab"));
  radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                shootvals.shoot_type == SHOOT_REGION);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (SHOOT_REGION));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (shoot_radio_button_toggled),
                    notebook);

  /*  Delay  */
  frame = gimp_frame_new (_("Delay"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  adj = (GtkAdjustment *)
    gtk_adjustment_new (shootvals.select_delay,
                        0.0, 100.0, 1.0, 5.0, 0.0);
  spinner = gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_widget_show (spinner);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &shootvals.select_delay);

  /* this is the unit label of a spinbutton */
  label = gtk_label_new (_("seconds"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      /* get the screen on which we are running */
      *screen = gtk_widget_get_screen (dialog);
    }

  gtk_widget_destroy (dialog);

  if (run)
    {
      /*  A short timeout to give the server a chance to
       *  redraw the area that was obscured by our dialog.
       */
      g_timeout_add (100, shoot_quit_timeout, NULL);
      gtk_main ();
    }

  return run;
}

static gboolean
shoot_quit_timeout (gpointer data)
{
  gtk_main_quit ();

  return FALSE;
}


static gboolean
shoot_delay_timeout (gpointer data)
{
  gint *seconds_left = data;

  (*seconds_left)--;

  if (!*seconds_left)
    gtk_main_quit ();

  return *seconds_left;
}


/*  public functions  */

void
screenshot_delay (gint seconds)
{
  g_timeout_add (1000, shoot_delay_timeout, &seconds);
  gtk_main ();
}
