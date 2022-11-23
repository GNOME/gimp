/* LIGMA - The GNU Image Manipulation Program
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include <webkit2/webkit2.h>

#include "libligma/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-web-page"
#define PLUG_IN_BINARY "web-page"
#define PLUG_IN_ROLE   "ligma-web-page"
#define MAX_URL_LEN    2048


typedef struct
{
  char      *url;
  gint32     width;
  gint       font_size;
  LigmaImage *image;
  GError    *error;
} WebpageVals;

typedef struct
{
  char   url[MAX_URL_LEN];
  gint32 width;
  gint   font_size;
} WebpageSaveVals;


typedef struct _Webpage      Webpage;
typedef struct _WebpageClass WebpageClass;

struct _Webpage
{
  LigmaPlugIn parent_instance;
};

struct _WebpageClass
{
  LigmaPlugInClass parent_class;
};


#define WEBPAGE_TYPE  (webpage_get_type ())
#define WEBPAGE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBPAGE_TYPE, Webpage))

GType                   webpage_get_type         (void) G_GNUC_CONST;

static GList          * webpage_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * webpage_create_procedure (LigmaPlugIn           *plug_in,
                                                  const gchar          *name);

static LigmaValueArray * webpage_run              (LigmaProcedure        *procedure,
                                                  const LigmaValueArray *args,
                                                  gpointer              run_data);

static gboolean         webpage_dialog           (void);
static LigmaImage      * webpage_capture          (void);


G_DEFINE_TYPE (Webpage, webpage, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (WEBPAGE_TYPE)
DEFINE_STD_SET_I18N


static WebpageVals webpagevals;


static void
webpage_class_init (WebpageClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = webpage_query_procedures;
  plug_in_class->create_procedure = webpage_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
webpage_init (Webpage *webpage)
{
}

static GList *
webpage_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
webpage_create_procedure (LigmaPlugIn  *plug_in,
                          const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = ligma_procedure_new (plug_in, name,
                                      LIGMA_PDB_PROC_TYPE_PLUGIN,
                                      webpage_run, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("From _Webpage..."));
      ligma_procedure_add_menu_path (procedure, "<Image>/File/Create/Acquire");

      ligma_procedure_set_documentation (procedure,
                                        _("Create an image of a webpage"),
                                        "The plug-in allows you to take a "
                                        "screenshot of a webpage.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Mukund Sivaraman <muks@banu.com>",
                                      "2011",
                                      "2011");

      LIGMA_PROC_ARG_ENUM (procedure, "run-mode",
                          "Run mode",
                          "The run mode",
                          LIGMA_TYPE_RUN_MODE,
                          LIGMA_RUN_INTERACTIVE,
                          G_PARAM_READWRITE);

      LIGMA_PROC_ARG_STRING (procedure, "url",
                            "URL",
                            "URL of the webpage to screenshot",
                            "http://www.ligma.org/",
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "width",
                         "Width",
                         "The width of the screenshot (in pixels)",
                         100, LIGMA_MAX_IMAGE_SIZE, 1024,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "font-size",
                         "Font size",
                         "The font size to use in the page (in pt)",
                         1, 1000, 12,
                         G_PARAM_READWRITE);

      LIGMA_PROC_VAL_IMAGE (procedure, "image",
                           "Image",
                           "The output image",
                           FALSE,
                           G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
webpage_run (LigmaProcedure        *procedure,
             const LigmaValueArray *args,
             gpointer              run_data)
{
  LigmaValueArray  *return_vals;
  LigmaRunMode      run_mode;
  LigmaImage       *image;
  WebpageSaveVals  save = { "https://www.ligma.org/", 1024, 12 };

  ligma_get_data (PLUG_IN_PROC, &save);

  run_mode = LIGMA_VALUES_GET_ENUM (args, 0);

  webpagevals.url       = g_strdup (save.url);
  webpagevals.width     = save.width;
  webpagevals.font_size = save.font_size;

  /* how are we running today? */
  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
      if (! webpage_dialog ())
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    case LIGMA_RUN_WITH_LAST_VALS:
      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               NULL);
      break;

    case LIGMA_RUN_NONINTERACTIVE:
      webpagevals.url       = (gchar *) LIGMA_VALUES_GET_STRING (args, 1);
      webpagevals.width     = LIGMA_VALUES_GET_INT              (args, 2);
      webpagevals.font_size = LIGMA_VALUES_GET_INT              (args, 3);
      break;

    default:
      break;
    }

  image = webpage_capture ();

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             webpagevals.error);

  save.width     = webpagevals.width;
  save.font_size = webpagevals.font_size;

  if (strlen (webpagevals.url) < MAX_URL_LEN)
    {
      g_strlcpy (save.url, webpagevals.url, MAX_URL_LEN);
    }
  else
    {
      memset (save.url, 0, MAX_URL_LEN);
    }

  ligma_set_data (PLUG_IN_PROC, &save, sizeof save);

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    ligma_display_new (image);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static gboolean
webpage_dialog (void)
{
  GtkWidget     *dialog;
  GtkWidget     *hbox;
  GtkWidget     *vbox;
  GtkWidget     *image;
  GtkWidget     *label;
  GtkWidget     *entry;
  GtkSizeGroup  *sizegroup;
  GtkAdjustment *adjustment;
  GtkWidget     *spinbutton;
  GtkWidget     *combo;
  gint           active;
  gint           status;
  gboolean       ret = FALSE;

  ligma_ui_init (PLUG_IN_BINARY);

  dialog = ligma_dialog_new (_("Create from webpage"), PLUG_IN_ROLE,
                            NULL, 0,
                            ligma_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("Cre_ate"), GTK_RESPONSE_OK,

                            NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  ligma_window_set_transient (GTK_WINDOW (dialog));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_WEB,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Enter location (URI):"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_widget_set_size_request (entry, 400, -1);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  if (webpagevals.url)
    gtk_entry_set_text (GTK_ENTRY (entry),
                        webpagevals.url);
  gtk_widget_show (entry);

  sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* Width */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Width (pixels):"));
  gtk_size_group_add_widget (sizegroup, label);

  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (webpagevals.width,
                                   1, 8192, 1, 10, 0);
  spinbutton = ligma_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  /* Font size */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Font size:"));
  gtk_size_group_add_widget (sizegroup, label);

  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = ligma_int_combo_box_new (_("Huge"), 16,
                                  _("Large"), 14,
                                  C_("web-page", "Default"), 12,
                                  _("Small"), 10,
                                  _("Tiny"), 8,
                                  NULL);

  switch (webpagevals.font_size)
    {
    case 16:
    case 14:
    case 12:
    case 10:
    case 8:
      active = webpagevals.font_size;
      break;
    default:
      active = 12;
    }

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (combo), active);

  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  g_object_unref (sizegroup);

  status = ligma_dialog_run (LIGMA_DIALOG (dialog));
  if (status == GTK_RESPONSE_OK)
    {
      g_free (webpagevals.url);
      webpagevals.url = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

      webpagevals.width = (gint) gtk_adjustment_get_value (adjustment);

      ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (combo),
                                     &webpagevals.font_size);

      ret = TRUE;
    }

  gtk_widget_destroy (dialog);

  return ret;
}

static void
notify_progress_cb (WebKitWebView  *view,
                    GParamSpec     *pspec,
                    gpointer        user_data)
{
  static gdouble old_progress = 0.0;
  gdouble progress = webkit_web_view_get_estimated_load_progress (view);

  if ((progress - old_progress) > 0.01)
    {
      ligma_progress_update (progress);
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
  webpagevals.error = g_error_copy ((GError *) web_error);

  gtk_main_quit ();

  return TRUE;
}

static void
snapshot_ready (GObject      *source_object,
                GAsyncResult *result,
                gpointer      user_data)
{
  WebKitWebView   *view = WEBKIT_WEB_VIEW (source_object);
  cairo_surface_t *surface;

  surface = webkit_web_view_get_snapshot_finish (view, result,
                                                 &webpagevals.error);

  if (surface)
    {
      gint       width;
      gint       height;
      LigmaLayer *layer;

      width  = cairo_image_surface_get_width (surface);
      height = cairo_image_surface_get_height (surface);

      webpagevals.image = ligma_image_new (width, height, LIGMA_RGB);

      ligma_image_undo_disable (webpagevals.image);
      layer = ligma_layer_new_from_surface (webpagevals.image, _("Webpage"),
                                           surface,
                                           0.25, 1.0);
      ligma_image_insert_layer (webpagevals.image, layer, NULL, 0);
      ligma_image_undo_enable (webpagevals.image);

      cairo_surface_destroy (surface);
    }

  ligma_progress_update (1.0);

  gtk_main_quit ();
}

static gboolean
load_finished_idle (gpointer data)
{
  static gint count = 0;

  ligma_progress_update ((gdouble) count * 0.025);

  count++;

  if (count < 10)
    return G_SOURCE_CONTINUE;

  webkit_web_view_get_snapshot (WEBKIT_WEB_VIEW (data),
                                WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT,
                                WEBKIT_SNAPSHOT_OPTIONS_NONE,
                                NULL,
                                snapshot_ready,
                                NULL);

  count = 0;

  return G_SOURCE_REMOVE;
}

static void
load_changed_cb (WebKitWebView   *view,
                 WebKitLoadEvent  event,
                 gpointer         user_data)
{
  if (event == WEBKIT_LOAD_FINISHED)
    {
      if (! webpagevals.error)
        {
          ligma_progress_init_printf (_("Transferring webpage image for '%s'"),
                                     webpagevals.url);

          g_timeout_add (100, load_finished_idle, view);

          return;
        }

      gtk_main_quit ();
    }
}

static LigmaImage *
webpage_capture (void)
{
  gchar          *scheme;
  GtkWidget      *window;
  GtkWidget      *view;
  WebKitSettings *settings;
  char           *ua;

  if (! webpagevals.url || strlen (webpagevals.url) == 0)
    {
      g_set_error (&webpagevals.error, 0, 0, _("No URL was specified"));
      return NULL;
    }

  scheme = g_uri_parse_scheme (webpagevals.url);
  if (! scheme)
    {
      char *url;

      /* If we were not given a well-formed URL, make one. */

      url = g_strconcat ("http://", webpagevals.url, NULL);
      g_free (webpagevals.url);
      webpagevals.url = url;

      g_free (scheme);
    }

  if (webpagevals.width < 32)
    {
      g_warning ("Width '%d' is too small. Clamped to 32.",
                 webpagevals.width);
      webpagevals.width = 32;
    }
  else if (webpagevals.width > 8192)
    {
      g_warning ("Width '%d' is too large. Clamped to 8192.",
                 webpagevals.width);
      webpagevals.width = 8192;
    }

  window = gtk_offscreen_window_new ();
  gtk_widget_show (window);

  view = webkit_web_view_new ();
  gtk_widget_show (view);

  gtk_widget_set_vexpand (view, TRUE);
  gtk_widget_set_size_request (view, webpagevals.width, -1);
  gtk_container_add (GTK_CONTAINER (window), view);

  /* Append "LIGMA/<LIGMA_VERSION>" to the user agent string */
  settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (view));
  ua = g_strdup_printf ("%s LIGMA/%s",
                        webkit_settings_get_user_agent (settings),
                        LIGMA_VERSION);
  webkit_settings_set_user_agent (settings, ua);
  g_free (ua);

  /* Set font size */
  webkit_settings_set_default_font_size (settings, webpagevals.font_size);

  g_signal_connect (view, "notify::estimated-load-progress",
                    G_CALLBACK (notify_progress_cb),
                    window);
  g_signal_connect (view, "load-failed",
                    G_CALLBACK (load_failed_cb),
                    window);
  g_signal_connect (view, "load-changed",
                    G_CALLBACK (load_changed_cb),
                    window);

  ligma_progress_init_printf (_("Downloading webpage '%s'"), webpagevals.url);

  webkit_web_view_load_uri (WEBKIT_WEB_VIEW (view),
                            webpagevals.url);

  gtk_main ();

  gtk_widget_destroy (window);

  ligma_progress_update (1.0);

  return webpagevals.image;
}
