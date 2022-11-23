/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafilter.c
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma.h"
#include "ligma-memsize.h"
#include "ligmafilter.h"


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


typedef struct _LigmaFilterPrivate LigmaFilterPrivate;

struct _LigmaFilterPrivate
{
  GeglNode       *node;

  guint           active       : 1;
  guint           is_last_node : 1;

  LigmaApplicator *applicator;
};

#define GET_PRIVATE(filter) ((LigmaFilterPrivate *) ligma_filter_get_instance_private ((LigmaFilter *) (filter)))


/*  local function prototypes  */

static void       ligma_filter_finalize      (GObject      *object);
static void       ligma_filter_set_property  (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void       ligma_filter_get_property  (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);

static gint64     ligma_filter_get_memsize   (LigmaObject   *object,
                                             gint64       *gui_size);

static GeglNode * ligma_filter_real_get_node (LigmaFilter   *filter);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaFilter, ligma_filter, LIGMA_TYPE_VIEWABLE)

#define parent_class ligma_filter_parent_class

static guint ligma_filter_signals[LAST_SIGNAL] = { 0 };


static void
ligma_filter_class_init (LigmaFilterClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);

  ligma_filter_signals[ACTIVE_CHANGED] =
    g_signal_new ("active-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaFilterClass, active_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize         = ligma_filter_finalize;
  object_class->set_property     = ligma_filter_set_property;
  object_class->get_property     = ligma_filter_get_property;

  ligma_object_class->get_memsize = ligma_filter_get_memsize;

  klass->active_changed          = NULL;
  klass->get_node                = ligma_filter_real_get_node;

  g_object_class_install_property (object_class, PROP_ACTIVE,
                                   g_param_spec_boolean ("active", NULL, NULL,
                                                         TRUE,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_IS_LAST_NODE,
                                   g_param_spec_boolean ("is-last-node",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
}

static void
ligma_filter_init (LigmaFilter *filter)
{
  LigmaFilterPrivate *private = GET_PRIVATE (filter);

  private->active = TRUE;
}

static void
ligma_filter_finalize (GObject *object)
{
  LigmaFilterPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->node);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_filter_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  LigmaFilter *filter = LIGMA_FILTER (object);

  switch (property_id)
    {
    case PROP_ACTIVE:
      ligma_filter_set_active (filter, g_value_get_boolean (value));
      break;
    case PROP_IS_LAST_NODE:
      ligma_filter_set_is_last_node (filter, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_filter_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  LigmaFilterPrivate *private = GET_PRIVATE (object);

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
ligma_filter_get_memsize (LigmaObject *object,
                         gint64     *gui_size)
{
  LigmaFilterPrivate *private = GET_PRIVATE (object);
  gint64             memsize = 0;

  memsize += ligma_g_object_get_memsize (G_OBJECT (private->node));

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static GeglNode *
ligma_filter_real_get_node (LigmaFilter *filter)
{
  LigmaFilterPrivate *private = GET_PRIVATE (filter);

  private->node = gegl_node_new ();

  return private->node;
}


/*  public functions  */

LigmaFilter *
ligma_filter_new (const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (LIGMA_TYPE_FILTER,
                       "name", name,
                       NULL);
}

GeglNode *
ligma_filter_get_node (LigmaFilter *filter)
{
  LigmaFilterPrivate *private;

  g_return_val_if_fail (LIGMA_IS_FILTER (filter), NULL);

  private = GET_PRIVATE (filter);

  if (private->node)
    return private->node;

  return LIGMA_FILTER_GET_CLASS (filter)->get_node (filter);
}

GeglNode *
ligma_filter_peek_node (LigmaFilter *filter)
{
  g_return_val_if_fail (LIGMA_IS_FILTER (filter), NULL);

  return GET_PRIVATE (filter)->node;
}

void
ligma_filter_set_active (LigmaFilter *filter,
                        gboolean    active)
{
  g_return_if_fail (LIGMA_IS_FILTER (filter));

  active = active ? TRUE : FALSE;

  if (active != ligma_filter_get_active (filter))
    {
      GET_PRIVATE (filter)->active = active;

      g_signal_emit (filter, ligma_filter_signals[ACTIVE_CHANGED], 0);

      g_object_notify (G_OBJECT (filter), "active");
    }
}

gboolean
ligma_filter_get_active (LigmaFilter *filter)
{
  g_return_val_if_fail (LIGMA_IS_FILTER (filter), FALSE);

  return GET_PRIVATE (filter)->active;
}

void
ligma_filter_set_is_last_node (LigmaFilter *filter,
                              gboolean    is_last_node)
{
  g_return_if_fail (LIGMA_IS_FILTER (filter));

  is_last_node = is_last_node ? TRUE : FALSE;

  if (is_last_node != ligma_filter_get_is_last_node (filter))
    {
      GET_PRIVATE (filter)->is_last_node = is_last_node;

      g_object_notify (G_OBJECT (filter), "is-last-node");
    }
}

gboolean
ligma_filter_get_is_last_node (LigmaFilter *filter)
{
  g_return_val_if_fail (LIGMA_IS_FILTER (filter), FALSE);

  return GET_PRIVATE (filter)->is_last_node;
}

void
ligma_filter_set_applicator (LigmaFilter     *filter,
                            LigmaApplicator *applicator)
{
  LigmaFilterPrivate *private;

  g_return_if_fail (LIGMA_IS_FILTER (filter));

  private = GET_PRIVATE (filter);

  private->applicator = applicator;
}

LigmaApplicator *
ligma_filter_get_applicator (LigmaFilter *filter)
{
  g_return_val_if_fail (LIGMA_IS_FILTER (filter), NULL);

  return GET_PRIVATE (filter)->applicator;
}
