/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-params.h"
#include "config/gimpconfig-utils.h"
#include "config/gimpconfigwriter.h"
#include "config/gimpscanner.h"

#include "core/gimp.h"

#include "gimpcontrollerinfo.h"
#include "gimpcontrollers.h"
#include "gimpcontrollerwheel.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


#define GIMP_CONTROLLER_MANAGER_DATA_KEY "gimp-controller-manager"


typedef struct _GimpControllerManager GimpControllerManager;

struct _GimpControllerManager
{
  GList          *controllers;
  GimpController *wheel;
  GimpUIManager  *ui_manager;
};


/*  local function prototypes  */

static GimpControllerManager * gimp_controller_manager_get  (Gimp *gimp);
static void   gimp_controller_manager_free (GimpControllerManager *manager);

static gboolean gimp_controller_info_event (GimpController            *controller,
                                            const GimpControllerEvent *event,
                                            GimpControllerInfo        *info);

static GTokenType  gimp_controller_deserialize (GimpControllerManager *manager,
                                                GScanner              *scanner);


/*  public functions  */

void
gimp_controllers_init (Gimp *gimp)
{
  GimpControllerManager *manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimp_controller_manager_get (gimp) == NULL);

  manager = g_new0 (GimpControllerManager, 1);

  g_object_set_data_full (G_OBJECT (gimp),
                          GIMP_CONTROLLER_MANAGER_DATA_KEY, manager,
                          (GDestroyNotify) gimp_controller_manager_free);

  /*  EEEEEEK  */
  {
    static const GInterfaceInfo config_iface_info =
    {
      NULL,  /* iface_init     */
      NULL,  /* iface_finalize */
      NULL   /* iface_data     */
    };

    g_type_add_interface_static (GIMP_TYPE_CONTROLLER, GIMP_TYPE_CONFIG,
                                 &config_iface_info);
  }

  g_type_class_ref (GIMP_TYPE_CONTROLLER_WHEEL);
}

void
gimp_controllers_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimp_controller_manager_get (gimp) != NULL);

  g_object_set_data (G_OBJECT (gimp), GIMP_CONTROLLER_MANAGER_DATA_KEY, NULL);

  g_type_class_unref (g_type_class_peek (GIMP_TYPE_CONTROLLER_WHEEL));
}

enum
{
  CONTROLLER,
  CONTROLLER_OPTIONS,
  CONTROLLER_MAPPING
};

void
gimp_controllers_restore (Gimp          *gimp,
                          GimpUIManager *ui_manager)
{
  GimpControllerManager *manager;
  gchar                 *filename;
  GScanner              *scanner;
  GTokenType             token;
  GError                *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_UI_MANAGER (ui_manager));

  manager = gimp_controller_manager_get (gimp);

  g_return_if_fail (manager != NULL);
  g_return_if_fail (manager->ui_manager == NULL);

  manager->ui_manager = g_object_ref (ui_manager);

  filename = gimp_personal_rc_file ("controllerrc");

  scanner = gimp_scanner_new_file (filename, &error);

  if (! scanner)
    {
      g_clear_error (&error);
      g_free (filename);
      return;
    }

  g_scanner_scope_add_symbol (scanner, 0, "controller",
                              GINT_TO_POINTER (CONTROLLER));
  g_scanner_scope_add_symbol (scanner, CONTROLLER, "options",
                              GINT_TO_POINTER (CONTROLLER_OPTIONS));
  g_scanner_scope_add_symbol (scanner, CONTROLLER, "mapping",
                              GINT_TO_POINTER (CONTROLLER_MAPPING));

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
          if (scanner->value.v_symbol == GINT_TO_POINTER (CONTROLLER))
            {
              g_scanner_set_scope (scanner, CONTROLLER);
              token = gimp_controller_deserialize (manager, scanner);

              if (token == G_TOKEN_RIGHT_PAREN)
                g_scanner_set_scope (scanner, 0);
              else
                break;
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

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
    }

  if (error)
    {
      g_message (error->message);
      g_clear_error (&error);

      gimp_config_file_backup_on_error (filename, "controllerrc", NULL);
    }

  gimp_scanner_destroy (scanner);
  g_free (filename);

  manager->controllers = g_list_reverse (manager->controllers);
}

void
gimp_controllers_save (Gimp *gimp)
{
  GimpControllerManager *manager;
  gchar                 *filename;
  GError                *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp_controller_manager_get (gimp);

  g_return_if_fail (manager != NULL);

  filename = gimp_personal_rc_file ("controllerrc");
  g_free (filename);
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

/*  private functions  */

static GimpControllerManager *
gimp_controller_manager_get (Gimp *gimp)
{
  return g_object_get_data (G_OBJECT (gimp), GIMP_CONTROLLER_MANAGER_DATA_KEY);
}

static void
gimp_controller_manager_free (GimpControllerManager *manager)
{
  g_list_foreach (manager->controllers, (GFunc) g_object_unref, NULL);
  g_list_free (manager->controllers);

  g_object_unref (manager->ui_manager);

  g_free (manager);
}

static GTokenType
gimp_controller_deserialize (GimpControllerManager *manager,
                             GScanner              *scanner)
{
  GimpControllerInfo *info = NULL;
  GTokenType          token;
  gchar              *controller_name;
  GType               controller_type;

  token = G_TOKEN_STRING;

  if (! gimp_scanner_parse_string (scanner, &controller_name))
    goto error;

  controller_type = g_type_from_name (controller_name);
  g_free (controller_name);

  if (! g_type_is_a (controller_type, GIMP_TYPE_CONTROLLER))
    goto error;

  info = g_object_new (GIMP_TYPE_CONTROLLER_INFO, NULL);

  info->controller = gimp_controller_new (controller_type);

  /* EEEEEK */
  {
    GParamSpec **props;
    gint         n_props;
    gint         i;

    props = g_object_class_list_properties (g_type_class_peek (controller_type),
                                            &n_props);

    for (i = 0; i < n_props; i++)
      props[i]->flags |= GIMP_PARAM_SERIALIZE;

    g_free (props);
  }

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
            case CONTROLLER_OPTIONS:
              {
                GimpConfigInterface *config_iface;

                config_iface = GIMP_CONFIG_GET_INTERFACE (info->controller);

                if (! config_iface->deserialize (GIMP_CONFIG (info->controller),
                                                 scanner,
                                                 1,
                                                 FALSE))
                  {
                    goto error;
                  }
              }
              break;

            case CONTROLLER_MAPPING:
              {
                GtkAction *action = NULL;
                GList     *list;
                gchar     *event_name;
                gchar     *action_name;

                token = G_TOKEN_INT;
                if (! gimp_scanner_parse_string (scanner, &event_name))
                  goto error;

                token = G_TOKEN_STRING;
                if (! gimp_scanner_parse_string (scanner, &action_name))
                  goto error;

                for (list = gtk_ui_manager_get_action_groups (GTK_UI_MANAGER (manager->ui_manager));
                     list;
                     list = g_list_next (list))
                  {
                    GtkActionGroup *group = list->data;

                    action = gtk_action_group_get_action (group, action_name);

                    if (action)
                      break;
                  }

                if (action)
                  {
                    g_hash_table_insert (info->mapping, event_name,
                                         g_object_ref (action));
                  }
                else
                  {
                    g_free (event_name);
                    g_printerr ("%s: action '%s' not found\n",
                                G_STRFUNC, action_name);
                  }

                g_free (action_name);
              }
              break;

            default:
              break;
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

  if (token == G_TOKEN_LEFT_PAREN)
    {
      token = G_TOKEN_RIGHT_PAREN;

      if (g_scanner_peek_next_token (scanner) == token)
        {
          g_signal_connect (info->controller, "event",
                            G_CALLBACK (gimp_controller_info_event),
                            info);

          if (GIMP_IS_CONTROLLER_WHEEL (info->controller))
            manager->wheel = info->controller;

          manager->controllers = g_list_prepend (manager->controllers, info);
        }
      else
        {
          goto error;
        }
    }
  else
    {
    error:
      if (info)
        g_object_unref (info);
    }

  return token;
}

static gboolean
gimp_controller_info_event (GimpController            *controller,
                            const GimpControllerEvent *event,
                            GimpControllerInfo        *info)
{
  GtkAction   *action;
  const gchar *class_name;
  const gchar *event_name;
  const gchar *event_blurb;

  class_name = GIMP_CONTROLLER_GET_CLASS (controller)->name;

  event_name = gimp_controller_get_event_name (controller, event->any.event_id);
  event_blurb = gimp_controller_get_event_blurb (controller, event->any.event_id);

  g_print ("Received '%s' (class '%s')\n"
           "    controller event '%s (%s)'\n",
           controller->name, class_name,
           event_name, event_blurb);

  action = g_hash_table_lookup (info->mapping, event_name);

  if (action)
    {
      g_print ("    handled by action '%s'\n\n", gtk_action_get_name (action));
      gtk_action_activate (action);
      return TRUE;
    }
  else
    {
      g_print ("    not handled\n\n");
    }

  return FALSE;
}
