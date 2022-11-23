/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmacontainer.c
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligma.h"
#include "ligma-memsize.h"
#include "ligmacontainer.h"
#include "ligmamarshal.h"


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
  PROP_CHILDREN_TYPE,
  PROP_POLICY
};


typedef struct
{
  gchar     *signame;
  GCallback  callback;
  gpointer   callback_data;

  GQuark     quark;  /*  used to attach the signal id's of child signals  */
} LigmaContainerHandler;

struct _LigmaContainerPrivate
{
  GType                children_type;
  LigmaContainerPolicy  policy;
  gint                 n_children;

  GList               *handlers;
  gint                 freeze_count;
};


/*  local function prototypes  */

static void   ligma_container_config_iface_init   (LigmaConfigInterface *iface);

static void       ligma_container_dispose         (GObject          *object);

static void       ligma_container_set_property    (GObject          *object,
                                                  guint             property_id,
                                                  const GValue     *value,
                                                  GParamSpec       *pspec);
static void       ligma_container_get_property    (GObject          *object,
                                                  guint             property_id,
                                                  GValue           *value,
                                                  GParamSpec       *pspec);

static gint64     ligma_container_get_memsize     (LigmaObject       *object,
                                                  gint64           *gui_size);

static void       ligma_container_real_add        (LigmaContainer    *container,
                                                  LigmaObject       *object);
static void       ligma_container_real_remove     (LigmaContainer    *container,
                                                  LigmaObject       *object);

static gboolean   ligma_container_serialize       (LigmaConfig       *config,
                                                  LigmaConfigWriter *writer,
                                                  gpointer          data);
static gboolean   ligma_container_deserialize     (LigmaConfig       *config,
                                                  GScanner         *scanner,
                                                  gint              nest_level,
                                                  gpointer          data);

static void   ligma_container_disconnect_callback (LigmaObject       *object,
                                                  gpointer          data);

static void       ligma_container_free_handler    (LigmaContainer    *container,
                                                  LigmaContainerHandler *handler);


G_DEFINE_TYPE_WITH_CODE (LigmaContainer, ligma_container, LIGMA_TYPE_OBJECT,
                         G_ADD_PRIVATE (LigmaContainer)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_container_config_iface_init))

#define parent_class ligma_container_parent_class

static guint container_signals[LAST_SIGNAL] = { 0, };


static void
ligma_container_class_init (LigmaContainerClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);

  container_signals[ADD] =
    g_signal_new ("add",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContainerClass, add),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_OBJECT);

  container_signals[REMOVE] =
    g_signal_new ("remove",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContainerClass, remove),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_OBJECT);

  container_signals[REORDER] =
    g_signal_new ("reorder",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContainerClass, reorder),
                  NULL, NULL,
                  ligma_marshal_VOID__OBJECT_INT,
                  G_TYPE_NONE, 2,
                  LIGMA_TYPE_OBJECT,
                  G_TYPE_INT);

  container_signals[FREEZE] =
    g_signal_new ("freeze",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaContainerClass, freeze),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  container_signals[THAW] =
    g_signal_new ("thaw",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaContainerClass, thaw),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->dispose          = ligma_container_dispose;
  object_class->set_property     = ligma_container_set_property;
  object_class->get_property     = ligma_container_get_property;

  ligma_object_class->get_memsize = ligma_container_get_memsize;

  klass->add                     = ligma_container_real_add;
  klass->remove                  = ligma_container_real_remove;
  klass->reorder                 = NULL;
  klass->freeze                  = NULL;
  klass->thaw                    = NULL;

  klass->clear                   = NULL;
  klass->have                    = NULL;
  klass->foreach                 = NULL;
  klass->search                  = NULL;
  klass->get_unique_names        = NULL;
  klass->get_child_by_name       = NULL;
  klass->get_child_by_index      = NULL;
  klass->get_child_index         = NULL;

  g_object_class_install_property (object_class, PROP_CHILDREN_TYPE,
                                   g_param_spec_gtype ("children-type",
                                                       NULL, NULL,
                                                       LIGMA_TYPE_OBJECT,
                                                       LIGMA_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_POLICY,
                                   g_param_spec_enum ("policy",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_CONTAINER_POLICY,
                                                      LIGMA_CONTAINER_POLICY_STRONG,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_container_config_iface_init (LigmaConfigInterface *iface)
{
  iface->serialize   = ligma_container_serialize;
  iface->deserialize = ligma_container_deserialize;
}

static void
ligma_container_init (LigmaContainer *container)
{
  container->priv = ligma_container_get_instance_private (container);
  container->priv->handlers      = NULL;
  container->priv->freeze_count  = 0;

  container->priv->children_type = G_TYPE_NONE;
  container->priv->policy        = LIGMA_CONTAINER_POLICY_STRONG;
  container->priv->n_children    = 0;
}

static void
ligma_container_dispose (GObject *object)
{
  LigmaContainer *container = LIGMA_CONTAINER (object);

  ligma_container_clear (container);

  while (container->priv->handlers)
    ligma_container_remove_handler (container,
                                   ((LigmaContainerHandler *)
                                    container->priv->handlers->data)->quark);

  if (container->priv->children_type != G_TYPE_NONE)
    {
      g_type_class_unref (g_type_class_peek (container->priv->children_type));
      container->priv->children_type = G_TYPE_NONE;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_container_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  LigmaContainer *container = LIGMA_CONTAINER (object);

  switch (property_id)
    {
    case PROP_CHILDREN_TYPE:
      container->priv->children_type = g_value_get_gtype (value);
      g_type_class_ref (container->priv->children_type);
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
ligma_container_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  LigmaContainer *container = LIGMA_CONTAINER (object);

  switch (property_id)
    {
    case PROP_CHILDREN_TYPE:
      g_value_set_gtype (value, container->priv->children_type);
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
ligma_container_get_memsize (LigmaObject *object,
                            gint64     *gui_size)
{
  LigmaContainer *container = LIGMA_CONTAINER (object);
  gint64         memsize   = 0;
  GList         *list;

  for (list = container->priv->handlers; list; list = g_list_next (list))
    {
      LigmaContainerHandler *handler = list->data;

      memsize += (sizeof (GList) +
                  sizeof (LigmaContainerHandler) +
                  ligma_string_get_memsize (handler->signame));
    }

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_container_real_add (LigmaContainer *container,
                         LigmaObject    *object)
{
  container->priv->n_children++;
}

static void
ligma_container_real_remove (LigmaContainer *container,
                            LigmaObject    *object)
{
  container->priv->n_children--;
}


typedef struct
{
  LigmaConfigWriter *writer;
  gpointer          data;
  gboolean          success;
} SerializeData;

static void
ligma_container_serialize_foreach (GObject       *object,
                                  SerializeData *serialize_data)
{
  LigmaConfigInterface *config_iface;
  const gchar         *name;

  config_iface = LIGMA_CONFIG_GET_IFACE (object);

  if (! config_iface)
    serialize_data->success = FALSE;

  if (! serialize_data->success)
    return;

  ligma_config_writer_open (serialize_data->writer,
                           g_type_name (G_TYPE_FROM_INSTANCE (object)));

  name = ligma_object_get_name (object);

  if (name)
    ligma_config_writer_string (serialize_data->writer, name);
  else
    ligma_config_writer_print (serialize_data->writer, "NULL", 4);

  serialize_data->success = config_iface->serialize (LIGMA_CONFIG (object),
                                                     serialize_data->writer,
                                                     serialize_data->data);
  ligma_config_writer_close (serialize_data->writer);
}

static gboolean
ligma_container_serialize (LigmaConfig       *config,
                          LigmaConfigWriter *writer,
                          gpointer          data)
{
  LigmaContainer *container = LIGMA_CONTAINER (config);
  SerializeData  serialize_data;

  serialize_data.writer  = writer;
  serialize_data.data    = data;
  serialize_data.success = TRUE;

  ligma_container_foreach (container,
                          (GFunc) ligma_container_serialize_foreach,
                          &serialize_data);

  return serialize_data.success;
}

static gboolean
ligma_container_deserialize (LigmaConfig *config,
                            GScanner   *scanner,
                            gint        nest_level,
                            gpointer    data)
{
  LigmaContainer *container = LIGMA_CONTAINER (config);
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
            LigmaObject *child     = NULL;
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

            if (! g_type_is_a (type, container->priv->children_type))
              {
                g_scanner_error (scanner,
                                 "'%s' is not a subclass of '%s'",
                                 scanner->value.v_identifier,
                                 g_type_name (container->priv->children_type));
                return FALSE;
              }

            if (! g_type_is_a (type, LIGMA_TYPE_CONFIG))
              {
                g_scanner_error (scanner,
                                 "'%s' does not implement LigmaConfigInterface",
                                 scanner->value.v_identifier);
                return FALSE;
              }

            if (! ligma_scanner_parse_string (scanner, &name))
              {
                token = G_TOKEN_STRING;
                break;
              }

            if (! name)
              name = g_strdup ("");

            if (ligma_container_get_unique_names (container))
              child = ligma_container_get_child_by_name (container, name);

            if (! child)
              {
                if (LIGMA_IS_LIGMA (data))
                  child = g_object_new (type, "ligma", data, NULL);
                else
                  child = g_object_new (type, NULL);

                add_child = TRUE;
              }

            /*  always use the deserialized name. while it normally
             *  doesn't make a difference there are obscure case like
             *  template migration.
             */
            ligma_object_take_name (child, name);

            if (! LIGMA_CONFIG_GET_IFACE (child)->deserialize (LIGMA_CONFIG (child),
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
                ligma_container_add (container, child);

                if (container->priv->policy == LIGMA_CONTAINER_POLICY_STRONG)
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

  return ligma_config_deserialize_return (scanner, token, nest_level);
}

static void
ligma_container_disconnect_callback (LigmaObject *object,
                                    gpointer    data)
{
  LigmaContainer *container = LIGMA_CONTAINER (data);

  ligma_container_remove (container, object);
}

static void
ligma_container_free_handler_foreach_func (LigmaObject           *object,
                                          LigmaContainerHandler *handler)
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
ligma_container_free_handler (LigmaContainer        *container,
                             LigmaContainerHandler *handler)
{
  D (g_print ("%s: id = %d\n", G_STRFUNC, handler->quark));

  ligma_container_foreach (container,
                          (GFunc) ligma_container_free_handler_foreach_func,
                          handler);

  g_free (handler->signame);
  g_slice_free (LigmaContainerHandler, handler);
}

GType
ligma_container_get_children_type (LigmaContainer *container)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), G_TYPE_NONE);

  return container->priv->children_type;
}

LigmaContainerPolicy
ligma_container_get_policy (LigmaContainer *container)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), 0);

  return container->priv->policy;
}

gint
ligma_container_get_n_children (LigmaContainer *container)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), 0);

  return container->priv->n_children;
}

gboolean
ligma_container_add (LigmaContainer *container,
                    LigmaObject    *object)
{
  GList *list;
  gint   n_children;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (object,
                                                    container->priv->children_type),
                        FALSE);

  if (ligma_container_have (container, object))
    {
      g_warning ("%s: container %p already contains object %p",
                 G_STRFUNC, container, object);
      return FALSE;
    }

  for (list = container->priv->handlers; list; list = g_list_next (list))
    {
      LigmaContainerHandler *handler = list->data;
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
    case LIGMA_CONTAINER_POLICY_STRONG:
      g_object_ref (object);
      break;

    case LIGMA_CONTAINER_POLICY_WEAK:
      g_signal_connect (object, "disconnect",
                        G_CALLBACK (ligma_container_disconnect_callback),
                        container);
      break;
    }

  n_children = container->priv->n_children;

  g_signal_emit (container, container_signals[ADD], 0, object);

  if (n_children == container->priv->n_children)
    {
      g_warning ("%s: LigmaContainer::add() implementation did not "
                 "chain up. Please report this at https://www.ligma.org/bugs/",
                 G_STRFUNC);

      container->priv->n_children++;
    }

  return TRUE;
}

gboolean
ligma_container_remove (LigmaContainer *container,
                       LigmaObject    *object)
{
  GList *list;
  gint   n_children;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (object,
                                                    container->priv->children_type),
                        FALSE);

  if (! ligma_container_have (container, object))
    {
      g_warning ("%s: container %p does not contain object %p",
                 G_STRFUNC, container, object);
      return FALSE;
    }

  for (list = container->priv->handlers; list; list = g_list_next (list))
    {
      LigmaContainerHandler *handler = list->data;
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

  g_signal_emit (container, container_signals[REMOVE], 0, object);

  if (n_children == container->priv->n_children)
    {
      g_warning ("%s: LigmaContainer::remove() implementation did not "
                 "chain up. Please report this at https://www.ligma.org/bugs/",
                 G_STRFUNC);

      container->priv->n_children--;
    }

  switch (container->priv->policy)
    {
    case LIGMA_CONTAINER_POLICY_STRONG:
      g_object_unref (object);
      break;

    case LIGMA_CONTAINER_POLICY_WEAK:
      g_signal_handlers_disconnect_by_func (object,
                                            ligma_container_disconnect_callback,
                                            container);
      break;
    }

  return TRUE;
}

gboolean
ligma_container_insert (LigmaContainer *container,
                       LigmaObject    *object,
                       gint           index)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (object,
                                                    container->priv->children_type),
                        FALSE);

  g_return_val_if_fail (index >= -1 &&
                        index <= container->priv->n_children, FALSE);

  if (ligma_container_have (container, object))
    {
      g_warning ("%s: container %p already contains object %p",
                 G_STRFUNC, container, object);
      return FALSE;
    }

  if (ligma_container_add (container, object))
    {
      return ligma_container_reorder (container, object, index);
    }

  return FALSE;
}

gboolean
ligma_container_reorder (LigmaContainer *container,
                        LigmaObject    *object,
                        gint           new_index)
{
  gint index;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (object != NULL, FALSE);
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (object,
                                                    container->priv->children_type),
                        FALSE);

  g_return_val_if_fail (new_index >= -1 &&
                        new_index < container->priv->n_children, FALSE);

  if (new_index == -1)
    new_index = container->priv->n_children - 1;

  index = ligma_container_get_child_index (container, object);

  if (index == -1)
    {
      g_warning ("%s: container %p does not contain object %p",
                 G_STRFUNC, container, object);
      return FALSE;
    }

  if (index != new_index)
    g_signal_emit (container, container_signals[REORDER], 0,
                   object, new_index);

  return TRUE;
}

void
ligma_container_freeze (LigmaContainer *container)
{
  g_return_if_fail (LIGMA_IS_CONTAINER (container));

  container->priv->freeze_count++;

  if (container->priv->freeze_count == 1)
    g_signal_emit (container, container_signals[FREEZE], 0);
}

void
ligma_container_thaw (LigmaContainer *container)
{
  g_return_if_fail (LIGMA_IS_CONTAINER (container));

  if (container->priv->freeze_count > 0)
    container->priv->freeze_count--;

  if (container->priv->freeze_count == 0)
    g_signal_emit (container, container_signals[THAW], 0);
}

gboolean
ligma_container_frozen (LigmaContainer *container)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), FALSE);

  return (container->priv->freeze_count > 0) ? TRUE : FALSE;
}

gint
ligma_container_freeze_count (LigmaContainer *container)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), 0);

  return container->priv->freeze_count;
}

void
ligma_container_clear (LigmaContainer *container)
{
  g_return_if_fail (LIGMA_IS_CONTAINER (container));

  if (container->priv->n_children > 0)
    {
      ligma_container_freeze (container);
      LIGMA_CONTAINER_GET_CLASS (container)->clear (container);
      ligma_container_thaw (container);
    }
}

gboolean
ligma_container_is_empty (LigmaContainer *container)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), FALSE);

  return (container->priv->n_children == 0);
}

gboolean
ligma_container_have (LigmaContainer *container,
                     LigmaObject    *object)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), FALSE);

  if (container->priv->n_children < 1)
    return FALSE;

  return LIGMA_CONTAINER_GET_CLASS (container)->have (container, object);
}

void
ligma_container_foreach (LigmaContainer *container,
                        GFunc          func,
                        gpointer       user_data)
{
  g_return_if_fail (LIGMA_IS_CONTAINER (container));
  g_return_if_fail (func != NULL);

  if (container->priv->n_children > 0)
    LIGMA_CONTAINER_GET_CLASS (container)->foreach (container, func, user_data);
}

LigmaObject *
ligma_container_search (LigmaContainer           *container,
                       LigmaContainerSearchFunc  func,
                       gpointer                 user_data)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (func != NULL, NULL);

  if (container->priv->n_children > 0)
    {
      return LIGMA_CONTAINER_GET_CLASS (container)->search (container,
                                                           func, user_data);
    }

  return NULL;
}

gboolean
ligma_container_get_unique_names (LigmaContainer *container)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), FALSE);

  if (LIGMA_CONTAINER_GET_CLASS (container)->get_unique_names)
    return LIGMA_CONTAINER_GET_CLASS (container)->get_unique_names (container);

  return FALSE;
}

LigmaObject *
ligma_container_get_child_by_name (LigmaContainer *container,
                                  const gchar   *name)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);

  if (!name)
    return NULL;

  return LIGMA_CONTAINER_GET_CLASS (container)->get_child_by_name (container,
                                                                  name);
}

LigmaObject *
ligma_container_get_child_by_index (LigmaContainer *container,
                                   gint           index)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);

  if (index < 0 || index >= container->priv->n_children)
    return NULL;

  return LIGMA_CONTAINER_GET_CLASS (container)->get_child_by_index (container,
                                                                   index);
}

/**
 * ligma_container_get_first_child:
 * @container: a #LigmaContainer
 *
 * Returns: (nullable) (transfer none): the first child object stored in
 *          @container or %NULL if the container is empty.
 */
LigmaObject *
ligma_container_get_first_child (LigmaContainer *container)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);

  if (container->priv->n_children > 0)
    return LIGMA_CONTAINER_GET_CLASS (container)->get_child_by_index (container,
                                                                     0);

  return NULL;
}

/**
 * ligma_container_get_last_child:
 * @container: a #LigmaContainer
 *
 * Returns: (nullable) (transfer none): the last child object stored in
 *          @container or %NULL if the container is empty
 */
LigmaObject *
ligma_container_get_last_child (LigmaContainer *container)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);

  if (container->priv->n_children > 0)
    return LIGMA_CONTAINER_GET_CLASS (container)->get_child_by_index (container,
                                                                     container->priv->n_children - 1);

  return NULL;
}

gint
ligma_container_get_child_index (LigmaContainer *container,
                                LigmaObject    *object)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), -1);
  g_return_val_if_fail (object != NULL, -1);
  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (object,
                                                    container->priv->children_type),
                        -1);

  return LIGMA_CONTAINER_GET_CLASS (container)->get_child_index (container,
                                                                object);
}

LigmaObject *
ligma_container_get_neighbor_of (LigmaContainer *container,
                                LigmaObject    *object)
{
  gint index;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (LIGMA_IS_OBJECT (object), NULL);

  index = ligma_container_get_child_index (container, object);

  if (index != -1)
    {
      LigmaObject *new;

      new = ligma_container_get_child_by_index (container, index + 1);

      if (! new && index > 0)
        new = ligma_container_get_child_by_index (container, index - 1);

      return new;
    }

  return NULL;
}

static void
ligma_container_get_name_array_foreach_func (LigmaObject   *object,
                                            gchar      ***iter)
{
  gchar **array = *iter;

  *array = g_strdup (ligma_object_get_name (object));
  (*iter)++;
}

gchar **
ligma_container_get_name_array (LigmaContainer *container)
{
  gchar **names;
  gchar **iter;
  gint    length;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);

  length = ligma_container_get_n_children (container);

  names = iter = g_new0 (gchar *, length + 1);

  ligma_container_foreach (container,
                          (GFunc) ligma_container_get_name_array_foreach_func,
                          &iter);

  return names;
}

static void
ligma_container_add_handler_foreach_func (LigmaObject           *object,
                                         LigmaContainerHandler *handler)
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
ligma_container_add_handler (LigmaContainer *container,
                            const gchar   *signame,
                            GCallback      callback,
                            gpointer       callback_data)
{
  LigmaContainerHandler *handler;
  gchar                *key;

  static gint           handler_id = 0;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), 0);
  g_return_val_if_fail (signame != NULL, 0);
  g_return_val_if_fail (callback != NULL, 0);

  if (! g_str_has_prefix (signame, "notify::"))
    g_return_val_if_fail (g_signal_lookup (signame,
                                           container->priv->children_type), 0);

  handler = g_slice_new0 (LigmaContainerHandler);

  /*  create a unique key for this handler  */
  key = g_strdup_printf ("%s-%d", signame, handler_id++);

  handler->signame       = g_strdup (signame);
  handler->callback      = callback;
  handler->callback_data = callback_data;
  handler->quark         = g_quark_from_string (key);

  D (g_print ("%s: key = %s, id = %d\n", G_STRFUNC, key, handler->quark));

  g_free (key);

  container->priv->handlers = g_list_prepend (container->priv->handlers, handler);

  ligma_container_foreach (container,
                          (GFunc) ligma_container_add_handler_foreach_func,
                          handler);

  return handler->quark;
}

void
ligma_container_remove_handler (LigmaContainer *container,
                               GQuark         id)
{
  LigmaContainerHandler *handler;
  GList                *list;

  g_return_if_fail (LIGMA_IS_CONTAINER (container));
  g_return_if_fail (id != 0);

  for (list = container->priv->handlers; list; list = g_list_next (list))
    {
      handler = (LigmaContainerHandler *) list->data;

      if (handler->quark == id)
        break;
    }

  if (! list)
    {
      g_warning ("%s: tried to remove handler which unknown id %d",
                 G_STRFUNC, id);
      return;
    }

  ligma_container_free_handler (container, handler);

  container->priv->handlers = g_list_delete_link (container->priv->handlers,
                                                  list);
}

void
ligma_container_remove_handlers_by_func (LigmaContainer *container,
                                        GCallback      callback,
                                        gpointer       callback_data)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_CONTAINER (container));
  g_return_if_fail (callback != NULL);

  list = container->priv->handlers;

  while (list)
    {
      LigmaContainerHandler *handler = list->data;
      GList                *next    = g_list_next (list);

      if (handler->callback      == callback &&
          handler->callback_data == callback_data)
        {
          ligma_container_free_handler (container, handler);

          container->priv->handlers = g_list_delete_link (
            container->priv->handlers, list);
        }

      list = next;
    }
}

void
ligma_container_remove_handlers_by_data (LigmaContainer *container,
                                        gpointer       callback_data)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_CONTAINER (container));

  list = container->priv->handlers;

  while (list)
    {
      LigmaContainerHandler *handler = list->data;
      GList                *next    = g_list_next (list);

      if (handler->callback_data == callback_data)
        {
          ligma_container_free_handler (container, handler);

          container->priv->handlers = g_list_delete_link (
            container->priv->handlers, list);
        }

      list = next;
    }
}
