/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfiledialog.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"

#include "config/gimpguiconfig.h"

#include "file/file-utils.h"
#include "file/gimp-file.h"

#include "pdb/gimppdb.h"

#include "plug-in/gimppluginmanager.h"
#include "plug-in/gimppluginprocedure.h"

#include "gimpfiledialog.h"
#include "gimpfileprocview.h"
#include "gimphelp-ids.h"
#include "gimpprogressbox.h"
#include "gimpview.h"
#include "gimpviewrendererimagefile.h"
#include "gimpthumbbox.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


/*  an arbitrary limit to keep the file dialog from becoming too wide  */
#define MAX_EXTENSIONS  4


struct _GimpFileDialogState
{
  gchar *filter_name;
};


static void     gimp_file_dialog_progress_iface_init    (GimpProgressInterface *iface);

static void     gimp_file_dialog_dispose                (GObject          *object);

static gboolean gimp_file_dialog_delete_event           (GtkWidget        *widget,
                                                         GdkEventAny      *event);
static void     gimp_file_dialog_response               (GtkDialog        *dialog,
                                                         gint              response_id);
static GimpProgress *
                gimp_file_dialog_progress_start         (GimpProgress     *progress,
                                                         const gchar      *message,
                                                         gboolean          cancelable);
static void     gimp_file_dialog_progress_end           (GimpProgress     *progress);
static gboolean gimp_file_dialog_progress_is_active     (GimpProgress     *progress);
static void     gimp_file_dialog_progress_set_text      (GimpProgress     *progress,
                                                         const gchar      *message);
static void     gimp_file_dialog_progress_set_value     (GimpProgress     *progress,
                                                         gdouble           percentage);
static gdouble  gimp_file_dialog_progress_get_value     (GimpProgress     *progress);
static void     gimp_file_dialog_progress_pulse         (GimpProgress     *progress);
static guint32  gimp_file_dialog_progress_get_window_id (GimpProgress     *progress);

static void     gimp_file_dialog_add_user_dir           (GimpFileDialog   *dialog,
                                                         GUserDirectory    directory);
static void     gimp_file_dialog_add_preview            (GimpFileDialog   *dialog,
                                                         Gimp             *gimp);
static void     gimp_file_dialog_add_filters            (GimpFileDialog   *dialog,
                                                         Gimp             *gimp,
                                                         GimpFileChooserAction
                                                                           action,
                                                         GSList           *file_procs,
                                                         GSList           *file_procs_all_images);
static void     gimp_file_dialog_process_procedure      (GimpPlugInProcedure
                                                                          *file_proc,
                                                         GtkFileFilter   **filter_out,
                                                         GtkFileFilter    *all,
                                                         GtkFileFilter    *all_savable);
static void     gimp_file_dialog_add_proc_selection     (GimpFileDialog   *dialog,
                                                         Gimp             *gimp,
                                                         GSList           *file_procs,
                                                         const gchar      *automatic,
                                                         const gchar      *automatic_help_id);

static void     gimp_file_dialog_selection_changed      (GtkFileChooser   *chooser,
                                                         GimpFileDialog   *dialog);
static void     gimp_file_dialog_update_preview         (GtkFileChooser   *chooser,
                                                         GimpFileDialog   *dialog);

static void     gimp_file_dialog_proc_changed           (GimpFileProcView *view,
                                                         GimpFileDialog   *dialog);

static void     gimp_file_dialog_help_func              (const gchar      *help_id,
                                                         gpointer          help_data);
static void     gimp_file_dialog_help_clicked           (GtkWidget        *widget,
                                                         gpointer          dialog);

static gchar  * gimp_file_dialog_pattern_from_extension (const gchar   *extension);
static gchar  * gimp_file_dialog_get_documents_uri      (void);
static gchar  * gimp_file_dialog_get_dirname_from_uri   (const gchar   *uri);



G_DEFINE_TYPE_WITH_CODE (GimpFileDialog, gimp_file_dialog,
                         GTK_TYPE_FILE_CHOOSER_DIALOG,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_file_dialog_progress_iface_init))

#define parent_class gimp_file_dialog_parent_class


static void
gimp_file_dialog_class_init (GimpFileDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->dispose      = gimp_file_dialog_dispose;

  widget_class->delete_event = gimp_file_dialog_delete_event;

  dialog_class->response     = gimp_file_dialog_response;
}

static void
gimp_file_dialog_init (GimpFileDialog *dialog)
{
}

static void
gimp_file_dialog_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start         = gimp_file_dialog_progress_start;
  iface->end           = gimp_file_dialog_progress_end;
  iface->is_active     = gimp_file_dialog_progress_is_active;
  iface->set_text      = gimp_file_dialog_progress_set_text;
  iface->set_value     = gimp_file_dialog_progress_set_value;
  iface->get_value     = gimp_file_dialog_progress_get_value;
  iface->pulse         = gimp_file_dialog_progress_pulse;
  iface->get_window_id = gimp_file_dialog_progress_get_window_id;
}

static void
gimp_file_dialog_dispose (GObject *object)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (object);

  G_OBJECT_CLASS (parent_class)->dispose (object);

  dialog->progress = NULL;
}

static gboolean
gimp_file_dialog_delete_event (GtkWidget   *widget,
                               GdkEventAny *event)
{
  return TRUE;
}

static void
gimp_file_dialog_response (GtkDialog *dialog,
                           gint       response_id)
{
  GimpFileDialog *file_dialog = GIMP_FILE_DIALOG (dialog);

  if (response_id != GTK_RESPONSE_OK && file_dialog->busy)
    {
      file_dialog->canceled = TRUE;

      if (file_dialog->progress                             &&
          GIMP_PROGRESS_BOX (file_dialog->progress)->active &&
          GIMP_PROGRESS_BOX (file_dialog->progress)->cancelable)
        {
          gimp_progress_cancel (GIMP_PROGRESS (dialog));
        }
    }
}

static GimpProgress *
gimp_file_dialog_progress_start (GimpProgress *progress,
                                 const gchar  *message,
                                 gboolean      cancelable)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);
  GimpProgress   *retval = NULL;

  if (dialog->progress)
    {
      retval = gimp_progress_start (GIMP_PROGRESS (dialog->progress),
                                    message, cancelable);
      gtk_widget_show (dialog->progress);

      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_CANCEL, cancelable);
    }

  return retval;
}

static void
gimp_file_dialog_progress_end (GimpProgress *progress)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  if (dialog->progress)
    {
      gimp_progress_end (GIMP_PROGRESS (dialog->progress));
      gtk_widget_hide (dialog->progress);
    }
}

static gboolean
gimp_file_dialog_progress_is_active (GimpProgress *progress)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  if (dialog->progress)
    return gimp_progress_is_active (GIMP_PROGRESS (dialog->progress));

  return FALSE;
}

static void
gimp_file_dialog_progress_set_text (GimpProgress *progress,
                                    const gchar  *message)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  if (dialog->progress)
    gimp_progress_set_text (GIMP_PROGRESS (dialog->progress), message);
}

static void
gimp_file_dialog_progress_set_value (GimpProgress *progress,
                                     gdouble       percentage)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  if (dialog->progress)
    gimp_progress_set_value (GIMP_PROGRESS (dialog->progress), percentage);
}

static gdouble
gimp_file_dialog_progress_get_value (GimpProgress *progress)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  if (dialog->progress)
    return gimp_progress_get_value (GIMP_PROGRESS (dialog->progress));

  return 0.0;
}

static void
gimp_file_dialog_progress_pulse (GimpProgress *progress)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  if (dialog->progress)
    gimp_progress_pulse (GIMP_PROGRESS (dialog->progress));
}

static guint32
gimp_file_dialog_progress_get_window_id (GimpProgress *progress)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  return gimp_window_get_native_id (GTK_WINDOW (dialog));
}


/*  public functions  */

GtkWidget *
gimp_file_dialog_new (Gimp                  *gimp,
                      GimpFileChooserAction  action,
                      const gchar           *title,
                      const gchar           *role,
                      const gchar           *stock_id,
                      const gchar           *help_id)
{
  GimpFileDialog       *dialog                = NULL;
  GSList               *file_procs            = NULL;
  GSList               *file_procs_all_images = NULL;
  const gchar          *automatic             = NULL;
  const gchar          *automatic_help_id     = NULL;
  gboolean              local_only            = FALSE;
  GtkFileChooserAction  gtk_action            = 0;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);

  switch (action)
    {
    case GIMP_FILE_CHOOSER_ACTION_OPEN:
      gtk_action            = GTK_FILE_CHOOSER_ACTION_OPEN;
      file_procs            = gimp->plug_in_manager->load_procs;
      file_procs_all_images = NULL;
      automatic             = _("Automatically Detected");
      automatic_help_id     = GIMP_HELP_FILE_OPEN_BY_EXTENSION;

      /* FIXME */
      local_only = (gimp_pdb_lookup_procedure (gimp->pdb,
                                               "file-uri-load") == NULL);
      break;

    case GIMP_FILE_CHOOSER_ACTION_SAVE:
    case GIMP_FILE_CHOOSER_ACTION_EXPORT:
      gtk_action            = GTK_FILE_CHOOSER_ACTION_SAVE;
      file_procs            = (action == GIMP_FILE_CHOOSER_ACTION_SAVE ?
                               gimp->plug_in_manager->save_procs :
                               gimp->plug_in_manager->export_procs);
      file_procs_all_images = (action == GIMP_FILE_CHOOSER_ACTION_SAVE ?
                               gimp->plug_in_manager->export_procs :
                               gimp->plug_in_manager->save_procs);
      automatic             = _("By Extension");
      automatic_help_id     = GIMP_HELP_FILE_SAVE_BY_EXTENSION;

      /* FIXME */
      local_only = (gimp_pdb_lookup_procedure (gimp->pdb,
                                               "file-uri-save") == NULL);
      break;

    default:
      g_return_val_if_reached (NULL);
      return NULL;
    }

  dialog = g_object_new (GIMP_TYPE_FILE_DIALOG,
                         "title",                     title,
                         "role",                      role,
                         "action",                    gtk_action,
                         "local-only",                local_only,
                         "do-overwrite-confirmation", TRUE,
                         NULL);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          stock_id,         GTK_RESPONSE_OK,
                          NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_help_connect (GTK_WIDGET (dialog),
                     gimp_file_dialog_help_func, help_id, dialog);

  if (GIMP_GUI_CONFIG (gimp->config)->show_help_button && help_id)
    {
      GtkWidget *action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));
      GtkWidget *button      = gtk_button_new_from_stock (GTK_STOCK_HELP);

      gtk_box_pack_end (GTK_BOX (action_area), button, FALSE, TRUE, 0);
      gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (action_area),
                                          button, TRUE);
      gtk_widget_show (button);

      g_object_set_data_full (G_OBJECT (dialog), "gimp-dialog-help-id",
                              g_strdup (help_id),
                              (GDestroyNotify) g_free);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (gimp_file_dialog_help_clicked),
                        dialog);

      g_object_set_data (G_OBJECT (dialog), "gimp-dialog-help-button", button);
    }

  gimp_file_dialog_add_user_dir (dialog, G_USER_DIRECTORY_PICTURES);
  gimp_file_dialog_add_user_dir (dialog, G_USER_DIRECTORY_DOCUMENTS);

  gimp_file_dialog_add_preview (dialog, gimp);

  gimp_file_dialog_add_filters (dialog, gimp, action,
                                file_procs,
                                file_procs_all_images);

  gimp_file_dialog_add_proc_selection (dialog, gimp, file_procs, automatic,
                                       automatic_help_id);

  dialog->progress = gimp_progress_box_new ();
  gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                    dialog->progress, FALSE, FALSE, 0);

  return GTK_WIDGET (dialog);
}

void
gimp_file_dialog_set_sensitive (GimpFileDialog *dialog,
                                gboolean        sensitive)
{
  GtkWidget *content_area;
  GList     *children;
  GList     *list;

  g_return_if_fail (GIMP_IS_FILE_DIALOG (dialog));

  /*  bail out if we are already destroyed  */
  if (! dialog->progress)
    return;

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  children = gtk_container_get_children (GTK_CONTAINER (content_area));

  for (list = children; list; list = g_list_next (list))
    {
      /*  skip the last item (the action area) */
      if (! g_list_next (list))
        break;

      gtk_widget_set_sensitive (list->data, sensitive);
    }

  g_list_free (children);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                     GTK_RESPONSE_CANCEL, sensitive);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                     GTK_RESPONSE_OK, sensitive);

  dialog->busy     = ! sensitive;
  dialog->canceled = FALSE;
}

void
gimp_file_dialog_set_file_proc (GimpFileDialog      *dialog,
                                GimpPlugInProcedure *file_proc)
{
  g_return_if_fail (GIMP_IS_FILE_DIALOG (dialog));

  if (file_proc != dialog->file_proc)
    gimp_file_proc_view_set_proc (GIMP_FILE_PROC_VIEW (dialog->proc_view),
                                  file_proc);
}

void
gimp_file_dialog_set_open_image (GimpFileDialog *dialog,
                                 GimpImage      *image,
                                 gboolean        open_as_layers)
{
  g_return_if_fail (GIMP_IS_FILE_DIALOG (dialog));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  dialog->image          = image;
  dialog->open_as_layers = open_as_layers;
}

void
gimp_file_dialog_set_save_image (GimpFileDialog *dialog,
                                 Gimp           *gimp,
                                 GimpImage      *image,
                                 gboolean        save_a_copy,
                                 gboolean        export,
                                 gboolean        close_after_saving,
                                 GimpObject     *display)
{
  const gchar *dir_uri  = NULL;
  const gchar *name_uri = NULL;
  const gchar *ext_uri  = NULL;
  gchar       *docs_uri = NULL;
  gchar       *dirname  = NULL;
  gchar       *basename = NULL;

  g_return_if_fail (GIMP_IS_FILE_DIALOG (dialog));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  docs_uri = gimp_file_dialog_get_documents_uri ();

  dialog->image              = image;
  dialog->save_a_copy        = save_a_copy;
  dialog->export             = export;
  dialog->close_after_saving = close_after_saving;
  dialog->display_to_close   = display;

  gimp_file_dialog_set_file_proc (dialog, NULL);

  if (! export)
    {
      /*
       * Priority of default paths for Save:
       *
       *   1. Last Save a copy-path (applies only to Save a copy)
       *   2. Last Save path
       *   3. Path of source XCF
       *   4. Path of Import source
       *   5. Last Save path of any GIMP document
       *   6. The OS 'Documents' path
       */

      if (save_a_copy)
        dir_uri = gimp_image_get_save_a_copy_uri (image);

      if (! dir_uri)
        dir_uri = gimp_image_get_uri (image);

      if (! dir_uri)
        dir_uri = g_object_get_data (G_OBJECT (image),
                                     "gimp-image-source-uri");

      if (! dir_uri)
        dir_uri = gimp_image_get_imported_uri (image);

      if (! dir_uri)
        dir_uri = g_object_get_data (G_OBJECT (gimp),
                                     GIMP_FILE_SAVE_LAST_URI_KEY);

      if (! dir_uri)
        dir_uri = docs_uri;


      /* Priority of default basenames for Save:
       *
       *   1. Last Save a copy-name (applies only to Save a copy)
       *   2. Last Save name
       *   3. Last Export name
       *   3. The source image path
       *   3. 'Untitled'
       */

      if (save_a_copy)
        name_uri = gimp_image_get_save_a_copy_uri (image);

      if (! name_uri)
        name_uri = gimp_image_get_uri (image);

      if (! name_uri)
        name_uri = gimp_image_get_exported_uri (image);

      if (! name_uri)
        name_uri = gimp_image_get_imported_uri (image);

      if (! name_uri)
        name_uri = gimp_image_get_string_untitled ();


      /* Priority of default type/extension for Save:
       *
       *   1. Type of last Save
       *   2. .xcf (which we don't explicitly append)
       */
      ext_uri = gimp_image_get_uri (image);

      if (! ext_uri)
        ext_uri = "file:///we/only/care/about/extension.xcf";
    }
  else /* if (export) */
    {
      /*
       * Priority of default paths for Export:
       *
       *   1. Last Export path
       *   2. Path of import source
       *   3. Path of XCF source
       *   4. Last path of any save to XCF
       *   5. Last Export path of any document
       *   6. The OS 'Documents' path
       */

      dir_uri = gimp_image_get_exported_uri (image);

      if (! dir_uri)
        dir_uri = g_object_get_data (G_OBJECT (image),
                                     "gimp-image-source-uri");

      if (! dir_uri)
        dir_uri = gimp_image_get_imported_uri (image);

      if (! dir_uri)
        dir_uri = gimp_image_get_uri (image);

      if (! dir_uri)
        dir_uri = g_object_get_data (G_OBJECT (gimp),
                                     GIMP_FILE_SAVE_LAST_URI_KEY);

      if (! dir_uri)
        dir_uri = g_object_get_data (G_OBJECT (gimp),
                                     GIMP_FILE_EXPORT_LAST_URI_KEY);

      if (! dir_uri)
        dir_uri = docs_uri;


      /* Priority of default basenames for Export:
       *
       *   1. Last Export name
       *   3. Save URI
       *   2. Source file name
       *   3. 'Untitled'
       */

      name_uri = gimp_image_get_exported_uri (image);

      if (! name_uri)
        name_uri = gimp_image_get_uri (image);

      if (! name_uri)
        name_uri = gimp_image_get_imported_uri (image);

      if (! name_uri)
        name_uri = gimp_image_get_string_untitled ();


      /* Priority of default type/extension for Export:
       *
       *   1. Type of last Export
       *   2. Type of the image Import
       *   3. Type of latest Export of any document
       *   4. .png
       */
      ext_uri = gimp_image_get_exported_uri (image);

      if (! ext_uri)
        ext_uri = gimp_image_get_imported_uri (image);

      if (! ext_uri)
        ext_uri = g_object_get_data (G_OBJECT (gimp),
                                     GIMP_FILE_EXPORT_LAST_URI_KEY);

      if (! ext_uri)
        ext_uri = "file:///we/only/care/about/extension.png";
    }

  dirname = gimp_file_dialog_get_dirname_from_uri (dir_uri);

  if (ext_uri)
    {
      gchar *uri_new_ext = file_utils_uri_with_new_ext (name_uri,
                                                        ext_uri);
      basename = file_utils_uri_display_basename (uri_new_ext);
      g_free (uri_new_ext);
    }
  else
    {
      basename = file_utils_uri_display_basename (name_uri);
    }

  gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dialog), dirname);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), basename);

  g_free (docs_uri);
  g_free (basename);
  g_free (dirname);
}

GimpFileDialogState *
gimp_file_dialog_get_state (GimpFileDialog *dialog)
{
  GimpFileDialogState *state;
  GtkFileFilter       *filter;

  g_return_val_if_fail (GIMP_IS_FILE_DIALOG (dialog), NULL);

  state = g_slice_new0 (GimpFileDialogState);

  filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (dialog));

  if (filter)
    state->filter_name = g_strdup (gtk_file_filter_get_name (filter));

  return state;
}

void
gimp_file_dialog_set_state (GimpFileDialog      *dialog,
                            GimpFileDialogState *state)
{
  g_return_if_fail (GIMP_IS_FILE_DIALOG (dialog));
  g_return_if_fail (state != NULL);

  if (state->filter_name)
    {
      GSList *filters;
      GSList *list;

      filters = gtk_file_chooser_list_filters (GTK_FILE_CHOOSER (dialog));

      for (list = filters; list; list = list->next)
        {
          GtkFileFilter *filter = GTK_FILE_FILTER (list->data);
          const gchar   *name   = gtk_file_filter_get_name (filter);

          if (name && strcmp (state->filter_name, name) == 0)
            {
              gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);
              break;
            }
        }

      g_slist_free (filters);
    }
}

void
gimp_file_dialog_state_destroy (GimpFileDialogState *state)
{
  g_return_if_fail (state != NULL);

  g_free (state->filter_name);
  g_slice_free (GimpFileDialogState, state);
}


/*  private functions  */

static void
gimp_file_dialog_add_user_dir (GimpFileDialog *dialog,
                               GUserDirectory  directory)
{
  const gchar *user_dir = g_get_user_special_dir (directory);

  if (user_dir)
    gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
                                          user_dir, NULL);
}

static void
gimp_file_dialog_add_preview (GimpFileDialog *dialog,
                              Gimp           *gimp)
{
  if (gimp->config->thumbnail_size <= 0)
    return;

  gtk_file_chooser_set_use_preview_label (GTK_FILE_CHOOSER (dialog), FALSE);

  g_signal_connect (dialog, "selection-changed",
                    G_CALLBACK (gimp_file_dialog_selection_changed),
                    dialog);
  g_signal_connect (dialog, "update-preview",
                    G_CALLBACK (gimp_file_dialog_update_preview),
                    dialog);

  dialog->thumb_box = gimp_thumb_box_new (gimp_get_user_context (gimp));
  gtk_widget_set_sensitive (GTK_WIDGET (dialog->thumb_box), FALSE);
  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (dialog),
                                       dialog->thumb_box);
  gtk_widget_show (dialog->thumb_box);

#ifdef ENABLE_FILE_SYSTEM_ICONS
  GIMP_VIEW_RENDERER_IMAGEFILE (GIMP_VIEW (GIMP_THUMB_BOX (dialog->thumb_box)->preview)->renderer)->file_system = _gtk_file_chooser_get_file_system (GTK_FILE_CHOOSER (dialog));
#endif
}

/**
 * gimp_file_dialog_add_filters:
 * @dialog:
 * @gimp:
 * @action:
 * @file_procs:            The image types that can be chosen from
 *                         the file type list
 * @file_procs_all_images: The additional images types shown when
 *                         "All images" is selected
 *
 **/
static void
gimp_file_dialog_add_filters (GimpFileDialog        *dialog,
                              Gimp                  *gimp,
                              GimpFileChooserAction  action,
                              GSList                *file_procs,
                              GSList                *file_procs_all_images)
{
  GtkFileFilter *all;
  GtkFileFilter *all_savable = NULL;
  GSList        *list;

  all = gtk_file_filter_new ();
  gtk_file_filter_set_name (all, _("All files"));
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), all);
  gtk_file_filter_add_pattern (all, "*");

  all = gtk_file_filter_new ();
  gtk_file_filter_set_name (all, _("All images"));
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), all);

  if (file_procs_all_images)
    {
      all_savable = gtk_file_filter_new ();
      if (action == GIMP_FILE_CHOOSER_ACTION_SAVE)
        gtk_file_filter_set_name (all_savable, _("All XCF images"));
      else
        gtk_file_filter_set_name (all_savable, _("All export images"));
      gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), all_savable);
    }

  /* Add the normal file procs */
  for (list = file_procs; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *file_proc = list->data;
      GtkFileFilter       *filter    = NULL;

      gimp_file_dialog_process_procedure (file_proc,
                                          &filter,
                                          all, all_savable);
      if (filter)
        {
          gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog),
                                       filter);
          g_object_unref (filter);
        }
    }

  /* Add the "rest" of the file procs only as filters to
   * "All images"
   */
  for (list = file_procs_all_images; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *file_proc = list->data;

      gimp_file_dialog_process_procedure (file_proc,
                                          NULL,
                                          all, NULL);
    }

  if (all_savable)
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), all_savable);
  else
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), all);
}


/**
 * gimp_file_dialog_process_procedure:
 * @file_proc:
 * @filter_out:
 * @all:
 * @all_savable:
 *
 * Creates a #GtkFileFilter of @file_proc and adds the extensions to
 * the @all filter. The returned #GtkFileFilter has a normal ref and
 * must be unreffed when used.
 **/
static void
gimp_file_dialog_process_procedure (GimpPlugInProcedure  *file_proc,
                                    GtkFileFilter       **filter_out,
                                    GtkFileFilter        *all,
                                    GtkFileFilter        *all_savable)
{
  GtkFileFilter *filter = NULL;
  GString       *str    = NULL;
  GSList        *ext    = NULL;
  gint           i      = 0;

  if (!file_proc->extensions_list)
    return;

  filter = gtk_file_filter_new ();
  str    = g_string_new (gimp_plug_in_procedure_get_label (file_proc));

  /* Take ownership directly so we don't have to mess with a floating
   * ref
   */
  g_object_ref_sink (filter);

  for (ext = file_proc->extensions_list, i = 0;
       ext;
       ext = g_slist_next (ext), i++)
    {
      const gchar *extension = ext->data;
      gchar       *pattern;

      pattern = gimp_file_dialog_pattern_from_extension (extension);
      gtk_file_filter_add_pattern (filter, pattern);
      gtk_file_filter_add_pattern (all, pattern);
      if (all_savable)
        gtk_file_filter_add_pattern (all_savable, pattern);
      g_free (pattern);

      if (i == 0)
        {
          g_string_append (str, " (");
        }
      else if (i <= MAX_EXTENSIONS)
        {
          g_string_append (str, ", ");
        }

      if (i < MAX_EXTENSIONS)
        {
          g_string_append (str, "*.");
          g_string_append (str, extension);
        }
      else if (i == MAX_EXTENSIONS)
        {
          g_string_append (str, "...");
        }

      if (! ext->next)
        {
          g_string_append (str, ")");
        }
    }

  gtk_file_filter_set_name (filter, str->str);
  g_string_free (str, TRUE);

  if (filter_out)
    *filter_out = g_object_ref (filter);

  g_object_unref (filter);
}

static void
gimp_file_dialog_add_proc_selection (GimpFileDialog *dialog,
                                     Gimp           *gimp,
                                     GSList         *file_procs,
                                     const gchar    *automatic,
                                     const gchar    *automatic_help_id)
{
  GtkWidget *scrolled_window;

  dialog->proc_expander = gtk_expander_new_with_mnemonic (NULL);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog),
                                     dialog->proc_expander);
  gtk_widget_show (dialog->proc_expander);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (dialog->proc_expander), scrolled_window);
  gtk_widget_show (scrolled_window);

  gtk_widget_set_size_request (scrolled_window, -1, 200);

  dialog->proc_view = gimp_file_proc_view_new (gimp, file_procs, automatic,
                                               automatic_help_id);
  gtk_container_add (GTK_CONTAINER (scrolled_window), dialog->proc_view);
  gtk_widget_show (dialog->proc_view);

  g_signal_connect (dialog->proc_view, "changed",
                    G_CALLBACK (gimp_file_dialog_proc_changed),
                    dialog);

  gimp_file_proc_view_set_proc (GIMP_FILE_PROC_VIEW (dialog->proc_view), NULL);
}

static void
gimp_file_dialog_selection_changed (GtkFileChooser *chooser,
                                    GimpFileDialog *dialog)
{
  gimp_thumb_box_take_uris (GIMP_THUMB_BOX (dialog->thumb_box),
                            gtk_file_chooser_get_uris (chooser));
}

static void
gimp_file_dialog_update_preview (GtkFileChooser *chooser,
                                 GimpFileDialog *dialog)
{
  gimp_thumb_box_take_uri (GIMP_THUMB_BOX (dialog->thumb_box),
                           gtk_file_chooser_get_preview_uri (chooser));
}

static void
gimp_file_dialog_proc_changed (GimpFileProcView *view,
                               GimpFileDialog   *dialog)
{
  GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
  gchar          *name;

  dialog->file_proc = gimp_file_proc_view_get_proc (view, &name);

  if (name)
    {
      gchar *label = g_strdup_printf (_("Select File _Type (%s)"), name);

      gtk_expander_set_label (GTK_EXPANDER (dialog->proc_expander), label);

      g_free (label);
      g_free (name);
    }

  if (gtk_file_chooser_get_action (chooser) == GTK_FILE_CHOOSER_ACTION_SAVE)
    {
      GimpPlugInProcedure *proc = dialog->file_proc;

      if (proc && proc->extensions_list)
        {
          gchar *uri = gtk_file_chooser_get_uri (chooser);

          if (uri && strlen (uri))
            {
              const gchar *last_dot = strrchr (uri, '.');

              /*  if the dot is before the last slash, ignore it  */
              if (last_dot && strrchr (uri, '/') > last_dot)
                last_dot = NULL;

              /*  check if the uri has a "meta extension" (e.g. foo.bar.gz)
               *  and try to truncate both extensions away.
               */
              if (last_dot && last_dot != uri)
                {
                  GList *list;

                  for (list = view->meta_extensions;
                       list;
                       list = g_list_next (list))
                    {
                      const gchar *ext = list->data;

                      if (! strcmp (ext, last_dot + 1))
                        {
                          const gchar *p = last_dot - 1;

                          while (p > uri && *p != '.')
                            p--;

                          if (p != uri && *p == '.')
                            {
                              last_dot = p;
                              break;
                            }
                        }
                    }
                }

              if (last_dot != uri)
                {
                  GString *s = g_string_new (uri);
                  gchar   *basename;

                  if (last_dot)
                    g_string_truncate (s, last_dot - uri);

                  g_string_append (s, ".");
                  g_string_append (s, (gchar *) proc->extensions_list->data);

                  gtk_file_chooser_set_uri (chooser, s->str);

                  basename = file_utils_uri_display_basename (s->str);
                  gtk_file_chooser_set_current_name (chooser, basename);
                  g_free (basename);

                  g_string_free (s, TRUE);
                }
            }

          g_free (uri);
        }
    }
}

static void
gimp_file_dialog_help_func (const gchar *help_id,
                            gpointer     help_data)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (help_data);
  GtkWidget      *focus;

  focus = gtk_window_get_focus (GTK_WINDOW (dialog));

  if (focus == dialog->proc_view)
    {
      gchar *proc_help_id;

      proc_help_id =
        gimp_file_proc_view_get_help_id (GIMP_FILE_PROC_VIEW (dialog->proc_view));

      gimp_standard_help_func (proc_help_id, NULL);

      g_free (proc_help_id);
    }
  else
    {
      gimp_standard_help_func (help_id, NULL);
    }
}

static void
gimp_file_dialog_help_clicked (GtkWidget *widget,
                               gpointer   dialog)
{
  gimp_standard_help_func (g_object_get_data (dialog, "gimp-dialog-help-id"),
                           NULL);
}

static gchar *
gimp_file_dialog_pattern_from_extension (const gchar *extension)
{
  gchar *pattern;
  gchar *p;
  gint   len, i;

  g_return_val_if_fail (extension != NULL, NULL);

  /* This function assumes that file extensions are 7bit ASCII.  It
   * could certainly be rewritten to handle UTF-8 if this assumption
   * turns out to be incorrect.
   */

  len = strlen (extension);

  pattern = g_new (gchar, 4 + 4 * len);

  strcpy (pattern, "*.");

  for (i = 0, p = pattern + 2; i < len; i++, p+= 4)
    {
      p[0] = '[';
      p[1] = g_ascii_tolower (extension[i]);
      p[2] = g_ascii_toupper (extension[i]);
      p[3] = ']';
    }

  *p = '\0';

  return pattern;
}

static gchar *
gimp_file_dialog_get_documents_uri (void)
{
  gchar *path;
  gchar *uri;

  /* Make sure it ends in '/' */
  path = g_build_path (G_DIR_SEPARATOR_S,
                       g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS),
                       G_DIR_SEPARATOR_S,
                       NULL);
  uri = g_filename_to_uri (path, NULL, NULL);
  g_free (path);

  /* Paranoia fallback, see bug #722400 */
  if (! uri)
    {
      path = g_build_path (G_DIR_SEPARATOR_S,
                           g_get_home_dir (),
                           G_DIR_SEPARATOR_S,
                           NULL);
      uri = g_filename_to_uri (path, NULL, NULL);
      g_free (path);
    }

  return uri;
}

static gchar *
gimp_file_dialog_get_dirname_from_uri (const gchar *uri)
{
  gchar *dirname = NULL;

#ifndef G_OS_WIN32
  dirname  = g_path_get_dirname (uri);
#else
  /* g_path_get_dirname() is supposed to work on pathnames, not URIs.
   *
   * If uri points to a file on the root of a drive
   * "file:///d:/foo.png", g_path_get_dirname() would return
   * "file:///d:". (What we really would want is "file:///d:/".) When
   * this then is passed inside gtk+ to g_filename_from_uri() we get
   * "d:" which is not an absolute pathname. This currently causes an
   * assertion failure in gtk+. This scenario occurs if we have opened
   * an image from the root of a drive and then do Save As.
   *
   * Of course, gtk+ shouldn't assert even if we feed it slighly bogus
   * data, and that problem should be fixed, too. But to get the
   * correct default current folder in the filechooser combo box, we
   * need to pass it the proper URI for an absolute path anyway. So
   * don't use g_path_get_dirname() on file: URIs.
   */
  if (g_str_has_prefix (uri, "file:///"))
    {
      gchar *filepath = g_filename_from_uri (uri, NULL, NULL);
      gchar *dirpath  = NULL;

      if (filepath != NULL)
        {
          dirpath = g_path_get_dirname (filepath);
          g_free (filepath);
        }

      if (dirpath != NULL)
        {
          dirname = g_filename_to_uri (dirpath, NULL, NULL);
          g_free (dirpath);
        }
      else
        {
          dirname = NULL;
        }
    }
  else
    {
      dirname = g_path_get_dirname (uri);
    }
#endif

  return dirname;
}
