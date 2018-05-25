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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"

#include "widgets/gimpdeviceeditor.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimphelp-ids.h"

#include "input-devices-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   input_devices_dialog_response (GtkWidget *dialog,
                                             guint      response_id,
                                             Gimp      *gimp);


/*  public functions  */

GtkWidget *
input_devices_dialog_new (Gimp *gimp)
{
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *editor;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  dialog = gimp_dialog_new (_("Configure Input Devices"),
                            "gimp-input-devices-dialog",
                            NULL, 0,
                            gimp_standard_help_func,
                            GIMP_HELP_INPUT_DEVICES,

                            _("_Close"), GTK_RESPONSE_CLOSE,
                            _("_Save"),  GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CLOSE,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (input_devices_dialog_response),
                    gimp);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  editor = gimp_device_editor_new (gimp);
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);
  gtk_box_pack_start (GTK_BOX (content_area), editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);

  return dialog;
}


/*  private functions  */

static void
input_devices_dialog_response (GtkWidget *dialog,
                               guint      response_id,
                               Gimp      *gimp)
{
  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      gimp_devices_save (gimp, TRUE);
      break;

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}
