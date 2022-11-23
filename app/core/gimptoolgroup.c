/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolgroup.c
 * Copyright (C) 2020 Ell
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligmalist.h"
#include "ligmatoolgroup.h"
#include "ligmatoolinfo.h"

#include "ligma-intl.h"


enum
{
  ACTIVE_TOOL_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ACTIVE_TOOL,
  PROP_CHILDREN
};


struct _LigmaToolGroupPrivate
{
  gchar         *active_tool;
  LigmaContainer *children;
};


/*  local function prototypes  */


static void            ligma_tool_group_finalize        (GObject        *object);
static void            ligma_tool_group_get_property    (GObject        *object,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);
static void            ligma_tool_group_set_property    (GObject        *object,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);

static gint64          ligma_tool_group_get_memsize     (LigmaObject     *object,
                                                        gint64         *gui_size);

static gchar         * ligma_tool_group_get_description (LigmaViewable   *viewable,
                                                        gchar         **tooltip);
static LigmaContainer * ligma_tool_group_get_children    (LigmaViewable   *viewable);
static void            ligma_tool_group_set_expanded    (LigmaViewable   *viewable,
                                                        gboolean        expand);
static gboolean        ligma_tool_group_get_expanded    (LigmaViewable   *viewable);

static void            ligma_tool_group_child_add       (LigmaContainer  *container,
                                                        LigmaToolInfo   *tool_info,
                                                        LigmaToolGroup  *tool_group);
static void            ligma_tool_group_child_remove    (LigmaContainer  *container,
                                                        LigmaToolInfo   *tool_info,
                                                        LigmaToolGroup  *tool_group);

static void            ligma_tool_group_shown_changed   (LigmaToolItem   *tool_item);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaToolGroup, ligma_tool_group, LIGMA_TYPE_TOOL_ITEM)

#define parent_class ligma_tool_group_parent_class

static guint ligma_tool_group_signals[LAST_SIGNAL] = { 0 };


/*  private functions  */

static void
ligma_tool_group_class_init (LigmaToolGroupClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaToolItemClass *tool_item_class   = LIGMA_TOOL_ITEM_CLASS (klass);

  ligma_tool_group_signals[ACTIVE_TOOL_CHANGED] =
    g_signal_new ("active-tool-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolGroupClass, active_tool_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize            = ligma_tool_group_finalize;
  object_class->get_property        = ligma_tool_group_get_property;
  object_class->set_property        = ligma_tool_group_set_property;

  ligma_object_class->get_memsize    = ligma_tool_group_get_memsize;

  viewable_class->default_icon_name = "folder";
  viewable_class->get_description   = ligma_tool_group_get_description;
  viewable_class->get_children      = ligma_tool_group_get_children;
  viewable_class->get_expanded      = ligma_tool_group_get_expanded;
  viewable_class->set_expanded      = ligma_tool_group_set_expanded;

  tool_item_class->shown_changed    = ligma_tool_group_shown_changed;

  LIGMA_CONFIG_PROP_STRING (object_class, PROP_ACTIVE_TOOL,
                           "active-tool", NULL, NULL,
                           NULL,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_CHILDREN,
                           "children", NULL, NULL,
                           LIGMA_TYPE_CONTAINER,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_CONFIG_PARAM_AGGREGATE);
}

static void
ligma_tool_group_init (LigmaToolGroup *tool_group)
{
  tool_group->priv = ligma_tool_group_get_instance_private (tool_group);

  tool_group->priv->children = g_object_new (
    LIGMA_TYPE_LIST,
    "children-type", LIGMA_TYPE_TOOL_INFO,
    "append",        TRUE,
    NULL);

  g_signal_connect (tool_group->priv->children, "add",
                    G_CALLBACK (ligma_tool_group_child_add),
                    tool_group);
  g_signal_connect (tool_group->priv->children, "remove",
                    G_CALLBACK (ligma_tool_group_child_remove),
                    tool_group);
}

static void
ligma_tool_group_finalize (GObject *object)
{
  LigmaToolGroup *tool_group = LIGMA_TOOL_GROUP (object);

  g_clear_pointer (&tool_group->priv->active_tool, g_free);

  g_clear_object (&tool_group->priv->children);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_tool_group_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaToolGroup *tool_group = LIGMA_TOOL_GROUP (object);

  switch (property_id)
    {
    case PROP_ACTIVE_TOOL:
      g_value_set_string (value, tool_group->priv->active_tool);
      break;

    case PROP_CHILDREN:
      g_value_set_object (value, tool_group->priv->children);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_tool_group_set_property_add_tool (LigmaToolInfo  *tool_info,
                                       LigmaToolGroup *tool_group)
{
  ligma_container_add (tool_group->priv->children, LIGMA_OBJECT (tool_info));
}

static void
ligma_tool_group_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaToolGroup *tool_group = LIGMA_TOOL_GROUP (object);

  switch (property_id)
    {
    case PROP_ACTIVE_TOOL:
      g_free (tool_group->priv->active_tool);

      tool_group->priv->active_tool = g_value_dup_string (value);
      break;

    case PROP_CHILDREN:
      {
        LigmaContainer *container = g_value_get_object (value);

        ligma_container_clear (tool_group->priv->children);

        if (! container)
          break;

        ligma_container_foreach (container,
                                (GFunc) ligma_tool_group_set_property_add_tool,
                                tool_group);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
ligma_tool_group_get_memsize (LigmaObject *object,
                             gint64     *gui_size)
{
  LigmaToolGroup *tool_group = LIGMA_TOOL_GROUP (object);
  gint64         memsize = 0;

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (tool_group->priv->children),
                                      gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gchar *
ligma_tool_group_get_description (LigmaViewable  *viewable,
                                 gchar        **tooltip)
{
  /* Translators: this is a noun */
  return g_strdup (C_("tool-item", "Group"));
}

static LigmaContainer *
ligma_tool_group_get_children (LigmaViewable *viewable)
{
  LigmaToolGroup *tool_group = LIGMA_TOOL_GROUP (viewable);

  return tool_group->priv->children;
}

static void
ligma_tool_group_set_expanded (LigmaViewable *viewable,
                              gboolean      expand)
{
  if (! expand)
    ligma_viewable_expanded_changed (viewable);
}

static gboolean
ligma_tool_group_get_expanded (LigmaViewable *viewable)
{
  return TRUE;
}

static void
ligma_tool_group_child_add (LigmaContainer *container,
                           LigmaToolInfo  *tool_info,
                           LigmaToolGroup *tool_group)
{
  g_return_if_fail (
    ligma_viewable_get_parent (LIGMA_VIEWABLE (tool_info)) == NULL);

  ligma_viewable_set_parent (LIGMA_VIEWABLE (tool_info),
                            LIGMA_VIEWABLE (tool_group));

  if (! tool_group->priv->active_tool)
    ligma_tool_group_set_active_tool_info (tool_group, tool_info);
}

static void
ligma_tool_group_child_remove (LigmaContainer *container,
                              LigmaToolInfo  *tool_info,
                              LigmaToolGroup *tool_group)
{
  ligma_viewable_set_parent (LIGMA_VIEWABLE (tool_info), NULL);

  if (! g_strcmp0 (tool_group->priv->active_tool,
                   ligma_object_get_name (LIGMA_OBJECT (tool_info))))
    {
      LigmaToolInfo *active_tool_info = NULL;

      if (! ligma_container_is_empty (tool_group->priv->children))
        {
          active_tool_info = LIGMA_TOOL_INFO (
            ligma_container_get_first_child (tool_group->priv->children));
        }

      ligma_tool_group_set_active_tool_info (tool_group, active_tool_info);
    }
}

static void
ligma_tool_group_shown_changed (LigmaToolItem *tool_item)
{
  LigmaToolGroup *tool_group = LIGMA_TOOL_GROUP (tool_item);
  GList         *iter;

  if (LIGMA_TOOL_ITEM_CLASS (parent_class)->shown_changed)
    LIGMA_TOOL_ITEM_CLASS (parent_class)->shown_changed (tool_item);

  for (iter = LIGMA_LIST (tool_group->priv->children)->queue->head;
       iter;
       iter = g_list_next (iter))
    {
      LigmaToolItem *tool_item = iter->data;

      if (ligma_tool_item_get_visible (tool_item))
        ligma_tool_item_shown_changed (tool_item);
    }
}


/*  public functions  */

LigmaToolGroup *
ligma_tool_group_new (void)
{
  LigmaToolGroup *tool_group;

  tool_group = g_object_new (LIGMA_TYPE_TOOL_GROUP, NULL);

  ligma_object_set_static_name (LIGMA_OBJECT (tool_group), "tool group");

  return tool_group;
}

void
ligma_tool_group_set_active_tool (LigmaToolGroup *tool_group,
                                 const gchar   *tool_name)
{
  g_return_if_fail (LIGMA_IS_TOOL_GROUP (tool_group));

  if (g_strcmp0 (tool_group->priv->active_tool, tool_name))
    {
      g_return_if_fail (tool_name == NULL ||
                        ligma_container_get_child_by_name (
                          tool_group->priv->children, tool_name) != NULL);

      g_free (tool_group->priv->active_tool);

      tool_group->priv->active_tool = g_strdup (tool_name);;

      g_signal_emit (tool_group,
                     ligma_tool_group_signals[ACTIVE_TOOL_CHANGED], 0);

      g_object_notify (G_OBJECT (tool_group), "active-tool");
    }
}

const gchar *
ligma_tool_group_get_active_tool (LigmaToolGroup *tool_group)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_GROUP (tool_group), NULL);

  return tool_group->priv->active_tool;
}

void
ligma_tool_group_set_active_tool_info (LigmaToolGroup *tool_group,
                                      LigmaToolInfo  *tool_info)
{
  g_return_if_fail (LIGMA_IS_TOOL_GROUP (tool_group));
  g_return_if_fail (tool_info == NULL || LIGMA_IS_TOOL_INFO (tool_info));

  ligma_tool_group_set_active_tool (
    tool_group,
    tool_info ? ligma_object_get_name (LIGMA_OBJECT (tool_info)) : NULL);
}

LigmaToolInfo *
ligma_tool_group_get_active_tool_info (LigmaToolGroup *tool_group)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_GROUP (tool_group), NULL);

  return LIGMA_TOOL_INFO (
    ligma_container_get_child_by_name (tool_group->priv->children,
                                      tool_group->priv->active_tool));
}
