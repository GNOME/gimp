/* GIMP - The GNU Image Manipulation Program
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"

#include "widgets/gimpactioneditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpuimanager.h"

#include "menus/menus.h"

#include "keyboard-shortcuts-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_SAVE 1


/*  local function prototypes  */

static void   keyboard_shortcuts_dialog_response (GtkWidget *dialog,
                                                  gint       response,
                                                  Gimp      *gimp);


/*  public functions  */

GtkWidget *
keyboard_shortcuts_dialog_new (Gimp *gimp)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *editor;
  GtkWidget *box;
  GtkWidget *button;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  dialog = gimp_dialog_new (_("Configure Keyboard Shortcuts"),
                            "gimp-keyboard-shortcuts-dialog",
                            NULL, 0,
                            gimp_standard_help_func,
                            GIMP_HELP_KEYBOARD_SHORTCUTS,

                            _("_Save"),  RESPONSE_SAVE,
                            _("_Close"), GTK_RESPONSE_CLOSE,

                            NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (keyboard_shortcuts_dialog_response),
                    gimp);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  editor = gimp_action_editor_new (gimp_ui_managers_from_name ("<Image>")->data,
                                   NULL, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);

  box = gimp_hint_box_new (_("To edit a shortcut key, click on the "
                             "corresponding row and type a new "
                             "accelerator, or press backspace to "
                             "clear."));
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  button = gimp_prop_check_button_new (G_OBJECT (gimp->config), "save-accels",
                                       _("S_ave keyboard shortcuts on exit"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  return dialog;
}


/*  private functions  */

static void
keyboard_shortcuts_dialog_response (GtkWidget *dialog,
                                    gint       response,
                                    Gimp      *gimp)
{
  switch (response)
    {
    case RESPONSE_SAVE:
      menus_save (gimp, TRUE);
      break;

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}
