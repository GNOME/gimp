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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print-page-setup.h"


void
print_page_setup_dialog (GtkPrintOperation *operation)
{
  GtkPrintSettings *settings;
  GtkPageSetup     *setup;

  g_return_if_fail (GTK_IS_PRINT_OPERATION (operation));

  setup = gtk_print_operation_get_default_page_setup (operation);

  settings = gtk_print_settings_new ();
  setup = gtk_print_run_page_setup_dialog (NULL, setup, settings);
  g_object_unref (settings);

  gtk_print_operation_set_default_page_setup (operation, setup);
}

gboolean
print_page_setup_load (GtkPrintOperation *operation,
                       gint32             image_ID)
{
  GtkPageSetup *setup;
  gchar        *filename;

  filename = g_build_filename (gimp_directory (), "print-page-setup", NULL);

  setup = gtk_page_setup_new_from_file (filename, NULL);

  g_free (filename);

  if (setup)
    {
      gtk_print_operation_set_default_page_setup (operation, setup);
      g_object_unref (setup);

      return TRUE;
    }

  return FALSE;
}

void
print_page_setup_save (GtkPrintOperation *operation)
{
  GtkPageSetup *setup;
  gchar        *filename;
  GError       *error = NULL;

  setup = gtk_print_operation_get_default_page_setup (operation);

  filename = g_build_filename (gimp_directory (), "print-page-setup", NULL);

  gtk_page_setup_to_file (setup, filename, &error);

  if (error)
    {
      g_message ("Error saving page setup as resource file: %s",
                 error->message);
      g_error_free (error);
    }

  g_free (filename);
}
