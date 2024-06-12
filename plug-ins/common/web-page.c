/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* Webpage plug-in.
 * Copyright (C) 2011 Mukund Sivaraman <muks@banu.com>.
 * Portions are copyright of the author of the
 * file-open-location-dialog.c code.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <webkit2/webkit2.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-web-page"
#define PLUG_IN_BINARY "web-page"
#define PLUG_IN_ROLE   "gimp-web-page"
#define MAX_URL_LEN    2048


typedef struct
{
  GimpImage *image;
  GError    *error;
} WebpageResult;

typedef struct _Webpage      Webpage;
typedef struct _WebpageClass WebpageClass;

struct _Webpage
{
  GimpPlugIn parent_instance;
};

struct _WebpageClass
{
  GimpPlugInClass parent_class;
};


#define WEBPAGE_TYPE  (webpage_get_type ())
#define WEBPAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBPAGE_TYPE, Webpage))

GType                   webpage_get_type         (void) G_GNUC_CONST;

static GList          * webpage_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * webpage_create_procedure (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray * webpage_run              (GimpProcedure        *procedure,
                                                  GimpProcedureConfig  *config,
                                                  gpointer              run_data);

static gboolean         webpage_dialog           (GimpProcedure        *procedure,
                                                  GimpProcedureConfig  *config);
static GimpImage      * webpage_capture          (GimpProcedureConfig  *config,
                                                  GError              **error);


G_DEFINE_TYPE (Webpage, webpage, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (WEBPAGE_TYPE)
DEFINE_STD_SET_I18N


static void
webpage_class_init (WebpageClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = webpage_query_procedures;
  plug_in_class->create_procedure = webpage_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
webpage_init (Webpage *webpage)
{
}

static GList *
webpage_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
webpage_create_procedure (GimpPlugIn  *plug_in,
                          const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      webpage_run, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("From _Webpage..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/File/Create");

      gimp_procedure_set_documentation (procedure,
                                        _("Create an image of a webpage"),
                                        "The plug-in allows you to take a "
                                        "screenshot of a webpage.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Mukund Sivaraman <muks@banu.com>",
                                      "2011",
                                      "2011");

      gimp_procedure_add_enum_argument (procedure, "run-mode",
                                        "Run mode",
                                        "The run mode",
                                        GIMP_TYPE_RUN_MODE,
                                        GIMP_RUN_INTERACTIVE,
                                        G_PARAM_READWRITE);

      gimp_procedure_add_string_argument (procedure, "url",
                                          _("Enter location (_URI)"),
                                          _("URL of the webpage to screenshot"),
                                          "https://www.gimp.org/",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "width",
                                       _("_Width (pixels)"),
                                       _("The width of the screenshot (in pixels)"),
                                       100, GIMP_MAX_IMAGE_SIZE, 1024,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "font-size",
                                       _("_Font size"),
                                       _("The font size to use in the page (in pt)"),
                                       1, 1000, 12,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_image_return_value (procedure, "image",
                                         "Image",
                                         "The output image",
                                         FALSE,
                                         G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
webpage_run (GimpProcedure        *procedure,
             GimpProcedureConfig  *config,
             gpointer              run_data)
{
  GimpValueArray  *return_vals;
  GimpRunMode      run_mode;
  GimpImage       *image;
  GError          *error = NULL;

  g_object_get (config, "run-mode", &run_mode, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE &&
      ! webpage_dialog (procedure, config))
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_CANCEL,
                                             NULL);
  image = webpage_capture (config, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_display_new (image);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static gboolean
webpage_dialog (GimpProcedure       *procedure,
                GimpProcedureConfig *config)
{
  GtkWidget    *dialog;
  GtkListStore *store;
  gboolean      run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Create from webpage"));
  gimp_procedure_dialog_set_ok_label (GIMP_PROCEDURE_DIALOG (dialog),
                                      _("Cre_ate"));
  store = gimp_int_store_new (_("Huge"),                 16,
                              _("Large"),                14,
                              C_("web-page", "Default"), 12,
                              _("Small"),                10,
                              _("Tiny"),                 8,
                              NULL);
  gimp_procedure_dialog_get_int_combo (GIMP_PROCEDURE_DIALOG (dialog),
                                       "font-size", GIMP_INT_STORE (store));
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "url", "width", "font-size", NULL);
  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return run;
}

static void
notify_progress_cb (WebKitWebView  *view,
                    GParamSpec     *pspec,
                    gpointer        user_data)
{
  static gdouble old_progress = 0.0;
  gdouble        progress;

  progress = webkit_web_view_get_estimated_load_progress (view);
  if ((progress - old_progress) > 0.01)
    {
      gimp_progress_update (progress);
      old_progress = progress;
    }
}

static gboolean
load_failed_cb (WebKitWebView   *view,
                WebKitLoadEvent  event,
                gchar           *uri,
                gpointer         web_error,
                gpointer         user_data)
{
  GError **error = user_data;

  *error = g_error_copy ((GError *) web_error);

  gtk_main_quit ();

  return TRUE;
}

static void
snapshot_ready (GObject      *source_object,
                GAsyncResult *result,
                gpointer      user_data)
{
  WebpageResult   *retval = user_data;
  WebKitWebView   *view   = WEBKIT_WEB_VIEW (source_object);
  cairo_surface_t *surface;

  gimp_progress_pulse ();
  surface = webkit_web_view_get_snapshot_finish (view, result,
                                                 &retval->error);
  gimp_progress_pulse ();

  if (surface)
    {
      gint       width;
      gint       height;
      GimpLayer *layer;

      width  = cairo_image_surface_get_width (surface);
      height = cairo_image_surface_get_height (surface);

      retval->image = gimp_image_new (width, height, GIMP_RGB);

      gimp_image_undo_disable (retval->image);
      layer = gimp_layer_new_from_surface (retval->image, _("Webpage"),
                                           surface,
                                           0.25, 1.0);
      gimp_image_insert_layer (retval->image, layer, NULL, 0);
      gimp_image_undo_enable (retval->image);

      cairo_surface_destroy (surface);
    }

  gtk_main_quit ();
}

static void
load_changed_cb (WebKitWebView   *view,
                 WebKitLoadEvent  event,
                 gpointer         user_data)
{
  if (event == WEBKIT_LOAD_FINISHED)
    gtk_main_quit ();
}

static GimpImage *
webpage_capture (GimpProcedureConfig  *config,
                 GError              **error)
{
  gchar          *scheme;
  GtkWidget      *window;
  GtkWidget      *view;
  WebKitSettings *settings;
  char           *ua;

  gchar          *url;
  gint            width;
  gint            font_size;
  WebpageResult   result;

  g_object_get (config,
                "url",       &url,
                "width",     &width,
                "font-size", &font_size,
                NULL);
  if (! url || strlen (url) == 0)
    {
      g_set_error (error, 0, 0, _("No URL was specified"));
      return NULL;
    }

  scheme = g_uri_parse_scheme (url);
  if (! scheme)
    {
      gchar *scheme_url;

      /* If we were not given a well-formed URL, make one. */
      scheme_url = g_strconcat ("https://", url, NULL);
      g_free (url);
      url = scheme_url;
    }
  g_free (scheme);

  if (width < 32)
    {
      g_warning ("Width '%d' is too small. Clamped to 32.", width);
      width = 32;
    }
  else if (width > 8192)
    {
      g_warning ("Width '%d' is too large. Clamped to 8192.", width);
      width = 8192;
    }

  window = gtk_offscreen_window_new ();
  gtk_widget_show (window);

  view = webkit_web_view_new ();
  gtk_widget_show (view);

  gtk_widget_set_vexpand (view, TRUE);
  gtk_widget_set_size_request (view, width, -1);
  gtk_container_add (GTK_CONTAINER (window), view);

  /* Append "GIMP/<GIMP_VERSION>" to the user agent string */
  settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (view));
  ua = g_strdup_printf ("%s GIMP/%s",
                        webkit_settings_get_user_agent (settings),
                        GIMP_VERSION);
  webkit_settings_set_user_agent (settings, ua);
  g_free (ua);

  /* Set font size */
  webkit_settings_set_default_font_size (settings, font_size);

  g_signal_connect (view, "notify::estimated-load-progress",
                    G_CALLBACK (notify_progress_cb),
                    window);
  g_signal_connect (view, "load-failed",
                    G_CALLBACK (load_failed_cb),
                    error);
  g_signal_connect (view, "load-changed",
                    G_CALLBACK (load_changed_cb),
                    window);

  gimp_progress_init_printf (_("Downloading webpage '%s'"), url);

  webkit_web_view_load_uri (WEBKIT_WEB_VIEW (view), url);
  gtk_main ();

  result.error = NULL;
  result.image = NULL;
  if (*error == NULL)
    {
      gimp_progress_init_printf (_("Transferring webpage image for '%s'"), url);
      gimp_progress_pulse ();
      webkit_web_view_get_snapshot (WEBKIT_WEB_VIEW (view),
                                    WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT,
                                    WEBKIT_SNAPSHOT_OPTIONS_NONE,
                                    NULL,
                                    snapshot_ready,
                                    &result);

      gtk_main ();

      if (result.error != NULL)
        g_propagate_error (error, result.error);
    }

  gtk_widget_destroy (window);

  gimp_progress_update (1.0);

  return result.image;
}
