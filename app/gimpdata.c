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

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include "apptypes.h"

#include "gimpdata.h"
#include "gimpmarshal.h"

#include "libgimp/gimpenv.h"


enum
{
  DIRTY,
  SAVE,
  GET_EXTENSION,
  DUPLICATE,
  LAST_SIGNAL
};


static void          gimp_data_class_init         (GimpDataClass *klass);
static void          gimp_data_init               (GimpData      *data);
static void          gimp_data_destroy            (GtkObject     *object);
static void          gimp_data_name_changed       (GimpObject    *object);
static void          gimp_data_real_dirty         (GimpData      *data);


static guint data_signals[LAST_SIGNAL] = { 0 };

static GimpViewableClass *parent_class = NULL;


GtkType
gimp_data_get_type (void)
{
  static GtkType data_type = 0;

  if (! data_type)
    {
      static const GtkTypeInfo data_info =
      {
        "GimpData",
        sizeof (GimpData),
        sizeof (GimpDataClass),
        (GtkClassInitFunc) gimp_data_class_init,
        (GtkObjectInitFunc) gimp_data_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      data_type = gtk_type_unique (GIMP_TYPE_VIEWABLE, &data_info);
  }

  return data_type;
}

static void
gimp_data_class_init (GimpDataClass *klass)
{
  GtkObjectClass  *object_class;
  GimpObjectClass *gimp_object_class;

  object_class      = (GtkObjectClass *) klass;
  gimp_object_class = (GimpObjectClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_VIEWABLE);

  data_signals[DIRTY] = 
    gtk_signal_new ("dirty",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpDataClass,
                                       dirty),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);

  data_signals[SAVE] = 
    gtk_signal_new ("save",
                    GTK_RUN_LAST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpDataClass,
                                       save),
                    gtk_marshal_BOOL__NONE,
                    GTK_TYPE_BOOL, 0);

  data_signals[GET_EXTENSION] = 
    gtk_signal_new ("get_extension",
                    GTK_RUN_LAST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpDataClass,
                                       get_extension),
                    gimp_marshal_POINTER__NONE,
                    GTK_TYPE_POINTER, 0);

  data_signals[DUPLICATE] = 
    gtk_signal_new ("duplicate",
                    GTK_RUN_LAST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpDataClass,
                                       duplicate),
                    gimp_marshal_POINTER__NONE,
                    GTK_TYPE_POINTER, 0);

  object_class->destroy = gimp_data_destroy;

  gimp_object_class->name_changed = gimp_data_name_changed;

  klass->dirty         = gimp_data_real_dirty;
  klass->save          = NULL;
  klass->get_extension = NULL;
  klass->duplicate     = NULL;
}

static void
gimp_data_init (GimpData *data)
{
  data->filename = NULL;
  data->dirty    = FALSE;
}

static void
gimp_data_destroy (GtkObject *object)
{
  GimpData *data;

  data = GIMP_DATA (object);

  g_free (data->filename);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_data_name_changed (GimpObject *object)
{
  if (GIMP_OBJECT_CLASS (parent_class)->name_changed)
    GIMP_OBJECT_CLASS (parent_class)->name_changed (object);

  gimp_data_dirty (GIMP_DATA (object));
}

gboolean
gimp_data_save (GimpData *data)
{
  gboolean success = FALSE;

  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);

  if (! data->filename)
    {
      g_warning ("%s(): can't save data with NULL filename",
		 G_GNUC_FUNCTION);
      return FALSE;
    }

  gtk_signal_emit (GTK_OBJECT (data), data_signals[SAVE],
		   &success);

  if (success)
    data->dirty = FALSE;

  return success;
}

void
gimp_data_dirty (GimpData *data)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (GIMP_IS_DATA (data));

  gtk_signal_emit (GTK_OBJECT (data), data_signals[DIRTY]);
}

static void
gimp_data_real_dirty (GimpData *data)
{
  data->dirty = TRUE;

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (data));
}

gboolean
gimp_data_delete_from_disk (GimpData *data)
{
  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);
  g_return_val_if_fail (data->filename != NULL, FALSE);

  if (unlink (data->filename) == -1)
    {
      g_message ("%s(): could not unlink() %s: %s",
		 G_GNUC_FUNCTION, data->filename, g_strerror (errno));
      return FALSE;
    }

  return TRUE;
}

const gchar *
gimp_data_get_extension (GimpData *data)
{
  const gchar *extension = NULL;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DATA (data), NULL);

  gtk_signal_emit (GTK_OBJECT (data), data_signals[GET_EXTENSION],
		   &extension);

  return extension;
}

void
gimp_data_set_filename (GimpData    *data,
			const gchar *filename)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (GIMP_IS_DATA (data));

  g_free (data->filename);

  data->filename = g_strdup (filename);
}

void
gimp_data_create_filename (GimpData    *data,
			   const gchar *basename,
			   const gchar *data_path)
{
  GList *path;
  gchar *dir;
  gchar *filename;
  gchar *safe_name;
  gint   i;
  gint   unum = 1;
  FILE  *file;

  g_return_if_fail (data != NULL);
  g_return_if_fail (GIMP_IS_DATA (data));
  g_return_if_fail (basename != NULL);
  g_return_if_fail (data_path != NULL);

  path = gimp_path_parse (data_path, 16, TRUE, NULL);
  dir  = gimp_path_get_user_writable_dir (path);
  gimp_path_free (path);

  if (! dir)
    return;

  safe_name = g_strdup (basename);
  if (safe_name[0] == '.')
    safe_name[0] = '_';
  for (i = 0; safe_name[i]; i++)
    if (safe_name[i] == G_DIR_SEPARATOR || isspace (safe_name[i]))
      safe_name[i] = '_';

  filename = g_strdup_printf ("%s%s%s",
			      dir, safe_name,
			      gimp_data_get_extension (data));

  while ((file = fopen (filename, "r")))
    {
      fclose (file);

      g_free (filename);
      filename = g_strdup_printf ("%s%s_%d%s",
				  dir, safe_name, unum,
				  gimp_data_get_extension (data));
      unum++;
    }

  g_free (dir);
  g_free (safe_name);

  gimp_data_set_filename (data, filename);

  g_free (filename);
}

GimpData *
gimp_data_duplicate (GimpData *data)
{
  GimpData *new_data = NULL;

  gtk_signal_emit (GTK_OBJECT (data), data_signals[DUPLICATE],
		   &new_data);

  return new_data;
}
