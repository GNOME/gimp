/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdlib.h>
#include <string.h> /* memcmp */

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimppadactions.h"
#include "gimpparamspecs.h"

#include "gimp-intl.h"


typedef struct _GimpPadAction GimpPadAction;
struct _GimpPadAction
{
  GimpPadActionType  type;
  gint               number;
  gint               mode;
  gchar             *action_name;
};

struct _GimpPadActionsPrivate
{
  GArray *actions;
};


/*  local function prototypes  */

static void     gimp_pad_actions_config_iface_init (GimpConfigInterface *iface);

static void     gimp_pad_actions_finalize          (GObject             *object);

static gboolean gimp_pad_actions_serialize         (GimpConfig          *config,
                                                    GimpConfigWriter    *writer,
                                                    gpointer             data);
static gboolean gimp_pad_actions_deserialize       (GimpConfig          *config,
                                                    GScanner            *scanner,
                                                    gint                 nest_level,
                                                    gpointer             data);

static gboolean gimp_pad_actions_equal             (GimpConfig          *a,
                                                    GimpConfig          *b);
static void     gimp_pad_actions_reset             (GimpConfig          *config);
static gboolean gimp_pad_actions_config_copy       (GimpConfig          *src,
                                                    GimpConfig          *dest,
                                                    GParamFlags          flags);


G_DEFINE_TYPE_WITH_CODE (GimpPadActions, gimp_pad_actions, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (GimpPadActions)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_pad_actions_config_iface_init))

#define parent_class gimp_pad_actions_parent_class


static void
gimp_pad_actions_class_init (GimpPadActionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_pad_actions_finalize;
}

static void
gimp_pad_actions_config_iface_init (GimpConfigInterface *iface)
{
  iface->serialize   = gimp_pad_actions_serialize;
  iface->deserialize = gimp_pad_actions_deserialize;
  iface->equal       = gimp_pad_actions_equal;
  iface->reset       = gimp_pad_actions_reset;
  iface->copy        = gimp_pad_actions_config_copy;
}

static void
clear_action (gpointer user_data)
{
  GimpPadAction *pad_action = user_data;

  g_free (pad_action->action_name);
}

static void
gimp_pad_actions_init (GimpPadActions *pad_actions)
{
  pad_actions->priv = gimp_pad_actions_get_instance_private (pad_actions);

  pad_actions->priv->actions =
    g_array_new (FALSE, FALSE, sizeof (GimpPadAction));
  g_array_set_clear_func (pad_actions->priv->actions, clear_action);
}

static void
gimp_pad_actions_finalize (GObject *object)
{
  GimpPadActions *pad_actions = GIMP_PAD_ACTIONS (object);

  g_clear_pointer (&pad_actions->priv->actions, g_array_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static gboolean
gimp_pad_actions_serialize (GimpConfig       *config,
                            GimpConfigWriter *writer,
                            gpointer          data)
{
  GimpPadActions *pad_actions = GIMP_PAD_ACTIONS (config);
  guint           i;

  for (i = 0; i < pad_actions->priv->actions->len; i++)
    {
      GimpPadAction *pad_action;
      const gchar   *name        = NULL;

      pad_action = &g_array_index (pad_actions->priv->actions,
                                   GimpPadAction, i);

      switch (pad_action->type)
        {
        case GIMP_PAD_ACTION_BUTTON:
          name = "button";
          break;
        case GIMP_PAD_ACTION_RING:
          name = "ring";
          break;
        case GIMP_PAD_ACTION_STRIP:
          name = "strip";
          break;
        }

      gimp_config_writer_open (writer, name);
      gimp_config_writer_printf (writer, "%d", pad_action->number);
      gimp_config_writer_printf (writer, "%d", pad_action->mode);
      gimp_config_writer_string (writer, pad_action->action_name);
      gimp_config_writer_close (writer);
    }

  return TRUE;
}

static gboolean
gimp_pad_actions_deserialize (GimpConfig *config,
                              GScanner   *scanner,
                              gint        nest_level,
                              gpointer    data)
{
  GimpPadActions *pad_actions = GIMP_PAD_ACTIONS (config);
  GTokenType      token;
  guint           scope_id;
  guint           old_scope_id;

  scope_id = g_type_qname (G_TYPE_FROM_INSTANCE (config));
  old_scope_id = g_scanner_set_scope (scanner, scope_id);

  g_scanner_scope_add_symbol (scanner, scope_id, "button",
                              GINT_TO_POINTER (GIMP_PAD_ACTION_BUTTON));
  g_scanner_scope_add_symbol (scanner, scope_id, "ring",
                              GINT_TO_POINTER (GIMP_PAD_ACTION_RING));
  g_scanner_scope_add_symbol (scanner, scope_id, "strip",
                              GINT_TO_POINTER (GIMP_PAD_ACTION_STRIP));

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      GimpPadActionType  type;
      gint               number;
      gint               mode;
      gchar             *action_name = NULL;

      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          if (scanner->value.v_symbol == GUINT_TO_POINTER (GIMP_PAD_ACTION_BUTTON) ||
              scanner->value.v_symbol == GUINT_TO_POINTER (GIMP_PAD_ACTION_RING) ||
              scanner->value.v_symbol == GUINT_TO_POINTER (GIMP_PAD_ACTION_STRIP))
            type = GPOINTER_TO_UINT (scanner->value.v_symbol);
          else
            goto error;

          if (! gimp_scanner_parse_int (scanner, &number))
            goto error;
          if (! gimp_scanner_parse_int (scanner, &mode))
            goto error;
          if (! gimp_scanner_parse_string (scanner, &action_name))
            goto error;

          gimp_pad_actions_set_action (pad_actions, type, number, mode,
                                       action_name);
          g_clear_pointer (&action_name, g_free);
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

 error:

  g_scanner_scope_remove_symbol (scanner, scope_id, "button");
  g_scanner_scope_remove_symbol (scanner, scope_id, "ring");
  g_scanner_scope_remove_symbol (scanner, scope_id, "strip");

  g_scanner_set_scope (scanner, old_scope_id);

  return G_TOKEN_LEFT_PAREN;
}

static gboolean
gimp_pad_actions_equal (GimpConfig *a,
                        GimpConfig *b)
{
  GimpPadActions *a_pad_actions = GIMP_PAD_ACTIONS (a);
  GimpPadActions *b_pad_actions = GIMP_PAD_ACTIONS (b);

  if (a_pad_actions->priv->actions->len != b_pad_actions->priv->actions->len ||
      memcmp (a_pad_actions->priv->actions->data,
              b_pad_actions->priv->actions->data,
              sizeof (GimpPadAction) * a_pad_actions->priv->actions->len))
    {
      return FALSE;
    }

  return TRUE;
}

static void
gimp_pad_actions_reset (GimpConfig *config)
{
  GimpPadActions *pad_actions = GIMP_PAD_ACTIONS (config);

  g_array_set_size (pad_actions->priv->actions, 0);
}

static gboolean
gimp_pad_actions_config_copy (GimpConfig  *src,
                              GimpConfig  *dest,
                              GParamFlags  flags)
{
  gimp_config_sync (G_OBJECT (src), G_OBJECT (dest), flags);
  gimp_data_dirty (GIMP_DATA (dest));

  return TRUE;
}


static gint
gimp_pad_actions_find_action (GArray            *array,
                              GimpPadActionType  type,
                              guint              number,
                              guint              mode)
{
  GimpPadAction *pad_action;
  guint          i;

  for (i = 0; i < array->len; i++)
    {
      pad_action = &g_array_index (array, GimpPadAction, i);
      if (pad_action->type == type && pad_action->number == number &&
          pad_action->mode == mode)
        return i;
    }

  return -1;
}


/*  public functions  */

GimpPadActions *
gimp_pad_actions_new (void)
{
  return g_object_new (GIMP_TYPE_PAD_ACTIONS, NULL);
}

void
gimp_pad_actions_foreach (GimpPadActions       *pad_actions,
                          GimpPadActionForeach  func,
                          gpointer              data)
{
  GimpPadActionsPrivate *private;
  GimpPadAction         *pad_action;
  guint                  i;

  private = pad_actions->priv;

  for (i = 0; i < private->actions->len; i++)
    {
      pad_action = &g_array_index (private->actions, GimpPadAction, i);

      func (pad_actions, pad_action->type, pad_action->number, pad_action->mode,
            pad_action->action_name, data);
    }
}

gint
gimp_pad_actions_set_action (GimpPadActions    *pad_actions,
                             GimpPadActionType  type,
                             guint              number,
                             guint              mode,
                             const gchar       *action_name)
{
  GimpPadActionsPrivate *private;
  GimpPadAction          new_action = { 0, };
  guint                  i;

  g_return_val_if_fail (GIMP_IS_PAD_ACTIONS (pad_actions), -1);
  g_return_val_if_fail (type <= GIMP_PAD_ACTION_STRIP, -1);
  g_return_val_if_fail (action_name, -1);
  g_return_val_if_fail (*action_name, -1);

  new_action.type = type;
  new_action.number = number;
  new_action.mode = mode;
  new_action.action_name = g_strdup (action_name);

  private = pad_actions->priv;

  /* Find insertion point */
  for (i = 0; i < private->actions->len; i++)
    {
      GimpPadAction *pad_action;

      pad_action = &g_array_index (private->actions,
                                   GimpPadAction, i);
      /* If the number/mode is already there, this is an existing item */
      if (pad_action->type == type && pad_action->number == number &&
          pad_action->mode == mode)
        {
          g_array_remove_index (private->actions, i);
          break;
        }

      if (pad_action->type > type ||
          (pad_action->type == type && pad_action->mode > mode) ||
          (pad_action->type == type && pad_action->mode == mode &&
           pad_action->number > number))
        break;
    }

  if (i == pad_actions->priv->actions->len)
    g_array_append_val (private->actions, new_action);
  else
    g_array_insert_val (private->actions, i, new_action);

  return i;
}

void
gimp_pad_actions_clear_action (GimpPadActions    *pad_actions,
                               GimpPadActionType  type,
                               guint              number,
                               guint              mode)
{
  GimpPadActionsPrivate *private;
  gint                   pos;

  g_return_if_fail (GIMP_IS_PAD_ACTIONS (pad_actions));
  g_return_if_fail (type <= GIMP_PAD_ACTION_STRIP);

  private = pad_actions->priv;
  pos = gimp_pad_actions_find_action (private->actions, type,
                                      number, mode);
  if (pos >= 0)
    g_array_remove_index (private->actions, pos);
}

gint
gimp_pad_actions_get_n_actions (GimpPadActions *pad_actions)
{
  GimpPadActionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_PAD_ACTIONS (pad_actions), -1);

  private = pad_actions->priv;

  return private->actions->len;
}

const gchar *
gimp_pad_actions_get_action (GimpPadActions    *pad_actions,
                             GimpPadActionType  type,
                             guint              number,
                             guint              mode)
{
  GimpPadActionsPrivate *private;
  GimpPadAction         *pad_action;
  gint                   pos;

  g_return_val_if_fail (GIMP_IS_PAD_ACTIONS (pad_actions), NULL);
  g_return_val_if_fail (type <= GIMP_PAD_ACTION_STRIP, NULL);

  private = pad_actions->priv;

  pos = gimp_pad_actions_find_action (private->actions, type,
                                      number, mode);
  if (pos >= 0)
    {
      pad_action = &g_array_index (private->actions,
                                   GimpPadAction, pos);
      return pad_action->action_name;
    }
  else
    {
      return NULL;
    }
}
