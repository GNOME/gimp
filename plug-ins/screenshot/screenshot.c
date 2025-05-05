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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "screenshot.h"
#include "screenshot-freedesktop.h"
#include "screenshot-osx.h"
#include "screenshot-x11.h"
#include "screenshot-win32.h"

#include "libgimp/stdplugins-intl.h"


/* Defines */

#define PLUG_IN_PROC   "plug-in-screenshot"
#define PLUG_IN_BINARY "screenshot"
#define PLUG_IN_ROLE   "gimp-screenshot"

typedef struct _Screenshot      Screenshot;
typedef struct _ScreenshotClass ScreenshotClass;

struct _Screenshot
{
  GimpPlugIn      parent_instance;
};

struct _ScreenshotClass
{
  GimpPlugInClass parent_class;
};

#define SCREENSHOT_TYPE  (screenshot_get_type ())
#define SCREENSHOT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCREENSHOT_TYPE, Screenshot))

GType                   screenshot_get_type         (void) G_GNUC_CONST;

static GList          * screenshot_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * screenshot_create_procedure (GimpPlugIn           *plug_in,
                                                     const gchar          *name);

static GimpValueArray * screenshot_run              (GimpProcedure        *procedure,
                                                     GimpProcedureConfig  *config,
                                                     gpointer              run_data);

static GimpPDBStatusType   shoot               (GdkMonitor           *monitor,
                                                GimpImage           **image,
                                                GimpProcedureConfig  *config,
                                                GError              **error);

static gboolean            shoot_dialog        (GimpProcedure        *procedure,
                                                GimpProcedureConfig  *config,
                                                GdkMonitor          **monitor);
static gboolean            shoot_quit_timeout  (gpointer              data);
static gboolean            shoot_delay_timeout (gpointer              data);


G_DEFINE_TYPE (Screenshot, screenshot, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SCREENSHOT_TYPE)
DEFINE_STD_SET_I18N


static ScreenshotBackend       backend      = SCREENSHOT_BACKEND_NONE;
static ScreenshotCapabilities  capabilities = 0;

static void
screenshot_class_init (ScreenshotClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = screenshot_query_procedures;
  plug_in_class->create_procedure = screenshot_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
screenshot_init (Screenshot *screenshot)
{
}

static GList *
screenshot_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
screenshot_create_procedure (GimpPlugIn  *plug_in,
                             const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      screenshot_run, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("_Screenshot..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/File/Create");

      gimp_procedure_set_documentation
        (procedure,
         _("Create an image from an area of the screen"),
           "The plug-in takes screenshots of an "
           "interactively selected window or of the desktop, "
           "either the whole desktop or an interactively "
           "selected region. When called non-interactively, it "
           "may grab the root window or use the window-id "
           "passed as a parameter.  The last four parameters "
           "are optional and can be used to specify the corners "
           "of the region to be grabbed."
           "On Mac OS X, "
           "when called non-interactively, the plug-in"
           "only can take screenshots of the entire root window."
           "Grabbing a window or a region is not supported"
           "non-interactively. To grab a region or a particular"
           "window, you need to use the interactive mode.",
         name);

      gimp_procedure_set_attribution (procedure,
                                      "Sven Neumann <sven@gimp.org>, "
                                      "Henrik Brix Andersen <brix@gimp.org>,"
                                      "Simone Karin Lehmann",
                                      "1998 - 2008",
                                      "v1.1 (2008/04)");

      gimp_procedure_set_icon_pixbuf (procedure,
                                      gdk_pixbuf_new_from_resource ("/org/gimp/screenshot-icons/screenshot-icon.png", NULL));

      gimp_procedure_add_enum_argument (procedure, "run-mode",
                                        "Run mode",
                                        "The run mode",
                                        GIMP_TYPE_RUN_MODE,
                                        GIMP_RUN_NONINTERACTIVE,
                                        G_PARAM_READWRITE);

      /* TODO: Windows does not currently implement selecting by region to grab,
       * so we'll hide this option for now */
      gimp_procedure_add_choice_argument (procedure, "shoot-type",
                                          _("Shoot _area"),
                                          _("The shoot type"),
                                          gimp_choice_new_with_values ("window",  SHOOT_WINDOW, _("Take a screenshot of a single window"),   NULL,
                                                                       "screen",  SHOOT_ROOT,   _("Take a screenshot of the entire screen"), NULL,
#ifndef G_OS_WIN32
                                                                       "region",  SHOOT_REGION, _("Select a region to grab"),                NULL,
#endif
                                                                       NULL),
                                          "window",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "x1",
                                       "X1",
                                       "Region left x coord for SHOOT-WINDOW",
                                       G_MININT, G_MAXINT, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "y1",
                                       "Y1",
                                       "Region top y coord for SHOOT-WINDOW",
                                       G_MININT, G_MAXINT, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "x2",
                                       "X2",
                                       "Region right x coord for SHOOT-WINDOW",
                                       G_MININT, G_MAXINT, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "y2",
                                       "Y2",
                                       "Region bottom y coord for SHOOT-WINDOW",
                                       G_MININT, G_MAXINT, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "include-pointer",
                                           _("Include _mouse pointer"),
                                           _("Your pointing device's cursor will be part of the image"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      /* Since no backends allow window screenshot non-interactively so far, no
       * need to expose this argument to the API.
       */
      gimp_procedure_add_boolean_aux_argument (procedure, "include-decoration",
                                               _("Include window _decoration"),
                                               _("Title bar, window borders and shadow will be part of the image"),
#ifdef PLATFORM_OSX
                                 /* on OS X, this just means shoot the shadow, default to nope */
                                 FALSE,
#else
                                 TRUE,
#endif
                                 G_PARAM_READWRITE);
      gimp_procedure_add_int_aux_argument     (procedure, "selection-delay",
                                               _("Selection d_elay"),
                                               _("Delay before selection of the window or the region"),
                                               0, 20, 0,
                                               G_PARAM_READWRITE);
      gimp_procedure_add_int_aux_argument     (procedure, "screenshot-delay",
                                               _("Screenshot dela_y"),
                                               _("Delay before snapping the screenshot"),
                                               0, 20, 0,
                                               G_PARAM_READWRITE);
      gimp_procedure_add_choice_argument (procedure, "color-profile",
                                          _("Color _Profile"),
                                          "",
                                          gimp_choice_new_with_values ("monitor", SCREENSHOT_PROFILE_POLICY_MONITOR, _("Tag image with monitor profile"), NULL,
                                                                       "srgb",    SCREENSHOT_PROFILE_POLICY_SRGB,    _("Convert image with sRGB"),        NULL,
                                                                       NULL),
                                          "monitor",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_image_return_value (procedure, "image",
                                             "Image",
                                             "Output image",
                                             FALSE,
                                             G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
screenshot_run (GimpProcedure        *procedure,
                GimpProcedureConfig  *config,
                gpointer              run_data)
{
  GimpValueArray    *return_vals;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  GdkMonitor        *monitor = NULL;
  GimpImage         *image   = NULL;
  GError            *error   = NULL;

  ShootType               shoot_type;
  ScreenshotProfilePolicy profile_policy;

  gegl_init (NULL, NULL);

  g_object_get (config, "run-mode", &run_mode, NULL);
  shoot_type = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                    "shoot-type");

  if (! gdk_init_check (NULL, NULL))
    {
      g_set_error_literal (&error, GIMP_PLUG_IN_ERROR, 0, _("GDK initialization failed."));
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }

#ifdef PLATFORM_OSX
  if (! backend && screenshot_osx_available ())
    {
      backend      = SCREENSHOT_BACKEND_OSX;
      capabilities = screenshot_osx_get_capabilities ();
    }
#endif

#ifdef G_OS_WIN32
  if (! backend && screenshot_win32_available ())
    {
      backend      = SCREENSHOT_BACKEND_WIN32;
      capabilities = screenshot_win32_get_capabilities ();
    }
#endif

#ifdef GDK_WINDOWING_X11
  if (! backend && screenshot_x11_available ())
    {
      backend      = SCREENSHOT_BACKEND_X11;
      capabilities = screenshot_x11_get_capabilities ();
    }
#endif
  if (! backend && screenshot_freedesktop_available ())
    {
      backend      = SCREENSHOT_BACKEND_FREEDESKTOP;
      capabilities = screenshot_freedesktop_get_capabilities ();
    }

  if (backend == SCREENSHOT_BACKEND_NONE)
    {
      g_set_error_literal (&error, GIMP_PLUG_IN_ERROR, 0,
                           _("No screenshot backends available."));
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }
  /* how are we running today? */
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_ui_init (PLUG_IN_BINARY);

      /* In interactive mode, coords are always reset. */
      g_object_set (config,
                    "x1", 0, "y1", 0,
                    "x2", 0, "y2", 0,
                    NULL);

      if ((shoot_type == SHOOT_WINDOW &&
           ! (capabilities & SCREENSHOT_CAN_SHOOT_WINDOW)) ||
          (shoot_type == SHOOT_REGION &&
           ! (capabilities & SCREENSHOT_CAN_SHOOT_REGION)))
        {
          /* Shoot root is the only type of shoot which is definitely
           * shared by all screenshot backends (basically just snap the
           * whole display setup).
           */
          g_object_set (config, "shoot-type", "screen", NULL);
        }

      /* Get information from the dialog. Freedesktop portal comes with
       * its own dialog.
       */
      if (backend != SCREENSHOT_BACKEND_FREEDESKTOP)
        {
          if (! shoot_dialog (procedure, config, &monitor))
            status = GIMP_PDB_CANCEL;
        }
      else
        {
          /* This is ugly but in reality we have no idea on which monitor
           * a screenshot was taken from with portals. It's like a
           * better-than-nothing trick for easy single-display cases.
           */
          monitor = gdk_display_get_monitor (gdk_display_get_default (), 0);
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (backend == SCREENSHOT_BACKEND_FREEDESKTOP)
        {
          /* With portals, even the basic full screenshot is interactive. */
          status = GIMP_PDB_CALLING_ERROR;
        }
      else if (! (capabilities & SCREENSHOT_CAN_PICK_NONINTERACTIVELY))
        {
          if (shoot_type == SHOOT_WINDOW ||
              shoot_type == SHOOT_REGION)
            status = GIMP_PDB_CALLING_ERROR;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    status = shoot (monitor, &image, config, &error);

  if (status == GIMP_PDB_SUCCESS)
    {
      gchar *comment = gimp_get_default_comment ();

      gimp_image_undo_disable (image);

      profile_policy = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                            "color-profile");

      if (run_mode == GIMP_RUN_NONINTERACTIVE)
        profile_policy = SCREENSHOT_PROFILE_POLICY_MONITOR;

      if (profile_policy == SCREENSHOT_PROFILE_POLICY_SRGB)
        {
          GimpColorProfile *srgb_profile = gimp_color_profile_new_rgb_srgb ();

          gimp_image_convert_color_profile (image,
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

          gimp_image_attach_parasite (image, parasite);
          gimp_parasite_free (parasite);

          g_free (comment);
        }

      gimp_image_undo_enable (image);

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          guint select_delay;

          gimp_display_new (image);

          g_object_get (config,
                        "selection-delay", &select_delay,
                        NULL);
          /* Give some sort of feedback that the shot is done */
          if (select_delay > 0)
            {
              gdk_display_beep (gdk_monitor_get_display (monitor));
              /* flush so the beep makes it to the server */
              gdk_display_flush (gdk_monitor_get_display (monitor));
            }
        }
    }

  return_vals = gimp_procedure_new_return_values (procedure, status, error);

  if (status == GIMP_PDB_SUCCESS)
    GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}


/* The main Screenshot function */

static GimpPDBStatusType
shoot (GdkMonitor           *monitor,
       GimpImage           **image,
       GimpProcedureConfig  *config,
       GError              **error)
{
#if defined(PLATFORM_OSX) || defined(G_OS_WIN32)
  ShootType shoot_type;
#endif
  gboolean  decorate;
  guint     select_delay;
  guint     screenshot_delay;
  gboolean  show_cursor;

  g_object_get (config,
                "screenshot-delay",   &screenshot_delay,
                "selection-delay",    &select_delay,
                "include-decoration", &decorate,
                "include-pointer",    &show_cursor,
                NULL);
#if defined(PLATFORM_OSX) || defined(G_OS_WIN32)
  shoot_type = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                    "shoot-type");
#endif

#ifdef PLATFORM_OSX
  if (backend == SCREENSHOT_BACKEND_OSX)
    return screenshot_osx_shoot (shoot_type,
                                 select_delay, screenshot_delay,
                                 decorate, show_cursor,
                                 image, error);
#endif

#ifdef G_OS_WIN32
  if (backend == SCREENSHOT_BACKEND_WIN32)
    return screenshot_win32_shoot (shoot_type, screenshot_delay, show_cursor,
                                   monitor, image, error);
#endif

  if (backend == SCREENSHOT_BACKEND_FREEDESKTOP)
    return screenshot_freedesktop_shoot (monitor, image, error);

#ifdef GDK_WINDOWING_X11
  if (backend == SCREENSHOT_BACKEND_X11)
    return screenshot_x11_shoot (config, monitor, image, error);
#endif

  return GIMP_PDB_CALLING_ERROR; /* silence compiler */
}


/*  Screenshot dialog  */

static gboolean
shoot_dialog (GimpProcedure        *procedure,
              GimpProcedureConfig  *config,
              GdkMonitor          **monitor)
{
  GtkWidget      *dialog;
  GimpValueArray *values;
  GValue          value = G_VALUE_INIT;
  gboolean        run;

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Screenshot"));
  gimp_procedure_dialog_set_ok_label (GIMP_PROCEDURE_DIALOG (dialog), _("S_nap"));

  if (capabilities & SCREENSHOT_CAN_SHOOT_POINTER ||
      capabilities & SCREENSHOT_CAN_SHOOT_DECORATIONS)
    {
      if (! (capabilities & SCREENSHOT_CAN_SHOOT_POINTER))
        gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                        "contents-box",
                                        "include-decoration",
                                        NULL);
      else if (! (capabilities & SCREENSHOT_CAN_SHOOT_DECORATIONS))
        gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                        "contents-box",
                                        "include-pointer",
                                        NULL);
      else
        gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                        "contents-box",
                                        "include-decoration",
                                        "include-pointer",
                                        NULL);

      gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                       "contents-frame-title",
                                       _("Contents"), FALSE, FALSE);
      gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                        "contents-frame", "contents-frame-title",
                                        FALSE, "contents-box");

      if (capabilities & SCREENSHOT_CAN_SHOOT_DECORATIONS)
        {
          values = gimp_value_array_new (1);
          g_value_init (&value, G_TYPE_STRING);
          g_value_set_string (&value, "window");
          gimp_value_array_append (values, &value);
          g_value_unset (&value);
          gimp_procedure_dialog_set_sensitive_if_in (GIMP_PROCEDURE_DIALOG (dialog),
                                                     "include-decoration",
                                                     NULL, "shoot-type",
                                                     values, TRUE);
        }
      if (capabilities & SCREENSHOT_CAN_SHOOT_POINTER)
        {
          values = gimp_value_array_new (1);
          g_value_init (&value, G_TYPE_STRING);
          g_value_set_string (&value, "region");
          gimp_value_array_append (values, &value);
          g_value_unset (&value);
          gimp_procedure_dialog_set_sensitive_if_in (GIMP_PROCEDURE_DIALOG (dialog),
                                                     "include-pointer",
                                                     NULL, "shoot-type",
                                                     values, FALSE);
        }
    }

  if (capabilities & SCREENSHOT_CAN_DELAY_WINDOW_SHOT)
    {
      if (capabilities & SCREENSHOT_CAN_PICK_WINDOW)
        {
          gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                          "delay-box",
                                          "selection-delay",
                                          "screenshot-delay",
                                          NULL);

          values = gimp_value_array_new (1);
          g_value_init (&value, G_TYPE_STRING);
          g_value_set_string (&value, "screen");
          gimp_value_array_append (values, &value);
          g_value_unset (&value);
          gimp_procedure_dialog_set_sensitive_if_in (GIMP_PROCEDURE_DIALOG (dialog),
                                                     "selection-delay",
                                                     NULL, "shoot-type",
                                                     values, FALSE);
        }
      else
        {
          gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                          "delay-box", "screenshot-delay", NULL);
        }

      gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                       "delay-frame-title",
                                       _("Delay"), FALSE, FALSE);
      gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                        "delay-frame", "delay-frame-title",
                                        FALSE, "delay-box");
    }

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), "shoot-type", NULL);

  if ((capabilities & SCREENSHOT_CAN_SHOOT_POINTER) ||
      (capabilities & SCREENSHOT_CAN_SHOOT_DECORATIONS))
    gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), "contents-frame", NULL);

  if (capabilities & SCREENSHOT_CAN_DELAY_WINDOW_SHOT)
    gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), "delay-frame", NULL);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), "color-profile", NULL);
  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  /* Get the screen on which we are running */
  if (run)
    *monitor = gimp_widget_get_monitor (dialog);

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
screenshot_wait_delay (gint seconds)
{
  g_timeout_add (1000, shoot_delay_timeout, &seconds);
  gtk_main ();
}
