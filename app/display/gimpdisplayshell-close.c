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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "file/file-utils.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-close.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void  gimp_display_shell_close_dialog   (GimpDisplayShell *shell,
                                                GimpImage        *gimage);
static void  gimp_display_shell_close_response (GtkWidget        *widget,
                                                gboolean          close,
                                                GimpDisplayShell *shell);


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

static void
gimp_display_shell_close_dialog (GimpDisplayShell *shell,
                                 GimpImage        *gimage)
{
  GtkWidget *dialog;
  GtkWidget *box;
  gchar     *name;
  gchar     *title;

  if (shell->close_dialog)
    {
      gtk_window_present (GTK_WINDOW (shell->close_dialog));
      return;
    }

  name = file_utils_uri_to_utf8_basename (gimp_image_get_uri (gimage));

  title = g_strdup_printf (_("Close %s?"), name);

  shell->close_dialog =
    dialog = gimp_dialog_new (title,
                              "gimp-display-shell-close",
                              GTK_WIDGET (shell), 0,
                              gimp_standard_help_func,
                              GIMP_HELP_FILE_CLOSE_CONFIRM,

                              GTK_STOCK_CANCEL,      GTK_RESPONSE_CANCEL,
                              _("_Discard changes"), GTK_RESPONSE_OK,

                              NULL);

  g_free (title);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &shell->close_dialog);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gimp_display_shell_close_response),
                    shell);

  box = gimp_message_box_new (GIMP_STOCK_WARNING);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), box);
  gtk_widget_show (box);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_BOX (box),
                                     _("Changes were made to '%s'."), name);

  g_free (name);

  gimp_message_box_set_text (GIMP_MESSAGE_BOX (box),
                             _("Unsaved changes will be lost."));

  gtk_widget_show (dialog);
}

static void
gimp_display_shell_close_response (GtkWidget        *widget,
                                   gint              response_id,
                                   GimpDisplayShell *shell)
{
  gtk_widget_destroy (widget);

  if (response_id == GTK_RESPONSE_OK)
    gimp_display_delete (shell->gdisp);
}
