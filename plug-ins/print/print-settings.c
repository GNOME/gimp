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
#include "print-settings.h"
#include "print-utils.h"


#define PRINT_SETTINGS_MAJOR_VERSION 0
#define PRINT_SETTINGS_MINOR_VERSION 4

#define PRINT_SETTINGS_NAME          "print-settings"


static GKeyFile * print_settings_key_file_from_settings      (PrintData         *data);

static void       print_settings_add_to_key_file             (const gchar       *key,
                                                              const gchar       *value,
                                                              gpointer           data);

static GKeyFile * print_settings_key_file_from_resource_file (void);

static GKeyFile * print_settings_key_file_from_parasite      (gint32             image_ID);

static gboolean   print_settings_load_from_key_file          (PrintData         *data,
                                                              GKeyFile          *key_file);

static gboolean   print_settings_check_version               (GKeyFile          *key_file);

/*
 * set GtkPrintSettings from the contents of a "print-settings"
 * image parasite, or, if none exists, from a resource
 * file of the same name
 */
gboolean
print_settings_load (PrintData *data)
{
  GKeyFile *key_file = print_settings_key_file_from_parasite (data->image_id);

  if (! key_file)
    key_file = print_settings_key_file_from_resource_file ();

  if (key_file)
    {
      print_settings_load_from_key_file (data, key_file);
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
print_settings_save (PrintData *data)
{
  GKeyFile *key_file = print_settings_key_file_from_settings (data);

  /* image setup */
  if (gimp_image_is_valid (data->image_id))
    {
      gdouble xres;
      gdouble yres;

      gimp_image_get_resolution (data->image_id, &xres, &yres);

      g_key_file_set_integer (key_file, "image-setup",
                              "unit", data->unit);
      /* Do not save the print resolution when it is the expected image
       * resolution so that changing it (i.e. in "print size" dialog)
       * is not overridden by any previous prints.
       */
      if ((data->min_xres <= xres && ABS (xres - data->xres) > 0.1)          ||
          (data->min_yres <= yres && ABS (yres - data->yres) > 0.1)          ||
          (data->min_xres > xres && ABS (data->min_xres - data->xres) > 0.1) ||
          (data->min_yres > yres && ABS (data->min_yres - data->yres) > 0.1))
        {
          g_key_file_set_double  (key_file, "image-setup",
                                  "x-resolution", data->xres);
          g_key_file_set_double  (key_file, "image-setup",
                                  "y-resolution", data->yres);
        }
      g_key_file_set_double  (key_file, "image-setup",
                              "x-offset", data->offset_x);
      g_key_file_set_double  (key_file, "image-setup",
                              "y-offset", data->offset_y);
      g_key_file_set_integer (key_file, "image-setup",
                              "center-mode", data->center);
      g_key_file_set_boolean (key_file, "image-setup",
                              "use-full-page", data->use_full_page);
      g_key_file_set_boolean (key_file, "image-setup",
                              "crop-marks", data->draw_crop_marks);

      print_utils_key_file_save_as_parasite (key_file,
                                             data->image_id,
                                             PRINT_SETTINGS_NAME);
    }

  /* some settings shouldn't be made persistent on a global level,
   * so they are only stored in the image, not in the rcfile
   */

  g_key_file_remove_key (key_file, "image-setup", "x-resolution", NULL);
  g_key_file_remove_key (key_file, "image-setup", "y-resolution", NULL);
  g_key_file_remove_key (key_file, "image-setup", "x-offset", NULL);
  g_key_file_remove_key (key_file, "image-setup", "y-offset", NULL);

  g_key_file_remove_key (key_file, PRINT_SETTINGS_NAME, "n-copies", NULL);

  print_utils_key_file_save_as_rcfile (key_file, PRINT_SETTINGS_NAME);

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

  /* put version information into the file */
  g_key_file_set_integer (key_file, "meta", "major-version",
                          PRINT_SETTINGS_MAJOR_VERSION);
  g_key_file_set_integer (key_file, "meta", "minor-version",
                          PRINT_SETTINGS_MINOR_VERSION);

  /* save the contents of the GtkPrintSettings for the operation */
  settings = gtk_print_operation_get_print_settings (operation);

  if (settings)
    gtk_print_settings_foreach (settings,
                                print_settings_add_to_key_file, key_file);

  return key_file;
}

/*
 * callback used in gtk_print_settings_foreach loop
 */
static void
print_settings_add_to_key_file (const gchar *key,
                                const gchar *value,
                                gpointer     data)
{
  GKeyFile *key_file = data;

  g_key_file_set_value (key_file, PRINT_SETTINGS_NAME, key, value);
}

/*
 * deserialize a "print-settings" resource file into a GKeyFile
 */
static GKeyFile *
print_settings_key_file_from_resource_file (void)
{
  GKeyFile *key_file;

  key_file = print_utils_key_file_load_from_rcfile (PRINT_SETTINGS_NAME);

  if (key_file && ! print_settings_check_version (key_file))
    {
      g_key_file_free (key_file);
      return NULL;
    }

  return key_file;
}

/* load information from an image parasite called "print-settings"
 * return a GKeyFile containing the information if a valid parasite is found,
 * NULL otherwise
 */
static GKeyFile *
print_settings_key_file_from_parasite (gint32 image_ID)
{
  GKeyFile *key_file;

  key_file = print_utils_key_file_load_from_parasite (image_ID,
                                                      PRINT_SETTINGS_NAME);

  if (key_file && ! print_settings_check_version (key_file))
    {
      g_key_file_free (key_file);
      return NULL;
    }

  return key_file;
}

static gboolean
print_settings_load_from_key_file (PrintData *data,
                                   GKeyFile  *key_file)
{
  GtkPrintOperation  *operation = data->operation;
  GtkPrintSettings   *settings;
  gchar             **keys;
  gsize               n_keys;
  gint                i;

  settings = gtk_print_operation_get_print_settings (operation);
  if (! settings)
    settings = gtk_print_settings_new ();

  keys = g_key_file_get_keys (key_file, PRINT_SETTINGS_NAME, &n_keys, NULL);

  if (! keys)
    return FALSE;

  for (i = 0; i < n_keys; i++)
    {
      gchar *value;

      value = g_key_file_get_value (key_file,
                                    PRINT_SETTINGS_NAME, keys[i], NULL);

      if (value)
        {
          gtk_print_settings_set (settings, keys[i], value);
          g_free (value);
        }
    }

  g_strfreev (keys);

  if (g_key_file_has_key (key_file, "image-setup", "unit", NULL))
    {
      data->unit = g_key_file_get_integer (key_file, "image-setup",
                                           "unit", NULL);
    }

  if (g_key_file_has_key (key_file, "image-setup", "x-resolution", NULL) &&
      g_key_file_has_key (key_file, "image-setup", "y-resolution", NULL))
    {
      data->xres = g_key_file_get_double (key_file, "image-setup",
                                          "x-resolution", NULL);
      data->yres = g_key_file_get_double (key_file, "image-setup",
                                          "y-resolution", NULL);
    }

  if (g_key_file_has_key (key_file, "image-setup", "x-offset", NULL) &&
      g_key_file_has_key (key_file, "image-setup", "y-offset", NULL))
    {
      data->offset_x = g_key_file_get_double (key_file, "image-setup",
                                              "x-offset", NULL);
      data->offset_y = g_key_file_get_double (key_file, "image-setup",
                                              "y-offset", NULL);
    }

  if (g_key_file_has_key (key_file, "image-setup", "center-mode", NULL))
    {
      data->center = g_key_file_get_integer (key_file, "image-setup",
                                             "center-mode", NULL);
    }

  if (g_key_file_has_key (key_file, "image-setup", "use-full-page", NULL))
    {
      data->use_full_page = g_key_file_get_boolean (key_file, "image-setup",
                                                    "use-full-page", NULL);
    }

  if (g_key_file_has_key (key_file, "image-setup", "crop-marks", NULL))
    {
      data->draw_crop_marks = g_key_file_get_boolean (key_file, "image-setup",
                                                      "crop-marks", NULL);
    }

  gtk_print_operation_set_print_settings (operation, settings);

  return TRUE;
}

static gboolean
print_settings_check_version (GKeyFile *key_file)
{
  gint  major_version;
  gint  minor_version;

  if (! g_key_file_has_group (key_file, "meta"))
    return FALSE;

  major_version = g_key_file_get_integer (key_file,
                                          "meta", "major-version", NULL);

  if (major_version != PRINT_SETTINGS_MAJOR_VERSION)
    return FALSE;

  minor_version = g_key_file_get_integer (key_file,
                                          "meta", "minor-version", NULL);

  if (minor_version != PRINT_SETTINGS_MINOR_VERSION)
    return FALSE;

  return TRUE;
}
