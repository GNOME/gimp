/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpimagesaveview.h"
#include "widgets/gimpmessagedialog.h"

#include "quit-dialog.h"

#include "gimp-intl.h"


typedef struct _QuitDialog QuitDialog;

struct _QuitDialog
{
  Gimp                  *gimp;
  GimpContainer         *images;
  GimpContext           *context;

  gboolean               do_quit;

  GtkWidget             *dialog;
  GimpContainerTreeView *tree_view;
  GtkTreeViewColumn     *save_column;
  GtkWidget             *ok_button;
  GimpMessageBox        *box;
  GtkWidget             *lost_label;
  GtkWidget             *hint_label;

  guint                  accel_key;
  GdkModifierType        accel_mods;
};


static GtkWidget * quit_close_all_dialog_new               (Gimp              *gimp,
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
static void        quit_close_all_dialog_container_changed (GimpContainer     *images,
                                                            GimpObject        *image,
                                                            QuitDialog        *private);
static void        quit_close_all_dialog_images_selected   (GimpContainerView *view,
                                                            QuitDialog        *private);

static gboolean    quit_close_all_idle                     (QuitDialog        *private);


/*  public functions  */

GtkWidget *
quit_dialog_new (Gimp *gimp)
{
  return quit_close_all_dialog_new (gimp, TRUE);
}

GtkWidget *
close_all_dialog_new (Gimp *gimp)
{
  return quit_close_all_dialog_new (gimp, FALSE);
}


/*  private functions  */

static GtkWidget *
quit_close_all_dialog_new (Gimp     *gimp,
                           gboolean  do_quit)
{
  QuitDialog    *private;
  GtkWidget     *view;
  GtkAccelGroup *accel_group;
  GClosure      *closure;
  gint           rows;
  gint           view_size;
  GdkRectangle   geometry;
  GdkMonitor    *monitor;
  gint           max_rows;
  gint           scale_factor;
  const gfloat   rows_per_height   = 32 / 1440.0f;
  const gint     greatest_max_rows = 36;
  const gint     least_max_rows    = 6;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  private = g_slice_new0 (QuitDialog);

  private->gimp    = gimp;
  private->do_quit = do_quit;
  private->images  = gimp_displays_get_dirty_images (gimp);
  private->context = gimp_context_new (gimp, "close-all-dialog",
                                       gimp_get_user_context (gimp));

  g_return_val_if_fail (private->images != NULL, NULL);

  private->dialog =
    gimp_message_dialog_new (do_quit ? _("Quit GIMP") : _("Close All Images"),
                             GIMP_ICON_DIALOG_WARNING,
                             NULL, 0,
                             gimp_standard_help_func,
                             do_quit ?
                             GIMP_HELP_FILE_QUIT : GIMP_HELP_FILE_CLOSE_ALL,

                             _("_Cancel"), GTK_RESPONSE_CANCEL,

                             NULL);

  private->ok_button = gtk_dialog_add_button (GTK_DIALOG (private->dialog),
                                              "", GTK_RESPONSE_OK);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (private->dialog),
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

  private->box = GIMP_MESSAGE_DIALOG (private->dialog)->box;

  monitor      = gimp_widget_get_monitor (private->dialog);
  scale_factor = gdk_monitor_get_scale_factor (monitor);
  gdk_monitor_get_geometry (monitor, &geometry);

  if (scale_factor > 1)
    {
      #ifdef GDK_WINDOWING_WIN32
        max_rows = (geometry.height * scale_factor * rows_per_height)
                      / (scale_factor + 1);
      #else
        max_rows = (geometry.height * rows_per_height) / (scale_factor + 1);
      #endif
    }
  else
    {
      max_rows = geometry.height * rows_per_height;
    }

  max_rows = CLAMP (max_rows, least_max_rows, greatest_max_rows);

  view_size = gimp->config->layer_preview_size;
  rows      = CLAMP (gimp_container_get_n_children (private->images), 3, max_rows);

  view = gimp_image_save_view_new (private->images,
                                   private->context,
                                   view_size, 0);
  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (view),
                                       -1,
                                       rows * (view_size + 2));
  gtk_box_pack_start (GTK_BOX (private->box), view, TRUE, TRUE, 0);
  gtk_widget_show (view);

  g_signal_connect (view, "selection-changed",
                    G_CALLBACK (quit_close_all_dialog_images_selected),
                    private);

  if (do_quit)
    private->lost_label = gtk_label_new (_("If you quit GIMP now, "
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
  g_signal_connect_swapped (private->dialog, "destroy", G_CALLBACK (g_closure_invalidate), closure);
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
  Gimp     *gimp    = private->gimp;
  gboolean  do_quit = private->do_quit;

  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      if (do_quit)
        gimp_exit (gimp, TRUE);
      else
        gimp_displays_close (gimp);
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
quit_close_all_dialog_container_changed (GimpContainer *images,
                                         GimpObject    *image,
                                         QuitDialog    *private)
{
  gint   num_images = gimp_container_get_n_children (images);
  gchar *accel_string;
  gchar *hint;
  gchar *markup;

  accel_string = gtk_accelerator_get_label (private->accel_key,
                                            private->accel_mods);

  gimp_message_box_set_primary_text (private->box,
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
      gtk_widget_set_visible (private->lost_label, FALSE);

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

      icon = gtk_image_new_from_icon_name (GIMP_ICON_EDIT_DELETE,
                                           GTK_ICON_SIZE_BUTTON);
      g_object_set (private->ok_button,
                    "label",     _("_Discard Changes"),
                    "use-stock", FALSE,
                    "image",     icon,
                    NULL);
      gtk_style_context_add_class (gtk_widget_get_style_context (private->ok_button),
                                   "text-button");

      gtk_dialog_set_default_response (GTK_DIALOG (private->dialog),
                                       GTK_RESPONSE_CANCEL);
    }

  markup = g_strdup_printf ("<i><small>%s</small></i>", hint);

  gtk_label_set_markup (GTK_LABEL (private->hint_label), markup);

  g_free (markup);
  g_free (hint);
  g_free (accel_string);
}

static void
quit_close_all_dialog_images_selected (GimpContainerView *view,
                                       QuitDialog        *private)
{
  GimpViewable *image = gimp_container_view_get_1_selected (view);

  if (image)
    {
      GList *list;

      for (list = gimp_get_display_iter (private->gimp);
           list;
           list = g_list_next (list))
        {
          GimpDisplay *display = list->data;

          if (gimp_display_get_image (display) == GIMP_IMAGE (image))
            {
              gimp_display_shell_present (gimp_display_get_shell (display));

              /* We only want to update the active shell. Give back keyboard
               * focus to the quit dialog after this.
               */
              gtk_window_present (GTK_WINDOW (private->dialog));
            }
        }
    }
}

static gboolean
quit_close_all_idle (QuitDialog *private)
{
  gtk_dialog_response (GTK_DIALOG (private->dialog), GTK_RESPONSE_OK);

  return FALSE;
}
