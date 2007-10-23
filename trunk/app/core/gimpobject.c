/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp-utils.h"
#include "gimpmarshal.h"
#include "gimpobject.h"


enum
{
  DISCONNECT,
  NAME_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_NAME
};


static void    gimp_object_dispose          (GObject      *object);
static void    gimp_object_finalize         (GObject      *object);
static void    gimp_object_set_property     (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void    gimp_object_get_property     (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);
static gint64  gimp_object_real_get_memsize (GimpObject   *object,
                                             gint64       *gui_size);
static void    gimp_object_name_normalize   (GimpObject   *object);


G_DEFINE_TYPE (GimpObject, gimp_object, G_TYPE_OBJECT)

#define parent_class gimp_object_parent_class

static guint object_signals[LAST_SIGNAL] = { 0 };


static void
gimp_object_class_init (GimpObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_signals[DISCONNECT] =
    g_signal_new ("disconnect",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpObjectClass, disconnect),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_signals[NAME_CHANGED] =
    g_signal_new ("name-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpObjectClass, name_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->dispose      = gimp_object_dispose;
  object_class->finalize     = gimp_object_finalize;
  object_class->set_property = gimp_object_set_property;
  object_class->get_property = gimp_object_get_property;

  klass->disconnect          = NULL;
  klass->name_changed        = NULL;
  klass->get_memsize         = gimp_object_real_get_memsize;

  g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_object_init (GimpObject *object)
{
  object->name       = NULL;
  object->normalized = NULL;
}

static void
gimp_object_dispose (GObject *object)
{
  GimpObject *gimp_object = GIMP_OBJECT (object);

  if (! gimp_object->disconnected)
    {
      g_signal_emit (object, object_signals[DISCONNECT], 0);

      gimp_object->disconnected = TRUE;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_object_finalize (GObject *object)
{
  gimp_object_name_free (GIMP_OBJECT (object));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_object_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GimpObject *gimp_object = GIMP_OBJECT (object);

  switch (property_id)
    {
    case PROP_NAME:
      gimp_object_set_name (gimp_object, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_object_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GimpObject *gimp_object = GIMP_OBJECT (object);

  switch (property_id)
    {
    case PROP_NAME:
      if (gimp_object->static_name)
        g_value_set_static_string (value, gimp_object->name);
      else
        g_value_set_string (value, gimp_object->name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_object_set_name:
 * @object: a #GimpObject
 * @name: the @object's new name
 *
 * Sets the @object's name. Takes care of freeing the old name and
 * emitting the "name_changed" signal if the old and new name differ.
 **/
void
gimp_object_set_name (GimpObject  *object,
                      const gchar *name)
{
  g_return_if_fail (GIMP_IS_OBJECT (object));

  if ((!object->name && !name) ||
      (object->name && name && !strcmp (object->name, name)))
    return;

  gimp_object_name_free (object);

  object->name = g_strdup (name);
  object->static_name = FALSE;

  gimp_object_name_changed (object);
  g_object_notify (G_OBJECT (object), "name");
}

/**
 * gimp_object_set_name_safe:
 * @object: a #GimpObject
 * @name: the @object's new name
 *
 * A safe version of gimp_object_set_name() that takes care of
 * handling newlines and overly long names. The actual name set
 * may be different to the @name you pass.
 **/
void
gimp_object_set_name_safe (GimpObject  *object,
                           const gchar *name)
{
  g_return_if_fail (GIMP_IS_OBJECT (object));

  if ((!object->name && !name) ||
      (object->name && name && !strcmp (object->name, name)))
    return;

  gimp_object_name_free (object);

  object->name = gimp_utf8_strtrim (name, 30);
  object->static_name = FALSE;

  gimp_object_name_changed (object);
  g_object_notify (G_OBJECT (object), "name");
}

void
gimp_object_set_static_name (GimpObject  *object,
                             const gchar *name)
{
  g_return_if_fail (GIMP_IS_OBJECT (object));

  gimp_object_name_free (object);

  object->name = (gchar *) name;
  object->static_name = TRUE;

  gimp_object_name_changed (object);
  g_object_notify (G_OBJECT (object), "name");
}

void
gimp_object_take_name (GimpObject *object,
                       gchar      *name)
{
  g_return_if_fail (GIMP_IS_OBJECT (object));

  gimp_object_name_free (object);

  object->name = name;
  object->static_name = FALSE;

  gimp_object_name_changed (object);
  g_object_notify (G_OBJECT (object), "name");
}

/**
 * gimp_object_get_name:
 * @object: a #GimpObject
 *
 * This function gives access to the name of a GimpObject. The
 * returned name belongs to the object and must not be freed.
 *
 * Return value: a pointer to the @object's name
 **/
const gchar *
gimp_object_get_name (const GimpObject *object)
{
  g_return_val_if_fail (GIMP_IS_OBJECT (object), NULL);

  return object->name;
}

/**
 * gimp_object_name_changed:
 * @object: a #GimpObject
 *
 * Causes the "name-changed" signal to be emitted.
 **/
void
gimp_object_name_changed (GimpObject *object)
{
  g_return_if_fail (GIMP_IS_OBJECT (object));

  g_signal_emit (object, object_signals[NAME_CHANGED], 0);
}

/**
 * gimp_object_name_free:
 * @object: a #GimpObject
 *
 * Frees the name of @object and sets the name pointer to %NULL. Also
 * takes care of the normalized name that the object might be caching.
 *
 * In general you should be using gimp_object_set_name() instead. But
 * if you ever need to free the object name but don't want the
 * "name-changed" signal to be emitted, then use this function. Never
 * ever free the object name directly!
 **/
void
gimp_object_name_free (GimpObject *object)
{
  if (object->normalized)
    {
      if (object->normalized != object->name)
        g_free (object->normalized);

      object->normalized = NULL;
    }

  if (object->name)
    {
      if (! object->static_name)
        g_free (object->name);

      object->name = NULL;
      object->static_name = FALSE;
    }
}

/**
 * gimp_object_name_collate:
 * @object1: a #GimpObject
 * @object2: another #GimpObject
 *
 * Compares two object names for ordering using the linguistically
 * correct rules for the current locale. It caches the normalized
 * version of the object name to speed up subsequent calls.
 *
 * Return value: -1 if object1 compares before object2,
 *                0 if they compare equal,
 *                1 if object1 compares after object2.
 **/
gint
gimp_object_name_collate (GimpObject *object1,
                          GimpObject *object2)
{
  if (! object1->normalized)
    gimp_object_name_normalize (object1);

  if (! object2->normalized)
    gimp_object_name_normalize (object2);

  return strcmp (object1->normalized, object2->normalized);
}

static void
gimp_object_name_normalize (GimpObject *object)
{
  g_return_if_fail (object->normalized == NULL);

  if (object->name)
    {
      gchar *key = g_utf8_collate_key (object->name, -1);

      if (strcmp (key, object->name))
        {
          object->normalized = key;
        }
      else
        {
          g_free (key);
          object->normalized = object->name;
        }
    }
}


#define DEBUG_MEMSIZE 1

#ifdef DEBUG_MEMSIZE
gboolean gimp_debug_memsize = FALSE;
#endif

gint64
gimp_object_get_memsize (GimpObject *object,
                         gint64     *gui_size)
{
  gint64 my_size     = 0;
  gint64 my_gui_size = 0;

  g_return_val_if_fail (GIMP_IS_OBJECT (object), 0);

#ifdef DEBUG_MEMSIZE
  if (gimp_debug_memsize)
    {
      static gint   indent_level     = 0;
      static GList *aggregation_tree = NULL;
      static gchar  indent_buf[256];

      gint64  memsize;
      gint64  gui_memsize = 0;
      gint    i;
      gint    my_indent_level;
      gchar  *object_size;

      indent_level++;

      my_indent_level = indent_level;

      memsize = GIMP_OBJECT_GET_CLASS (object)->get_memsize (object,
                                                             &gui_memsize);

      indent_level--;

      for (i = 0; i < MIN (my_indent_level * 2, sizeof (indent_buf) - 1); i++)
        indent_buf[i] = ' ';

      indent_buf[i] = '\0';

      object_size = g_strdup_printf ("%s%s \"%s\": "
                                     "%" G_GINT64_FORMAT
                                     "(%" G_GINT64_FORMAT ")\n",
                                     indent_buf,
                                     g_type_name (G_TYPE_FROM_INSTANCE (object)),
                                     object->name ? object->name : "anonymous",
                                     memsize,
                                     gui_memsize);

      aggregation_tree = g_list_prepend (aggregation_tree, object_size);

      if (indent_level == 0)
        {
          GList *list;

          for (list = aggregation_tree; list; list = g_list_next (list))
            {
              g_print ("%s", (gchar *) list->data);
              g_free (list->data);
            }

          g_list_free (aggregation_tree);
          aggregation_tree = NULL;
        }

      return memsize;
    }
#endif /* DEBUG_MEMSIZE */

  my_size = GIMP_OBJECT_GET_CLASS (object)->get_memsize (object,
                                                         &my_gui_size);

  if (gui_size)
    *gui_size = my_gui_size;

  return my_size;
}

static gint64
gimp_object_real_get_memsize (GimpObject *object,
                              gint64     *gui_size)
{
  gint64 memsize = 0;

  if (object->name && ! object->static_name)
    memsize += strlen (object->name) + 1;

  return memsize + gimp_g_object_get_memsize ((GObject *) object);
}
