/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2004  Sven Neumann <sven@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplay-foreach.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmaimagewindow.h"

#include "widgets/ligmacellrendererbutton.h"
#include "widgets/ligmacontainertreestore.h"
#include "widgets/ligmacontainertreeview.h"
#include "widgets/ligmacontainerview.h"
#include "widgets/ligmadnd.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamessagebox.h"
#include "widgets/ligmamessagedialog.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmaviewrenderer.h"
#include "widgets/ligmawidgets-utils.h"

#include "quit-dialog.h"

#include "ligma-intl.h"


typedef struct _QuitDialog QuitDialog;

struct _QuitDialog
{
  Ligma                  *ligma;
  LigmaContainer         *images;
  LigmaContext           *context;

  gboolean               do_quit;

  GtkWidget             *dialog;
  LigmaContainerTreeView *tree_view;
  GtkTreeViewColumn     *save_column;
  GtkWidget             *ok_button;
  LigmaMessageBox        *box;
  GtkWidget             *lost_label;
  GtkWidget             *hint_label;

  guint                  accel_key;
  GdkModifierType        accel_mods;
};


static GtkWidget * quit_close_all_dialog_new               (Ligma              *ligma,
                                                            gboolean           do_quit);
static void        quit_close_all_dialog_free              (QuitDialog        *private);
static void        quit_close_all_dialog_response          (GtkWidget         *dialog,
                                                            gint               response_id,
                                                            QuitDialog        *private);
static void        quit_close_all_dialog_accel_marshal     (GClosure          *closure,
                                                            GValue            *return_value,
                                                            guint              n_param_values,
                                                            const GValue      *param_values,
                                                            gpointer           invocation_hint,
                                                            gpointer           marshal_data);
static void        quit_close_all_dialog_container_changed (LigmaContainer     *images,
                                                            LigmaObject        *image,
                                                            QuitDialog        *private);
static gboolean    quit_close_all_dialog_images_selected   (LigmaContainerView *view,
                                                            GList             *images,
                                                            GList             *paths,
                                                            QuitDialog        *private);
static void        quit_close_all_dialog_name_cell_func    (GtkTreeViewColumn *tree_column,
                                                            GtkCellRenderer   *cell,
                                                            GtkTreeModel      *tree_model,
                                                            GtkTreeIter       *iter,
                                                            gpointer           data);
static void        quit_close_all_dialog_save_clicked      (GtkCellRenderer   *cell,
                                                            const gchar       *path,
                                                            GdkModifierType    state,
                                                            QuitDialog        *private);
static gboolean    quit_close_all_dialog_query_tooltip     (GtkWidget         *widget,
                                                            gint               x,
                                                            gint               y,
                                                            gboolean           keyboard_tip,
                                                            GtkTooltip        *tooltip,
                                                            QuitDialog        *private);
static gboolean    quit_close_all_idle                     (QuitDialog        *private);


/*  public functions  */

GtkWidget *
quit_dialog_new (Ligma *ligma)
{
  return quit_close_all_dialog_new (ligma, TRUE);
}

GtkWidget *
close_all_dialog_new (Ligma *ligma)
{
  return quit_close_all_dialog_new (ligma, FALSE);
}


/*  private functions  */

static GtkWidget *
quit_close_all_dialog_new (Ligma     *ligma,
                           gboolean  do_quit)
{
  QuitDialog            *private;
  GtkWidget             *view;
  LigmaContainerTreeView *tree_view;
  GtkTreeViewColumn     *column;
  GtkCellRenderer       *renderer;
  GtkWidget             *dnd_widget;
  GtkAccelGroup         *accel_group;
  GClosure              *closure;
  gint                   rows;
  gint                   view_size;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  private = g_slice_new0 (QuitDialog);

  private->ligma    = ligma;
  private->do_quit = do_quit;
  private->images  = ligma_displays_get_dirty_images (ligma);
  private->context = ligma_context_new (ligma, "close-all-dialog",
                                       ligma_get_user_context (ligma));

  g_return_val_if_fail (private->images != NULL, NULL);

  private->dialog =
    ligma_message_dialog_new (do_quit ? _("Quit LIGMA") : _("Close All Images"),
                             LIGMA_ICON_DIALOG_WARNING,
                             NULL, 0,
                             ligma_standard_help_func,
                             do_quit ?
                             LIGMA_HELP_FILE_QUIT : LIGMA_HELP_FILE_CLOSE_ALL,

                             _("_Cancel"), GTK_RESPONSE_CANCEL,

                             NULL);

  private->ok_button = gtk_dialog_add_button (GTK_DIALOG (private->dialog),
                                              "", GTK_RESPONSE_OK);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (private->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_object_weak_ref (G_OBJECT (private->dialog),
                     (GWeakNotify) quit_close_all_dialog_free, private);

  g_signal_connect (private->dialog, "response",
                    G_CALLBACK (quit_close_all_dialog_response),
                    private);

  /* connect <Primary>D to the quit/close button */
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (private->dialog), accel_group);
  g_object_unref (accel_group);

  closure = g_closure_new_object (sizeof (GClosure), G_OBJECT (private->dialog));
  g_closure_set_marshal (closure, quit_close_all_dialog_accel_marshal);
  gtk_accelerator_parse ("<Primary>D",
                         &private->accel_key, &private->accel_mods);
  gtk_accel_group_connect (accel_group,
                           private->accel_key, private->accel_mods,
                           0, closure);

  private->box = LIGMA_MESSAGE_DIALOG (private->dialog)->box;

  view_size = ligma->config->layer_preview_size;
  rows      = CLAMP (ligma_container_get_n_children (private->images), 3, 6);

  view = ligma_container_tree_view_new (private->images, private->context,
                                       view_size, 1);
  ligma_container_box_set_size_request (LIGMA_CONTAINER_BOX (view),
                                       -1,
                                       rows * (view_size + 2));

  private->tree_view = tree_view = LIGMA_CONTAINER_TREE_VIEW (view);

  gtk_tree_view_column_set_expand (tree_view->main_column, TRUE);

  renderer = ligma_container_tree_view_get_name_cell (tree_view);
  gtk_tree_view_column_set_cell_data_func (tree_view->main_column,
                                           renderer,
                                           quit_close_all_dialog_name_cell_func,
                                           NULL, NULL);

  private->save_column = column = gtk_tree_view_column_new ();
  renderer = ligma_cell_renderer_button_new ();
  g_object_set (renderer,
                "icon-name", "document-save",
                NULL);
  gtk_tree_view_column_pack_end (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, renderer, NULL);

  gtk_tree_view_append_column (tree_view->view, column);
  ligma_container_tree_view_add_toggle_cell (tree_view, renderer);

  g_signal_connect (renderer, "clicked",
                    G_CALLBACK (quit_close_all_dialog_save_clicked),
                    private);

  gtk_box_pack_start (GTK_BOX (private->box), view, TRUE, TRUE, 0);
  gtk_widget_show (view);

  g_signal_connect (view, "select-items",
                    G_CALLBACK (quit_close_all_dialog_images_selected),
                    private);

  dnd_widget = ligma_container_view_get_dnd_widget (LIGMA_CONTAINER_VIEW (view));
  ligma_dnd_xds_source_add (dnd_widget,
                           (LigmaDndDragViewableFunc) ligma_dnd_get_drag_viewable,
                           NULL);

  g_signal_connect (tree_view->view, "query-tooltip",
                    G_CALLBACK (quit_close_all_dialog_query_tooltip),
                    private);

  if (do_quit)
    private->lost_label = gtk_label_new (_("If you quit LIGMA now, "
                                           "these changes will be lost."));
  else
    private->lost_label = gtk_label_new (_("If you close these images now, "
                                           "changes will be lost."));
  gtk_label_set_xalign (GTK_LABEL (private->lost_label), 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (private->lost_label), TRUE);
  gtk_box_pack_start (GTK_BOX (private->box), private->lost_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (private->lost_label);

  private->hint_label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (private->hint_label), 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (private->hint_label), TRUE);
  gtk_box_pack_start (GTK_BOX (private->box), private->hint_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (private->hint_label);

  closure = g_cclosure_new (G_CALLBACK (quit_close_all_dialog_container_changed),
                            private, NULL);
  g_object_watch_closure (G_OBJECT (private->dialog), closure);
  g_signal_connect_closure (private->images, "add", closure, FALSE);
  g_signal_connect_closure (private->images, "remove", closure, FALSE);

  quit_close_all_dialog_container_changed (private->images, NULL,
                                           private);

  return private->dialog;
}

static void
quit_close_all_dialog_free (QuitDialog *private)
{
  g_idle_remove_by_data (private);
  g_object_unref (private->images);
  g_object_unref (private->context);

  g_slice_free (QuitDialog, private);
}

static void
quit_close_all_dialog_response (GtkWidget  *dialog,
                                gint        response_id,
                                QuitDialog *private)
{
  Ligma     *ligma    = private->ligma;
  gboolean  do_quit = private->do_quit;

  gtk_widget_destroy (dialog);
  private->box = NULL;
  private->dialog = NULL;

  if (response_id == GTK_RESPONSE_OK)
    {
      if (do_quit)
        ligma_exit (ligma, TRUE);
      else
        ligma_displays_close (ligma);
    }
}

static void
quit_close_all_dialog_accel_marshal (GClosure     *closure,
                                     GValue       *return_value,
                                     guint         n_param_values,
                                     const GValue *param_values,
                                     gpointer      invocation_hint,
                                     gpointer      marshal_data)
{
  gtk_dialog_response (GTK_DIALOG (closure->data), GTK_RESPONSE_OK);

  /* we handled the accelerator */
  g_value_set_boolean (return_value, TRUE);
}

static void
quit_close_all_dialog_container_changed (LigmaContainer *images,
                                         LigmaObject    *image,
                                         QuitDialog    *private)
{
  gint   num_images = ligma_container_get_n_children (images);
  gchar *accel_string;
  gchar *hint;
  gchar *markup;

  accel_string = gtk_accelerator_get_label (private->accel_key,
                                            private->accel_mods);

  ligma_message_box_set_primary_text (private->box,
                                     /* TRANSLATORS: unless your language
                                        msgstr[0] applies to 1 only (as
                                        in English), replace "one" with %d. */
                                     ngettext ("There is one image with "
                                               "unsaved changes:",
                                               "There are %d images with "
                                               "unsaved changes:",
                                               num_images), num_images);

  if (num_images == 0)
    {
      gtk_widget_hide (private->lost_label);

      if (private->do_quit)
        hint = g_strdup_printf (_("Press %s to quit."),
                                accel_string);
      else
        hint = g_strdup_printf (_("Press %s to close all images."),
                                accel_string);

      g_object_set (private->ok_button,
                    "label",     private->do_quit ? _("_Quit") : _("Cl_ose"),
                    "use-stock", TRUE,
                    "image",     NULL,
                    NULL);

      gtk_widget_grab_default (private->ok_button);

      /* When no image requires saving anymore, there is no harm in
       * assuming completing the original quit or close-all action is
       * the expected end-result.
       * I don't immediately exit though because of some unfinished
       * actions provoking warnings. Let's just close as soon as
       * possible with an idle source.
       * Also the idle source has another benefit: allowing to change
       * one's mind and not exit after the last save, for instance by
       * hitting Esc quickly while the last save is in progress.
       */
      g_idle_add ((GSourceFunc) quit_close_all_idle, private);
    }
  else
    {
      GtkWidget *icon;

      if (private->do_quit)
        hint = g_strdup_printf (_("Press %s to discard all changes and quit."),
                                accel_string);
      else
        hint = g_strdup_printf (_("Press %s to discard all changes and close all images."),
                                accel_string);

      gtk_widget_show (private->lost_label);

      icon = gtk_image_new_from_icon_name ("edit-delete",
                                           GTK_ICON_SIZE_BUTTON);
      g_object_set (private->ok_button,
                    "label",     _("_Discard Changes"),
                    "use-stock", FALSE,
                    "image",     icon,
                    NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (private->dialog),
                                       GTK_RESPONSE_CANCEL);
    }

  markup = g_strdup_printf ("<i><small>%s</small></i>", hint);

  gtk_label_set_markup (GTK_LABEL (private->hint_label), markup);

  g_free (markup);
  g_free (hint);
  g_free (accel_string);
}

static gboolean
quit_close_all_dialog_images_selected (LigmaContainerView *view,
                                       GList             *images,
                                       GList             *paths,
                                       QuitDialog        *private)
{
  /* The signal allows for multiple selection cases, but this specific
   * dialog only allows one image selected at a time.
   */
  g_return_val_if_fail (g_list_length (images) <= 1, FALSE);

  if (images)
    {
      LigmaImage *image = images->data;
      GList     *list;

      for (list = ligma_get_display_iter (private->ligma);
           list;
           list = g_list_next (list))
        {
          LigmaDisplay *display = list->data;

          if (ligma_display_get_image (display) == image)
            {
              ligma_display_shell_present (ligma_display_get_shell (display));

              /* We only want to update the active shell. Give back keyboard
               * focus to the quit dialog after this.
               */
              gtk_window_present (GTK_WINDOW (private->dialog));
            }
        }
    }

  return TRUE;
}

static void
quit_close_all_dialog_name_cell_func (GtkTreeViewColumn *tree_column,
                                      GtkCellRenderer   *cell,
                                      GtkTreeModel      *tree_model,
                                      GtkTreeIter       *iter,
                                      gpointer           data)
{
  LigmaViewRenderer *renderer;
  LigmaImage        *image;
  gchar            *name;

  gtk_tree_model_get (tree_model, iter,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      LIGMA_CONTAINER_TREE_STORE_COLUMN_NAME,     &name,
                      -1);

  image = LIGMA_IMAGE (renderer->viewable);

  if (ligma_image_is_export_dirty (image))
    {
      g_object_set (cell,
                    "markup", NULL,
                    "text",   name,
                    NULL);
    }
  else
    {
      GFile       *file;
      const gchar *filename;
      gchar       *escaped_name;
      gchar       *escaped_filename;
      gchar       *exported;
      gchar       *markup;

      file = ligma_image_get_exported_file (image);
      if (! file)
        file = ligma_image_get_imported_file (image);

      filename = ligma_file_get_utf8_name (file);

      escaped_name     = g_markup_escape_text (name, -1);
      escaped_filename = g_markup_escape_text (filename, -1);

      exported = g_strdup_printf (_("Exported to %s"), escaped_filename);
      markup = g_strdup_printf ("%s\n<i>%s</i>", escaped_name, exported);
      g_free (exported);

      g_free (escaped_name);
      g_free (escaped_filename);

      g_object_set (cell,
                    "text",   NULL,
                    "markup", markup,
                    NULL);

      g_free (markup);
    }

  g_object_unref (renderer);
  g_free (name);
}

static void
quit_close_all_dialog_save_clicked (GtkCellRenderer *cell,
                                    const gchar     *path_str,
                                    GdkModifierType  state,
                                    QuitDialog      *private)
{
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  GtkTreeIter  iter;

  if (gtk_tree_model_get_iter (private->tree_view->model, &iter, path))
    {
      LigmaViewRenderer *renderer;
      LigmaImage        *image;
      GList            *list;

      gtk_tree_model_get (private->tree_view->model, &iter,
                          LIGMA_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                          -1);

      image = LIGMA_IMAGE (renderer->viewable);
      g_object_unref (renderer);

      for (list = ligma_get_display_iter (private->ligma);
           list;
           list = g_list_next (list))
        {
          LigmaDisplay *display = list->data;

          if (ligma_display_get_image (display) == image)
            {
              LigmaDisplayShell *shell  = ligma_display_get_shell (display);
              LigmaImageWindow  *window = ligma_display_shell_get_window (shell);

              if (window)
                {
                  LigmaUIManager *manager;

                  manager = ligma_image_window_get_ui_manager (window);

                  ligma_display_shell_present (shell);
                  /* Make sure the quit dialog kept keyboard focus when
                   * the save dialog will exit. */
                  gtk_window_present (GTK_WINDOW (private->dialog));

                  if (state & GDK_SHIFT_MASK)
                    {
                      ligma_ui_manager_activate_action (manager, "file",
                                                       "file-save-as");
                    }
                  else
                    {
                      ligma_ui_manager_activate_action (manager, "file",
                                                       "file-save");
                    }
                }

              break;
            }
        }
    }

  gtk_tree_path_free (path);
}

static gboolean
quit_close_all_dialog_query_tooltip (GtkWidget  *widget,
                                     gint        x,
                                     gint        y,
                                     gboolean    keyboard_tip,
                                     GtkTooltip *tooltip,
                                     QuitDialog *private)
{
  GtkTreePath *path;
  gboolean     show_tip = FALSE;

  if (gtk_tree_view_get_tooltip_context (GTK_TREE_VIEW (widget), &x, &y,
                                         keyboard_tip,
                                         NULL, &path, NULL))
    {
      GtkTreeViewColumn *column = NULL;

      gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), x, y,
                                     NULL, &column, NULL, NULL);

      if (column == private->save_column)
        {
          gchar *tip = g_strconcat (_("Save this image"), "\n<b>",
                                    ligma_get_mod_string (GDK_SHIFT_MASK),
                                    "</b>  ", _("Save as"),
                                    NULL);

          gtk_tooltip_set_markup (tooltip, tip);
          gtk_tree_view_set_tooltip_row (GTK_TREE_VIEW (widget), tooltip, path);

          g_free (tip);

          show_tip = TRUE;
        }

      gtk_tree_path_free (path);
    }

  return show_tip;
}

static gboolean
quit_close_all_idle (QuitDialog *private)
{
  gtk_dialog_response (GTK_DIALOG (private->dialog), GTK_RESPONSE_OK);

  return FALSE;
}
