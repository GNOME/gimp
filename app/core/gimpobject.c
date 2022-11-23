/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gio/gio.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma-memsize.h"
#include "ligmaobject.h"

#include "ligma-debug.h"


enum
{
  DISCONNECT,
  NAME_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_NAME,
  N_PROPS
};

struct _LigmaObjectPrivate
{
  gchar *name;
  gchar *normalized;
  guint  static_name  : 1;
  guint  disconnected : 1;
};


static void    ligma_object_constructed      (GObject         *object);
static void    ligma_object_dispose          (GObject         *object);
static void    ligma_object_finalize         (GObject         *object);
static void    ligma_object_set_property     (GObject         *object,
                                             guint            property_id,
                                             const GValue    *value,
                                             GParamSpec      *pspec);
static void    ligma_object_get_property     (GObject         *object,
                                             guint            property_id,
                                             GValue          *value,
                                             GParamSpec      *pspec);
static gint64  ligma_object_real_get_memsize (LigmaObject      *object,
                                             gint64          *gui_size);
static void    ligma_object_name_normalize   (LigmaObject      *object);


G_DEFINE_TYPE_WITH_CODE (LigmaObject, ligma_object, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (LigmaObject))

#define parent_class ligma_object_parent_class

static guint object_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *object_props[N_PROPS] = { NULL, };


static void
ligma_object_class_init (LigmaObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_signals[DISCONNECT] =
    g_signal_new ("disconnect",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaObjectClass, disconnect),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_signals[NAME_CHANGED] =
    g_signal_new ("name-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaObjectClass, name_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed  = ligma_object_constructed;
  object_class->dispose      = ligma_object_dispose;
  object_class->finalize     = ligma_object_finalize;
  object_class->set_property = ligma_object_set_property;
  object_class->get_property = ligma_object_get_property;

  klass->disconnect          = NULL;
  klass->name_changed        = NULL;
  klass->get_memsize         = ligma_object_real_get_memsize;

  object_props[PROP_NAME] = g_param_spec_string ("name",
                                                 NULL, NULL,
                                                 NULL,
                                                 LIGMA_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT);

  g_object_class_install_properties (object_class, N_PROPS, object_props);
}

static void
ligma_object_init (LigmaObject *object)
{
  object->p = ligma_object_get_instance_private (object);

  object->p->name       = NULL;
  object->p->normalized = NULL;
}

static void
ligma_object_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_debug_add_instance (object, G_OBJECT_GET_CLASS (object));
}

static void
ligma_object_dispose (GObject *object)
{
  LigmaObject *ligma_object = LIGMA_OBJECT (object);

  if (! ligma_object->p->disconnected)
    {
      g_signal_emit (object, object_signals[DISCONNECT], 0);

      ligma_object->p->disconnected = TRUE;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_object_finalize (GObject *object)
{
  ligma_object_name_free (LIGMA_OBJECT (object));

  ligma_debug_remove_instance (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_object_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  LigmaObject *ligma_object = LIGMA_OBJECT (object);

  switch (property_id)
    {
    case PROP_NAME:
      ligma_object_set_name (ligma_object, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_object_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  LigmaObject *ligma_object = LIGMA_OBJECT (object);

  switch (property_id)
    {
    case PROP_NAME:
      if (ligma_object->p->static_name)
        g_value_set_static_string (value, ligma_object->p->name);
      else
        g_value_set_string (value, ligma_object->p->name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * ligma_object_set_name:
 * @object: a #LigmaObject
 * @name: the @object's new name (transfer none)
 *
 * Sets the @object's name. Takes care of freeing the old name and
 * emitting the ::name_changed signal if the old and new name differ.
 **/
void
ligma_object_set_name (LigmaObject  *object,
                      const gchar *name)
{
  g_return_if_fail (LIGMA_IS_OBJECT (object));

  if (! g_strcmp0 (object->p->name, name))
    return;

  ligma_object_name_free (object);

  object->p->name = g_strdup (name);
  object->p->static_name = FALSE;

  ligma_object_name_changed (object);
  g_object_notify_by_pspec (G_OBJECT (object), object_props[PROP_NAME]);
}

/**
 * ligma_object_set_name_safe:
 * @object: a #LigmaObject
 * @name: the @object's new name (transfer none)
 *
 * A safe version of ligma_object_set_name() that takes care of
 * handling newlines and overly long names. The actual name set
 * may be different to the @name you pass.
 **/
void
ligma_object_set_name_safe (LigmaObject  *object,
                           const gchar *name)
{
  g_return_if_fail (LIGMA_IS_OBJECT (object));

  if (! g_strcmp0 (object->p->name, name))
    return;

  ligma_object_name_free (object);

  object->p->name = ligma_utf8_strtrim (name, 30);
  object->p->static_name = FALSE;

  ligma_object_name_changed (object);
  g_object_notify_by_pspec (G_OBJECT (object), object_props[PROP_NAME]);
}

/**
 * ligma_object_set_static_name:
 * @object: a #LigmaObject
 * @name: the @object's new name as a static string
 *
 * Sets the @object's name.
 * Takes care of freeing the old name (when it was not statically set)
 * and emits the ::name_changed signal if the old and new name differ.
 *
 * This function is a variant of ligma_object_set_name() which assumes that
 * the string is static and optimizes for this use case.
 * Do not ever use this function with allocated strings.
 **/
void
ligma_object_set_static_name (LigmaObject  *object,
                             const gchar *name)
{
  g_return_if_fail (LIGMA_IS_OBJECT (object));

  if (! g_strcmp0 (object->p->name, name))
    return;

  ligma_object_name_free (object);

  object->p->name = (gchar *) name;
  object->p->static_name = TRUE;

  ligma_object_name_changed (object);
  g_object_notify_by_pspec (G_OBJECT (object), object_props[PROP_NAME]);
}

/**
 * ligma_object_take_name:
 * @object: a #LigmaObject
 * @name: the @object's new name (transfer full)
 *
 * Sets the @object's name. Takes care of freeing the old name and
 * emitting the ::name_changed signal if the old and new name differ.
 *
 * Only use this function with GLib allocated strings.
 *
 * LigmaObject will own the @name pointer and will dispose it when
 * no longer needed or when it is the same as stored name.
 * Calling code should *not* g_free() passed @name at all.
 **/
void
ligma_object_take_name (LigmaObject *object,
                       gchar      *name)
{
  g_return_if_fail (LIGMA_IS_OBJECT (object));

  if (! g_strcmp0 (object->p->name, name))
    {
      g_free (name);
      return;
    }

  ligma_object_name_free (object);

  object->p->name = name;
  object->p->static_name = FALSE;

  ligma_object_name_changed (object);
  g_object_notify_by_pspec (G_OBJECT (object), object_props[PROP_NAME]);
}

/**
 * ligma_object_get_name:
 * @object: a #LigmaObject
 *
 * This function gives access to the name of a LigmaObject. The
 * returned name belongs to the object and must not be freed.
 *
 * Returns: a pointer to the @object's name
 **/
const gchar *
ligma_object_get_name (gconstpointer object)
{
  const LigmaObject *object_typed = object;
  g_return_val_if_fail (LIGMA_IS_OBJECT (object_typed), NULL);

  return object_typed->p->name;
}

/**
 * ligma_object_name_changed:
 * @object: a #LigmaObject
 *
 * Causes the ::name-changed signal to be emitted.
 **/
void
ligma_object_name_changed (LigmaObject *object)
{
  g_return_if_fail (LIGMA_IS_OBJECT (object));

  g_signal_emit (object, object_signals[NAME_CHANGED], 0);
}

/**
 * ligma_object_name_free:
 * @object: a #LigmaObject
 *
 * Frees the name of @object and sets the name pointer to %NULL. Also
 * takes care of the normalized name that the object might be caching.
 *
 * In general you should be using ligma_object_set_name() instead. But
 * if you ever need to free the object name but don't want the
 * ::name-changed signal to be emitted, then use this function. Never
 * ever free the object name directly!
 **/
void
ligma_object_name_free (LigmaObject *object)
{
  if (object->p->normalized)
    {
      if (object->p->normalized != object->p->name)
        g_free (object->p->normalized);

      object->p->normalized = NULL;
    }

  if (object->p->name)
    {
      if (! object->p->static_name)
        g_free (object->p->name);

      object->p->name = NULL;
      object->p->static_name = FALSE;
    }
}

/**
 * ligma_object_name_collate:
 * @object1: a #LigmaObject
 * @object2: another #LigmaObject
 *
 * Compares two object names for ordering using the linguistically
 * correct rules for the current locale. It caches the normalized
 * version of the object name to speed up subsequent calls.
 *
 * Returns: -1 if object1 compares before object2,
 *                0 if they compare equal,
 *                1 if object1 compares after object2.
 **/
gint
ligma_object_name_collate (LigmaObject *object1,
                          LigmaObject *object2)
{
  if (! object1->p->normalized)
    ligma_object_name_normalize (object1);

  if (! object2->p->normalized)
    ligma_object_name_normalize (object2);

  return strcmp (object1->p->normalized, object2->p->normalized);
}

static void
ligma_object_name_normalize (LigmaObject *object)
{
  g_return_if_fail (object->p->normalized == NULL);

  if (object->p->name)
    {
      gchar *key = g_utf8_collate_key (object->p->name, -1);

      if (strcmp (key, object->p->name))
        {
          object->p->normalized = key;
        }
      else
        {
          g_free (key);
          object->p->normalized = object->p->name;
        }
    }
}


#define DEBUG_MEMSIZE 1

#ifdef DEBUG_MEMSIZE
gboolean ligma_debug_memsize = FALSE;
#endif

gint64
ligma_object_get_memsize (LigmaObject *object,
                         gint64     *gui_size)
{
  gint64 my_size     = 0;
  gint64 my_gui_size = 0;

  g_return_val_if_fail (object == NULL || LIGMA_IS_OBJECT (object), 0);

  if (! object)
    {
      if (gui_size)
        *gui_size = 0;

      return 0;
    }

#ifdef DEBUG_MEMSIZE
  if (ligma_debug_memsize)
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

      memsize = LIGMA_OBJECT_GET_CLASS (object)->get_memsize (object,
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
                                     object->p->name ? object->p->name : "anonymous",
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

  my_size = LIGMA_OBJECT_GET_CLASS (object)->get_memsize (object,
                                                         &my_gui_size);

  if (gui_size)
    *gui_size = my_gui_size;

  return my_size;
}

static gint64
ligma_object_real_get_memsize (LigmaObject *object,
                              gint64     *gui_size)
{
  gint64 memsize = 0;

  if (! object->p->static_name)
    memsize += ligma_string_get_memsize (object->p->name);

  return memsize + ligma_g_object_get_memsize ((GObject *) object);
}
