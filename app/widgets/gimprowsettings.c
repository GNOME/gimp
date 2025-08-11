/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprowsettings.c
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpsettings.h"

#include "gimprowsettings.h"


struct _GimpRowSettings
{
  GimpRow  parent_instance;
};


static gboolean   gimp_row_settings_name_edited (GimpRow     *row,
                                                 const gchar *new_name);


G_DEFINE_TYPE (GimpRowSettings,
               gimp_row_settings,
               GIMP_TYPE_ROW)

#define parent_class gimp_row_settings_parent_class


static void
gimp_row_settings_class_init (GimpRowSettingsClass *klass)
{
  GimpRowClass *row_class = GIMP_ROW_CLASS (klass);

  row_class->name_edited = gimp_row_settings_name_edited;
}

static void
gimp_row_settings_init (GimpRowSettings *row)
{
}

static gboolean
gimp_row_settings_name_edited (GimpRow     *row,
                               const gchar *new_name)
{
  GimpObject  *object;
  const gchar *old_name;
  gchar       *name;

  object = GIMP_OBJECT (gimp_row_get_viewable (row));

  old_name = gimp_object_get_name (object);

  if (! old_name) old_name = "";
  if (! new_name) new_name = "";

  name = g_strstrip (g_strdup (new_name));

  if (! strlen (name))
    {
      gtk_widget_error_bell (GTK_WIDGET (row));
      g_free (name);

      return FALSE;
    }

  if (strcmp (old_name, name))
    {
      gint64 t;

      g_object_get (object,
                    "time", &t,
                    NULL);

      if (t > 0)
        g_object_set (object,
                      "time", (gint64) 0,
                      NULL);

      /*  set name after time so the object is reordered correctly  */
      gimp_object_take_name (object, name);
    }
  else
    {
      g_free (name);
    }


  return TRUE;
}
