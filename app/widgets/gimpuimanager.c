/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpuimanager.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "core/gimp.h"

#include "gimpactiongroup.h"
#include "gimpuimanager.h"


enum
{
  PROP_0,
  PROP_GIMP
};


static void   gimp_ui_manager_init         (GimpUIManager      *manager);
static void   gimp_ui_manager_class_init   (GimpUIManagerClass *klass);

static void   gimp_ui_manager_finalize     (GObject            *object);
static void   gimp_ui_manager_set_property (GObject            *object,
                                            guint               prop_id,
                                            const GValue       *value,
                                            GParamSpec         *pspec);
static void   gimp_ui_manager_get_property (GObject            *object,
                                            guint               prop_id,
                                            GValue             *value,
                                            GParamSpec         *pspec);


static GtkUIManagerClass *parent_class = NULL;


GType
gimp_ui_manager_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GimpUIManagerClass),
	NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_ui_manager_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpUIManager),
        0, /* n_preallocs */
        (GInstanceInitFunc) gimp_ui_manager_init,
      };

      type = g_type_register_static (GTK_TYPE_UI_MANAGER,
                                     "GimpUIManager",
				     &type_info, 0);
    }

  return type;
}

static void
gimp_ui_manager_class_init (GimpUIManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_ui_manager_finalize;
  object_class->set_property = gimp_ui_manager_set_property;
  object_class->get_property = gimp_ui_manager_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_ui_manager_init (GimpUIManager *manager)
{
  manager->gimp = NULL;
}

static void
gimp_ui_manager_finalize (GObject *object)
{
  GimpUIManager *manager = GIMP_UI_MANAGER (object);
  GList         *list;

  for (list = manager->registered_uis; list; list = g_list_next (list))
    {
      GimpUIManagerUIEntry *entry = list->data;

      g_free (entry->identifier);
      g_free (entry->basename);

      if (entry->widget)
        g_object_unref (entry->widget);

      g_free (entry);
    }

  g_list_free (manager->registered_uis);
  manager->registered_uis = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_ui_manager_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpUIManager *manager = GIMP_UI_MANAGER (object);

  switch (prop_id)
    {
    case PROP_GIMP:
      manager->gimp = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_ui_manager_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpUIManager *manager = GIMP_UI_MANAGER (object);

  switch (prop_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, manager->gimp);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * gimp_ui_manager_new:
 * @gimp: the @Gimp instance this ui manager belongs to
 *
 * Creates a new #GimpUIManager object.
 *
 * Returns: the new #GimpUIManager
 */
GimpUIManager *
gimp_ui_manager_new (Gimp *gimp)
{
  GimpUIManager *manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  manager = g_object_new (GIMP_TYPE_UI_MANAGER,
                          "gimp", gimp,
                          NULL);

  return manager;
}

void
gimp_ui_manager_update (GimpUIManager *manager,
                        gpointer       update_data)
{
  GList *list;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));

  for (list = gtk_ui_manager_get_action_groups (GTK_UI_MANAGER (manager));
           list;
           list = g_list_next (list))
    {
      gimp_action_group_update (list->data, update_data);
    }
}

void
gimp_ui_manager_ui_register (GimpUIManager *manager,
                             const gchar   *identifier,
                             const gchar   *basename)
{
  GimpUIManagerUIEntry *entry;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (basename != NULL);

  entry = g_new0 (GimpUIManagerUIEntry, 1);

  entry->identifier = g_strdup (identifier);
  entry->basename   = g_strdup (basename);
  entry->merge_id   = 0;
  entry->widget     = NULL;

  manager->registered_uis = g_list_prepend (manager->registered_uis, entry);

  {
    gchar  *filename;
    GError *error = NULL;

    filename = g_build_filename (gimp_data_directory (), "menus",
                                 entry->basename, NULL);

    g_print ("loading menu: %s for %s\n", filename,
             entry->identifier);

    entry->merge_id =
      gtk_ui_manager_add_ui_from_file (GTK_UI_MANAGER (manager),
                                       filename, &error);

    g_free (filename);

    if (! entry->merge_id)
      {
        g_message (error->message);
        g_clear_error (&error);
      }
  }
}

GtkWidget *
gimp_ui_manager_ui_get (GimpUIManager *manager,
                        const gchar   *identifier)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  for (list = manager->registered_uis; list; list = g_list_next (list))
    {
      GimpUIManagerUIEntry *entry = list->data;

      if (! strcmp (entry->identifier, identifier))
        {
          if (! entry->merge_id)
            {
              gchar  *filename;
              GError *error = NULL;

              filename = g_build_filename (gimp_data_directory (), "menus",
                                           entry->basename, NULL);

              g_print ("loading menu: %s for %s\n", filename,
                       entry->identifier);

              entry->merge_id =
                gtk_ui_manager_add_ui_from_file (GTK_UI_MANAGER (manager),
                                                 filename, &error);

              g_free (filename);

              if (! entry->merge_id)
                {
                  g_message (error->message);
                  g_clear_error (&error);

                  return NULL;
                }
            }

          if (! entry->widget)
            {
              entry->widget =
                gtk_ui_manager_get_widget (GTK_UI_MANAGER (manager),
                                           entry->identifier);

              if (entry->widget)
                g_object_ref (entry->widget);
            }

          return entry->widget;
        }
    }

  g_warning ("%s: no entry registered for \"%s\"",
             G_STRFUNC, identifier);

  return NULL;
}

static void
gimp_ui_manager_menu_position (GtkMenu  *menu,
                               gint     *x,
                               gint     *y,
                               gpointer  data)
{
  GdkScreen      *screen;
  GtkRequisition  requisition;
  GdkRectangle    rect;
  gint            monitor;
  gint            pointer_x;
  gint            pointer_y;

  g_return_if_fail (GTK_IS_MENU (menu));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);
  g_return_if_fail (GTK_IS_WIDGET (data));

  gdk_display_get_pointer (gtk_widget_get_display (GTK_WIDGET (data)),
                           &screen, &pointer_x, &pointer_y, NULL);

  monitor = gdk_screen_get_monitor_at_point (screen, pointer_x, pointer_y);
  gdk_screen_get_monitor_geometry (screen, monitor, &rect);

  gtk_menu_set_screen (menu, screen);

  gtk_widget_size_request (GTK_WIDGET (menu), &requisition);

  if (gtk_widget_get_direction (GTK_WIDGET (menu)) == GTK_TEXT_DIR_RTL)
    {
      *x = pointer_x - 2 - requisition.width;

      if (*x < rect.x)
        *x = pointer_x + 2;
    }
  else
    {
      *x = pointer_x + 2;

      if (*x + requisition.width > rect.x + rect.width)
        *x = pointer_x - 2 - requisition.width;
    }

  *y = pointer_y + 2;

  if (*y + requisition.height > rect.y + rect.height)
    *y = pointer_y - 2 - requisition.height;

  if (*x < rect.x) *x = rect.x;
  if (*y < rect.y) *y = rect.y;
}

typedef struct
{
  guint x;
  guint y;
} MenuPos;

static void
gimp_ui_manager_menu_pos (GtkMenu  *menu,
                          gint     *x,
                          gint     *y,
                          gboolean *push_in,
                          gpointer  data)
{
  MenuPos *menu_pos = data;

  *x = menu_pos->x;
  *y = menu_pos->y;
}

static void
gimp_ui_manager_delete_popup_data (GtkObject     *object,
                                   GimpUIManager *manager)
{
  g_signal_handlers_disconnect_by_func (object,
                                        gimp_ui_manager_delete_popup_data,
                                        manager);
  g_object_set_data (G_OBJECT (manager), "popup-data", NULL);
}

void
gimp_ui_manager_ui_popup (GimpUIManager        *manager,
                          const gchar          *ui_path,
                          gpointer              popup_data,
                          GtkWidget            *parent,
                          GimpMenuPositionFunc  position_func,
                          gpointer              position_data,
                          GtkDestroyNotify      popdown_func)
{
  GtkWidget *widget;
  GdkEvent  *current_event;
  gint       x, y;
  guint      button;
  guint32    activate_time;
  MenuPos   *menu_pos;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);
  g_return_if_fail (parent == NULL || GTK_IS_WIDGET (parent));

  widget = gimp_ui_manager_ui_get (manager, ui_path);

  g_return_if_fail (GTK_IS_MENU (widget));

  if (! position_func)
    {
      position_func = gimp_ui_manager_menu_position;
      position_data = parent;
    }

  (* position_func) (GTK_MENU (widget), &x, &y, position_data);

  current_event = gtk_get_current_event ();

  if (current_event && current_event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *bevent = (GdkEventButton *) current_event;

      button        = bevent->button;
      activate_time = bevent->time;
    }
  else
    {
      button        = 0;
      activate_time = 0;
    }

  menu_pos = g_object_get_data (G_OBJECT (widget), "menu-pos");

  if (! menu_pos)
    {
      menu_pos = g_new0 (MenuPos, 1);
      g_object_set_data_full (G_OBJECT (widget), "menu-pos", menu_pos, g_free);
    }

  menu_pos->x = x;
  menu_pos->y = y;

  if (popup_data != NULL)
    {
      g_object_set_data_full (G_OBJECT (manager), "popup-data",
                              popup_data, popdown_func);
      g_signal_connect (widget, "selection-done",
                        G_CALLBACK (gimp_ui_manager_delete_popup_data),
                        manager);
    }

  gtk_menu_popup (GTK_MENU (widget),
                  NULL, NULL,
                  gimp_ui_manager_menu_pos, menu_pos,
                  button, activate_time);
}
