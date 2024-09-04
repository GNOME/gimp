/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpdisplay.c
 * Copyright (C) Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

#include "libgimpbase/gimpwire.h" /* FIXME kill this include */

#include "gimpplugin-private.h"


enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};


struct _GimpDisplay
{
  GObject parent_instance;
  gint    id;
};


static void   gimp_display_set_property  (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void   gimp_display_get_property  (GObject      *object,
                                          guint         property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);


G_DEFINE_TYPE (GimpDisplay, gimp_display, G_TYPE_OBJECT)

#define parent_class gimp_display_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
gimp_display_class_init (GimpDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_display_set_property;
  object_class->get_property = gimp_display_get_property;

  props[PROP_ID] =
    g_param_spec_int ("id",
                      "The display id",
                      "The display id for internal use",
                      0, G_MAXINT32, 0,
                      GIMP_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
gimp_display_init (GimpDisplay *display)
{
}

static void
gimp_display_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpDisplay *display = GIMP_DISPLAY (object);

  switch (property_id)
    {
    case PROP_ID:
      display->id = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_display_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpDisplay *display = GIMP_DISPLAY (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, display->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public API */

/**
 * gimp_display_get_id:
 * @display: The display.
 *
 * Note: in most use cases, you should not need a display's ID which is
 * mostly internal data and not reusable across sessions.
 *
 * Returns: the display ID.
 *
 * Since: 3.0
 **/
gint32
gimp_display_get_id (GimpDisplay *display)
{
  return display ? display->id : -1;
}

/**
 * gimp_display_get_by_id:
 * @display_id: The display id.
 *
 * Returns a #GimpDisplay representing @display_id.
 *
 * Note: in most use cases, you should not need to retrieve a
 * #GimpDisplay by its ID, which is mostly internal data and not
 * reusable across sessions. Use the appropriate functions for your use
 * case instead.
 *
 * Returns: (nullable) (transfer none): a #GimpDisplay for @display_id or
 *          %NULL if @display_id does not represent a valid display.
 *          The object belongs to libgimp and you must not modify or
 *          unref it.
 *
 * Since: 3.0
 **/
GimpDisplay *
gimp_display_get_by_id (gint32 display_id)
{
  if (display_id > 0)
    {
      GimpPlugIn *plug_in = gimp_get_plug_in ();

      return _gimp_plug_in_get_display (plug_in, display_id);
    }

  return NULL;
}

/**
 * gimp_display_is_valid:
 * @display: The display to check.
 *
 * Returns TRUE if the display is valid.
 *
 * This procedure checks if the given display is valid and refers to
 * an existing display.
 *
 * Returns: Whether the display is valid.
 *
 * Since: 2.4
 **/
gboolean
gimp_display_is_valid (GimpDisplay *display)
{
  return gimp_display_id_is_valid (gimp_display_get_id (display));
}
