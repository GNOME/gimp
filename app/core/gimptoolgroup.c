/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolgroup.c
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimplist.h"
#include "gimptoolgroup.h"
#include "gimptoolinfo.h"

#include "gimp-intl.h"


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


struct _GimpToolGroupPrivate
{
  gchar         *active_tool;
  GimpContainer *children;
};


/*  local function prototypes  */


static void            gimp_tool_group_finalize        (GObject        *object);
static void            gimp_tool_group_get_property    (GObject        *object,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);
static void            gimp_tool_group_set_property    (GObject        *object,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);

static gint64          gimp_tool_group_get_memsize     (GimpObject     *object,
                                                        gint64         *gui_size);

static gchar         * gimp_tool_group_get_description (GimpViewable   *viewable,
                                                        gchar         **tooltip);
static GimpContainer * gimp_tool_group_get_children    (GimpViewable   *viewable);
static void            gimp_tool_group_set_expanded    (GimpViewable   *viewable,
                                                        gboolean        expand);
static gboolean        gimp_tool_group_get_expanded    (GimpViewable   *viewable);

static void            gimp_tool_group_child_add       (GimpContainer  *container,
                                                        GimpToolInfo   *tool_info,
                                                        GimpToolGroup  *tool_group);
static void            gimp_tool_group_child_remove    (GimpContainer  *container,
                                                        GimpToolInfo   *tool_info,
                                                        GimpToolGroup  *tool_group);

static void            gimp_tool_group_shown_changed   (GimpToolItem   *tool_item);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolGroup, gimp_tool_group, GIMP_TYPE_TOOL_ITEM)

#define parent_class gimp_tool_group_parent_class

static guint gimp_tool_group_signals[LAST_SIGNAL] = { 0 };


/*  private functions  */

static void
gimp_tool_group_class_init (GimpToolGroupClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpToolItemClass *tool_item_class   = GIMP_TOOL_ITEM_CLASS (klass);

  gimp_tool_group_signals[ACTIVE_TOOL_CHANGED] =
    g_signal_new ("active-tool-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolGroupClass, active_tool_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize            = gimp_tool_group_finalize;
  object_class->get_property        = gimp_tool_group_get_property;
  object_class->set_property        = gimp_tool_group_set_property;

  gimp_object_class->get_memsize    = gimp_tool_group_get_memsize;

  viewable_class->default_icon_name = "folder";
  viewable_class->get_description   = gimp_tool_group_get_description;
  viewable_class->get_children      = gimp_tool_group_get_children;
  viewable_class->get_expanded      = gimp_tool_group_get_expanded;
  viewable_class->set_expanded      = gimp_tool_group_set_expanded;

  tool_item_class->shown_changed    = gimp_tool_group_shown_changed;

  GIMP_CONFIG_PROP_STRING (object_class, PROP_ACTIVE_TOOL,
                           "active-tool", NULL, NULL,
                           NULL,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_CHILDREN,
                           "children", NULL, NULL,
                           GIMP_TYPE_CONTAINER,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_CONFIG_PARAM_AGGREGATE);
}

static void
gimp_tool_group_init (GimpToolGroup *tool_group)
{
  tool_group->priv = gimp_tool_group_get_instance_private (tool_group);

  tool_group->priv->children = g_object_new (GIMP_TYPE_LIST,
                                             "child-type", GIMP_TYPE_TOOL_INFO,
                                             "append",     TRUE,
                                             NULL);

  g_signal_connect (tool_group->priv->children, "add",
                    G_CALLBACK (gimp_tool_group_child_add),
                    tool_group);
  g_signal_connect (tool_group->priv->children, "remove",
                    G_CALLBACK (gimp_tool_group_child_remove),
                    tool_group);
}

static void
gimp_tool_group_finalize (GObject *object)
{
  GimpToolGroup *tool_group = GIMP_TOOL_GROUP (object);

  g_clear_pointer (&tool_group->priv->active_tool, g_free);

  g_clear_object (&tool_group->priv->children);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tool_group_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpToolGroup *tool_group = GIMP_TOOL_GROUP (object);

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
gimp_tool_group_set_property_add_tool (GimpToolInfo  *tool_info,
                                       GimpToolGroup *tool_group)
{
  gimp_container_add (tool_group->priv->children, GIMP_OBJECT (tool_info));
}

static void
gimp_tool_group_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpToolGroup *tool_group = GIMP_TOOL_GROUP (object);

  switch (property_id)
    {
    case PROP_ACTIVE_TOOL:
      g_free (tool_group->priv->active_tool);

      tool_group->priv->active_tool = g_value_dup_string (value);
      break;

    case PROP_CHILDREN:
      {
        GimpContainer *container = g_value_get_object (value);

        gimp_container_clear (tool_group->priv->children);

        if (! container)
          break;

        gimp_container_foreach (container,
                                (GFunc) gimp_tool_group_set_property_add_tool,
                                tool_group);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_tool_group_get_memsize (GimpObject *object,
                             gint64     *gui_size)
{
  GimpToolGroup *tool_group = GIMP_TOOL_GROUP (object);
  gint64         memsize = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (tool_group->priv->children),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gchar *
gimp_tool_group_get_description (GimpViewable  *viewable,
                                 gchar        **tooltip)
{
  /* Translators: this is a noun */
  return g_strdup (C_("tool-item", "Group"));
}

static GimpContainer *
gimp_tool_group_get_children (GimpViewable *viewable)
{
  GimpToolGroup *tool_group = GIMP_TOOL_GROUP (viewable);

  return tool_group->priv->children;
}

static void
gimp_tool_group_set_expanded (GimpViewable *viewable,
                              gboolean      expand)
{
  if (! expand)
    gimp_viewable_expanded_changed (viewable);
}

static gboolean
gimp_tool_group_get_expanded (GimpViewable *viewable)
{
  return TRUE;
}

static void
gimp_tool_group_child_add (GimpContainer *container,
                           GimpToolInfo  *tool_info,
                           GimpToolGroup *tool_group)
{
  g_return_if_fail (
    gimp_viewable_get_parent (GIMP_VIEWABLE (tool_info)) == NULL);

  gimp_viewable_set_parent (GIMP_VIEWABLE (tool_info),
                            GIMP_VIEWABLE (tool_group));

  if (! tool_group->priv->active_tool)
    gimp_tool_group_set_active_tool_info (tool_group, tool_info);
}

static void
gimp_tool_group_child_remove (GimpContainer *container,
                              GimpToolInfo  *tool_info,
                              GimpToolGroup *tool_group)
{
  gimp_viewable_set_parent (GIMP_VIEWABLE (tool_info), NULL);

  if (! g_strcmp0 (tool_group->priv->active_tool,
                   gimp_object_get_name (GIMP_OBJECT (tool_info))))
    {
      GimpToolInfo *active_tool_info = NULL;

      if (! gimp_container_is_empty (tool_group->priv->children))
        {
          active_tool_info = GIMP_TOOL_INFO (
            gimp_container_get_first_child (tool_group->priv->children));
        }

      gimp_tool_group_set_active_tool_info (tool_group, active_tool_info);
    }
}

static void
gimp_tool_group_shown_changed (GimpToolItem *tool_item)
{
  GimpToolGroup *tool_group = GIMP_TOOL_GROUP (tool_item);
  GList         *iter;

  if (GIMP_TOOL_ITEM_CLASS (parent_class)->shown_changed)
    GIMP_TOOL_ITEM_CLASS (parent_class)->shown_changed (tool_item);

  for (iter = GIMP_LIST (tool_group->priv->children)->queue->head;
       iter;
       iter = g_list_next (iter))
    {
      GimpToolItem *tool_item = iter->data;

      if (gimp_tool_item_get_visible (tool_item))
        gimp_tool_item_shown_changed (tool_item);
    }
}


/*  public functions  */

GimpToolGroup *
gimp_tool_group_new (void)
{
  GimpToolGroup *tool_group;

  tool_group = g_object_new (GIMP_TYPE_TOOL_GROUP, NULL);

  gimp_object_set_static_name (GIMP_OBJECT (tool_group), "tool group");

  return tool_group;
}

void
gimp_tool_group_set_active_tool (GimpToolGroup *tool_group,
                                 const gchar   *tool_name)
{
  g_return_if_fail (GIMP_IS_TOOL_GROUP (tool_group));

  if (g_strcmp0 (tool_group->priv->active_tool, tool_name))
    {
      g_return_if_fail (tool_name == NULL ||
                        gimp_container_get_child_by_name (
                          tool_group->priv->children, tool_name) != NULL);

      g_free (tool_group->priv->active_tool);

      tool_group->priv->active_tool = g_strdup (tool_name);;

      g_signal_emit (tool_group,
                     gimp_tool_group_signals[ACTIVE_TOOL_CHANGED], 0);

      g_object_notify (G_OBJECT (tool_group), "active-tool");
    }
}

const gchar *
gimp_tool_group_get_active_tool (GimpToolGroup *tool_group)
{
  g_return_val_if_fail (GIMP_IS_TOOL_GROUP (tool_group), NULL);

  return tool_group->priv->active_tool;
}

void
gimp_tool_group_set_active_tool_info (GimpToolGroup *tool_group,
                                      GimpToolInfo  *tool_info)
{
  g_return_if_fail (GIMP_IS_TOOL_GROUP (tool_group));
  g_return_if_fail (tool_info == NULL || GIMP_IS_TOOL_INFO (tool_info));

  gimp_tool_group_set_active_tool (
    tool_group,
    tool_info ? gimp_object_get_name (GIMP_OBJECT (tool_info)) : NULL);
}

GimpToolInfo *
gimp_tool_group_get_active_tool_info (GimpToolGroup *tool_group)
{
  g_return_val_if_fail (GIMP_IS_TOOL_GROUP (tool_group), NULL);

  return GIMP_TOOL_INFO (
    gimp_container_get_child_by_name (tool_group->priv->children,
                                      tool_group->priv->active_tool));
}
