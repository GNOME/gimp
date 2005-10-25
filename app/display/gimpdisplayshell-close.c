/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <time.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "file/file-utils.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"
#include "widgets/gimpuimanager.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-close.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void      gimp_display_shell_close_dialog       (GimpDisplayShell *shell,
                                                        GimpImage        *gimage);
static void      gimp_display_shell_close_name_changed (GimpImage        *image,
                                                        GimpMessageBox   *box);
static gboolean  gimp_display_shell_close_time_changed (GimpMessageBox   *box);
static void      gimp_display_shell_close_response     (GtkWidget        *widget,
                                                        gboolean          close,
                                                        GimpDisplayShell *shell);

static gchar   * gimp_time_since                       (guint             then);


/*  public functions  */

void
gimp_display_shell_close (GimpDisplayShell *shell,
                          gboolean          kill_it)
{
  GimpImage *gimage;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimage = shell->gdisp->gimage;

  /*  FIXME: gimp_busy HACK not really appropriate here because we only
   *  want to prevent the busy image and display to be closed.  --Mitch
   */
  if (gimage->gimp->busy)
    return;

  /*  If the image has been modified, give the user a chance to save
   *  it before nuking it--this only applies if its the last view
   *  to an image canvas.  (a gimage with disp_count = 1)
   */
  if (! kill_it               &&
      gimage->disp_count == 1 &&
      gimage->dirty           &&
      GIMP_DISPLAY_CONFIG (gimage->gimp->config)->confirm_on_close)
    {
      gimp_display_shell_close_dialog (shell, gimage);
    }
  else
    {
      gimp_display_delete (shell->gdisp);
    }
}


/*  private functions  */

#define RESPONSE_SAVE  1


static void
gimp_display_shell_close_dialog (GimpDisplayShell *shell,
                                 GimpImage        *gimage)
{
  GtkWidget      *dialog;
  GtkWidget      *button;
  GimpMessageBox *box;
  GClosure       *closure;
  GSource        *source;
  gchar          *name;
  gchar          *title;

  if (shell->close_dialog)
    {
      gtk_window_present (GTK_WINDOW (shell->close_dialog));
      return;
    }

  name = file_utils_uri_display_basename (gimp_image_get_uri (gimage));

  title = g_strdup_printf (_("Close %s"), name);
  g_free (name);

  shell->close_dialog =
    dialog = gimp_message_dialog_new (title, GIMP_STOCK_WARNING,
                                      GTK_WIDGET (shell),
                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                      gimp_standard_help_func, NULL,
                                      NULL);
  g_free (title);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  _("Do_n't Save"), GTK_RESPONSE_CLOSE);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_stock (GTK_STOCK_DELETE,
                                                  GTK_ICON_SIZE_BUTTON));

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_SAVE,   RESPONSE_SAVE,
                          NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_SAVE,
                                           GTK_RESPONSE_CLOSE,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &shell->close_dialog);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gimp_display_shell_close_response),
                    shell);

  box = GIMP_MESSAGE_DIALOG (dialog)->box;

  g_signal_connect_object (gimage, "name-changed",
                           G_CALLBACK (gimp_display_shell_close_name_changed),
                           box, 0);

  gimp_display_shell_close_name_changed (gimage, box);

  closure =
    g_cclosure_new_object (G_CALLBACK (gimp_display_shell_close_time_changed),
                           G_OBJECT (box));

  source = g_timeout_source_new (1000);
  g_source_set_closure (source, closure);
  g_source_attach (source, NULL);
  g_source_unref (source);

  /*  The dialog is destroyed with the shell, so it should be safe
   *  to hold an image pointer for the lifetime of the dialog.
   */
  g_object_set_data (G_OBJECT (box), "gimp-image", gimage);

  gimp_display_shell_close_time_changed (box);

  gtk_widget_show (dialog);
}

static void
gimp_display_shell_close_name_changed (GimpImage      *image,
                                       GimpMessageBox *box)
{
  GtkWidget *window = gtk_widget_get_toplevel (GTK_WIDGET (box));
  gchar     *name;

  name = file_utils_uri_display_basename (gimp_image_get_uri (image));

  if (window)
    {
      gchar *title = g_strdup_printf (_("Close %s"), name);

      gtk_window_set_title (GTK_WINDOW (window), title);
      g_free (title);
    }

  gimp_message_box_set_primary_text (box,
                                     _("Save the changes to image '%s' "
                                       "before closing?"),
                                     name);
  g_free (name);
}


static gboolean
gimp_display_shell_close_time_changed (GimpMessageBox *box)
{
  GimpImage *image  = g_object_get_data (G_OBJECT (box), "gimp-image");

  if (image->dirty_time)
    {
      gchar *period = gimp_time_since (image->dirty_time);

      gimp_message_box_set_text (box,
                                 _("If you don't save the image, "
                                   "changes from the last %s will be lost."),
                                 period);
      g_free (period);
    }
  else
    {
      gimp_message_box_set_text (box, NULL);
    }

  return TRUE;
}

static void
gimp_display_shell_close_response (GtkWidget        *widget,
                                   gint              response_id,
                                   GimpDisplayShell *shell)
{
  gtk_widget_destroy (widget);

  switch (response_id)
    {
    case GTK_RESPONSE_CLOSE:
      gimp_display_delete (shell->gdisp);
      break;

    case RESPONSE_SAVE:
      {
        GtkAction *action;

        action = gimp_ui_manager_find_action (shell->menubar_manager,
                                              "file", "file-save");

        g_return_if_fail (action != NULL);

        gtk_action_activate (action);

        if (! shell->gdisp->gimage->dirty)
          gimp_display_delete (shell->gdisp);
      }
      break;

    default:
      break;
    }
}

static gchar *
gimp_time_since (guint  then)
{
  guint  now  = time (NULL);
  guint  diff = 1 + now - then;

  g_return_val_if_fail (now >= then, NULL);

  /* one second, the time period  */
  if (diff < 60)
    return g_strdup_printf (ngettext ("second", "%d seconds", diff), diff);

  /*  round to the nearest minute  */
  diff = (diff + 30) / 60;

  return g_strdup_printf (ngettext ("minute", "%d minutes", diff), diff);
}
