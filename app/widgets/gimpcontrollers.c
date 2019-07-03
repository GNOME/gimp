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
#include "gimpcontrollerinfo.h"
#include "gimpcontrollers.h"
#include "gimpcontrollerkeyboard.h"
#include "gimpcontrollermouse.h"
#include "gimpcontrollerwheel.h"
#include "gimpenumaction.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


#define GIMP_CONTROLLER_MANAGER_DATA_KEY "gimp-controller-manager"


typedef struct _GimpControllerManager GimpControllerManager;

struct _GimpControllerManager
{
  GimpContainer  *controllers;
  GQuark          event_mapped_id;
  GimpController *mouse;
  GimpController *wheel;
  GimpController *keyboard;
  GimpUIManager  *ui_manager;
};


/*  local function prototypes  */

static GimpControllerManager * gimp_controller_manager_get  (Gimp *gimp);
static void   gimp_controller_manager_free (GimpControllerManager *manager);

static void   gimp_controllers_add         (GimpContainer         *container,
                                            GimpControllerInfo    *info,
                                            GimpControllerManager *manager);
static void   gimp_controllers_remove      (GimpContainer         *container,
                                            GimpControllerInfo    *info,
                                            GimpControllerManager *manager);

static gboolean gimp_controllers_event_mapped (GimpControllerInfo        *info,
                                               GimpController            *controller,
                                               const GimpControllerEvent *event,
                                               const gchar               *action_name,
                                               GimpControllerManager     *manager);


/*  public functions  */

void
gimp_controllers_init (Gimp *gimp)
{
  GimpControllerManager *manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimp_controller_manager_get (gimp) == NULL);

  manager = g_slice_new0 (GimpControllerManager);

  g_object_set_data_full (G_OBJECT (gimp),
                          GIMP_CONTROLLER_MANAGER_DATA_KEY, manager,
                          (GDestroyNotify) gimp_controller_manager_free);

  manager->controllers = gimp_list_new (GIMP_TYPE_CONTROLLER_INFO, TRUE);

  g_signal_connect (manager->controllers, "add",
                    G_CALLBACK (gimp_controllers_add),
                    manager);
  g_signal_connect (manager->controllers, "remove",
                    G_CALLBACK (gimp_controllers_remove),
                    manager);

  manager->event_mapped_id =
    gimp_container_add_handler (manager->controllers, "event-mapped",
                                G_CALLBACK (gimp_controllers_event_mapped),
                                manager);

  g_type_class_ref (GIMP_TYPE_CONTROLLER_MOUSE);
  g_type_class_ref (GIMP_TYPE_CONTROLLER_WHEEL);
  g_type_class_ref (GIMP_TYPE_CONTROLLER_KEYBOARD);
}

void
gimp_controllers_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimp_controller_manager_get (gimp) != NULL);

  g_object_set_data (G_OBJECT (gimp), GIMP_CONTROLLER_MANAGER_DATA_KEY, NULL);

  g_type_class_unref (g_type_class_peek (GIMP_TYPE_CONTROLLER_WHEEL));
  g_type_class_unref (g_type_class_peek (GIMP_TYPE_CONTROLLER_KEYBOARD));
}

void
gimp_controllers_restore (Gimp          *gimp,
                          GimpUIManager *ui_manager)
{
  GimpControllerManager *manager;
  GFile                 *file;
  GError                *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_UI_MANAGER (ui_manager));

  manager = gimp_controller_manager_get (gimp);

  g_return_if_fail (manager != NULL);
  g_return_if_fail (manager->ui_manager == NULL);

  manager->ui_manager = g_object_ref (ui_manager);

  file = gimp_directory_file ("controllerrc", NULL);

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  if (! gimp_config_deserialize_gfile (GIMP_CONFIG (manager->controllers),
                                       file, NULL, &error))
    {
      if (error->code == GIMP_CONFIG_ERROR_OPEN_ENOENT)
        {
          g_clear_error (&error);
          g_object_unref (file);

          file = gimp_sysconf_directory_file ("controllerrc", NULL);

          if (! gimp_config_deserialize_gfile (GIMP_CONFIG (manager->controllers),
                                               file, NULL, &error))
            {
              gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR,
                                    error->message);
            }
        }
      else
        {
          gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
        }

      g_clear_error (&error);
    }

  gimp_list_reverse (GIMP_LIST (manager->controllers));

  g_object_unref (file);
}

void
gimp_controllers_save (Gimp *gimp)
{
  const gchar *header =
    "GIMP controllerrc\n"
    "\n"
    "This file will be entirely rewritten each time you exit.";
  const gchar *footer =
    "end of controllerrc";

  GimpControllerManager *manager;
  GFile                 *file;
  GError                *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_controller_manager_get (gimp);

  g_return_if_fail (manager != NULL);

  file = gimp_directory_file ("controllerrc", NULL);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  if (! gimp_config_serialize_to_gfile (GIMP_CONFIG (manager->controllers),
                                        file,
                                        header, footer, NULL,
                                        &error))
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);
}

GimpContainer *
gimp_controllers_get_list (Gimp *gimp)
{
  GimpControllerManager *manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  manager = gimp_controller_manager_get (gimp);

  g_return_val_if_fail (manager != NULL, NULL);

  return manager->controllers;
}

GimpUIManager *
gimp_controllers_get_ui_manager (Gimp *gimp)
{
  GimpControllerManager *manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  manager = gimp_controller_manager_get (gimp);

  g_return_val_if_fail (manager != NULL, NULL);

  return manager->ui_manager;
}

GimpController *
gimp_controllers_get_mouse (Gimp *gimp)
{
  GimpControllerManager *manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  manager = gimp_controller_manager_get (gimp);

  g_return_val_if_fail (manager != NULL, NULL);

  return manager->mouse;
}

GimpController *
gimp_controllers_get_wheel (Gimp *gimp)
{
  GimpControllerManager *manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  manager = gimp_controller_manager_get (gimp);

  g_return_val_if_fail (manager != NULL, NULL);

  return manager->wheel;
}

GimpController *
gimp_controllers_get_keyboard (Gimp *gimp)
{
  GimpControllerManager *manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  manager = gimp_controller_manager_get (gimp);

  g_return_val_if_fail (manager != NULL, NULL);

  return manager->keyboard;
}


/*  private functions  */

static GimpControllerManager *
gimp_controller_manager_get (Gimp *gimp)
{
  return g_object_get_data (G_OBJECT (gimp), GIMP_CONTROLLER_MANAGER_DATA_KEY);
}

static void
gimp_controller_manager_free (GimpControllerManager *manager)
{
  gimp_container_remove_handler (manager->controllers,
                                 manager->event_mapped_id);

  g_clear_object (&manager->controllers);
  g_clear_object (&manager->ui_manager);

  g_slice_free (GimpControllerManager, manager);
}

static void
gimp_controllers_add (GimpContainer         *container,
                      GimpControllerInfo    *info,
                      GimpControllerManager *manager)
{
  if (GIMP_IS_CONTROLLER_WHEEL (info->controller))
    manager->wheel = info->controller;
  else if (GIMP_IS_CONTROLLER_KEYBOARD (info->controller))
    manager->keyboard = info->controller;
  else if (GIMP_IS_CONTROLLER_MOUSE (info->controller))
    manager->mouse = info->controller;
}

static void
gimp_controllers_remove (GimpContainer         *container,
                         GimpControllerInfo    *info,
                         GimpControllerManager *manager)
{
  if (info->controller == manager->wheel)
    manager->wheel = NULL;
  else if (info->controller == manager->keyboard)
    manager->keyboard = NULL;
}

static gboolean
gimp_controllers_event_mapped (GimpControllerInfo        *info,
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
