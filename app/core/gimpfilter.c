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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-memsize.h"
#include "gimpfilter.h"


enum
{
  ACTIVE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ACTIVE,
  PROP_IS_LAST_NODE
};


typedef struct _GimpFilterPrivate GimpFilterPrivate;

struct _GimpFilterPrivate
{
  GeglNode       *node;

  guint           active       : 1;
  guint           is_last_node : 1;

  GimpApplicator *applicator;
};

#define GET_PRIVATE(filter) \
  ((GimpFilterPrivate *) gimp_filter_get_instance_private ((GimpFilter *) (filter)))


/*  local function prototypes  */

static void       gimp_filter_finalize      (GObject      *object);
static void       gimp_filter_set_property  (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void       gimp_filter_get_property  (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);

static gint64     gimp_filter_get_memsize   (GimpObject   *object,
                                             gint64       *gui_size);

static GeglNode * gimp_filter_real_get_node (GimpFilter   *filter);


G_DEFINE_TYPE_WITH_PRIVATE (GimpFilter, gimp_filter, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_filter_parent_class

static guint gimp_filter_signals[LAST_SIGNAL] = { 0 };


static void
gimp_filter_class_init (GimpFilterClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  gimp_filter_signals[ACTIVE_CHANGED] =
    g_signal_new ("active-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpFilterClass, active_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize         = gimp_filter_finalize;
  object_class->set_property     = gimp_filter_set_property;
  object_class->get_property     = gimp_filter_get_property;

  gimp_object_class->get_memsize = gimp_filter_get_memsize;

  klass->active_changed          = NULL;
  klass->get_node                = gimp_filter_real_get_node;

  g_object_class_install_property (object_class, PROP_ACTIVE,
                                   g_param_spec_boolean ("active", NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_IS_LAST_NODE,
                                   g_param_spec_boolean ("is-last-node",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));
}

static void
gimp_filter_init (GimpFilter *filter)
{
  GimpFilterPrivate *private = GET_PRIVATE (filter);

  private->active = TRUE;
}

static void
gimp_filter_finalize (GObject *object)
{
  GimpFilterPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->node);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_filter_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GimpFilter *filter = GIMP_FILTER (object);

  switch (property_id)
    {
    case PROP_ACTIVE:
      gimp_filter_set_active (filter, g_value_get_boolean (value));
      break;
    case PROP_IS_LAST_NODE:
      gimp_filter_set_is_last_node (filter, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_filter_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GimpFilterPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, private->active);
      break;
    case PROP_IS_LAST_NODE:
      g_value_set_boolean (value, private->is_last_node);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_filter_get_memsize (GimpObject *object,
                         gint64     *gui_size)
{
  GimpFilterPrivate *private = GET_PRIVATE (object);
  gint64             memsize = 0;

  memsize += gimp_g_object_get_memsize (G_OBJECT (private->node));

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


/*  public functions  */

GimpFilter *
gimp_filter_new (const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (GIMP_TYPE_FILTER,
                       "name", name,
                       NULL);
}

GeglNode *
gimp_filter_get_node (GimpFilter *filter)
{
  GimpFilterPrivate *private = GET_PRIVATE (filter);

  g_return_val_if_fail (GIMP_IS_FILTER (filter), NULL);

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

void
gimp_filter_set_active (GimpFilter *filter,
                        gboolean    active)
{
  g_return_if_fail (GIMP_IS_FILTER (filter));

  active = active ? TRUE : FALSE;

  if (active != gimp_filter_get_active (filter))
    {
      GET_PRIVATE (filter)->active = active;

      g_signal_emit (filter, gimp_filter_signals[ACTIVE_CHANGED], 0);

      g_object_notify (G_OBJECT (filter), "active");
    }
}

gboolean
gimp_filter_get_active (GimpFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_FILTER (filter), FALSE);

  return GET_PRIVATE (filter)->active;
}

void
gimp_filter_set_is_last_node (GimpFilter *filter,
                              gboolean    is_last_node)
{
  g_return_if_fail (GIMP_IS_FILTER (filter));

  is_last_node = is_last_node ? TRUE : FALSE;

  if (is_last_node != gimp_filter_get_is_last_node (filter))
    {
      GET_PRIVATE (filter)->is_last_node = is_last_node;

      g_object_notify (G_OBJECT (filter), "is-last-node");
    }
}

gboolean
gimp_filter_get_is_last_node (GimpFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_FILTER (filter), FALSE);

  return GET_PRIVATE (filter)->is_last_node;
}

void
gimp_filter_set_applicator (GimpFilter     *filter,
                            GimpApplicator *applicator)
{
  GimpFilterPrivate *private = GET_PRIVATE (filter);

  g_return_if_fail (GIMP_IS_FILTER (filter));

  private->applicator = applicator;
}

GimpApplicator *
gimp_filter_get_applicator (GimpFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_FILTER (filter), NULL);

  return GET_PRIVATE (filter)->applicator;
}
