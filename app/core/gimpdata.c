/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdata.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpdata.h"
#include "gimpmarshal.h"


enum
{
  DIRTY,
  SAVE,
  GET_EXTENSION,
  DUPLICATE,
  LAST_SIGNAL
};


static void    gimp_data_class_init   (GimpDataClass *klass);
static void    gimp_data_init         (GimpData      *data);

static void    gimp_data_finalize     (GObject       *object);

static void    gimp_data_name_changed (GimpObject    *object);
static gsize   gimp_data_get_memsize  (GimpObject    *object);

static void    gimp_data_real_dirty   (GimpData      *data);


static guint data_signals[LAST_SIGNAL] = { 0 };

static GimpViewableClass *parent_class = NULL;


GType
gimp_data_get_type (void)
{
  static GType data_type = 0;

  if (! data_type)
    {
      static const GTypeInfo data_info =
      {
        sizeof (GimpDataClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_data_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpData),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_data_init,
      };

      data_type = g_type_register_static (GIMP_TYPE_VIEWABLE,
					  "GimpData",
					  &data_info, 0);
  }

  return data_type;
}

static void
gimp_data_class_init (GimpDataClass *klass)
{
  GObjectClass    *object_class;
  GimpObjectClass *gimp_object_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  data_signals[DIRTY] = 
    g_signal_new ("dirty",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpDataClass, dirty),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  data_signals[SAVE] = 
    g_signal_new ("save",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpDataClass, save),
		  NULL, NULL,
		  gimp_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);

  data_signals[GET_EXTENSION] = 
    g_signal_new ("get_extension",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpDataClass, get_extension),
		  NULL, NULL,
		  gimp_marshal_POINTER__VOID,
		  G_TYPE_POINTER, 0);

  data_signals[DUPLICATE] = 
    g_signal_new ("duplicate",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpDataClass, duplicate),
		  NULL, NULL,
		  gimp_marshal_POINTER__VOID,
		  G_TYPE_POINTER, 0);

  object_class->finalize          = gimp_data_finalize;

  gimp_object_class->name_changed = gimp_data_name_changed;
  gimp_object_class->get_memsize  = gimp_data_get_memsize;

  klass->dirty                    = gimp_data_real_dirty;
  klass->save                     = NULL;
  klass->get_extension            = NULL;
  klass->duplicate                = NULL;
}

static void
gimp_data_init (GimpData *data)
{
  data->filename = NULL;
  data->dirty    = FALSE;
}

static void
gimp_data_finalize (GObject *object)
{
  GimpData *data;

  data = GIMP_DATA (object);

  if (data->filename)
    {
      g_free (data->filename);
      data->filename = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_data_name_changed (GimpObject *object)
{
  if (GIMP_OBJECT_CLASS (parent_class)->name_changed)
    GIMP_OBJECT_CLASS (parent_class)->name_changed (object);

  gimp_data_dirty (GIMP_DATA (object));
}

static gsize
gimp_data_get_memsize (GimpObject *object)
{
  GimpData *data;
  gsize     memsize = 0;

  data = GIMP_DATA (object);

  if (data->filename)
    memsize += strlen (data->filename) + 1;

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object);
}

gboolean
gimp_data_save (GimpData *data)
{
  gboolean success = FALSE;

  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);

  if (! data->filename)
    {
      g_warning ("%s(): can't save data with NULL filename",
		 G_GNUC_FUNCTION);
      return FALSE;
    }

  g_signal_emit (G_OBJECT (data), data_signals[SAVE], 0,
		 &success);

  if (success)
    data->dirty = FALSE;

  return success;
}

void
gimp_data_dirty (GimpData *data)
{
  g_return_if_fail (GIMP_IS_DATA (data));

  g_signal_emit (G_OBJECT (data), data_signals[DIRTY], 0);
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

  g_return_val_if_fail (GIMP_IS_DATA (data), NULL);

  g_signal_emit (G_OBJECT (data), data_signals[GET_EXTENSION], 0,
		 &extension);

  return extension;
}

void
gimp_data_set_filename (GimpData    *data,
			const gchar *filename)
{
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
  gchar *fullpath;
  gchar *safe_name;
  gint   i;
  gint   unum = 1;
  FILE  *file;

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
    if (safe_name[i] == G_DIR_SEPARATOR || g_ascii_isspace (safe_name[i]))
      safe_name[i] = '_';

  filename = g_strdup_printf ("%s%s",
			      safe_name,
			      gimp_data_get_extension (data));

  fullpath = g_build_filename (dir, filename, NULL);

  g_free (filename);

  while ((file = fopen (fullpath, "r")))
    {
      fclose (file);

      g_free (fullpath);

      filename = g_strdup_printf ("%s_%d%s",
				  safe_name,
                                  unum,
				  gimp_data_get_extension (data));

      fullpath = g_build_filename (dir, filename, NULL);

      g_free (filename);

      unum++;
    }

  g_free (dir);
  g_free (safe_name);

  gimp_data_set_filename (data, fullpath);

  g_free (fullpath);
}

GimpData *
gimp_data_duplicate (GimpData *data)
{
  GimpData *new_data = NULL;

  g_signal_emit (G_OBJECT (data), data_signals[DUPLICATE], 0,
		 &new_data);

  return new_data;
}
