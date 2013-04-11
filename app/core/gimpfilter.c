/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfilter.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

#include "core-types.h"

#include "gimp.h"
#include "gimpfilter.h"


typedef struct _GimpFilterPrivate GimpFilterPrivate;

struct _GimpFilterPrivate
{
  GeglNode *node;
};

#define GET_PRIVATE(filter) G_TYPE_INSTANCE_GET_PRIVATE (filter, \
                                                         GIMP_TYPE_FILTER, \
                                                         GimpFilterPrivate)


/*  local function prototypes  */

static void       gimp_filter_finalize      (GObject    *object);

static gint64     gimp_filter_get_memsize   (GimpObject *object,
                                             gint64     *gui_size);

static GeglNode * gimp_filter_real_get_node (GimpFilter *filter);


G_DEFINE_TYPE (GimpFilter, gimp_filter, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_filter_parent_class


static void
gimp_filter_class_init (GimpFilterClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->finalize         = gimp_filter_finalize;

  gimp_object_class->get_memsize = gimp_filter_get_memsize;

  klass->get_node                = gimp_filter_real_get_node;

  g_type_class_add_private (klass, sizeof (GimpFilterPrivate));
}

static void
gimp_filter_init (GimpFilter *filter)
{
}

static void
gimp_filter_finalize (GObject *object)
{
  GimpFilterPrivate *private = GET_PRIVATE (object);

  if (private->node)
    {
      g_object_unref (private->node);
      private->node = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_filter_get_memsize (GimpObject *object,
                         gint64     *gui_size)
{
  GimpFilterPrivate *private = GET_PRIVATE (object);
  gint64             memsize = 0;

  memsize += gimp_g_object_get_memsize (G_OBJECT (private->node),
                                        gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static GeglNode *
gimp_filter_real_get_node (GimpFilter *filter)
{
  GimpFilterPrivate *private = GET_PRIVATE (filter);

  private->node = gegl_node_new ();

  return private->node;
}

GeglNode *
gimp_filter_get_node (GimpFilter *filter)
{
  GimpFilterPrivate *private;

  g_return_val_if_fail (GIMP_IS_FILTER (filter), NULL);

  private = GET_PRIVATE (filter);

  if (private->node)
    return private->node;

  return GIMP_FILTER_GET_CLASS (filter)->get_node (filter);
}

GeglNode *
gimp_filter_peek_node (GimpFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_FILTER (filter), NULL);

  return GET_PRIVATE (filter)->node;
}
