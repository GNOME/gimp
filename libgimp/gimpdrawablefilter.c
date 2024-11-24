/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpdrawablefilter.c
 * Copyright (C) 2024 Jehan
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

struct _GimpDrawableFilter
{
  GObject parent_instance;
  gint    id;
};


static void   gimp_drawable_filter_set_property  (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void   gimp_drawable_filter_get_property  (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);


G_DEFINE_TYPE (GimpDrawableFilter, gimp_drawable_filter, G_TYPE_OBJECT)

#define parent_class gimp_drawable_filter_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
gimp_drawable_filter_class_init (GimpDrawableFilterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_drawable_filter_set_property;
  object_class->get_property = gimp_drawable_filter_get_property;

  props[PROP_ID] =
    g_param_spec_int ("id",
                      "The drawable_filter id",
                      "The drawable_filter id for internal use",
                      0, G_MAXINT32, 0,
                      GIMP_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
gimp_drawable_filter_init (GimpDrawableFilter *drawable_filter)
{
}

static void
gimp_drawable_filter_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpDrawableFilter *drawable_filter = GIMP_DRAWABLE_FILTER (object);

  switch (property_id)
    {
    case PROP_ID:
      drawable_filter->id = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_drawable_filter_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpDrawableFilter *drawable_filter = GIMP_DRAWABLE_FILTER (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, drawable_filter->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public API */

/**
 * gimp_drawable_filter_get_id:
 * @filter: The [class@Gimp.Drawable]'s filter.
 *
 * Returns: the drawable's filter ID.
 *
 * Since: 3.0
 **/
gint32
gimp_drawable_filter_get_id (GimpDrawableFilter *filter)
{
  return filter ? filter->id : -1;
}

/**
 * gimp_drawable_filter_get_by_id:
 * @filter_id: The %GimpDrawableFilter id.
 *
 * Returns: (nullable) (transfer none): a #GimpDrawableFilter for @filter_id or
 *          %NULL if @filter_id does not represent a valid drawable's filter.
 *          The object belongs to libgimp and you must not modify
 *          or unref it.
 *
 * Since: 3.0
 **/
GimpDrawableFilter *
gimp_drawable_filter_get_by_id (gint32 filter_id)
{
  if (filter_id > 0)
    {
      GimpPlugIn *plug_in = gimp_get_plug_in ();

      return _gimp_plug_in_get_filter (plug_in, filter_id);
    }

  return NULL;
}

/**
 * gimp_drawable_filter_is_valid:
 * @filter: The drawable's filter to check.
 *
 * Returns TRUE if the @drawable_filter is valid.
 *
 * This procedure checks if the given filter is valid and refers to an
 * existing %GimpDrawableFilter.
 *
 * Returns: Whether @drawable_filter is valid.
 *
 * Since: 3.0
 **/
gboolean
gimp_drawable_filter_is_valid (GimpDrawableFilter *filter)
{
  return gimp_drawable_filter_id_is_valid (gimp_drawable_filter_get_id (filter));
}
