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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#define GIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libgimpwidgets/gimpcontroller.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimplist.h"

#include "gimpaction.h"
#include "gimpactiongroup.h"
#include "gimpcontrollercategory.h"
#include "gimpcontrollerinfo.h"
#include "gimpcontrollerkeyboard.h"
#include "gimpcontrollermanager.h"
#include "gimpcontrollerwheel.h"
#include "gimpenumaction.h"
#include "gimpuimanager.h"


struct _GimpControllerManager
{
  GObject         parent_instance;

  Gimp           *gimp;

  GimpContainer  *controllers;
  GQuark          event_mapped_id;
  GimpController *wheel;
  GimpController *keyboard;
  GimpUIManager  *ui_manager;
};


/*  local function prototypes  */

static void       gimp_controller_manager_finalize      (GObject                   *object);
static void       gimp_controller_manager_dispose       (GObject                   *object);
static gboolean   gimp_controller_manager_event_mapped  (GimpControllerInfo        *info,
                                                         GimpController            *controller,
                                                         const GimpControllerEvent *event,
                                                         const gchar               *action_name,
                                                         GimpControllerManager     *manager);

static void       g_list_model_iface_init               (GListModelInterface       *iface);


G_DEFINE_TYPE_WITH_CODE (GimpControllerManager, gimp_controller_manager,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
                                                g_list_model_iface_init))

#define parent_class gimp_controller_manager_parent_class


static void
gimp_controller_manager_class_init (GimpControllerManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = gimp_controller_manager_dispose;
  object_class->finalize = gimp_controller_manager_finalize;
}

static void
gimp_controller_manager_init (GimpControllerManager *self)
{
  self->controllers = gimp_list_new (GIMP_TYPE_CONTROLLER_INFO, TRUE);

  g_type_class_ref (GIMP_TYPE_CONTROLLER_WHEEL);
  g_type_class_ref (GIMP_TYPE_CONTROLLER_KEYBOARD);
}

static void
gimp_controller_manager_finalize (GObject *object)
{
  g_type_class_unref (g_type_class_peek (GIMP_TYPE_CONTROLLER_WHEEL));
  g_type_class_unref (g_type_class_peek (GIMP_TYPE_CONTROLLER_KEYBOARD));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_controller_manager_dispose (GObject *object)
{
  GimpControllerManager *self = GIMP_CONTROLLER_MANAGER (object);

  g_clear_object (&self->controllers);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */

GimpControllerManager *
gimp_controller_manager_new (Gimp *gimp)
{
  GimpControllerManager *self;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  self = g_object_new (GIMP_TYPE_CONTROLLER_MANAGER, NULL);

  self->gimp = gimp;

  return self;
}

void
gimp_controller_manager_restore (GimpControllerManager *self,
                                 GimpUIManager         *ui_manager)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_CONTROLLER_MANAGER (self));
  g_return_if_fail (GIMP_IS_UI_MANAGER (ui_manager));
  g_return_if_fail (self->ui_manager == NULL);

  self->ui_manager = ui_manager;

  file = gimp_directory_file ("controllerrc", NULL);

  if (self->gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  if (! gimp_config_deserialize_file (GIMP_CONFIG (self->controllers),
                                      file, NULL, &error))
    {
      if (error->code == GIMP_CONFIG_ERROR_OPEN_ENOENT)
        {
          g_clear_error (&error);
          g_object_unref (file);

          if (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"))
            {
              gchar *path;
              path = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                                       "etc", "controllerrc", NULL);
              file = g_file_new_for_path (path);
              g_free (path);
            }
          else
            {
              file = gimp_sysconf_directory_file ("controllerrc", NULL);
            }

          if (! gimp_config_deserialize_file (GIMP_CONFIG (self->controllers),
                                              file, NULL, &error))
            {
              gimp_message_literal (self->gimp, NULL, GIMP_MESSAGE_ERROR,
                                    error->message);
            }
        }
      else
        {
          gimp_message_literal (self->gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
        }

      g_clear_error (&error);
    }

  gimp_list_reverse (GIMP_LIST (self->controllers));

  g_object_unref (file);
}

void
gimp_controller_manager_save (GimpControllerManager *self)
{
  const gchar *header =
    "GIMP controllerrc\n"
    "\n"
    "This file will be entirely rewritten each time you exit.";
  const gchar *footer =
    "end of controllerrc";

  GFile                 *file;
  GError                *error = NULL;

  g_return_if_fail (GIMP_IS_CONTROLLER_MANAGER (self));

  file = gimp_directory_file ("controllerrc", NULL);

  if (self->gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  if (! gimp_config_serialize_to_file (GIMP_CONFIG (self->controllers),
                                       file,
                                       header, footer, NULL,
                                       &error))
    {
      gimp_message_literal (self->gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);
}

Gimp *
gimp_controller_manager_get_gimp (GimpControllerManager *self)
{
  g_return_val_if_fail (GIMP_IS_CONTROLLER_MANAGER (self), NULL);

  return self->gimp;
}

GimpUIManager *
gimp_controller_manager_get_ui_manager (GimpControllerManager *self)
{
  g_return_val_if_fail (GIMP_IS_CONTROLLER_MANAGER (self), NULL);

  return self->ui_manager;
}

GimpController *
gimp_controller_manager_get_wheel (GimpControllerManager *self)
{
  g_return_val_if_fail (GIMP_IS_CONTROLLER_MANAGER (self), NULL);

  return self->wheel;
}

GimpController *
gimp_controller_manager_get_keyboard (GimpControllerManager *self)
{
  g_return_val_if_fail (GIMP_IS_CONTROLLER_MANAGER (self), NULL);

  return self->keyboard;
}

gboolean
gimp_controller_manager_find (GimpControllerManager *self,
                              GimpControllerInfo    *info,
                              guint                 *position)
{
  gint index;

  g_return_val_if_fail (GIMP_IS_CONTROLLER_MANAGER (self), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTROLLER_INFO (info), FALSE);

  index = gimp_container_get_child_index (self->controllers, GIMP_OBJECT (info));

  if (index < 0)
      return FALSE;

  if (position)
      *position = (guint) index;
  return TRUE;
}

void
gimp_controller_manager_add (GimpControllerManager *self,
                             GimpControllerInfo    *info)
{
  g_return_if_fail (GIMP_IS_CONTROLLER_MANAGER (self));
  g_return_if_fail (GIMP_IS_CONTROLLER_INFO (info));

  gimp_container_add (self->controllers, GIMP_OBJECT (info));

  g_signal_connect_object (info, "event-mapped",
                           G_CALLBACK (gimp_controller_manager_event_mapped),
                           G_OBJECT (self), 0);

  if (GIMP_IS_CONTROLLER_WHEEL (info->controller))
    self->wheel = info->controller;
  else if (GIMP_IS_CONTROLLER_KEYBOARD (info->controller))
    self->keyboard = info->controller;

  g_list_model_items_changed (G_LIST_MODEL (self),
                              gimp_container_get_n_children (self->controllers) - 1,
                              0, 1);
}

void
gimp_controller_manager_remove (GimpControllerManager *self,
                                GimpControllerInfo    *info)
{
  gboolean found;
  guint index;

  g_return_if_fail (GIMP_IS_CONTROLLER_MANAGER (self));
  g_return_if_fail (GIMP_IS_CONTROLLER_INFO (info));

  found = gimp_controller_manager_find (self, info, &index);
  g_return_if_fail (found);

  if (info->controller == self->wheel)
    self->wheel = NULL;
  else if (info->controller == self->keyboard)
    self->keyboard = NULL;

  gimp_container_remove (self->controllers, GIMP_OBJECT (info));

  g_list_model_items_changed (G_LIST_MODEL (self), index, 1, 0);
}

void
gimp_controller_manager_move (GimpControllerManager *self,
                              GimpControllerInfo    *info,
                              guint                  new_position)
{
  gboolean found;
  guint    old_position;
  gint     diff;

  g_return_if_fail (GIMP_IS_CONTROLLER_MANAGER (self));
  g_return_if_fail (GIMP_IS_CONTROLLER_INFO (info));

  found = gimp_controller_manager_find (self, info, &old_position);
  g_return_if_fail (found);

  if (new_position == old_position)
      return;

  gimp_container_reorder (self->controllers,
                          GIMP_OBJECT (info),
                          new_position);

  diff = (old_position - (int) new_position);
  g_list_model_items_changed (G_LIST_MODEL (self),
                              MIN (old_position, new_position),
                              ABS (diff) + 1,
                              ABS (diff) + 1);
}

GListModel *
gimp_controller_manager_get_categories (GimpControllerManager *self)
{
  GListStore *categories;
  GType      *controller_types;
  guint       n_controller_types;

  g_return_val_if_fail (GIMP_IS_CONTROLLER_MANAGER (self), NULL);

  categories = g_list_store_new (GIMP_TYPE_CONTROLLER_CATEGORY);

  controller_types = g_type_children (GIMP_TYPE_CONTROLLER,
                                      &n_controller_types);

  for (guint i = 0; i < n_controller_types; i++)
    {
      GimpControllerCategory *category;

      category = gimp_controller_category_new (controller_types[i]);
      g_list_store_append (G_LIST_STORE (categories), category);
      g_object_unref (category);
    }

  g_free (controller_types);

  return G_LIST_MODEL (categories);
}

/*  private functions  */

static gboolean
gimp_controller_manager_event_mapped (GimpControllerInfo        *info,
                                      GimpController            *controller,
                                      const GimpControllerEvent *event,
                                      const gchar               *action_name,
                                      GimpControllerManager     *manager)
{
  GList *list;

  for (list = gimp_ui_manager_get_action_groups (manager->ui_manager);
       list;
       list = g_list_next (list))
    {
      GimpActionGroup *group = list->data;
      GimpAction      *action;

      action = gimp_action_group_get_action (group, action_name);

      if (action)
        {
          switch (event->type)
            {
            case GIMP_CONTROLLER_EVENT_VALUE:
              if (G_VALUE_HOLDS_DOUBLE (&event->value.value) &&
                  GIMP_IS_ENUM_ACTION (action)               &&
                  GIMP_ENUM_ACTION (action)->value_variable)
                {
                  gdouble value = g_value_get_double (&event->value.value);

                  gimp_action_emit_activate (GIMP_ACTION (action),
                                             g_variant_new_int32 (value * 1000));

                  break;
                }
              /* else fallthru */

            case GIMP_CONTROLLER_EVENT_TRIGGER:
            default:
              gimp_action_activate (action);
              break;
            }

          return TRUE;
        }
    }

  return FALSE;
}

/* GListModel implementation */

static GType
gimp_controller_manager_get_item_type (GListModel *list)
{
  return GIMP_TYPE_CONTROLLER_INFO;
}

static guint
gimp_controller_manager_get_n_items (GListModel *list)
{
  GimpControllerManager *self = GIMP_CONTROLLER_MANAGER (list);
  return gimp_container_get_n_children (self->controllers);
}

static void *
gimp_controller_manager_get_item (GListModel *list,
                                  guint       index)
{
  GimpControllerManager *self = GIMP_CONTROLLER_MANAGER (list);

  if (index >= gimp_container_get_n_children (self->controllers))
    return NULL;
  return g_object_ref (gimp_container_get_child_by_index (self->controllers, index));
}

static void
g_list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = gimp_controller_manager_get_item_type;
  iface->get_n_items   = gimp_controller_manager_get_n_items;
  iface->get_item      = gimp_controller_manager_get_item;
}
