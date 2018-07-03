/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * extension-dialog.c
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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

#include <cairo-gobject.h>
#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpextensionmanager.h"
#include "core/gimpextension.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpprefsbox.h"

#include "extensions-dialog.h"

#include "gimp-intl.h"

static void        extensions_dialog_response                  (GtkWidget  *widget,
                                                                gint        response_id,
                                                                GtkWidget  *dialog);

/*  public function  */

GtkWidget *
extensions_dialog_new (Gimp *gimp)
{
  GtkWidget   *dialog;
  GtkWidget   *prefs_box;
  GtkWidget   *vbox;
  GtkWidget   *widget;
  const GList *extensions;
  GList       *iter;
  GtkTreeIter  top_iter;

  dialog = gimp_dialog_new (_("Extensions"), "gimp-extensions",
                            NULL, 0, NULL,
                            GIMP_HELP_EXTENSIONS_DIALOG,
                            _("_OK"), GTK_RESPONSE_OK,
                            NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (extensions_dialog_response),
                    dialog);

  prefs_box = gimp_prefs_box_new ();
  gtk_container_set_border_width (GTK_CONTAINER (prefs_box), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      prefs_box, TRUE, TRUE, 0);
  gtk_widget_show (prefs_box);

  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "system-software-install",
                                  /*"gimp-extensions-installed",*/
                                  _("Installed Extensions"),
                                  _("Installed Extensions"),
                                  GIMP_HELP_EXTENSIONS_INSTALLED,
                                  NULL,
                                  &top_iter);

  widget = gtk_list_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 1);
  gtk_widget_show (widget);

  extensions = gimp_extension_manager_get_user_extensions (gimp->extension_manager);
  iter = (GList *) extensions;

  for (; iter; iter = iter->next)
    {
      GimpExtension *extension = iter->data;
      GtkWidget     *frame;
      GtkWidget     *hbox;
      GtkWidget     *onoff;

      frame = gtk_frame_new (gimp_extension_get_name (extension));
      gtk_container_add (GTK_CONTAINER (widget), frame);
      gtk_widget_show (frame);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
      gtk_container_add (GTK_CONTAINER (frame), hbox);
      gtk_widget_show (hbox);

      if (gimp_extension_get_comment (extension))
        {
          GtkWidget     *desc = gtk_text_view_new ();
          GtkTextBuffer *buffer;

          buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (desc));
          gtk_text_buffer_set_text (buffer,
                                    gimp_extension_get_comment (extension),
                                    -1);
          gtk_text_view_set_editable (GTK_TEXT_VIEW (desc), FALSE);
          gtk_box_pack_start (GTK_BOX (hbox), desc, TRUE, TRUE, 1);
          gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (desc),
                                       GTK_WRAP_WORD_CHAR);
          gtk_widget_show (desc);
        }

      onoff = gtk_switch_new ();
      gtk_switch_set_active (GTK_SWITCH (onoff),
                             gimp_extension_manager_is_active (gimp->extension_manager,
                                                               gimp_object_get_name (extension)));
      gtk_box_pack_end (GTK_BOX (hbox), onoff, FALSE, FALSE, 1);
      gtk_widget_show (onoff);
    }

  /* TODO: system extensions show in a similar list except that they
   * cannot be uninstalled, yet they can be deactivated as well.
   * Also you can override a system extension by installing another
   * version (same id, same extension).
   */
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "system-software-install",
                                  _("System Extensions"),
                                  _("System Extensions"),
                                  GIMP_HELP_EXTENSIONS_SYSTEM,
                                  NULL,
                                  &top_iter);


  /* TODO: provide a search box and a list of uninstalled extension from
   * a remote repository list.
   */
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "system-software-install",
                                  _("Search Extensions"),
                                  _("Search Extensions"),
                                  GIMP_HELP_EXTENSIONS_INSTALL,
                                  NULL,
                                  &top_iter);
  return dialog;
}


static void
extensions_dialog_response (GtkWidget  *widget,
                            gint        response_id,
                            GtkWidget  *dialog)
{
  gtk_widget_destroy (dialog);
}
