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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"

#include "config/gimpguiconfig.h"

#include "plug-in/gimppluginmanager-file.h"
#include "plug-in/gimppluginprocedure.h"

#include "gimpfiledialog.h"
#include "gimpfileprocview.h"
#include "gimpprogressbox.h"
#include "gimpthumbbox.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_HELP_ID,
  PROP_OK_BUTTON_LABEL,
  PROP_AUTOMATIC_HELP_ID,
  PROP_AUTOMATIC_LABEL,
  PROP_FILE_FILTER_LABEL,
  PROP_FILE_PROCS,
  PROP_FILE_PROCS_ALL_IMAGES,
  PROP_SHOW_ALL_FILES,
};

typedef struct _GimpFileDialogState GimpFileDialogState;

struct _GimpFileDialogState
{
  gchar *filter_name;
};


static void     gimp_file_dialog_progress_iface_init     (GimpProgressInterface *iface);

static void     gimp_file_dialog_set_property            (GObject             *object,
                                                          guint                property_id,
                                                          const GValue        *value,
                                                          GParamSpec          *pspec);
static void     gimp_file_dialog_get_property            (GObject             *object,
                                                          guint                property_id,
                                                          GValue              *value,
                                                          GParamSpec          *pspec);
static void     gimp_file_dialog_constructed             (GObject             *object);
static void     gimp_file_dialog_dispose                 (GObject             *object);

static gboolean gimp_file_dialog_delete_event            (GtkWidget           *widget,
                                                          GdkEventAny         *event);
static void     gimp_file_dialog_response                (GtkDialog           *dialog,
                                                          gint                 response_id);
#ifdef G_OS_WIN32
static void     gimp_file_dialog_realize                 (GimpFileDialog      *dialog,
                                                          gpointer             data);
#endif
static GFile  * gimp_file_dialog_real_get_default_folder (GimpFileDialog      *dialog);
static void     gimp_file_dialog_real_save_state         (GimpFileDialog      *dialog,
                                                          const gchar         *state_name);
static void     gimp_file_dialog_real_load_state         (GimpFileDialog      *dialog,
                                                          const gchar         *state_name);

static GimpProgress *
                gimp_file_dialog_progress_start          (GimpProgress        *progress,
                                                          gboolean             cancellable,
                                                          const gchar         *message);
static void     gimp_file_dialog_progress_end            (GimpProgress        *progress);
static gboolean gimp_file_dialog_progress_is_active      (GimpProgress        *progress);
static void     gimp_file_dialog_progress_set_text       (GimpProgress        *progress,
                                                          const gchar         *message);
static void     gimp_file_dialog_progress_set_value      (GimpProgress        *progress,
                                                          gdouble              percentage);
static gdouble  gimp_file_dialog_progress_get_value      (GimpProgress        *progress);
static void     gimp_file_dialog_progress_pulse          (GimpProgress        *progress);
static GBytes * gimp_file_dialog_progress_get_window_id  (GimpProgress        *progress);

static void     gimp_file_dialog_add_user_dir            (GimpFileDialog      *dialog,
                                                          GUserDirectory       directory);
static void     gimp_file_dialog_add_preview             (GimpFileDialog      *dialog);
static void     gimp_file_dialog_add_proc_selection      (GimpFileDialog      *dialog);

static void     gimp_file_dialog_selection_changed       (GtkFileChooser      *chooser,
                                                          GimpFileDialog      *dialog);
static void     gimp_file_dialog_update_preview          (GtkFileChooser      *chooser,
                                                          GimpFileDialog      *dialog);

static void     gimp_file_dialog_proc_changed            (GimpFileProcView    *view,
                                                          GimpFileDialog      *dialog);

static void     gimp_file_dialog_help_func               (const gchar         *help_id,
                                                          gpointer             help_data);

static GimpFileDialogState
              * gimp_file_dialog_get_state               (GimpFileDialog      *dialog);
static void     gimp_file_dialog_set_state               (GimpFileDialog      *dialog,
                                                          GimpFileDialogState *state);
static void     gimp_file_dialog_state_destroy           (GimpFileDialogState *state);



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

  object_class->set_property = gimp_file_dialog_set_property;
  object_class->get_property = gimp_file_dialog_get_property;
  object_class->constructed  = gimp_file_dialog_constructed;
  object_class->dispose      = gimp_file_dialog_dispose;

  widget_class->delete_event = gimp_file_dialog_delete_event;

  dialog_class->response     = gimp_file_dialog_response;

  klass->get_default_folder  = gimp_file_dialog_real_get_default_folder;
  klass->save_state          = gimp_file_dialog_real_save_state;
  klass->load_state          = gimp_file_dialog_real_load_state;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp", NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_HELP_ID,
                                   g_param_spec_string ("help-id", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_OK_BUTTON_LABEL,
                                   g_param_spec_string ("ok-button-label",
                                                        NULL, NULL,
                                                        _("_OK"),
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_AUTOMATIC_HELP_ID,
                                   g_param_spec_string ("automatic-help-id",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_AUTOMATIC_LABEL,
                                   g_param_spec_string ("automatic-label",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_FILE_FILTER_LABEL,
                                   g_param_spec_string ("file-filter-label",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_FILE_PROCS,
                                   g_param_spec_enum ("file-procs",
                                                      NULL, NULL,
                                                      GIMP_TYPE_FILE_PROCEDURE_GROUP,
                                                      GIMP_FILE_PROCEDURE_GROUP_NONE,
                                                      GIMP_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_FILE_PROCS_ALL_IMAGES,
                                   g_param_spec_enum ("file-procs-all-images",
                                                      NULL, NULL,
                                                      GIMP_TYPE_FILE_PROCEDURE_GROUP,
                                                      GIMP_FILE_PROCEDURE_GROUP_NONE,
                                                      GIMP_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SHOW_ALL_FILES,
                                   g_param_spec_boolean ("show-all-files",
                                                         NULL, NULL, FALSE,
                                                         GIMP_PARAM_READWRITE));

  gtk_widget_class_set_css_name (widget_class, "GimpFileDialog");
}

static void
gimp_file_dialog_init (GimpFileDialog *dialog)
{
#ifdef G_OS_WIN32
  g_signal_connect (dialog, "realize",
                    G_CALLBACK (gimp_file_dialog_realize),
                    NULL);
#endif
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
gimp_file_dialog_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (object);

  switch (property_id)
    {
    case PROP_GIMP:
      dialog->gimp = g_value_get_object (value);
      break;
    case PROP_HELP_ID:
      dialog->help_id = g_value_dup_string (value);
      break;
    case PROP_OK_BUTTON_LABEL:
      dialog->ok_button_label = g_value_dup_string (value);
      break;
    case PROP_AUTOMATIC_HELP_ID:
      dialog->automatic_help_id = g_value_dup_string (value);
      break;
    case PROP_AUTOMATIC_LABEL:
      dialog->automatic_label = g_value_dup_string (value);
      break;
    case PROP_FILE_FILTER_LABEL:
      dialog->file_filter_label = g_value_dup_string (value);
      break;
    case PROP_FILE_PROCS:
      dialog->file_procs =
        gimp_plug_in_manager_get_file_procedures (dialog->gimp->plug_in_manager,
                                                  g_value_get_enum (value));
      break;
    case PROP_FILE_PROCS_ALL_IMAGES:
      dialog->file_procs_all_images =
        gimp_plug_in_manager_get_file_procedures (dialog->gimp->plug_in_manager,
                                                  g_value_get_enum (value));
      break;
    case PROP_SHOW_ALL_FILES:
      dialog->show_all_files = g_value_get_boolean (value);
      gimp_file_dialog_proc_changed (GIMP_FILE_PROC_VIEW (dialog->proc_view),
                                     dialog);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_file_dialog_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, dialog->gimp);
      break;
    case PROP_HELP_ID:
      g_value_set_string (value, dialog->help_id);
      break;
    case PROP_SHOW_ALL_FILES:
      g_value_set_boolean (value, dialog->show_all_files);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_file_dialog_constructed (GObject *object)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          _("_Cancel"),            GTK_RESPONSE_CANCEL,
                          dialog->ok_button_label, GTK_RESPONSE_OK,
                          NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (object), FALSE);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (object),
                                                  TRUE);

  if (dialog->help_id)
    {
      gimp_help_connect (GTK_WIDGET (dialog), NULL,
                         gimp_file_dialog_help_func, dialog->help_id,
                         dialog, NULL);

      if (GIMP_GUI_CONFIG (dialog->gimp->config)->show_help_button)
        {
          gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                                  _("_Help"), GTK_RESPONSE_HELP,
                                  NULL);
        }
    }

  /* All classes derivated from GimpFileDialog should show these. */
  gimp_file_dialog_add_user_dir (dialog, G_USER_DIRECTORY_PICTURES);
  gimp_file_dialog_add_user_dir (dialog, G_USER_DIRECTORY_DOCUMENTS);

  gimp_file_dialog_add_preview (dialog);

  dialog->extra_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog),
                                     dialog->extra_vbox);
  gtk_widget_show (dialog->extra_vbox);

  gimp_file_dialog_add_proc_selection (dialog);

  dialog->progress = gimp_progress_box_new ();
  gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                    dialog->progress, FALSE, FALSE, 0);

  gimp_widget_set_native_handle (GTK_WIDGET (dialog), &dialog->window_handle);
}

static void
gimp_file_dialog_dispose (GObject *object)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (object);

  G_OBJECT_CLASS (parent_class)->dispose (object);

  dialog->progress = NULL;

  g_clear_pointer (&dialog->help_id,           g_free);
  g_clear_pointer (&dialog->ok_button_label,   g_free);
  g_clear_pointer (&dialog->automatic_help_id, g_free);
  g_clear_pointer (&dialog->automatic_label,   g_free);
  g_clear_pointer (&dialog->file_filter_label, g_free);

  gimp_widget_free_native_handle (GTK_WIDGET (dialog), &dialog->window_handle);
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

  if (response_id == GTK_RESPONSE_HELP)
    {
      gimp_standard_help_func (file_dialog->help_id, NULL);
      return;
    }

  if (response_id != GTK_RESPONSE_OK && file_dialog->busy)
    {
      file_dialog->canceled = TRUE;

      if (file_dialog->progress                             &&
          GIMP_PROGRESS_BOX (file_dialog->progress)->active &&
          GIMP_PROGRESS_BOX (file_dialog->progress)->cancellable)
        {
          gimp_progress_cancel (GIMP_PROGRESS (dialog));
        }
    }
}

#ifdef G_OS_WIN32
static void
gimp_file_dialog_realize (GimpFileDialog *dialog,
                          gpointer        data)
{
  gimp_window_set_title_bar_theme (dialog->gimp, GTK_WIDGET (dialog));
}
#endif

static GFile *
gimp_file_dialog_real_get_default_folder (GimpFileDialog *dialog)
{
  GFile *file = NULL;

  if (dialog->gimp->default_folder)
    {
      file = dialog->gimp->default_folder;
    }
  else
    {
      file = g_object_get_data (G_OBJECT (dialog->gimp),
                                "gimp-default-folder");

      if (! file)
        {
          gchar *path;

          /* Make sure the paths end with G_DIR_SEPARATOR_S */

#ifdef PLATFORM_OSX
          /* See bug 753683, "Desktop" is expected on OS X */
          path = g_build_path (G_DIR_SEPARATOR_S,
                               g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP),
                               G_DIR_SEPARATOR_S,
                               NULL);
#else
          path = g_build_path (G_DIR_SEPARATOR_S,
                               g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS),
                               G_DIR_SEPARATOR_S,
                               NULL);
#endif

          /* Paranoia fallback, see bug #722400 */
          if (! path)
            path = g_build_path (G_DIR_SEPARATOR_S,
                                 g_get_home_dir (),
                                 G_DIR_SEPARATOR_S,
                                 NULL);

          file = g_file_new_for_path (path);
          g_free (path);

          g_object_set_data_full (G_OBJECT (dialog->gimp),
                                  "gimp-default-folder",
                                  file, (GDestroyNotify) g_object_unref);
        }
    }

  return file;
}

static void
gimp_file_dialog_real_save_state (GimpFileDialog *dialog,
                                  const gchar    *state_name)
{
  g_object_set_data_full (G_OBJECT (dialog->gimp), state_name,
                          gimp_file_dialog_get_state (dialog),
                          (GDestroyNotify) gimp_file_dialog_state_destroy);
}

static void
gimp_file_dialog_real_load_state (GimpFileDialog *dialog,
                                  const gchar    *state_name)
{
  GimpFileDialogState *state;

  state = g_object_get_data (G_OBJECT (dialog->gimp), state_name);

  if (state)
    gimp_file_dialog_set_state (GIMP_FILE_DIALOG (dialog), state);
}

static GimpProgress *
gimp_file_dialog_progress_start (GimpProgress *progress,
                                 gboolean      cancellable,
                                 const gchar  *message)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);
  GimpProgress   *retval = NULL;

  if (dialog->progress)
    {
      retval = gimp_progress_start (GIMP_PROGRESS (dialog->progress),
                                    cancellable, "%s", message);
      gtk_widget_show (dialog->progress);

      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_CANCEL, cancellable);
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
    gimp_progress_set_text_literal (GIMP_PROGRESS (dialog->progress), message);
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

static GBytes *
gimp_file_dialog_progress_get_window_id (GimpProgress *progress)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  if (dialog && dialog->window_handle)
    return g_bytes_ref (dialog->window_handle);

  return NULL;
}


/*  public functions  */

void
gimp_file_dialog_add_extra_widget (GimpFileDialog *dialog,
                                   GtkWidget      *widget,
                                   gboolean        expand,
                                   gboolean        fill,
                                   guint           padding)
{
  gtk_box_pack_start (GTK_BOX (dialog->extra_vbox),
                      widget, expand, fill, padding);
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

GFile *
gimp_file_dialog_get_default_folder (GimpFileDialog *dialog)
{
  g_return_val_if_fail (GIMP_IS_FILE_DIALOG (dialog), NULL);

  return GIMP_FILE_DIALOG_GET_CLASS (dialog)->get_default_folder (dialog);
}

void
gimp_file_dialog_save_state (GimpFileDialog *dialog,
                             const gchar    *state_name)
{
  g_return_if_fail (GIMP_IS_FILE_DIALOG (dialog));

  GIMP_FILE_DIALOG_GET_CLASS (dialog)->save_state (dialog, state_name);
}

void
gimp_file_dialog_load_state (GimpFileDialog *dialog,
                             const gchar    *state_name)
{
  g_return_if_fail (GIMP_IS_FILE_DIALOG (dialog));

  GIMP_FILE_DIALOG_GET_CLASS (dialog)->load_state (dialog, state_name);
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
gimp_file_dialog_add_preview (GimpFileDialog *dialog)
{
  if (dialog->gimp->config->thumbnail_size <= 0)
    return;

  gtk_file_chooser_set_use_preview_label (GTK_FILE_CHOOSER (dialog), FALSE);

  g_signal_connect (dialog, "selection-changed",
                    G_CALLBACK (gimp_file_dialog_selection_changed),
                    dialog);
  g_signal_connect (dialog, "update-preview",
                    G_CALLBACK (gimp_file_dialog_update_preview),
                    dialog);

  dialog->thumb_box = gimp_thumb_box_new (gimp_get_user_context (dialog->gimp));
  gtk_widget_set_sensitive (GTK_WIDGET (dialog->thumb_box), FALSE);
  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (dialog),
                                       dialog->thumb_box);
  gtk_widget_show (dialog->thumb_box);

#ifdef ENABLE_FILE_SYSTEM_ICONS
  GIMP_VIEW_RENDERER_IMAGEFILE (GIMP_VIEW (GIMP_THUMB_BOX (dialog->thumb_box)->preview)->renderer)->file_system = _gtk_file_chooser_get_file_system (GTK_FILE_CHOOSER (dialog));
#endif
}

static void
gimp_file_dialog_add_proc_selection (GimpFileDialog *dialog)
{
  GtkWidget *box;
  GtkWidget *scrolled_window;
  GtkWidget *checkbox;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
  gimp_file_dialog_add_extra_widget (dialog, box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  dialog->proc_expander = gtk_expander_new_with_mnemonic (NULL);
  gtk_expander_set_resize_toplevel (GTK_EXPANDER (dialog->proc_expander), TRUE);
  gtk_widget_set_hexpand (GTK_WIDGET (dialog->proc_expander), TRUE);
  gtk_box_pack_end (GTK_BOX (box), dialog->proc_expander, FALSE, FALSE, 1);
  gtk_widget_show (dialog->proc_expander);

  /* The list of file formats. */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (dialog->proc_expander), scrolled_window);
  gtk_widget_show (scrolled_window);

  gtk_widget_set_size_request (scrolled_window, -1, 200);

  dialog->proc_view = gimp_file_proc_view_new (dialog->gimp,
                                               dialog->file_procs,
                                               dialog->automatic_label,
                                               dialog->automatic_help_id);
  gtk_container_add (GTK_CONTAINER (scrolled_window), dialog->proc_view);
  gtk_widget_show (dialog->proc_view);

  g_signal_connect (dialog->proc_view, "changed",
                    G_CALLBACK (gimp_file_dialog_proc_changed),
                    dialog);

  gimp_file_proc_view_set_proc (GIMP_FILE_PROC_VIEW (dialog->proc_view), NULL);

  /* Checkbox to show all files. */
  checkbox = gimp_prop_check_button_new (G_OBJECT (dialog),
                                         "show-all-files",
                                         _("Show _All Files"));
  gtk_box_pack_end (GTK_BOX (box), checkbox, FALSE, FALSE, 1);
}

static void
gimp_file_dialog_selection_changed (GtkFileChooser *chooser,
                                    GimpFileDialog *dialog)
{
  gimp_thumb_box_take_files (GIMP_THUMB_BOX (dialog->thumb_box),
                             gtk_file_chooser_get_files (chooser));
}

static void
gimp_file_dialog_update_preview (GtkFileChooser *chooser,
                                 GimpFileDialog *dialog)
{
  gimp_thumb_box_take_file (GIMP_THUMB_BOX (dialog->thumb_box),
                            gtk_file_chooser_get_preview_file (chooser));
}

static void
gimp_file_dialog_proc_changed (GimpFileProcView *view,
                               GimpFileDialog   *dialog)
{
  GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
  GtkFileFilter  *filter;
  gchar          *name;
  gchar          *label;

  dialog->file_proc = gimp_file_proc_view_get_proc (view, &name, &filter);

  if (name)
    label = g_strdup_printf (_("Select File _Type (%s)"), name);
  else
    label = g_strdup (_("Select File _Type"));

  gtk_expander_set_label (GTK_EXPANDER (dialog->proc_expander), label);

  g_free (label);
  g_free (name);

  if (dialog->show_all_files)
    g_clear_object (&filter);

  if (! filter)
    {
      filter = g_object_ref_sink (gtk_file_filter_new ());

      gtk_file_filter_add_pattern (filter, "*");
    }

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  g_object_unref (filter);

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
                  GFile   *file;
                  gchar   *basename;

                  if (last_dot)
                    g_string_truncate (s, last_dot - uri);

                  g_string_append (s, ".");
                  g_string_append (s, (gchar *) proc->extensions_list->data);

                  file = g_file_new_for_uri (s->str);
                  g_string_free (s, TRUE);

                  gtk_file_chooser_set_file (chooser, file, NULL);

                  basename = g_path_get_basename (gimp_file_get_utf8_name (file));
                  gtk_file_chooser_set_current_name (chooser, basename);
                  g_free (basename);
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

static GimpFileDialogState *
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

static void
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

static void
gimp_file_dialog_state_destroy (GimpFileDialogState *state)
{
  g_return_if_fail (state != NULL);

  g_free (state->filter_name);
  g_slice_free (GimpFileDialogState, state);
}
