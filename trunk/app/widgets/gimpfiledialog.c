/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfiledialog.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"

#include "config/gimpguiconfig.h"

#include "file/file-utils.h"

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


static void    gimp_file_dialog_progress_iface_init (GimpProgressInterface *iface);
static gboolean gimp_file_dialog_delete_event       (GtkWidget        *widget,
                                                     GdkEventAny      *event);
static void     gimp_file_dialog_response           (GtkDialog        *dialog,
                                                     gint              response_id);

static GimpProgress *
                    gimp_file_dialog_progress_start (GimpProgress     *progress,
                                                     const gchar      *message,
                                                     gboolean          cancelable);
static void     gimp_file_dialog_progress_end       (GimpProgress     *progress);
static gboolean gimp_file_dialog_progress_is_active (GimpProgress     *progress);
static void     gimp_file_dialog_progress_set_text  (GimpProgress     *progress,
                                                     const gchar      *message);
static void     gimp_file_dialog_progress_set_value (GimpProgress     *progress,
                                                     gdouble           percentage);
static gdouble  gimp_file_dialog_progress_get_value (GimpProgress     *progress);
static void     gimp_file_dialog_progress_pulse     (GimpProgress     *progress);
static guint32  gimp_file_dialog_progress_get_window(GimpProgress     *progress);

static void     gimp_file_dialog_add_preview        (GimpFileDialog   *dialog,
                                                     Gimp             *gimp);
static void     gimp_file_dialog_add_filters        (GimpFileDialog   *dialog,
                                                     Gimp             *gimp,
                                                     GSList           *file_procs);
static void     gimp_file_dialog_add_proc_selection (GimpFileDialog   *dialog,
                                                     Gimp             *gimp,
                                                     GSList           *file_procs,
                                                     const gchar      *automatic,
                                                     const gchar      *automatic_help_id);

static void     gimp_file_dialog_selection_changed  (GtkFileChooser   *chooser,
                                                     GimpFileDialog   *dialog);
static void     gimp_file_dialog_update_preview     (GtkFileChooser   *chooser,
                                                     GimpFileDialog   *dialog);

static void     gimp_file_dialog_proc_changed       (GimpFileProcView *view,
                                                     GimpFileDialog   *dialog);

static void     gimp_file_dialog_help_func          (const gchar      *help_id,
                                                     gpointer          help_data);
static void     gimp_file_dialog_help_clicked       (GtkWidget        *widget,
                                                     gpointer          dialog);

static gchar  * gimp_file_dialog_pattern_from_extension (const gchar  *extension);


G_DEFINE_TYPE_WITH_CODE (GimpFileDialog, gimp_file_dialog,
                         GTK_TYPE_FILE_CHOOSER_DIALOG,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_file_dialog_progress_iface_init))

#define parent_class gimp_file_dialog_parent_class


static void
gimp_file_dialog_class_init (GimpFileDialogClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

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
  iface->start      = gimp_file_dialog_progress_start;
  iface->end        = gimp_file_dialog_progress_end;
  iface->is_active  = gimp_file_dialog_progress_is_active;
  iface->set_text   = gimp_file_dialog_progress_set_text;
  iface->set_value  = gimp_file_dialog_progress_set_value;
  iface->get_value  = gimp_file_dialog_progress_get_value;
  iface->pulse      = gimp_file_dialog_progress_pulse;
  iface->get_window = gimp_file_dialog_progress_get_window;
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

      if (GIMP_PROGRESS_BOX (file_dialog->progress)->active &&
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
  GimpProgress   *retval;

  retval = gimp_progress_start (GIMP_PROGRESS (dialog->progress),
                                message, cancelable);
  gtk_widget_show (dialog->progress);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                     GTK_RESPONSE_CANCEL, cancelable);

  return retval;
}

static void
gimp_file_dialog_progress_end (GimpProgress *progress)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  gimp_progress_end (GIMP_PROGRESS (dialog->progress));
  gtk_widget_hide (dialog->progress);
}

static gboolean
gimp_file_dialog_progress_is_active (GimpProgress *progress)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  return gimp_progress_is_active (GIMP_PROGRESS (dialog->progress));
}

static void
gimp_file_dialog_progress_set_text (GimpProgress *progress,
                                    const gchar  *message)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  gimp_progress_set_text (GIMP_PROGRESS (dialog->progress), message);
}

static void
gimp_file_dialog_progress_set_value (GimpProgress *progress,
                                     gdouble       percentage)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  gimp_progress_set_value (GIMP_PROGRESS (dialog->progress), percentage);
}

static gdouble
gimp_file_dialog_progress_get_value (GimpProgress *progress)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  return gimp_progress_get_value (GIMP_PROGRESS (dialog->progress));
}

static void
gimp_file_dialog_progress_pulse (GimpProgress *progress)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  gimp_progress_pulse (GIMP_PROGRESS (dialog->progress));
}

static guint32
gimp_file_dialog_progress_get_window (GimpProgress *progress)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (progress);

  return (guint32) gimp_window_get_native (GTK_WINDOW (dialog));
}


/*  public functions  */

GtkWidget *
gimp_file_dialog_new (Gimp                 *gimp,
                      GtkFileChooserAction  action,
                      const gchar          *title,
                      const gchar          *role,
                      const gchar          *stock_id,
                      const gchar          *help_id)
{
  GimpFileDialog *dialog;
  GSList         *file_procs;
  const gchar    *automatic;
  const gchar    *automatic_help_id;
  const gchar    *pictures;
  gboolean        local_only;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);

  switch (action)
    {
    case GTK_FILE_CHOOSER_ACTION_OPEN:
      file_procs = gimp->plug_in_manager->load_procs;
      automatic  = _("Automatically Detected");
      automatic_help_id = GIMP_HELP_FILE_OPEN_BY_EXTENSION;

      /* FIXME */
      local_only = (gimp_pdb_lookup_procedure (gimp->pdb,
                                               "file-uri-load") == NULL);
      break;

    case GTK_FILE_CHOOSER_ACTION_SAVE:
      file_procs = gimp->plug_in_manager->save_procs;
      automatic  = _("By Extension");
      automatic_help_id = GIMP_HELP_FILE_SAVE_BY_EXTENSION;

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
                         "action",                    action,
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
      GtkWidget *action_area = GTK_DIALOG (dialog)->action_area;
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

  pictures = gimp_user_directory (GIMP_USER_DIRECTORY_PICTURES);

  if (pictures)
    gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
                                          pictures, NULL);

  gimp_file_dialog_add_preview (dialog, gimp);

  gimp_file_dialog_add_filters (dialog, gimp, file_procs);

  gimp_file_dialog_add_proc_selection (dialog, gimp, file_procs, automatic,
                                       automatic_help_id);

  dialog->progress = gimp_progress_box_new ();
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->vbox), dialog->progress,
                    FALSE, FALSE, 0);

  return GTK_WIDGET (dialog);
}

void
gimp_file_dialog_set_sensitive (GimpFileDialog *dialog,
                                gboolean        sensitive)
{
  GList *children;
  GList *list;

  g_return_if_fail (GIMP_IS_FILE_DIALOG (dialog));

  children =
    gtk_container_get_children (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox));

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
gimp_file_dialog_set_image (GimpFileDialog *dialog,
                            GimpImage      *image,
                            gboolean        save_a_copy,
                            gboolean        close_after_saving)
{
  const gchar *uri = NULL;
  gchar       *dirname;
  gchar       *basename;

  g_return_if_fail (GIMP_IS_FILE_DIALOG (dialog));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  dialog->image              = image;
  dialog->save_a_copy        = save_a_copy;
  dialog->close_after_saving = close_after_saving;

  if (save_a_copy)
    uri = g_object_get_data (G_OBJECT (image), "gimp-image-save-a-copy");

  if (! uri)
    uri = gimp_image_get_uri (image);

  gimp_file_dialog_set_file_proc (dialog, NULL);

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

  if (dirname && strlen (dirname) && strcmp (dirname, "."))
    {
      gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dialog),
                                               dirname);
    }
  else
    {
      const gchar *folder;

      folder = g_object_get_data (G_OBJECT (image), "gimp-image-dirname");

      if (folder)
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), folder);
    }

  g_free (dirname);

  basename = file_utils_uri_display_basename (uri);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), basename);
  g_free (basename);
}


/*  private functions  */

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

static void
gimp_file_dialog_add_filters (GimpFileDialog *dialog,
                              Gimp           *gimp,
                              GSList         *file_procs)
{
  GtkFileFilter *all;
  GSList        *list;

  all = gtk_file_filter_new ();
  gtk_file_filter_set_name (all, _("All files"));
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), all);
  gtk_file_filter_add_pattern (all, "*");

  all = gtk_file_filter_new ();
  gtk_file_filter_set_name (all, _("All images"));
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), all);

  for (list = file_procs; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *file_proc = list->data;

      if (file_proc->extensions_list)
        {
          GtkFileFilter *filter = gtk_file_filter_new ();
          GString       *str;
          GSList        *ext;
          gint           i;

          str = g_string_new (gimp_plug_in_procedure_get_label (file_proc));

/*  an arbitrary limit to keep the file dialog from becoming too wide  */
#define MAX_EXTENSIONS  4

          for (ext = file_proc->extensions_list, i = 0;
               ext;
               ext = g_slist_next (ext), i++)
            {
              const gchar *extension = ext->data;
              gchar       *pattern;

              pattern = gimp_file_dialog_pattern_from_extension (extension);
              gtk_file_filter_add_pattern (filter, pattern);
              gtk_file_filter_add_pattern (all, pattern);
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

          gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
        }
    }

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), all);
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
