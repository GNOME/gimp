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

#define PRINT_SETTINGS_MAJOR_VERSION 0
#define PRINT_SETTINGS_MINOR_VERSION 1

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print.h"
#include "print-settings.h"

static GKeyFile * print_settings_key_file_from_settings      (PrintData         *data);

static void       save_print_settings_resource_file          (GKeyFile          *settings_key_file);

static void       save_print_settings_as_parasite            (GKeyFile          *settings_key_file,
                                                              gint32             image_ID);

static void       add_print_setting_to_key_file              (const gchar       *key,
                                                              const gchar       *value,
                                                              gpointer           data);

static GKeyFile * print_settings_key_file_from_resource_file (void);

static GKeyFile * print_settings_key_file_from_parasite      (gint32             image_ID);

static gboolean   load_print_settings_from_key_file          (PrintData         *data,
                                                              GKeyFile          *key_file);

static GKeyFile * check_version                              (GKeyFile          *key_file);

/*
 * set GtkPrintSettings from the contents of a "print-settings"
 * image parasite, or, if none exists, from a resource
 * file of the same name
 */
gboolean
load_print_settings (PrintData         *data)
{
  GKeyFile *key_file;

  key_file = print_settings_key_file_from_parasite (data->image_id);

  if (! key_file)
    key_file = print_settings_key_file_from_resource_file ();

  if (key_file)
    {
      load_print_settings_from_key_file (data, key_file);
      g_key_file_free (key_file);
      return TRUE;
    }

  return FALSE;
}

/*
 * save all settings as a resource file "print-settings"
 * and as an image parasite
 */
void
save_print_settings (PrintData         *data)
{
  GKeyFile *key_file;

  key_file = print_settings_key_file_from_settings (data);
  save_print_settings_resource_file (key_file);
  save_print_settings_as_parasite (key_file, data->image_id);

  g_key_file_free (key_file);
}

/*
 * serialize print settings into a GKeyFile
 */
static GKeyFile *
print_settings_key_file_from_settings (PrintData *data)
{
  GtkPrintOperation *operation = data->operation;
  GtkPrintSettings  *settings;
  GKeyFile          *key_file  = g_key_file_new ();
  GtkPageSetup      *page_setup;

  g_key_file_set_list_separator (key_file, '=');

  /* put version information into the file */
  g_key_file_set_integer (key_file, "meta", "major-version",
                          PRINT_SETTINGS_MAJOR_VERSION);
  g_key_file_set_integer (key_file, "meta", "minor-version",
                          PRINT_SETTINGS_MINOR_VERSION);

  /* save the contents of the GtkPrintSettings for the operation */
  settings = gtk_print_operation_get_print_settings (operation);
  if (settings)
    {
      gtk_print_settings_foreach (settings, add_print_setting_to_key_file,
                                  key_file);
    }

  /* page setup */
  page_setup = gtk_print_operation_get_default_page_setup (operation);
  if (page_setup)
    {
      GtkPageOrientation orientation;

      orientation = gtk_page_setup_get_orientation (page_setup);
      g_key_file_set_integer (key_file, "page-setup", "orientation",
                              orientation);
    }

  /* other settings */
  g_key_file_set_boolean (key_file, "other-settings", "show-header",
                          data->show_info_header);
  g_key_file_set_integer (key_file, "other-settings", "unit",
                          data->unit);

  return key_file;
}

/*
 * create a resource file from a GKeyFile holding the settings
 */
static void
save_print_settings_resource_file (GKeyFile *settings_key_file)
{
  gchar     *fname;
  FILE      *settings_file;
  gchar     *contents;
  gsize      length;
  GError    *error          = NULL;

  contents = g_key_file_to_data (settings_key_file, &length, &error);
  if (error)
    {
      g_message ("Unable to get contents of settings key file.\n");
      return;
    }

  fname = g_strconcat (gimp_directory (), G_DIR_SEPARATOR_S, "print-settings", NULL);
  settings_file = fopen (fname, "w");
  if (! settings_file)
    {
      g_message ("Unable to create resource file for print settings.\n");
      return;
    }

  fwrite (contents, sizeof (gchar), length, settings_file);

  fclose (settings_file);
  g_free (contents);
  g_free (fname);
}

/*
 * create an image parasite called "print-settings" from a GKeyFile
 * holding the print settings
 */
static void
save_print_settings_as_parasite (GKeyFile *settings_key_file,
                                 gint32    image_ID)
{
  gchar     *contents;
  gsize      length;
  GError    *error          = NULL;

  contents = g_key_file_to_data (settings_key_file, &length, &error);
  if (error)
    {
      g_message ("Unable to get contents of settings key file.\n");
      return;
    }

  gimp_image_attach_new_parasite (image_ID, "print-settings", 0,
                                  length, contents);

  g_free (contents);
}

/*
 * callback used in gtk_print_settings_foreach loop
 */
static void
add_print_setting_to_key_file (const gchar *key,
                         const gchar *value,
                         gpointer     data)
{
  GKeyFile *key_file = data;

  g_key_file_set_value (key_file, "print-settings", key, value);
}

/*
 * deserialize a "print-settings" resource file into a GKeyFile
 */
static GKeyFile *
print_settings_key_file_from_resource_file (void)
{
  GKeyFile  *key_file = g_key_file_new ();
  gchar     *fname;
  GError    *error    = NULL;

  g_key_file_set_list_separator (key_file, '=');

  fname = g_strconcat (gimp_directory (),
                       G_DIR_SEPARATOR_S,
                       "print-settings",
                       NULL);

  if (! g_key_file_load_from_file (key_file, fname,
                                   G_KEY_FILE_NONE, &error))
    {
      g_key_file_free (key_file);
      key_file = NULL;
    }

  g_free (fname);

  key_file = check_version (key_file);

  return key_file;
}

/* load information from an image parasite called "print-settings"
 * return a GKeyFile containing the information if a valid parasite is found,
 * NULL otherwise
 */
static GKeyFile *
print_settings_key_file_from_parasite (gint32 image_ID)
{
  GimpParasite *parasite;
  GError    *error    = NULL;

  parasite = gimp_image_parasite_find (image_ID, "print-settings");

  if (! parasite)
    return NULL;
  else
    {
      GKeyFile  *key_file = g_key_file_new ();

      g_key_file_set_list_separator (key_file, '=');

      if (! g_key_file_load_from_data (key_file,
                                       gimp_parasite_data (parasite),
                                       gimp_parasite_data_size (parasite),
                                       G_KEY_FILE_NONE, &error))
        {
          g_key_file_free (key_file);
          key_file = NULL;;
        }

      gimp_parasite_free (parasite);

      key_file = check_version (key_file);

      return key_file;
    }
}

static gboolean
load_print_settings_from_key_file (PrintData         *data,
                                   GKeyFile          *key_file)
{
  GtkPrintOperation  *operation = data->operation;
  gchar             **keys;
  gsize               n_keys;
  GError             *error     = NULL;
  gint                i;
  GtkPageSetup       *page_setup;
  GtkPrintSettings   *settings;

  settings = gtk_print_operation_get_print_settings (operation);
  if (! settings)
    settings = gtk_print_settings_new ();

  keys = g_key_file_get_keys (key_file, "print-settings", &n_keys, &error);

  if (! keys)
    return FALSE;

  for (i = 0; i < n_keys; i++)
    {
      gchar *value = g_key_file_get_value (key_file, "print-settings",
                                           keys[i], &error);

      gtk_print_settings_set (settings, keys[i], value);

      g_free (value);
    }
  g_strfreev (keys);


  /* page setup parameters */
  page_setup = gtk_print_operation_get_default_page_setup (operation);
  if (! page_setup)
    page_setup = gtk_page_setup_new ();

  if (g_key_file_has_key (key_file, "page-setup", "orientation", &error))
    {
      GtkPageOrientation orientation;

      orientation = g_key_file_get_integer (key_file, "page-setup",
                                            "orientation", &error);
      gtk_page_setup_set_orientation (page_setup, orientation);
      gtk_print_settings_set_orientation (settings, orientation);
      data->orientation = orientation;
    }

  gtk_print_operation_set_default_page_setup (operation, page_setup);

  /* other settings */
  if (g_key_file_has_key (key_file, "other-settings", "show-header", &error))
    {
      data->show_info_header = g_key_file_get_boolean (key_file, "other-settings",
                                                       "show-header", &error);
    }
  else
    data->show_info_header = FALSE;

  if (g_key_file_has_key (key_file, "other-settings", "unit", &error))
    {
      data->unit = g_key_file_get_integer (key_file, "other-settings",
                                           "unit", &error);
    }
  else
    data->unit = GIMP_UNIT_INCH;

  gtk_print_operation_set_print_settings (operation, settings);

  return TRUE;
}

static GKeyFile *
check_version (GKeyFile *key_file)
{
  gint    major_version;
  gint    minor_version;
  GError *error       = NULL;

  if (! g_key_file_has_group (key_file, "meta"))
    return NULL;

  major_version = g_key_file_get_integer (key_file, "meta", "major-version", &error);

  if (major_version != PRINT_SETTINGS_MAJOR_VERSION)
    return NULL;

  minor_version = g_key_file_get_integer (key_file, "meta", "minor-version", &error);

  if (minor_version != PRINT_SETTINGS_MINOR_VERSION)
    return NULL;

  return key_file;
}
