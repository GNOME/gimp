/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpcontainer.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include <gio/gio.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-memsize.h"
#include "gimpcontainer.h"
#include "gimpmarshal.h"


/* #define DEBUG_CONTAINER */

#ifdef DEBUG_CONTAINER
#define D(stmnt) stmnt
#else
#define D(stmnt)
#endif


enum
{
  ADD,
  REMOVE,
  REORDER,
  FREEZE,
  THAW,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CHILD_TYPE,
  PROP_POLICY
};


typedef struct
{
  gchar     *signame;
  GCallback  callback;
  gpointer   callback_data;

  GQuark     quark;  /*  used to attach the signal id's of child signals  */
} GimpContainerHandler;

struct _GimpContainerPrivate
{
  GType                child_type;
  GimpContainerPolicy  policy;
  gint                 n_children;

  GList               *handlers;
  gint                 freeze_count;
  gint                 n_children_before_freeze;
  gint                 suspend_items_changed;
};


/*  local function prototypes  */

static void   gimp_container_list_model_iface_init (GListModelInterface *iface);
static void   gimp_container_config_iface_init     (GimpConfigInterface *iface);

static void       gimp_container_dispose         (GObject          *object);

static void       gimp_container_set_property    (GObject          *object,
                                                  guint             property_id,
                                                  const GValue     *value,
                                                  GParamSpec       *pspec);
static void       gimp_container_get_property    (GObject          *object,
                                                  guint             property_id,
                                                  GValue           *value,
                                                  GParamSpec       *pspec);

static gint64     gimp_container_get_memsize     (GimpObject       *object,
                                                  gint64           *gui_size);

static void       gimp_container_real_add        (GimpContainer    *container,
                                                  GimpObject       *object);
static void       gimp_container_real_remove     (GimpContainer    *container,
                                                  GimpObject       *object);
static void       gimp_container_real_reorder    (GimpContainer    *container,
                                                  GimpObject       *object,
                                                  gint              old_index,
                                                  gint              new_index);

static GType      gimp_container_get_item_type   (GListModel       *list);
static guint      gimp_container_get_n_items     (GListModel       *list);
static gpointer   gimp_container_get_item        (GListModel       *list,
                                                  guint             index);

static gboolean   gimp_container_serialize       (GimpConfig       *config,
                                                  GimpConfigWriter *writer,
                                                  gpointer          data);
static gboolean   gimp_container_deserialize     (GimpConfig       *config,
                                                  GScanner         *scanner,
                                                  gint              nest_level,
                                                  gpointer          data);

static void   gimp_container_disconnect_callback (GimpObject       *object,
                                                  gpointer          data);

static void       gimp_container_free_handler    (GimpContainer    *container,
                                                  GimpContainerHandler *handler);


G_DEFINE_TYPE_WITH_CODE (GimpContainer, gimp_container, GIMP_TYPE_OBJECT,
                         G_ADD_PRIVATE (GimpContainer)
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
                                                gimp_container_list_model_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_container_config_iface_init))

#define parent_class gimp_container_parent_class

static guint container_signals[LAST_SIGNAL] = { 0, };


static void
gimp_container_class_init (GimpContainerClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  container_signals[ADD] =
    g_signal_new ("add",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContainerClass, add),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_OBJECT);

  container_signals[REMOVE] =
    g_signal_new ("remove",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContainerClass, remove),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_OBJECT);

  container_signals[REORDER] =
    g_signal_new ("reorder",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpContainerClass, reorder),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT_INT_INT,
                  G_TYPE_NONE, 3,
                  GIMP_TYPE_OBJECT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  container_signals[FREEZE] =
    g_signal_new ("freeze",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpContainerClass, freeze),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  container_signals[THAW] =
    g_signal_new ("thaw",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpContainerClass, thaw),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->dispose          = gimp_container_dispose;
  object_class->set_property     = gimp_container_set_property;
  object_class->get_property     = gimp_container_get_property;

  gimp_object_class->get_memsize = gimp_container_get_memsize;

  klass->add                     = gimp_container_real_add;
  klass->remove                  = gimp_container_real_remove;
  klass->reorder                 = gimp_container_real_reorder;
  klass->freeze                  = NULL;
  klass->thaw                    = NULL;

  klass->clear                   = NULL;
  klass->have                    = NULL;
  klass->foreach                 = NULL;
  klass->search                  = NULL;
  klass->get_unique_names        = NULL;
  klass->get_child_by_name       = NULL;
  klass->get_children_by_name    = NULL;
  klass->get_child_by_index      = NULL;
  klass->get_child_index         = NULL;

  g_object_class_install_property (object_class, PROP_CHILD_TYPE,
                                   g_param_spec_gtype ("child-type",
                                                       NULL, NULL,
                                                       GIMP_TYPE_OBJECT,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_POLICY,
                                   g_param_spec_enum ("policy",
                                                      NULL, NULL,
                                                      GIMP_TYPE_CONTAINER_POLICY,
                                                      GIMP_CONTAINER_POLICY_STRONG,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_container_list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = gimp_container_get_item_type;
  iface->get_n_items   = gimp_container_get_n_items;
  iface->get_item      = gimp_container_get_item;
}

static void
gimp_container_config_iface_init (GimpConfigInterface *iface)
{
  iface->serialize   = gimp_container_serialize;
  iface->deserialize = gimp_container_deserialize;
}

static void
gimp_container_init (GimpContainer *container)
{
  container->priv = gimp_container_get_instance_private (container);

  container->priv->handlers     = NULL;
  container->priv->freeze_count = 0;

  container->priv->child_type   = G_TYPE_NONE;
  container->priv->policy       = GIMP_CONTAINER_POLICY_STRONG;
  container->priv->n_children   = 0;
}

static void
gimp_container_dispose (GObject *object)
{
  GimpContainer *container = GIMP_CONTAINER (object);

  gimp_container_clear (container);

  while (container->priv->handlers)
    gimp_container_remove_handler (container,
                                   ((GimpContainerHandler *)
                                    container->priv->handlers->data)->quark);

  if (container->priv->child_type != G_TYPE_NONE)
    {
      g_type_class_unref (g_type_class_peek (container->priv->child_type));
      container->priv->child_type = G_TYPE_NONE;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_container_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpContainer *container = GIMP_CONTAINER (object);

  switch (property_id)
    {
    case PROP_CHILD_TYPE:
      container->priv->child_type = g_value_get_gtype (value);
      g_type_class_ref (container->priv->child_type);
      break;
    case PROP_POLICY:
      container->priv->policy = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_container_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpContainer *container = GIMP_CONTAINER (object);

  switch (property_id)
    {
    case PROP_CHILD_TYPE:
      g_value_set_gtype (value, container->priv->child_type);
      break;
    case PROP_POLICY:
      g_value_set_enum (value, container->priv->policy);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_container_get_memsize (GimpObject *object,
                            gint64     *gui_size)
{
  GimpContainer *container = GIMP_CONTAINER (object);
  gint64         memsize   = 0;
  GList         *list;

  for (list = container->priv->handlers; list; list = g_list_next (list))
    {
      GimpContainerHandler *handler = list->data;

      memsize += (sizeof (GList) +
                  sizeof (GimpContainerHandler) +
                  gimp_string_get_memsize (handler->signame));
    }

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_container_real_add (GimpContainer *container,
                         GimpObject    *object)
{
  container->priv->n_children++;
}

static void
gimp_container_real_remove (GimpContainer *container,
                            GimpObject    *object)
{
  container->priv->n_children--;
}

static void
gimp_container_real_reorder (GimpContainer *container,
                             GimpObject    *object,
                             gint           old_index,
                             gint           new_index)
{
}


/*  GListModel functions  */

static GType
gimp_container_get_item_type (GListModel *list)
{
  return gimp_container_get_child_type (GIMP_CONTAINER (list));
}

static guint
gimp_container_get_n_items (GListModel *list)
{
  return gimp_container_get_n_children (GIMP_CONTAINER (list));
}

static void *
gimp_container_get_item (GListModel *list,
                         guint       index)
{
  GimpContainer *self = GIMP_CONTAINER (list);

  if (index >= gimp_container_get_n_children (self))
    return NULL;

  return g_object_ref (gimp_container_get_child_by_index (self, index));
}


/*  GimpConfig functions  */

typedef struct
{
  GimpConfigWriter *writer;
  gpointer          data;
  gboolean          success;
} SerializeData;

static void
gimp_container_serialize_foreach (GObject       *object,
                                  SerializeData *serialize_data)
{
  GimpConfigInterface *config_iface;
  const gchar         *name;

  config_iface = GIMP_CONFIG_GET_IFACE (object);

  if (! config_iface)
    serialize_data->success = FALSE;

  if (! serialize_data->success)
    return;

  gimp_config_writer_open (serialize_data->writer,
                           g_type_name (G_TYPE_FROM_INSTANCE (object)));

  name = gimp_object_get_name (object);

  if (name)
    gimp_config_writer_string (serialize_data->writer, name);
  else
    gimp_config_writer_print (serialize_data->writer, "NULL", 4);

  serialize_data->success = config_iface->serialize (GIMP_CONFIG (object),
                                                     serialize_data->writer,
                                                     serialize_data->data);
  gimp_config_writer_close (serialize_data->writer);
}

static gboolean
gimp_container_serialize (GimpConfig       *config,
                          GimpConfigWriter *writer,
                          gpointer          data)
{
  GimpContainer *container = GIMP_CONTAINER (config);
  SerializeData  serialize_data;

  serialize_data.writer  = writer;
  serialize_data.data    = data;
  serialize_data.success = TRUE;

  gimp_container_foreach (container,
                          (GFunc) gimp_container_serialize_foreach,
                          &serialize_data);

  return serialize_data.success;
}

static gboolean
gimp_container_deserialize (GimpConfig *config,
                            GScanner   *scanner,
                            gint        nest_level,
                            gpointer    data)
{
  GimpContainer *container = GIMP_CONTAINER (config);
  GTokenType     token;

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_IDENTIFIER;
          break;

        case G_TOKEN_IDENTIFIER:
          {
            GimpObject *child     = NULL;
            GType       type;
            gchar      *name      = NULL;
            gboolean    add_child = FALSE;

            type = g_type_from_name (scanner->value.v_identifier);

            if (! type)
              {
                g_scanner_error (scanner,
                                 "unable to determine type of '%s'",
                                 scanner->value.v_identifier);
                return FALSE;
              }

            if (! g_type_is_a (type, container->priv->child_type))
              {
                g_scanner_error (scanner,
                                 "'%s' is not a subclass of '%s'",
                                 scanner->value.v_identifier,
                                 g_type_name (container->priv->child_type));
                return FALSE;
              }

            if (! g_type_is_a (type, GIMP_TYPE_CONFIG))
              {
                g_scanner_error (scanner,
                                 "'%s' does not implement GimpConfigInterface",
                                 scanner->value.v_identifier);
                return FALSE;
              }

            if (! gimp_scanner_parse_string (scanner, &name))
              {
                token = G_TOKEN_STRING;
                break;
              }

            if (! name)
              name = g_strdup ("");

            if (gimp_container_get_unique_names (container))
              child = gimp_container_get_child_by_name (container, name);

            if (! child)
              {
                if (GIMP_IS_GIMP (data))
                  child = g_object_new (type, "gimp", data, NULL);
                else
                  child = g_object_new (type, NULL);

                add_child = TRUE;
              }

            /*  always use the deserialized name. while it normally
             *  doesn't make a difference there are obscure case like
             *  template migration.
             */
            gimp_object_take_name (child, name);

            if (! GIMP_CONFIG_GET_IFACE (child)->deserialize (GIMP_CONFIG (child),
                                                              scanner,
                                                              nest_level + 1,
                                                              NULL))
              {
                if (add_child)
                  g_object_unref (child);

                /*  warning should be already set by child  */
                return FALSE;
              }

            if (add_child)
              {
                gimp_container_add (container, child);

                if (container->priv->policy == GIMP_CONTAINER_POLICY_STRONG)
                  g_object_unref (child);
              }
          }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

  return gimp_config_deserialize_return (scanner, token, nest_level);
}

static void
gimp_container_disconnect_callback (GimpObject *object,
                                    gpointer    data)
{
  GimpContainer *container = GIMP_CONTAINER (data);

  gimp_container_remove (container, object);
}

static void
gimp_container_free_handler_foreach_func (GimpObject           *object,
                                          GimpContainerHandler *handler)
{
  gulong handler_id;

  handler_id = GPOINTER_TO_UINT (g_object_get_qdata (G_OBJECT (object),
                                                     handler->quark));

  if (handler_id)
    {
      g_signal_handler_disconnect (object, handler_id);

      g_object_set_qdata (G_OBJECT (object), handler->quark, NULL);
    }
}

static void
gimp_container_free_handler (GimpContainer        *container,
                             GimpContainerHandler *handler)
{
  D (g_print ("%s: id = %d\n", G_STRFUNC, handler->quark));

  gimp_container_foreach (container,
                          (GFunc) gimp_container_free_handler_foreach_func,
                          handler);

  g_free (handler->signame);
  g_slice_free (GimpContainerHandler, handler);
}

GType
gimp_container_get_child_type (GimpContainer *container)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), G_TYPE_NONE);

  return container->priv->child_type;
}

GimpContainerPolicy
gimp_container_get_policy (GimpContainer *container)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), 0);

  return container->priv->policy;
}

gint
gimp_container_get_n_children (GimpContainer *container)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), 0);

  return container->priv->n_children;
}

gboolean
gimp_container_add (GimpContainer *container,
                    GimpObject    *object)
{
  GList *list;
  gint   n_children;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (object,
                                                    container->priv->child_type),
                        FALSE);

  if (gimp_container_have (container, object))
    {
      g_warning ("%s: container %p already contains object %p",
                 G_STRFUNC, container, object);
      return FALSE;
    }

  for (list = container->priv->handlers; list; list = g_list_next (list))
    {
      GimpContainerHandler *handler = list->data;
      gulong                handler_id;

      handler_id = g_signal_connect (object,
                                     handler->signame,
                                     handler->callback,
                                     handler->callback_data);

      g_object_set_qdata (G_OBJECT (object), handler->quark,
                          GUINT_TO_POINTER (handler_id));
    }

  switch (container->priv->policy)
    {
    case GIMP_CONTAINER_POLICY_STRONG:
      g_object_ref (object);
      break;

    case GIMP_CONTAINER_POLICY_WEAK:
      g_signal_connect (object, "disconnect",
                        G_CALLBACK (gimp_container_disconnect_callback),
                        container);
      break;
    }

  n_children = container->priv->n_children;

  g_signal_emit (container, container_signals[ADD], 0, object);

  if (n_children == container->priv->n_children)
    {
      g_warning ("%s: GimpContainer::add() implementation did not "
                 "chain up. Please report this at https://www.gimp.org/bugs/",
                 G_STRFUNC);

      container->priv->n_children++;
    }

  if (container->priv->freeze_count          == 0 &&
      container->priv->suspend_items_changed == 0)
    {
      gint index = gimp_container_get_child_index (container, object);

      g_list_model_items_changed (G_LIST_MODEL (container), index, 0, 1);
    }

  return TRUE;
}

gboolean
gimp_container_remove (GimpContainer *container,
                       GimpObject    *object)
{
  GList *list;
  gint   n_children;
  gint   index;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (object,
                                                    container->priv->child_type),
                        FALSE);

  if (! gimp_container_have (container, object))
    {
      g_warning ("%s: container %p does not contain object %p",
                 G_STRFUNC, container, object);
      return FALSE;
    }

  for (list = container->priv->handlers; list; list = g_list_next (list))
    {
      GimpContainerHandler *handler = list->data;
      gulong                handler_id;

      handler_id = GPOINTER_TO_UINT (g_object_get_qdata (G_OBJECT (object),
                                                         handler->quark));

      if (handler_id)
        {
          g_signal_handler_disconnect (object, handler_id);

          g_object_set_qdata (G_OBJECT (object), handler->quark, NULL);
        }
    }

  n_children = container->priv->n_children;
  index      = gimp_container_get_child_index (container, object);

  g_signal_emit (container, container_signals[REMOVE], 0, object);

  if (n_children == container->priv->n_children)
    {
      g_warning ("%s: GimpContainer::remove() implementation did not "
                 "chain up. Please report this at https://www.gimp.org/bugs/",
                 G_STRFUNC);

      container->priv->n_children--;
    }

  switch (container->priv->policy)
    {
    case GIMP_CONTAINER_POLICY_STRONG:
      g_object_unref (object);
      break;

    case GIMP_CONTAINER_POLICY_WEAK:
      g_signal_handlers_disconnect_by_func (object,
                                            gimp_container_disconnect_callback,
                                            container);
      break;
    }

  if (container->priv->freeze_count          == 0 &&
      container->priv->suspend_items_changed == 0)
    {
      g_list_model_items_changed (G_LIST_MODEL (container), index, 1, 0);
    }

  return TRUE;
}

gboolean
gimp_container_insert (GimpContainer *container,
                       GimpObject    *object,
                       gint           index)
{
  gboolean success = FALSE;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (object,
                                                    container->priv->child_type),
                        FALSE);

  g_return_val_if_fail (index >= -1 &&
                        index <= container->priv->n_children, FALSE);

  if (gimp_container_have (container, object))
    {
      g_warning ("%s: container %p already contains object %p",
                 G_STRFUNC, container, object);
      return FALSE;
    }

  container->priv->suspend_items_changed++;

  if (gimp_container_add (container, object))
    {
      /*  set success to TRUE even if reorder() fails, because the
       *  object has in fact been added
       */
      success = TRUE;

      gimp_container_reorder (container, object, index);
    }

  container->priv->suspend_items_changed--;

  if (success                                     &&
      container->priv->freeze_count          == 0 &&
      container->priv->suspend_items_changed == 0)
    {
      gint index = gimp_container_get_child_index (container, object);

      g_list_model_items_changed (G_LIST_MODEL (container), index, 0, 1);
    }

  return success;
}

gboolean
gimp_container_reorder (GimpContainer *container,
                        GimpObject    *object,
                        gint           new_index)
{
  gint old_index;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (object,
                                                    container->priv->child_type),
                        FALSE);

  g_return_val_if_fail (new_index >= -1 &&
                        new_index < container->priv->n_children, FALSE);

  if (new_index == -1)
    new_index = container->priv->n_children - 1;

  old_index = gimp_container_get_child_index (container, object);

  if (old_index == -1)
    {
      g_warning ("%s: container %p does not contain object %p",
                 G_STRFUNC, container, object);
      return FALSE;
    }

  if (old_index != new_index)
    {
      g_signal_emit (container, container_signals[REORDER], 0,
                     object, old_index, new_index);

      if (container->priv->freeze_count          == 0 &&
          container->priv->suspend_items_changed == 0)
        {
          gint new_index = gimp_container_get_child_index (container, object);

          g_list_model_items_changed (G_LIST_MODEL (container),
                                      MIN (old_index, new_index),
                                      ABS (old_index - new_index) + 1,
                                      ABS (old_index - new_index) + 1);
        }
    }

  return TRUE;
}

void
gimp_container_freeze (GimpContainer *container)
{
  g_return_if_fail (GIMP_IS_CONTAINER (container));

  container->priv->freeze_count++;

  if (container->priv->freeze_count == 1)
    {
      container->priv->n_children_before_freeze = container->priv->n_children;

      g_signal_emit (container, container_signals[FREEZE], 0);
    }
}

void
gimp_container_thaw (GimpContainer *container)
{
  g_return_if_fail (GIMP_IS_CONTAINER (container));
  g_return_if_fail (container->priv->freeze_count > 0);

  container->priv->freeze_count--;

  if (container->priv->freeze_count == 0)
    {
      g_list_model_items_changed (G_LIST_MODEL (container), 0,
                                  container->priv->n_children_before_freeze,
                                  container->priv->n_children);
      container->priv->n_children_before_freeze = 0;

      g_signal_emit (container, container_signals[THAW], 0);
    }
}

gboolean
gimp_container_frozen (GimpContainer *container)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);

  return (container->priv->freeze_count > 0) ? TRUE : FALSE;
}

gint
gimp_container_freeze_count (GimpContainer *container)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), 0);

  return container->priv->freeze_count;
}

void
gimp_container_clear (GimpContainer *container)
{
  g_return_if_fail (GIMP_IS_CONTAINER (container));

  if (container->priv->n_children > 0)
    {
      gimp_container_freeze (container);
      GIMP_CONTAINER_GET_CLASS (container)->clear (container);
      gimp_container_thaw (container);
    }
}

gboolean
gimp_container_is_empty (GimpContainer *container)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);

  return (container->priv->n_children == 0);
}

gboolean
gimp_container_have (GimpContainer *container,
                     GimpObject    *object)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);

  if (container->priv->n_children < 1)
    return FALSE;

  return GIMP_CONTAINER_GET_CLASS (container)->have (container, object);
}

void
gimp_container_foreach (GimpContainer *container,
                        GFunc          func,
                        gpointer       user_data)
{
  g_return_if_fail (GIMP_IS_CONTAINER (container));
  g_return_if_fail (func != NULL);

  if (container->priv->n_children > 0)
    GIMP_CONTAINER_GET_CLASS (container)->foreach (container, func, user_data);
}

GimpObject *
gimp_container_search (GimpContainer           *container,
                       GimpContainerSearchFunc  func,
                       gpointer                 user_data)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (func != NULL, NULL);

  if (container->priv->n_children > 0)
    {
      return GIMP_CONTAINER_GET_CLASS (container)->search (container,
                                                           func, user_data);
    }

  return NULL;
}

gboolean
gimp_container_get_unique_names (GimpContainer *container)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);

  if (GIMP_CONTAINER_GET_CLASS (container)->get_unique_names)
    return GIMP_CONTAINER_GET_CLASS (container)->get_unique_names (container);

  return FALSE;
}

GList *
gimp_container_get_children_by_name (GimpContainer *container,
                                     const gchar   *name)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);

  if (!name)
    return NULL;

  if (GIMP_CONTAINER_GET_CLASS (container)->get_children_by_name != NULL &&
      ! gimp_container_get_unique_names (container))
    {
      return GIMP_CONTAINER_GET_CLASS (container)->get_children_by_name (container,
                                                                         name);
    }
  else
    {
      GimpObject *child;

      child = GIMP_CONTAINER_GET_CLASS (container)->get_child_by_name (container, name);

      if (child != NULL)
        return g_list_prepend (NULL, child);
      else
        return NULL;
    }
}

GimpObject *
gimp_container_get_child_by_name (GimpContainer *container,
                                  const gchar   *name)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);

  if (!name)
    return NULL;

  return GIMP_CONTAINER_GET_CLASS (container)->get_child_by_name (container,
                                                                  name);
}

GimpObject *
gimp_container_get_child_by_index (GimpContainer *container,
                                   gint           index)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);

  if (index < 0 || index >= container->priv->n_children)
    return NULL;

  return GIMP_CONTAINER_GET_CLASS (container)->get_child_by_index (container,
                                                                   index);
}

/**
 * gimp_container_get_first_child:
 * @container: a #GimpContainer
 *
 * Returns: (nullable) (transfer none): the first child object stored in
 *          @container or %NULL if the container is empty.
 */
GimpObject *
gimp_container_get_first_child (GimpContainer *container)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);

  if (container->priv->n_children > 0)
    return GIMP_CONTAINER_GET_CLASS (container)->get_child_by_index (container,
                                                                     0);

  return NULL;
}

/**
 * gimp_container_get_last_child:
 * @container: a #GimpContainer
 *
 * Returns: (nullable) (transfer none): the last child object stored in
 *          @container or %NULL if the container is empty
 */
GimpObject *
gimp_container_get_last_child (GimpContainer *container)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);

  if (container->priv->n_children > 0)
    return GIMP_CONTAINER_GET_CLASS (container)->get_child_by_index (container,
                                                                     container->priv->n_children - 1);

  return NULL;
}

gint
gimp_container_get_child_index (GimpContainer *container,
                                GimpObject    *object)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), -1);
  g_return_val_if_fail (object != NULL, -1);
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (object,
                                                    container->priv->child_type),
                        -1);

  return GIMP_CONTAINER_GET_CLASS (container)->get_child_index (container,
                                                                object);
}

GimpObject *
gimp_container_get_neighbor_of (GimpContainer *container,
                                GimpObject    *object)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (GIMP_IS_OBJECT (object), NULL);

  index = gimp_container_get_child_index (container, object);

  if (index != -1)
    {
      GimpObject *new;

      new = gimp_container_get_child_by_index (container, index + 1);

      if (! new && index > 0)
        new = gimp_container_get_child_by_index (container, index - 1);

      return new;
    }

  return NULL;
}

static void
gimp_container_get_name_array_foreach_func (GimpObject   *object,
                                            gchar      ***iter)
{
  gchar **array = *iter;

  *array = g_strdup (gimp_object_get_name (object));
  (*iter)++;
}

gchar **
gimp_container_get_name_array (GimpContainer *container)
{
  gchar **names;
  gchar **iter;
  gint    length;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);

  length = gimp_container_get_n_children (container);

  names = iter = g_new0 (gchar *, length + 1);

  gimp_container_foreach (container,
                          (GFunc) gimp_container_get_name_array_foreach_func,
                          &iter);

  return names;
}

static void
gimp_container_add_handler_foreach_func (GimpObject           *object,
                                         GimpContainerHandler *handler)
{
  gulong handler_id;

  handler_id = g_signal_connect (object,
                                 handler->signame,
                                 handler->callback,
                                 handler->callback_data);

  g_object_set_qdata (G_OBJECT (object), handler->quark,
                      GUINT_TO_POINTER (handler_id));
}

GQuark
gimp_container_add_handler (GimpContainer *container,
                            const gchar   *signame,
                            GCallback      callback,
                            gpointer       callback_data)
{
  GimpContainerHandler *handler;
  gchar                *key;

  static gint           handler_id = 0;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), 0);
  g_return_val_if_fail (signame != NULL, 0);
  g_return_val_if_fail (callback != NULL, 0);

  if (! g_str_has_prefix (signame, "notify::"))
    g_return_val_if_fail (g_signal_lookup (signame,
                                           container->priv->child_type), 0);

  handler = g_slice_new0 (GimpContainerHandler);

  /*  create a unique key for this handler  */
  key = g_strdup_printf ("%s-%d", signame, handler_id++);

  handler->signame       = g_strdup (signame);
  handler->callback      = callback;
  handler->callback_data = callback_data;
  handler->quark         = g_quark_from_string (key);

  D (g_print ("%s: key = %s, id = %d\n", G_STRFUNC, key, handler->quark));

  g_free (key);

  container->priv->handlers = g_list_prepend (container->priv->handlers, handler);

  gimp_container_foreach (container,
                          (GFunc) gimp_container_add_handler_foreach_func,
                          handler);

  return handler->quark;
}

void
gimp_container_remove_handler (GimpContainer *container,
                               GQuark         id)
{
  GimpContainerHandler *handler;
  GList                *list;

  g_return_if_fail (GIMP_IS_CONTAINER (container));
  g_return_if_fail (id != 0);

  for (list = container->priv->handlers; list; list = g_list_next (list))
    {
      handler = (GimpContainerHandler *) list->data;

      if (handler->quark == id)
        break;
    }

  if (! list)
    {
      g_warning ("%s: tried to remove handler which unknown id %d",
                 G_STRFUNC, id);
      return;
    }

  gimp_container_free_handler (container, handler);

  container->priv->handlers = g_list_delete_link (container->priv->handlers,
                                                  list);
}

void
gimp_container_remove_handlers_by_func (GimpContainer *container,
                                        GCallback      callback,
                                        gpointer       callback_data)
{
  GList *list;

  g_return_if_fail (GIMP_IS_CONTAINER (container));
  g_return_if_fail (callback != NULL);

  list = container->priv->handlers;

  while (list)
    {
      GimpContainerHandler *handler = list->data;
      GList                *next    = g_list_next (list);

      if (handler->callback      == callback &&
          handler->callback_data == callback_data)
        {
          gimp_container_free_handler (container, handler);

          container->priv->handlers =
            g_list_delete_link (container->priv->handlers, list);
        }

      list = next;
    }
}

void
gimp_container_remove_handlers_by_data (GimpContainer *container,
                                        gpointer       callback_data)
{
  GList *list;

  g_return_if_fail (GIMP_IS_CONTAINER (container));

  list = container->priv->handlers;

  while (list)
    {
      GimpContainerHandler *handler = list->data;
      GList                *next    = g_list_next (list);

      if (handler->callback_data == callback_data)
        {
          gimp_container_free_handler (container, handler);

          container->priv->handlers =
            g_list_delete_link (container->priv->handlers, list);
        }

      list = next;
    }
}
