/* GIMP - The GNU Image Manipulation Program
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
                                                        GimpImage        *image);
static void      gimp_display_shell_close_name_changed (GimpImage        *image,
                                                        GimpMessageBox   *box);
static gboolean  gimp_display_shell_close_time_changed (GimpMessageBox   *box);
static void      gimp_display_shell_close_response     (GtkWidget        *widget,
                                                        gboolean          close,
                                                        GimpDisplayShell *shell);

static void      gimp_time_since                       (guint    then,
                                                        gchar  **hours,
                                                        gchar  **minutes);


/*  public functions  */

void
gimp_display_shell_close (GimpDisplayShell *shell,
                          gboolean          kill_it)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = shell->display->image;

  /*  FIXME: gimp_busy HACK not really appropriate here because we only
   *  want to prevent the busy image and display to be closed.  --Mitch
   */
  if (image->gimp->busy)
    return;

  /*  If the image has been modified, give the user a chance to save
   *  it before nuking it--this only applies if its the last view
   *  to an image canvas.  (a image with disp_count = 1)
   */
  if (! kill_it              &&
      image->disp_count == 1 &&
      image->dirty           &&
      GIMP_DISPLAY_CONFIG (image->gimp->config)->confirm_on_close)
    {
      gimp_display_shell_close_dialog (shell, image);
    }
  else
    {
      gimp_display_delete (shell->display);
    }
}


/*  private functions  */

#define RESPONSE_SAVE  1


static void
gimp_display_shell_close_dialog (GimpDisplayShell *shell,
                                 GimpImage        *image)
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

  name = file_utils_uri_display_basename (gimp_image_get_uri (image));

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

  g_signal_connect_object (image, "name-changed",
                           G_CALLBACK (gimp_display_shell_close_name_changed),
                           box, 0);

  gimp_display_shell_close_name_changed (image, box);

  closure =
    g_cclosure_new_object (G_CALLBACK (gimp_display_shell_close_time_changed),
                           G_OBJECT (box));

  /*  update every 10 seconds  */
  source = g_timeout_source_new (10 * 1000);
  g_source_set_closure (source, closure);
  g_source_attach (source, NULL);
  g_source_unref (source);

  /*  The dialog is destroyed with the shell, so it should be safe
   *  to hold an image pointer for the lifetime of the dialog.
   */
  g_object_set_data (G_OBJECT (box), "gimp-image", image);

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
      gchar *hours   = NULL;
      gchar *minutes = NULL;

      gimp_time_since (image->dirty_time, &hours, &minutes);

      if (hours && minutes)
        /* time period ("... from the last 3 hours and 20 minutes ...") */
        gimp_message_box_set_text (box,
                                   _("If you don't save the image, changes "
                                     "from the last %s and %s will be lost."),
                                   hours, minutes);
      else
        /* time period ("... from the last 20 minutes ...") */
        gimp_message_box_set_text (box,
                                   _("If you don't save the image, changes "
                                     "from the last %s will be lost."),
                                   hours ? hours : minutes);

      g_free (hours);
      g_free (minutes);
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
      gimp_display_delete (shell->display);
      break;

    case RESPONSE_SAVE:
      {
        GtkAction *action;

        action = gimp_ui_manager_find_action (shell->menubar_manager,
                                              "file", "file-save-and-close");

        g_return_if_fail (action != NULL);

        gtk_action_activate (action);
      }
      break;

    default:
      break;
    }
}

static void
gimp_time_since (guint   then,
                 gchar **hours,
                 gchar **minutes)
{
  guint now  = time (NULL);
  guint diff = 1 + now - then;

  *minutes = NULL;
  *hours   = NULL;

  g_return_if_fail (now >= then);

  /*  first round up to the nearest minute  */
  diff = (diff + 59) / 60;

  /*  then optionally round minutes to multiples of 5 or 10  */
  if (diff > 50)
    diff = ((diff + 8) / 10) * 10;
  else if (diff > 20)
    diff = ((diff + 3) / 5) * 5;

  if (diff >= 120)
    {
      *hours = g_strdup_printf (ngettext ("%d hour", "%d hours",
                                          diff / 60), diff / 60);
      diff = (diff % 60);
    }

  if (diff > 0)
    *minutes = g_strdup_printf (ngettext ("minute", "%d minutes", diff), diff);
}
