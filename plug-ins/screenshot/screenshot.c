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
#include "screenshot-freedesktop.h"
#include "screenshot-gnome-shell.h"
#include "screenshot-icon.h"
#include "screenshot-kwin.h"
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

static GimpPDBStatusType   shoot               (GdkMonitor       *monitor,
                                                gint32           *image_ID,
                                                GError          **error);

static gboolean            shoot_dialog        (GdkMonitor      **monitor);
static gboolean            shoot_quit_timeout  (gpointer          data);
static gboolean            shoot_delay_timeout (gpointer          data);


/* Global Variables */

static ScreenshotBackend       backend            = SCREENSHOT_BACKEND_NONE;
static ScreenshotCapabilities  capabilities       = 0;
static GtkWidget              *select_delay_table = NULL;
static GtkWidget              *shot_delay_table   = NULL;

static ScreenshotValues shootvals =
{
  SHOOT_WINDOW, /* root window            */
  TRUE,         /* include WM decorations */
  0,            /* window ID              */
  0,            /* select delay           */
  0,            /* screenshot delay       */
  0,            /* coords of region dragged out by pointer */
  0,
  0,
  0,
  FALSE,        /* show cursor */
  SCREENSHOT_PROFILE_POLICY_MONITOR
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
                          "when called non-interactively, the plug-in"
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
  GdkMonitor        *monitor = NULL;
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
  else if (! backend && screenshot_kwin_available ())
    {
      backend      = SCREENSHOT_BACKEND_KWIN;
      capabilities = screenshot_kwin_get_capabilities ();
    }

#ifdef GDK_WINDOWING_X11
  if (! backend && screenshot_x11_available ())
    {
      backend      = SCREENSHOT_BACKEND_X11;
      capabilities = screenshot_x11_get_capabilities ();
    }
#endif
  else if (! backend && screenshot_freedesktop_available ())
    {
      backend      = SCREENSHOT_BACKEND_FREEDESKTOP;
      capabilities = screenshot_freedesktop_get_capabilities ();
    }

  /* how are we running today? */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_PROC, &shootvals);
      shootvals.window_id = 0;

      if ((shootvals.shoot_type == SHOOT_WINDOW &&
           ! (capabilities & SCREENSHOT_CAN_SHOOT_WINDOW)) ||
          (shootvals.shoot_type == SHOOT_REGION &&
           ! (capabilities & SCREENSHOT_CAN_SHOOT_REGION)))
        {
          /* Shoot root is the only type of shoot which is definitely
           * shared by all screenshot backends (basically just snap the
           * whole display setup).
           */
          shootvals.shoot_type = SHOOT_ROOT;
        }

      /* Get information from the dialog */
      if (! shoot_dialog (&monitor))
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
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
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
      status = shoot (monitor, &image_ID, &error);
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      gchar *comment = gimp_get_default_comment ();

      if (shootvals.profile_policy == SCREENSHOT_PROFILE_POLICY_SRGB)
        {
          GimpColorProfile *srgb_profile = gimp_color_profile_new_rgb_srgb ();

          gimp_image_convert_color_profile (image_ID,
                                            srgb_profile,
                                            GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                            TRUE);
          g_object_unref (srgb_profile);
        }

      if (comment)
        {
          GimpParasite *parasite;

          parasite = gimp_parasite_new ("gimp-comment",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (comment) + 1, comment);

          gimp_image_attach_parasite (image_ID, parasite);
          gimp_parasite_free (parasite);

          g_free (comment);
        }

      gimp_image_clean_all (image_ID);

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          /* Store variable states for next run */
          gimp_set_data (PLUG_IN_PROC, &shootvals, sizeof (ScreenshotValues));

          gimp_display_new (image_ID);

          /* Give some sort of feedback that the shot is done */
          if (shootvals.select_delay > 0)
            {
              gdk_display_beep (gdk_monitor_get_display (monitor));
              /* flush so the beep makes it to the server */
              gdk_display_flush (gdk_monitor_get_display (monitor));
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
shoot (GdkMonitor *monitor,
       gint32     *image_ID,
       GError    **error)
{
#ifdef PLATFORM_OSX
  if (backend == SCREENSHOT_BACKEND_OSX)
    return screenshot_osx_shoot (&shootvals, monitor, image_ID, error);
#endif

#ifdef G_OS_WIN32
  if (backend == SCREENSHOT_BACKEND_WIN32)
    return screenshot_win32_shoot (&shootvals, monitor, image_ID, error);
#endif

  if (backend == SCREENSHOT_BACKEND_FREEDESKTOP)
    return screenshot_freedesktop_shoot (&shootvals, monitor, image_ID, error);
  else if (backend == SCREENSHOT_BACKEND_GNOME_SHELL)
    return screenshot_gnome_shell_shoot (&shootvals, monitor, image_ID, error);
  else if (backend == SCREENSHOT_BACKEND_KWIN)
    return screenshot_kwin_shoot (&shootvals, monitor, image_ID, error);

#ifdef GDK_WINDOWING_X11
  if (backend == SCREENSHOT_BACKEND_X11)
    return screenshot_x11_shoot (&shootvals, monitor, image_ID, error);
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

  if (select_delay_table)
    {
      if (shootvals.shoot_type == SHOOT_ROOT ||
          (shootvals.shoot_type == SHOOT_WINDOW &&
           ! (capabilities & SCREENSHOT_CAN_PICK_WINDOW)))
        {
          gtk_widget_hide (select_delay_table);
        }
      else
        {
          gtk_widget_show (select_delay_table);
        }
    }
  if (shot_delay_table)
    {
      if (shootvals.shoot_type == SHOOT_WINDOW        &&
          (capabilities & SCREENSHOT_CAN_PICK_WINDOW) &&
          ! (capabilities & SCREENSHOT_CAN_DELAY_WINDOW_SHOT))
        {
          gtk_widget_hide (shot_delay_table);
        }
      else
        {
          gtk_widget_show (shot_delay_table);
        }
    }
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), shootvals.shoot_type);
}

static gboolean
shoot_dialog (GdkMonitor **monitor)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *notebook1;
  GtkWidget     *notebook2;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *label;
  GtkWidget     *button;
  GtkWidget     *toggle;
  GtkWidget     *spinner;
  GtkWidget     *table;
  GSList        *radio_group = NULL;
  GtkAdjustment *adj;
  gboolean       run;
  GtkWidget     *cursor_toggle = NULL;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Screenshot"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("S_nap"),   GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);


  /*  Create delay hints notebooks early  */
  notebook1 = g_object_new (GTK_TYPE_NOTEBOOK,
                            "show-border", FALSE,
                            "show-tabs",   FALSE,
                            NULL);
  notebook2 = g_object_new (GTK_TYPE_NOTEBOOK,
                            "show-border", FALSE,
                            "show-tabs",   FALSE,
                            NULL);

  /*  Area  */
  frame = gimp_frame_new (_("Area"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  Single window  */
  if (capabilities & SCREENSHOT_CAN_SHOOT_WINDOW)
    {
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
                        notebook1);
      g_signal_connect (button, "toggled",
                        G_CALLBACK (shoot_radio_button_toggled),
                        notebook2);

      /*  Window decorations  */
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
      /*  Mouse pointer  */
      if (capabilities & SCREENSHOT_CAN_SHOOT_POINTER)
        {
          hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
          gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
          gtk_widget_show (hbox);

          cursor_toggle = gtk_check_button_new_with_mnemonic (_("Include _mouse pointer"));
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cursor_toggle),
                                        shootvals.show_cursor);
          gtk_box_pack_start (GTK_BOX (hbox), cursor_toggle, TRUE, TRUE, 24);
          gtk_widget_show (cursor_toggle);

          g_signal_connect (cursor_toggle, "toggled",
                            G_CALLBACK (gimp_toggle_button_update),
                            &shootvals.show_cursor);

          g_object_bind_property (button, "active",
                                  cursor_toggle, "sensitive",
                                  G_BINDING_SYNC_CREATE);
        }


      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    shootvals.shoot_type == SHOOT_WINDOW);
    }

  /*  Whole screen  */
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
                    notebook1);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (shoot_radio_button_toggled),
                    notebook2);

  /*  Mouse pointer  */
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

      if (cursor_toggle)
        {
          g_object_bind_property (cursor_toggle, "active",
                                  toggle, "active",
                                  G_BINDING_BIDIRECTIONAL);
        }
      g_object_bind_property (button, "active",
                              toggle, "sensitive",
                              G_BINDING_SYNC_CREATE);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                shootvals.shoot_type == SHOOT_ROOT);

  /*  Dragged region  */
  if (capabilities & SCREENSHOT_CAN_SHOOT_REGION)
    {
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
                        notebook1);
      g_signal_connect (button, "toggled",
                        G_CALLBACK (shoot_radio_button_toggled),
                        notebook2);
    }

  frame = gimp_frame_new (_("Delay"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* Selection delay  */
  table = gtk_table_new (2, 3, FALSE);
  select_delay_table = table;
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  /* Check if this delay must be hidden from start. */
  if (shootvals.shoot_type == SHOOT_REGION ||
      (shootvals.shoot_type == SHOOT_WINDOW &&
       capabilities & SCREENSHOT_CAN_PICK_WINDOW))
    {
      gtk_widget_show (select_delay_table);
    }

  label = gtk_label_new (_("Selection delay: "));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  adj = (GtkAdjustment *)
    gtk_adjustment_new (shootvals.select_delay,
                        0.0, 100.0, 1.0, 5.0, 0.0);
  spinner = gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
  gtk_table_attach (GTK_TABLE (table), spinner, 1, 2, 0, 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (spinner);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &shootvals.select_delay);

  /*  translators: this is the unit label of a spinbutton  */
  label = gtk_label_new (_("seconds"));
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 1.0, 0);
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_show (label);

  /*  Selection delay hints  */
  gtk_table_attach (GTK_TABLE (table), notebook1, 0, 3, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (notebook1);

  /* No selection delay for full-screen. */
  shoot_dialog_add_hint (GTK_NOTEBOOK (notebook1), SHOOT_ROOT, "");
  shoot_dialog_add_hint (GTK_NOTEBOOK (notebook1), SHOOT_REGION,
                         _("After the delay, drag your mouse to select "
                           "the region for the screenshot."));
#ifdef G_OS_WIN32
  shoot_dialog_add_hint (GTK_NOTEBOOK (notebook1), SHOOT_WINDOW,
                         _("Click in a window to snap it after delay."));
#else
  if (capabilities & SCREENSHOT_CAN_PICK_WINDOW)
    {
      shoot_dialog_add_hint (GTK_NOTEBOOK (notebook1), SHOOT_WINDOW,
                             _("At the end of the delay, click in a window "
                               "to snap it."));
    }
  else
    {
      shoot_dialog_add_hint (GTK_NOTEBOOK (notebook1), SHOOT_WINDOW, "");
    }
#endif
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook1), shootvals.shoot_type);

  /* Screenshot delay  */
  table = gtk_table_new (2, 3, FALSE);
  shot_delay_table = table;
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  if (shootvals.shoot_type != SHOOT_WINDOW          ||
      ! (capabilities & SCREENSHOT_CAN_PICK_WINDOW) ||
      (capabilities & SCREENSHOT_CAN_DELAY_WINDOW_SHOT))
    {
      gtk_widget_show (table);
    }

  label = gtk_label_new (_("Screenshot delay: "));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  adj = (GtkAdjustment *)
    gtk_adjustment_new (shootvals.screenshot_delay,
                        0.0, 100.0, 1.0, 5.0, 0.0);
  spinner = gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
  gtk_table_attach (GTK_TABLE (table), spinner, 1, 2, 0, 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (spinner);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &shootvals.screenshot_delay);

  /*  translators: this is the unit label of a spinbutton  */
  label = gtk_label_new (_("seconds"));
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 1.0, 0);
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_show (label);

  /*  Screenshot delay hints  */
  gtk_table_attach (GTK_TABLE (table), notebook2, 0, 3, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (notebook2);

  shoot_dialog_add_hint (GTK_NOTEBOOK (notebook2), SHOOT_ROOT,
                         _("After the delay, the screenshot is taken."));
  shoot_dialog_add_hint (GTK_NOTEBOOK (notebook2), SHOOT_REGION,
                         _("Once the region is selected, it will be "
                           "captured after this delay."));
  if (capabilities & SCREENSHOT_CAN_PICK_WINDOW)
    {
      shoot_dialog_add_hint (GTK_NOTEBOOK (notebook2), SHOOT_WINDOW,
                             _("Once the window is selected, it will be "
                               "captured after this delay."));
    }
  else
    {
      shoot_dialog_add_hint (GTK_NOTEBOOK (notebook2), SHOOT_WINDOW,
                             _("After the delay, the active window "
                               "will be captured."));
    }
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook2), shootvals.shoot_type);

  /*  Color profile  */
  frame = gimp_int_radio_group_new (TRUE,
                                    _("Color Profile"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &shootvals.profile_policy,
                                    shootvals.profile_policy,

                                    _("Tag image with _monitor profile"),
                                    SCREENSHOT_PROFILE_POLICY_MONITOR,
                                    NULL,

                                    _("Convert image to sR_GB"),
                                    SCREENSHOT_PROFILE_POLICY_SRGB,
                                    NULL,

                                    NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);


  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      /* get the screen on which we are running */
      *monitor = gimp_widget_get_monitor (dialog);
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
