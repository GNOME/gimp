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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print.h"
#include "print-page-setup.h"
#include "print-utils.h"

#define PRINT_PAGE_SETUP_NAME  "print-page-setup"


#ifndef EMBED_PAGE_SETUP
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
#endif

void
print_page_setup_load (GtkPrintOperation *operation,
                       GimpImage         *image)
{
  GKeyFile *key_file;

  g_return_if_fail (GTK_IS_PRINT_OPERATION (operation));

  key_file = print_utils_key_file_load_from_parasite (image,
                                                      PRINT_PAGE_SETUP_NAME);

  if (! key_file)
    key_file = print_utils_key_file_load_from_rcfile (PRINT_PAGE_SETUP_NAME);

  if (key_file)
    {
      GtkPageSetup *setup;

      setup = gtk_page_setup_new_from_key_file (key_file,
                                                PRINT_PAGE_SETUP_NAME, NULL);

      if (setup)
        {
          gtk_print_operation_set_default_page_setup (operation, setup);
          g_object_unref (setup);
        }

      g_key_file_free (key_file);
    }
}

void
print_page_setup_save (GtkPrintOperation *operation,
                       GimpImage         *image)
{
  GtkPageSetup *setup;
  GKeyFile     *key_file;

  g_return_if_fail (GTK_IS_PRINT_OPERATION (operation));

  key_file = g_key_file_new ();

  setup = gtk_print_operation_get_default_page_setup (operation);

  gtk_page_setup_to_key_file (setup, key_file, PRINT_PAGE_SETUP_NAME);

  print_utils_key_file_save_as_parasite (key_file,
                                         image, PRINT_PAGE_SETUP_NAME);
  print_utils_key_file_save_as_rcfile (key_file,
                                       PRINT_PAGE_SETUP_NAME);

  g_key_file_free (key_file);
}
