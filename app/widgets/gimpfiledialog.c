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


/*  an arbitrary limit to keep the file dialog from becoming too wide  */
#define MAX_EXTENSIONS  4

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
  PROP_FILE_PROCS_ALL_IMAGES
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
static guint32  gimp_file_dialog_progress_get_window_id  (GimpProgress        *progress);

static void     gimp_file_dialog_add_user_dir            (GimpFileDialog      *dialog,
                                                          GUserDirectory       directory);
static void     gimp_file_dialog_add_preview             (GimpFileDialog      *dialog);
static void     gimp_file_dialog_add_filters             (GimpFileDialog      *dialog);
static void     gimp_file_dialog_process_procedure       (GimpPlugInProcedure *file_proc,
                                                          GtkFileFilter      **filter_out,
                                                          GtkFileFilter       *all,
                                                          GtkFileFilter       *all_savable);
static void     gimp_file_dialog_add_proc_selection      (GimpFileDialog      *dialog);

static void     gimp_file_dialog_selection_changed       (GtkFileChooser      *chooser,
                                                          GimpFileDialog      *dialog);
static void     gimp_file_dialog_update_preview          (GtkFileChooser      *chooser,
                                                          GimpFileDialog      *dialog);

static void     gimp_file_dialog_proc_changed            (GimpFileProcView    *view,
                                                          GimpFileDialog      *dialog);

static void     gimp_file_dialog_help_func               (const gchar         *help_id,
                                                          gpointer             help_data);
static void     gimp_file_dialog_help_clicked            (GtkWidget           *widget,
                                                          gpointer             dialog);

static gchar  * gimp_file_dialog_pattern_from_extension  (const gchar         *extension);


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
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (object), FALSE);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (object),
                                                  TRUE);

  if (dialog->help_id)
    {
      gimp_help_connect (GTK_WIDGET (dialog),
                         gimp_file_dialog_help_func, dialog->help_id, dialog);

      if (GIMP_GUI_CONFIG (dialog->gimp->config)->show_help_button)
        {
          GtkWidget *action_area;
          GtkWidget *button;

          action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));

          button = gtk_button_new_with_mnemonic (_("_Help"));
          gtk_box_pack_end (GTK_BOX (action_area), button, FALSE, TRUE, 0);
          gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (action_area),
                                              button, TRUE);
          gtk_widget_show (button);

          g_object_set_data_full (G_OBJECT (dialog), "gimp-dialog-help-id",
                                  g_strdup (dialog->help_id),
                                  (GDestroyNotify) g_free);

          g_signal_connect (button, "clicked",
                            G_CALLBACK (gimp_file_dialog_help_clicked),
                            dialog);

          g_object_set_data (G_OBJECT (dialog), "gimp-dialog-help-button", button);
        }
    }

  /* All classes derivated from GimpFileDialog should show these. */
  gimp_file_dialog_add_user_dir (dialog, G_USER_DIRECTORY_PICTURES);
  gimp_file_dialog_add_user_dir (dialog, G_USER_DIRECTORY_DOCUMENTS);

  gimp_file_dialog_add_preview (dialog);
  gimp_file_dialog_add_filters (dialog);

  dialog->extra_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog),
                                     dialog->extra_vbox);
  gtk_widget_show (dialog->extra_vbox);

  gimp_file_dialog_add_proc_selection (dialog);

  dialog->progress = gimp_progress_box_new ();
  gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                    dialog->progress, FALSE, FALSE, 0);
}

static void
gimp_file_dialog_dispose (GObject *object)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (object);

  G_OBJECT_CLASS (parent_class)->dispose (object);

  dialog->progress = NULL;

  if (dialog->help_id)
    g_free (dialog->help_id);
  dialog->help_id = NULL;

  if (dialog->ok_button_label)
    g_free (dialog->ok_button_label);
  dialog->ok_button_label = NULL;

  if (dialog->automatic_help_id)
    g_free (dialog->automatic_help_id);
  dialog->automatic_help_id = NULL;

  if (dialog->automatic_label)
    g_free (dialog->automatic_label);
  dialog->automatic_label = NULL;

  if (dialog->file_filter_label)
    g_free (dialog->file_filter_label);
  dialog->file_filter_label = NULL;
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
          GIMP_PROGRESS_BOX (file_dialog->progress)->cancellable)
        {
          gimp_progress_cancel (GIMP_PROGRESS (dialog));
        }
    }
}

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

static guint32
gimp_file_dialog_progress_get_window_id (GimpProgress *progress)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  return gimp_window_get_native_id (GTK_WINDOW (dialog));
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
gimp_file_dialog_add_filters (GimpFileDialog *dialog)
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

  if (dialog->file_procs_all_images)
    {
      all_savable = gtk_file_filter_new ();
      gtk_file_filter_set_name (all_savable, dialog->file_filter_label);
      gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), all_savable);
    }

  /* Add the normal file procs */
  for (list = dialog->file_procs; list; list = g_slist_next (list))
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
  for (list = dialog->file_procs_all_images; list; list = g_slist_next (list))
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
  GtkFileFilter *filter;
  GString       *str;
  GSList        *list;
  gint           i;

  if (! file_proc->extensions_list)
    return;

  filter = gtk_file_filter_new ();
  str    = g_string_new (gimp_procedure_get_label (GIMP_PROCEDURE (file_proc)));

  /* Take ownership directly so we don't have to mess with a floating
   * ref
   */
  g_object_ref_sink (filter);

  for (list = file_proc->mime_types_list; list; list = g_slist_next (list))
    {
      const gchar *mime_type = list->data;

      gtk_file_filter_add_mime_type (filter, mime_type);
      gtk_file_filter_add_mime_type (all, mime_type);
      if (all_savable)
        gtk_file_filter_add_mime_type (all_savable, mime_type);
    }

  for (list = file_proc->extensions_list, i = 0;
       list;
       list = g_slist_next (list), i++)
    {
      const gchar *extension = list->data;
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

      if (! list->next)
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
gimp_file_dialog_add_proc_selection (GimpFileDialog *dialog)
{
  GtkWidget *scrolled_window;

  dialog->proc_expander = gtk_expander_new_with_mnemonic (NULL);
  gtk_expander_set_resize_toplevel (GTK_EXPANDER (dialog->proc_expander), TRUE);
  gimp_file_dialog_add_extra_widget (dialog,
                                     dialog->proc_expander,
                                     TRUE, TRUE, 0);
  gtk_widget_show (dialog->proc_expander);

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
