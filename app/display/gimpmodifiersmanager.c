/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmodifiersmanager.c
 * Copyright (C) 2022 Jehan
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "display-types.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpmodifiersmanager.h"

#include "gimp-intl.h"

enum
{
  MODIFIERS_MANAGER_MAPPING,
  MODIFIERS_MANAGER_MODIFIERS,
  MODIFIERS_MANAGER_MOD_ACTION,
};

typedef struct
{
  GdkModifierType     modifiers;
  GimpModifierAction  mod_action;
  gchar              *action_desc;
} GimpModifierMapping;

struct _GimpModifiersManagerPrivate
{
  GHashTable *actions;
  GList      *buttons;
};

static void      gimp_modifiers_manager_config_iface_init  (GimpConfigInterface    *iface);
static void      gimp_modifiers_manager_finalize           (GObject                *object);
static gboolean  gimp_modifiers_manager_serialize          (GimpConfig             *config,
                                                            GimpConfigWriter       *writer,
                                                            gpointer                data);
static gboolean  gimp_modifiers_manager_deserialize        (GimpConfig             *config,
                                                            GScanner               *scanner,
                                                            gint                    nest_level,
                                                            gpointer                data);

static void      gimp_modifiers_manager_free_mapping       (GimpModifierMapping       *mapping);

static void      gimp_modifiers_manager_get_keys           (GdkDevice              *device,
                                                            guint                   button,
                                                            GdkModifierType         modifiers,
                                                            gchar                 **actions_key,
                                                            gchar                 **buttons_key);
static void      gimp_modifiers_manager_initialize         (GimpModifiersManager   *manager,
                                                            GdkDevice              *device,
                                                            guint                   button);


G_DEFINE_TYPE_WITH_CODE (GimpModifiersManager, gimp_modifiers_manager, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (GimpModifiersManager)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_modifiers_manager_config_iface_init))

#define parent_class gimp_modifiers_manager_parent_class


static void
gimp_modifiers_manager_class_init (GimpModifiersManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_modifiers_manager_finalize;
}

static void
gimp_modifiers_manager_init (GimpModifiersManager *manager)
{
  manager->p = gimp_modifiers_manager_get_instance_private (manager);

  manager->p->actions = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                               (GDestroyNotify) gimp_modifiers_manager_free_mapping);
}

static void
gimp_modifiers_manager_config_iface_init (GimpConfigInterface *iface)
{
  iface->serialize   = gimp_modifiers_manager_serialize;
  iface->deserialize = gimp_modifiers_manager_deserialize;
}

static void
gimp_modifiers_manager_finalize (GObject *object)
{
  GimpModifiersManager *manager = GIMP_MODIFIERS_MANAGER (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);

  g_hash_table_unref (manager->p->actions);
  g_list_free_full (manager->p->buttons, g_free);
}

static gboolean
gimp_modifiers_manager_serialize (GimpConfig       *config,
                                  GimpConfigWriter *writer,
                                  gpointer          data)
{
  GimpModifiersManager *manager = GIMP_MODIFIERS_MANAGER (config);
  GEnumClass           *enum_class;
  GList                *keys;
  GList                *iter;

  enum_class = g_type_class_ref (GIMP_TYPE_MODIFIER_ACTION);
  keys       = g_hash_table_get_keys (manager->p->actions);

  for (iter = keys; iter; iter = iter->next)
    {
      const gchar         *button = iter->data;
      GimpModifierMapping *mapping;
      GEnumValue          *enum_value;

      gimp_config_writer_open (writer, "mapping");
      gimp_config_writer_string (writer, button);

      mapping = g_hash_table_lookup (manager->p->actions, button);

      gimp_config_writer_open (writer, "modifiers");
      gimp_config_writer_printf (writer, "%d", mapping->modifiers);
      gimp_config_writer_close (writer);

      enum_value = g_enum_get_value (enum_class, GPOINTER_TO_INT (mapping->mod_action));
      gimp_config_writer_open (writer, "mod-action");
      gimp_config_writer_identifier (writer, enum_value->value_nick);
      if (mapping->mod_action == GIMP_MODIFIER_ACTION_ACTION)
        gimp_config_writer_string (writer, mapping->action_desc);
      gimp_config_writer_close (writer);

      gimp_config_writer_close (writer);
    }

  g_list_free (keys);
  g_type_class_unref (enum_class);

  return TRUE;
}

static gboolean
gimp_modifiers_manager_deserialize (GimpConfig *config,
                                    GScanner   *scanner,
                                    gint        nest_level,
                                    gpointer    data)
{
  GimpModifiersManager *manager = GIMP_MODIFIERS_MANAGER (config);
  GEnumClass           *enum_class;
  GTokenType            token;
  guint                 scope_id;
  guint                 old_scope_id;
  gchar                *actions_key = NULL;
  GdkModifierType       modifiers   = 0;

  scope_id = g_type_qname (G_TYPE_FROM_INSTANCE (config));
  old_scope_id = g_scanner_set_scope (scanner, scope_id);
  enum_class = g_type_class_ref (GIMP_TYPE_MODIFIER_ACTION);

  g_scanner_scope_add_symbol (scanner, scope_id, "mapping",
                              GINT_TO_POINTER (MODIFIERS_MANAGER_MAPPING));
  g_scanner_scope_add_symbol (scanner, scope_id, "modifiers",
                              GINT_TO_POINTER (MODIFIERS_MANAGER_MODIFIERS));
  g_scanner_scope_add_symbol (scanner, scope_id, "mod-action",
                              GINT_TO_POINTER (MODIFIERS_MANAGER_MOD_ACTION));

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
            case MODIFIERS_MANAGER_MAPPING:
              token = G_TOKEN_LEFT_PAREN;
              if (! gimp_scanner_parse_string (scanner, &actions_key))
                goto error;
              break;

            case MODIFIERS_MANAGER_MOD_ACTION:
                {
                  GimpModifierMapping *mapping;
                  GEnumValue          *enum_value;

                  token = G_TOKEN_IDENTIFIER;
                  if (g_scanner_peek_next_token (scanner) != token)
                    goto error;

                  g_scanner_get_next_token (scanner);

                  enum_value = g_enum_get_value_by_nick (enum_class,
                                                         scanner->value.v_identifier);

                  if (! enum_value)
                    enum_value = g_enum_get_value_by_name (enum_class,
                                                           scanner->value.v_identifier);

                  if (! enum_value)
                    {
                      g_scanner_error (scanner,
                                       _("invalid value '%s' for contextual action"),
                                       scanner->value.v_identifier);
                      return G_TOKEN_NONE;
                    }

                  if (g_hash_table_lookup (manager->p->actions, actions_key))
                    {
                      /* This should not happen. But let's avoid breaking
                       * the whole parsing for a duplicate. Just output to
                       * stderr to track any problematic modifiersrc
                       * creation.
                       */
                      g_printerr ("%s: ignoring duplicate action %s for mapping %s\n",
                                  G_STRFUNC, scanner->value.v_identifier, actions_key);
                      g_clear_pointer (&actions_key, g_free);
                    }
                  else
                    {
                      gchar *suffix;
                      gchar *action_desc = NULL;

                      if (enum_value->value == GIMP_MODIFIER_ACTION_ACTION)
                        {
                          if (! gimp_scanner_parse_string (scanner, &action_desc))
                            {
                              g_printerr ("%s: missing action description for mapping %s\n",
                                          G_STRFUNC, actions_key);
                              goto error;
                            }
                        }

                      suffix = g_strdup_printf ("-%d", modifiers);

                      if (g_str_has_suffix (actions_key, suffix))
                        {
                          gchar *buttons_key = g_strndup (actions_key,
                                                          strlen (actions_key) - strlen (suffix));

                          mapping = g_slice_new (GimpModifierMapping);
                          mapping->modifiers   = modifiers;
                          mapping->mod_action  = enum_value->value;
                          mapping->action_desc = action_desc;
                          g_hash_table_insert (manager->p->actions, actions_key, mapping);

                          if (g_list_find_custom (manager->p->buttons, buttons_key, (GCompareFunc) g_strcmp0))
                            g_free (buttons_key);
                          else
                            manager->p->buttons = g_list_prepend (manager->p->buttons, buttons_key);
                        }
                      else
                        {
                          g_printerr ("%s: ignoring mapping %s with invalid modifiers %d\n",
                                      G_STRFUNC, actions_key, modifiers);
                          g_clear_pointer (&actions_key, g_free);
                        }

                      g_free (suffix);
                    }

                  /* Closing parentheses twice. */
                  token = G_TOKEN_RIGHT_PAREN;
                  if (g_scanner_peek_next_token (scanner) != token)
                    goto error;

                  g_scanner_get_next_token (scanner);
                }
              break;

            case MODIFIERS_MANAGER_MODIFIERS:
              token = G_TOKEN_RIGHT_PAREN;
              if (! gimp_scanner_parse_int (scanner, (int *) &modifiers))
                goto error;
              break;

            default:
              break;
            }
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

 error:

  g_scanner_scope_remove_symbol (scanner, scope_id, "mapping");
  g_scanner_scope_remove_symbol (scanner, scope_id, "modifiers");
  g_scanner_scope_remove_symbol (scanner, scope_id, "mod-action");

  g_scanner_set_scope (scanner, old_scope_id);
  g_type_class_unref (enum_class);

  return G_TOKEN_LEFT_PAREN;
}


/*  public functions  */

GimpModifiersManager *
gimp_modifiers_manager_new (void)
{
  return g_object_new (GIMP_TYPE_MODIFIERS_MANAGER, NULL);
}

GimpModifierAction
gimp_modifiers_manager_get_action (GimpModifiersManager *manager,
                                   GdkDevice            *device,
                                   guint                 button,
                                   GdkModifierType       state,
                                   const gchar         **action_desc)
{
  gchar              *actions_key = NULL;
  gchar              *buttons_key = NULL;
  GdkModifierType     mod_state;
  GimpModifierAction  retval      = GIMP_MODIFIER_ACTION_NONE;

  g_return_val_if_fail (GIMP_IS_MODIFIERS_MANAGER (manager), GIMP_MODIFIER_ACTION_NONE);
  g_return_val_if_fail (GDK_IS_DEVICE (device), GIMP_MODIFIER_ACTION_NONE);
  g_return_val_if_fail (action_desc != NULL && *action_desc == NULL, GIMP_MODIFIER_ACTION_NONE);

  mod_state = state & gimp_get_all_modifiers_mask ();

  gimp_modifiers_manager_get_keys (device, button, mod_state,
                                   &actions_key, &buttons_key);

  if (g_list_find_custom (manager->p->buttons, buttons_key, (GCompareFunc) g_strcmp0))
    {
      GimpModifierMapping *mapping;

      mapping = g_hash_table_lookup (manager->p->actions, actions_key);

      if (mapping == NULL)
        retval = GIMP_MODIFIER_ACTION_NONE;
      else
        retval = mapping->mod_action;

      if (retval == GIMP_MODIFIER_ACTION_ACTION)
        *action_desc = mapping->action_desc;
    }
  else if (button == 2)
    {
      if (mod_state == gimp_get_extend_selection_mask ())
        retval = GIMP_MODIFIER_ACTION_ROTATING;
      else if (mod_state == (gimp_get_extend_selection_mask () | GDK_CONTROL_MASK))
        retval = GIMP_MODIFIER_ACTION_STEP_ROTATING;
      else if (mod_state == gimp_get_toggle_behavior_mask ())
        retval = GIMP_MODIFIER_ACTION_ZOOMING;
      else if (mod_state == GDK_MOD1_MASK)
        retval = GIMP_MODIFIER_ACTION_LAYER_PICKING;
      else if (mod_state == 0)
        retval = GIMP_MODIFIER_ACTION_PANNING;
    }
  else if (button == 3)
    {
      if (mod_state == GDK_MOD1_MASK)
        retval = GIMP_MODIFIER_ACTION_BRUSH_PIXEL_SIZE;
      else if (mod_state == 0)
        retval = GIMP_MODIFIER_ACTION_MENU;
    }

  g_free (actions_key);
  g_free (buttons_key);

  return retval;
}

GList *
gimp_modifiers_manager_get_modifiers (GimpModifiersManager *manager,
                                      GdkDevice            *device,
                                      guint                 button)
{
  gchar *buttons_key = NULL;
  GList *modifiers   = NULL;
  GList *action_keys;
  GList *iter;
  gchar *action_prefix;

  gimp_modifiers_manager_initialize (manager, device, button);

  gimp_modifiers_manager_get_keys (device, button, 0, NULL,
                                   &buttons_key);
  action_prefix = g_strdup_printf ("%s-", buttons_key);
  g_free (buttons_key);

  action_keys = g_hash_table_get_keys (manager->p->actions);
  for (iter = action_keys; iter; iter = iter->next)
    {
      if (g_str_has_prefix (iter->data, action_prefix))
        {
          GimpModifierMapping *mapping;

          mapping = g_hash_table_lookup (manager->p->actions, iter->data);

          /* TODO: the modifiers list should be sorted to ensure
           * consistency.
           */
          modifiers = g_list_prepend (modifiers, GINT_TO_POINTER (mapping->modifiers));
        }
    }

  g_free (action_prefix);
  g_list_free (action_keys);

  return modifiers;
}

void
gimp_modifiers_manager_set (GimpModifiersManager *manager,
                            GdkDevice            *device,
                            guint                 button,
                            GdkModifierType       modifiers,
                            GimpModifierAction    action,
                            const gchar          *action_desc)
{
  gchar *actions_key = NULL;
  gchar *buttons_key = NULL;

  g_return_if_fail (GIMP_IS_MODIFIERS_MANAGER (manager));
  g_return_if_fail (GDK_IS_DEVICE (device));

  gimp_modifiers_manager_get_keys (device, button, modifiers,
                                   &actions_key, &buttons_key);
  g_free (buttons_key);

  gimp_modifiers_manager_initialize (manager, device, button);

  if (action == GIMP_MODIFIER_ACTION_NONE ||
      (action == GIMP_MODIFIER_ACTION_ACTION && action_desc == NULL))
    {
      g_hash_table_remove (manager->p->actions, actions_key);
      g_free (actions_key);
    }
  else
    {
      GimpModifierMapping *mapping;

      mapping = g_slice_new (GimpModifierMapping);
      mapping->modifiers   = modifiers;
      mapping->mod_action  = action;
      mapping->action_desc = action_desc ? g_strdup (action_desc) : NULL;
      g_hash_table_insert (manager->p->actions, actions_key,
                           mapping);
    }
}

void
gimp_modifiers_manager_remove (GimpModifiersManager *manager,
                               GdkDevice            *device,
                               guint                 button,
                               GdkModifierType       modifiers)
{
  gimp_modifiers_manager_set (manager, device, button, modifiers,
                              GIMP_MODIFIER_ACTION_NONE, NULL);
}

void
gimp_modifiers_manager_clear (GimpModifiersManager *manager)
{
  g_hash_table_remove_all (manager->p->actions);
  g_list_free_full (manager->p->buttons, g_free);
  manager->p->buttons = NULL;
}

/* Private functions */

static void
gimp_modifiers_manager_free_mapping (GimpModifierMapping *mapping)
{
  g_free (mapping->action_desc);
  g_slice_free (GimpModifierMapping, mapping);
}

static void
gimp_modifiers_manager_get_keys (GdkDevice        *device,
                                 guint             button,
                                 GdkModifierType   modifiers,
                                 gchar           **actions_key,
                                 gchar           **buttons_key)
{
  const gchar *vendor_id  = NULL;
  const gchar *product_id = NULL;

  g_return_if_fail (GDK_IS_DEVICE (device) || device == NULL);

  if (device && gdk_device_get_device_type (device) != GDK_DEVICE_TYPE_MASTER)
    {
      vendor_id  = gdk_device_get_vendor_id (device);
      product_id = gdk_device_get_product_id (device);
    }

  modifiers  = modifiers & gimp_get_all_modifiers_mask ();

  if (actions_key)
    *actions_key = g_strdup_printf ("%s:%s-%d-%d",
                                    vendor_id ? vendor_id : "(no-vendor-id)",
                                    product_id ? product_id : "(no-product-id)",
                                    button, modifiers);
  if (buttons_key)
    *buttons_key = g_strdup_printf ("%s:%s-%d",
                                    vendor_id ? vendor_id : "(no-vendor-id)",
                                    product_id ? product_id : "(no-product-id)",
                                    button);
}

static void
gimp_modifiers_manager_initialize (GimpModifiersManager *manager,
                                   GdkDevice            *device,
                                   guint                 button)
{
  gchar *buttons_key = NULL;

  g_return_if_fail (GIMP_IS_MODIFIERS_MANAGER (manager));
  g_return_if_fail (GDK_IS_DEVICE (device));

  gimp_modifiers_manager_get_keys (device, button, 0,
                                   NULL, &buttons_key);

  /* Add the button to buttons whether or not we insert or remove an
   * action. It mostly mean that we "touched" the settings for a given
   * device/button. So it's a per-button initialized flag.
   */
  if (g_list_find_custom (manager->p->buttons, buttons_key, (GCompareFunc) g_strcmp0))
    {
      g_free (buttons_key);
    }
  else
    {
      gchar               *actions_key = NULL;
      GimpModifierMapping *mapping;

      manager->p->buttons = g_list_prepend (manager->p->buttons, buttons_key);
      if (button == 2)
        {
          /* The default mapping for second (middle) button which had no explicit configuration. */

          mapping = g_slice_new0 (GimpModifierMapping);
          mapping->modifiers  = GDK_MOD1_MASK;
          mapping->mod_action = GIMP_MODIFIER_ACTION_LAYER_PICKING;
          gimp_modifiers_manager_get_keys (device, 2, mapping->modifiers,
                                           &actions_key, NULL);
          g_hash_table_insert (manager->p->actions, actions_key, mapping);

          mapping = g_slice_new0 (GimpModifierMapping);
          mapping->modifiers  = gimp_get_extend_selection_mask () | GDK_CONTROL_MASK;
          mapping->mod_action = GIMP_MODIFIER_ACTION_STEP_ROTATING;
          gimp_modifiers_manager_get_keys (device, 2, mapping->modifiers,
                                           &actions_key, NULL);
          g_hash_table_insert (manager->p->actions, actions_key, mapping);

          mapping = g_slice_new0 (GimpModifierMapping);
          mapping->modifiers  = gimp_get_extend_selection_mask ();
          mapping->mod_action = GIMP_MODIFIER_ACTION_ROTATING;
          gimp_modifiers_manager_get_keys (device, 2, mapping->modifiers,
                                           &actions_key, NULL);
          g_hash_table_insert (manager->p->actions, actions_key, mapping);

          mapping = g_slice_new0 (GimpModifierMapping);
          mapping->modifiers  = gimp_get_toggle_behavior_mask ();
          mapping->mod_action = GIMP_MODIFIER_ACTION_ZOOMING;
          gimp_modifiers_manager_get_keys (device, 2, mapping->modifiers,
                                           &actions_key, NULL);
          g_hash_table_insert (manager->p->actions, actions_key, mapping);

          mapping = g_slice_new0 (GimpModifierMapping);
          mapping->modifiers  = 0;
          mapping->mod_action = GIMP_MODIFIER_ACTION_PANNING;
          gimp_modifiers_manager_get_keys (device, 2, mapping->modifiers,
                                           &actions_key, NULL);
          g_hash_table_insert (manager->p->actions, actions_key, mapping);
        }
      else if (button == 3)
        {
          /* The default mapping for third button which had no explicit configuration. */

          mapping = g_slice_new0 (GimpModifierMapping);
          mapping->modifiers  = GDK_MOD1_MASK;
          mapping->mod_action = GIMP_MODIFIER_ACTION_BRUSH_PIXEL_SIZE;
          gimp_modifiers_manager_get_keys (device, 3, mapping->modifiers,
                                           &actions_key, NULL);
          g_hash_table_insert (manager->p->actions, actions_key, mapping);

          mapping = g_slice_new0 (GimpModifierMapping);
          mapping->modifiers  = 0;
          mapping->mod_action = GIMP_MODIFIER_ACTION_MENU;
          gimp_modifiers_manager_get_keys (device, 3, mapping->modifiers,
                                           &actions_key, NULL);
          g_hash_table_insert (manager->p->actions, actions_key, mapping);
        }
    }
}
