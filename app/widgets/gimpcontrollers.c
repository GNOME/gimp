/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#define LIGMA_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libligmawidgets/ligmacontroller.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmalist.h"

#include "ligmaaction.h"
#include "ligmaactiongroup.h"
#include "ligmacontrollerinfo.h"
#include "ligmacontrollers.h"
#include "ligmacontrollerkeyboard.h"
#include "ligmacontrollerwheel.h"
#include "ligmaenumaction.h"
#include "ligmauimanager.h"

#include "ligma-intl.h"


#define LIGMA_CONTROLLER_MANAGER_DATA_KEY "ligma-controller-manager"


typedef struct _LigmaControllerManager LigmaControllerManager;

struct _LigmaControllerManager
{
  LigmaContainer  *controllers;
  GQuark          event_mapped_id;
  LigmaController *wheel;
  LigmaController *keyboard;
  LigmaUIManager  *ui_manager;
};


/*  local function prototypes  */

static LigmaControllerManager * ligma_controller_manager_get  (Ligma *ligma);
static void   ligma_controller_manager_free (LigmaControllerManager *manager);

static void   ligma_controllers_add         (LigmaContainer         *container,
                                            LigmaControllerInfo    *info,
                                            LigmaControllerManager *manager);
static void   ligma_controllers_remove      (LigmaContainer         *container,
                                            LigmaControllerInfo    *info,
                                            LigmaControllerManager *manager);

static gboolean ligma_controllers_event_mapped (LigmaControllerInfo        *info,
                                               LigmaController            *controller,
                                               const LigmaControllerEvent *event,
                                               const gchar               *action_name,
                                               LigmaControllerManager     *manager);


/*  public functions  */

void
ligma_controllers_init (Ligma *ligma)
{
  LigmaControllerManager *manager;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (ligma_controller_manager_get (ligma) == NULL);

  manager = g_slice_new0 (LigmaControllerManager);

  g_object_set_data_full (G_OBJECT (ligma),
                          LIGMA_CONTROLLER_MANAGER_DATA_KEY, manager,
                          (GDestroyNotify) ligma_controller_manager_free);

  manager->controllers = ligma_list_new (LIGMA_TYPE_CONTROLLER_INFO, TRUE);

  g_signal_connect (manager->controllers, "add",
                    G_CALLBACK (ligma_controllers_add),
                    manager);
  g_signal_connect (manager->controllers, "remove",
                    G_CALLBACK (ligma_controllers_remove),
                    manager);

  manager->event_mapped_id =
    ligma_container_add_handler (manager->controllers, "event-mapped",
                                G_CALLBACK (ligma_controllers_event_mapped),
                                manager);

  g_type_class_ref (LIGMA_TYPE_CONTROLLER_WHEEL);
  g_type_class_ref (LIGMA_TYPE_CONTROLLER_KEYBOARD);
}

void
ligma_controllers_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (ligma_controller_manager_get (ligma) != NULL);

  g_object_set_data (G_OBJECT (ligma), LIGMA_CONTROLLER_MANAGER_DATA_KEY, NULL);

  g_type_class_unref (g_type_class_peek (LIGMA_TYPE_CONTROLLER_WHEEL));
  g_type_class_unref (g_type_class_peek (LIGMA_TYPE_CONTROLLER_KEYBOARD));
}

void
ligma_controllers_restore (Ligma          *ligma,
                          LigmaUIManager *ui_manager)
{
  LigmaControllerManager *manager;
  GFile                 *file;
  GError                *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_UI_MANAGER (ui_manager));

  manager = ligma_controller_manager_get (ligma);

  g_return_if_fail (manager != NULL);
  g_return_if_fail (manager->ui_manager == NULL);

  manager->ui_manager = g_object_ref (ui_manager);

  file = ligma_directory_file ("controllerrc", NULL);

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  if (! ligma_config_deserialize_file (LIGMA_CONFIG (manager->controllers),
                                      file, NULL, &error))
    {
      if (error->code == LIGMA_CONFIG_ERROR_OPEN_ENOENT)
        {
          g_clear_error (&error);
          g_object_unref (file);

          if (g_getenv ("LIGMA_TESTING_ABS_TOP_SRCDIR"))
            {
              gchar *path;
              path = g_build_filename (g_getenv ("LIGMA_TESTING_ABS_TOP_SRCDIR"),
                                       "etc", "controllerrc", NULL);
              file = g_file_new_for_path (path);
              g_free (path);
            }
          else
            {
              file = ligma_sysconf_directory_file ("controllerrc", NULL);
            }

          if (! ligma_config_deserialize_file (LIGMA_CONFIG (manager->controllers),
                                              file, NULL, &error))
            {
              ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR,
                                    error->message);
            }
        }
      else
        {
          ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
        }

      g_clear_error (&error);
    }

  ligma_list_reverse (LIGMA_LIST (manager->controllers));

  g_object_unref (file);
}

void
ligma_controllers_save (Ligma *ligma)
{
  const gchar *header =
    "LIGMA controllerrc\n"
    "\n"
    "This file will be entirely rewritten each time you exit.";
  const gchar *footer =
    "end of controllerrc";

  LigmaControllerManager *manager;
  GFile                 *file;
  GError                *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  manager = ligma_controller_manager_get (ligma);

  g_return_if_fail (manager != NULL);

  file = ligma_directory_file ("controllerrc", NULL);

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  if (! ligma_config_serialize_to_file (LIGMA_CONFIG (manager->controllers),
                                       file,
                                       header, footer, NULL,
                                       &error))
    {
      ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);
}

LigmaContainer *
ligma_controllers_get_list (Ligma *ligma)
{
  LigmaControllerManager *manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  manager = ligma_controller_manager_get (ligma);

  g_return_val_if_fail (manager != NULL, NULL);

  return manager->controllers;
}

LigmaUIManager *
ligma_controllers_get_ui_manager (Ligma *ligma)
{
  LigmaControllerManager *manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  manager = ligma_controller_manager_get (ligma);

  g_return_val_if_fail (manager != NULL, NULL);

  return manager->ui_manager;
}

LigmaController *
ligma_controllers_get_wheel (Ligma *ligma)
{
  LigmaControllerManager *manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  manager = ligma_controller_manager_get (ligma);

  g_return_val_if_fail (manager != NULL, NULL);

  return manager->wheel;
}

LigmaController *
ligma_controllers_get_keyboard (Ligma *ligma)
{
  LigmaControllerManager *manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  manager = ligma_controller_manager_get (ligma);

  g_return_val_if_fail (manager != NULL, NULL);

  return manager->keyboard;
}


/*  private functions  */

static LigmaControllerManager *
ligma_controller_manager_get (Ligma *ligma)
{
  return g_object_get_data (G_OBJECT (ligma), LIGMA_CONTROLLER_MANAGER_DATA_KEY);
}

static void
ligma_controller_manager_free (LigmaControllerManager *manager)
{
  ligma_container_remove_handler (manager->controllers,
                                 manager->event_mapped_id);

  g_clear_object (&manager->controllers);
  g_clear_object (&manager->ui_manager);

  g_slice_free (LigmaControllerManager, manager);
}

static void
ligma_controllers_add (LigmaContainer         *container,
                      LigmaControllerInfo    *info,
                      LigmaControllerManager *manager)
{
  if (LIGMA_IS_CONTROLLER_WHEEL (info->controller))
    manager->wheel = info->controller;
  else if (LIGMA_IS_CONTROLLER_KEYBOARD (info->controller))
    manager->keyboard = info->controller;
}

static void
ligma_controllers_remove (LigmaContainer         *container,
                         LigmaControllerInfo    *info,
                         LigmaControllerManager *manager)
{
  if (info->controller == manager->wheel)
    manager->wheel = NULL;
  else if (info->controller == manager->keyboard)
    manager->keyboard = NULL;
}

static gboolean
ligma_controllers_event_mapped (LigmaControllerInfo        *info,
                               LigmaController            *controller,
                               const LigmaControllerEvent *event,
                               const gchar               *action_name,
                               LigmaControllerManager     *manager)
{
  GList *list;

  for (list = ligma_ui_manager_get_action_groups (manager->ui_manager);
       list;
       list = g_list_next (list))
    {
      LigmaActionGroup *group = list->data;
      LigmaAction      *action;

      action = ligma_action_group_get_action (group, action_name);

      if (action)
        {
          switch (event->type)
            {
            case LIGMA_CONTROLLER_EVENT_VALUE:
              if (G_VALUE_HOLDS_DOUBLE (&event->value.value) &&
                  LIGMA_IS_ENUM_ACTION (action)               &&
                  LIGMA_ENUM_ACTION (action)->value_variable)
                {
                  gdouble value = g_value_get_double (&event->value.value);

                  ligma_action_emit_activate (LIGMA_ACTION (action),
                                             g_variant_new_int32 (value * 1000));

                  break;
                }
              /* else fallthru */

            case LIGMA_CONTROLLER_EVENT_TRIGGER:
            default:
              ligma_action_activate (action);
              break;
            }

          return TRUE;
        }
    }

  return FALSE;
}
