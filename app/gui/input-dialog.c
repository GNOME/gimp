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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"

#include "widgets/gimpdeviceinfo.h"
#include "widgets/gimpdevices.h"

#include "device-status-dialog.h"
#include "input-dialog.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes */

static void   input_dialog_able_callback (GtkWidget *widget,
                                          GdkDevice *device,
                                          Gimp      *gimp);


/*  private variables  */

static GtkWidget *input_dialog = NULL;


/*  public functions  */

GtkWidget *
input_dialog_create (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (input_dialog)
    return input_dialog;

  input_dialog = gtk_input_dialog_new ();

  g_signal_connect_swapped (G_OBJECT (GTK_INPUT_DIALOG (input_dialog)->save_button),
                            "clicked",
                            G_CALLBACK (gimp_devices_save),
                            gimp);
  g_signal_connect_swapped (G_OBJECT (GTK_INPUT_DIALOG (input_dialog)->close_button),
                            "clicked",
                            G_CALLBACK (gtk_widget_hide),
                            input_dialog);

  g_signal_connect (G_OBJECT (input_dialog), "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &input_dialog);

  g_signal_connect (G_OBJECT (input_dialog), "enable_device",
                    G_CALLBACK (input_dialog_able_callback),
                    gimp);
  g_signal_connect (G_OBJECT (input_dialog), "disable_device",
                    G_CALLBACK (input_dialog_able_callback),
                    gimp);

  /*  Connect the "F1" help key  */
  gimp_help_connect (input_dialog,
		     gimp_standard_help_func,
		     "dialogs/input_devices.html");

  return input_dialog;
}


/*  private functions  */

static void
input_dialog_able_callback (GtkWidget *widget,
			    GdkDevice *device,
			    Gimp      *gimp)
{
  gimp_device_info_changed_by_device (device);
}
