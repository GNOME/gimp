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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "string.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "gimphelp-ids.h"
#include "gimpitemfactory.h"
#include "gimpwidgets-utils.h"

#include "gimphelp.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void     gimp_item_factory_class_init      (GimpItemFactoryClass *klass);
static void     gimp_item_factory_init            (GimpItemFactory      *factory);

static void     gimp_item_factory_finalize        (GObject              *object);
static void     gimp_item_factory_destroy         (GtkObject            *object);

static void     gimp_item_factory_create_branches (GimpItemFactory      *factory,
                                                   GimpItemFactoryEntry *entry,
                                                   const gchar          *textdomain);
static void     gimp_item_factory_item_realize    (GtkWidget            *widget,
                                                   GimpItemFactory      *factory);
static gboolean gimp_item_factory_item_key_press  (GtkWidget            *widget,
                                                   GdkEventKey          *kevent,
                                                   GimpItemFactory      *factory);
static gchar *  gimp_item_factory_translate_func  (const gchar          *path,
                                                   gpointer              data);


static GtkItemFactoryClass *parent_class = NULL;


GType
gimp_item_factory_get_type (void)
{
  static GType factory_type = 0;

  if (! factory_type)
    {
      static const GTypeInfo factory_info =
      {
        sizeof (GimpItemFactoryClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_item_factory_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpItemFactory),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_item_factory_init,
      };

      factory_type = g_type_register_static (GTK_TYPE_ITEM_FACTORY,
					     "GimpItemFactory",
					     &factory_info, 0);
    }

  return factory_type;
}

static void
gimp_item_factory_class_init (GimpItemFactoryClass *klass)
{
  GObjectClass   *object_class;
  GtkObjectClass *gtk_object_class;

  object_class     = G_OBJECT_CLASS (klass);
  gtk_object_class = GTK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize    = gimp_item_factory_finalize;

  gtk_object_class->destroy = gimp_item_factory_destroy;

  klass->factories = g_hash_table_new_full (g_str_hash, g_str_equal,
                                            g_free, NULL);
}

static void
gimp_item_factory_init (GimpItemFactory *factory)
{
  factory->gimp              = NULL;
  factory->update_func       = NULL;
  factory->update_on_popup   = FALSE;
  factory->title             = NULL;
  factory->help_id           = NULL;
  factory->translation_trash = NULL;
}

static void
gimp_item_factory_finalize (GObject *object)
{
  GimpItemFactory *factory;

  factory = GIMP_ITEM_FACTORY (object);

  if (factory->title)
    {
      g_free (factory->title);
      factory->title = NULL;
    }

  if (factory->help_id)
    {
      g_free (factory->help_id);
      factory->help_id = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_item_factory_destroy (GtkObject *object)
{
  GimpItemFactory *factory;
  gchar           *factory_path;

  factory = GIMP_ITEM_FACTORY (object);

  factory_path = GTK_ITEM_FACTORY (object)->path;

  if (factory_path)
    {
      GimpItemFactoryClass *factory_class;
      GList                *list;

      factory_class = GIMP_ITEM_FACTORY_GET_CLASS (factory);

      list = g_hash_table_lookup (factory_class->factories, factory_path);

      if (list)
        {
          list = g_list_remove (list, factory);

          if (list)
            g_hash_table_replace (factory_class->factories,
                                  g_strdup (factory_path),
                                  list);
          else
            g_hash_table_remove (factory_class->factories, factory_path);
        }
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);

  if (GTK_ITEM_FACTORY (factory)->widget)
    g_object_unref (GTK_ITEM_FACTORY (factory)->widget);
}


/*  public functions  */

GimpItemFactory *
gimp_item_factory_new (Gimp                      *gimp,
                       GType                      container_type,
                       const gchar               *factory_path,
                       const gchar               *title,
                       const gchar               *help_id,
                       GimpItemFactoryUpdateFunc  update_func,
                       gboolean                   update_on_popup,
                       guint                      n_entries,
                       GimpItemFactoryEntry      *entries,
                       gpointer                   callback_data,
                       gboolean                   create_tearoff)
{
  GimpItemFactoryClass *factory_class;
  GimpItemFactory      *factory;
  GList                *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (factory_path != NULL, NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (factory_path[0] == '<', NULL);
  g_return_val_if_fail (factory_path[strlen (factory_path) - 1] == '>', NULL);

  factory_class = g_type_class_ref (GIMP_TYPE_ITEM_FACTORY);

  factory = g_object_new (GIMP_TYPE_ITEM_FACTORY, NULL);

  gtk_item_factory_construct (GTK_ITEM_FACTORY (factory),
                              container_type,
                              factory_path,
                              NULL);

  gtk_item_factory_set_translate_func (GTK_ITEM_FACTORY (factory),
				       gimp_item_factory_translate_func,
				       factory,
				       NULL);

  factory->gimp            = gimp;
  factory->update_func     = update_func;
  factory->update_on_popup = update_on_popup;
  factory->title           = g_strdup (title);
  factory->help_id         = g_strdup (help_id);

  list = g_hash_table_lookup (factory_class->factories, factory_path);

  list = g_list_append (list, factory);

  g_hash_table_replace (factory_class->factories,
                        g_strdup (factory_path),
                        list);

  gimp_item_factory_create_items (factory,
                                  n_entries,
                                  entries,
                                  callback_data,
                                  2,
                                  create_tearoff,
                                  TRUE);

  g_type_class_unref (factory_class);

  if (GTK_ITEM_FACTORY (factory)->widget)
    g_object_ref (GTK_ITEM_FACTORY (factory)->widget);

  return factory;
}

GimpItemFactory *
gimp_item_factory_from_path (const gchar *path)
{
  GList *list;

  list = gimp_item_factories_from_path (path);

  if (list)
    return list->data;

  return NULL;
}

GList *
gimp_item_factories_from_path (const gchar *path)
{
  GimpItemFactoryClass *factory_class;
  GList                *list;
  gchar                *base_path;
  gchar                *p;

  g_return_val_if_fail (path != NULL, NULL);

  base_path = g_strdup (path);

  p = strchr (base_path, '>');

  if (p)
    p[1] = '\0';

  factory_class = g_type_class_ref (GIMP_TYPE_ITEM_FACTORY);

  list = g_hash_table_lookup (factory_class->factories, base_path);

  g_type_class_unref (factory_class);

  g_free (base_path);

  return list;
}

void
gimp_item_factory_create_item (GimpItemFactory       *item_factory,
                               GimpItemFactoryEntry  *entry,
                               const gchar           *textdomain,
                               gpointer               callback_data,
                               guint                  callback_type,
                               gboolean               create_tearoff,
                               gboolean               static_entry)
{
  GtkWidget *menu_item;
  gchar     *menu_path;
  gboolean   tearoffs;

  g_return_if_fail (GIMP_IS_ITEM_FACTORY (item_factory));
  g_return_if_fail (entry != NULL);

  tearoffs = GIMP_GUI_CONFIG (item_factory->gimp->config)->tearoff_menus;

  if (! (strstr (entry->entry.path, "tearoff")))
    {
      if (tearoffs && create_tearoff)
	{
	  gimp_item_factory_create_branches (item_factory, entry, textdomain);
	}
    }
  else if (! tearoffs || ! create_tearoff)
    {
      return;
    }

  if (entry->quark_string)
    {
      GQuark quark;

      if (static_entry)
	quark = g_quark_from_static_string (entry->quark_string);
      else
	quark = g_quark_from_string (entry->quark_string);

      entry->entry.callback_action = (guint) quark;
    }

  if (textdomain)
    g_object_set_data (G_OBJECT (item_factory), "textdomain",
                       (gpointer) textdomain);

  gtk_item_factory_create_item (GTK_ITEM_FACTORY (item_factory),
				(GtkItemFactoryEntry *) entry,
				callback_data,
				callback_type);

  if (textdomain)
    g_object_set_data (G_OBJECT (item_factory), "textdomain", NULL);

  if (item_factory->translation_trash)
    {
      g_list_foreach (item_factory->translation_trash, (GFunc) g_free, NULL);
      g_list_free (item_factory->translation_trash);
      item_factory->translation_trash = NULL;
    }

  menu_path = gimp_strip_uline (((GtkItemFactoryEntry *) entry)->path);

  menu_item = gtk_item_factory_get_item (GTK_ITEM_FACTORY (item_factory),
                                         menu_path);

  g_free (menu_path);

  if (menu_item)
    {
      g_signal_connect_after (menu_item, "realize",
			      G_CALLBACK (gimp_item_factory_item_realize),
			      item_factory);

      if (entry->help_id)
        {
          if (static_entry)
            g_object_set_data (G_OBJECT (menu_item), "gimp-help-id",
                               (gpointer) entry->help_id);
          else
            g_object_set_data_full (G_OBJECT (menu_item), "gimp-help-id",
                                    g_strdup (entry->help_id),
                                    g_free);
        }
    }
}

void
gimp_item_factory_create_items (GimpItemFactory      *item_factory,
                                guint                 n_entries,
                                GimpItemFactoryEntry *entries,
                                gpointer              callback_data,
                                guint                 callback_type,
                                gboolean              create_tearoff,
                                gboolean              static_entries)
{
  gint i;

  for (i = 0; i < n_entries; i++)
    {
      gimp_item_factory_create_item (item_factory,
                                     entries + i,
                                     NULL,
                                     callback_data,
                                     callback_type,
                                     create_tearoff,
                                     static_entries);
    }
}

void
gimp_item_factory_update (GimpItemFactory *item_factory,
                          gpointer         popup_data)
{
  g_return_if_fail (GIMP_IS_ITEM_FACTORY (item_factory));

  if (item_factory->update_func)
    item_factory->update_func (GTK_ITEM_FACTORY (item_factory), popup_data);
}

void
gimp_item_factory_popup_with_data (GimpItemFactory      *item_factory,
				   gpointer              popup_data,
                                   GtkWidget            *parent,
                                   GimpMenuPositionFunc  position_func,
                                   gpointer              position_data,
                                   GtkDestroyNotify      popdown_func)
{
  GdkEvent *current_event;
  gint      x, y;
  guint     button;
  guint32   activate_time;

  g_return_if_fail (GIMP_IS_ITEM_FACTORY (item_factory));
  g_return_if_fail (GTK_IS_WIDGET (parent));

  if (item_factory->update_on_popup)
    gimp_item_factory_update (item_factory, popup_data);

  if (! position_func)
    {
      position_func = gimp_menu_position;
      position_data = parent;
    }

  (* position_func) (GTK_MENU (GTK_ITEM_FACTORY (item_factory)->widget),
                     &x, &y, position_data);

  current_event = gtk_get_current_event ();

  if (current_event && current_event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *bevent;

      bevent = (GdkEventButton *) current_event;

      button        = bevent->button;
      activate_time = bevent->time;
    }
  else
    {
      button        = 0;
      activate_time = 0;
    }

  gtk_item_factory_popup_with_data (GTK_ITEM_FACTORY (item_factory),
				    popup_data,
				    popdown_func,
				    x, y,
				    button,
				    activate_time);
}

void
gimp_item_factory_set_active (GtkItemFactory *factory,
                              const gchar    *path,
                              gboolean        active)
{
  GtkWidget *widget;

  g_return_if_fail (GTK_IS_ITEM_FACTORY (factory));
  g_return_if_fail (path != NULL);

  widget = gtk_item_factory_get_widget (factory, path);

  if (widget)
    {
      if (GTK_IS_CHECK_MENU_ITEM (widget))
        {
          gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), active);
        }
      else
        {
          g_warning ("%s: Unable to set \"active\" for menu item "
                     "of type \"%s\": %s",
                     G_STRLOC,
                     g_type_name (G_TYPE_FROM_INSTANCE (widget)),
                     path);
        }
    }
  else if (! strstr (path, "Script-Fu"))
    {
      g_warning ("%s: Unable to set \"active\" for menu item "
                 "which doesn't exist: %s",
                 G_STRLOC, path);
    }
}

void
gimp_item_factories_set_active (const gchar *factory_path,
                                const gchar *path,
                                gboolean     active)
{
  GList *list;

  g_return_if_fail (factory_path != NULL);
  g_return_if_fail (path != NULL);

  for (list = gimp_item_factories_from_path (factory_path);
       list;
       list = g_list_next (list))
    {
      gimp_item_factory_set_active (GTK_ITEM_FACTORY (list->data),
                                    path, active);
    }
}

void
gimp_item_factory_set_color (GtkItemFactory *factory,
                             const gchar    *path,
                             const GimpRGB  *color,
                             gboolean        set_label)
{
  GtkWidget *widget;
  GtkWidget *area  = NULL;
  GtkWidget *label = NULL;
  GdkScreen *screen;

  g_return_if_fail (GTK_IS_ITEM_FACTORY (factory));
  g_return_if_fail (path != NULL);
  g_return_if_fail (color != NULL);

  widget = gtk_item_factory_get_widget (factory, path);

  if (! widget)
    {
      g_warning ("%s: Unable to set color of menu item "
                 "which doesn't exist: %s",
                 G_STRLOC, path);
      return;
    }

  if (GTK_IS_HBOX (GTK_BIN (widget)->child))
    {
      area = g_object_get_data (G_OBJECT (GTK_BIN (widget)->child),
                                "color_area");
      label = g_object_get_data (G_OBJECT (GTK_BIN (widget)->child),
                                 "label");
    }
  else if (GTK_IS_LABEL (GTK_BIN (widget)->child))
    {
      GtkWidget *hbox;
      gint       width, height;

      label = GTK_BIN (widget)->child;

      g_object_ref (label);

      gtk_container_remove (GTK_CONTAINER (widget), label);

      hbox = gtk_hbox_new (FALSE, 4);
      gtk_container_add (GTK_CONTAINER (widget), hbox);
      gtk_widget_show (hbox);

      area = gimp_color_area_new (color, GIMP_COLOR_AREA_SMALL_CHECKS, 0);
      gimp_color_area_set_draw_border (GIMP_COLOR_AREA (area), TRUE);

      screen = gtk_widget_get_screen (area);
      gtk_icon_size_lookup_for_settings (gtk_settings_get_for_screen (screen),
                                         GTK_ICON_SIZE_MENU, &width, &height);

      gtk_widget_set_size_request (area, width, height);
      gtk_box_pack_start (GTK_BOX (hbox), area, FALSE, FALSE, 0);
      gtk_widget_show (area);

      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
      gtk_widget_show (label);

      g_object_unref (label);

      g_object_set_data (G_OBJECT (hbox), "color_area", area);
      g_object_set_data (G_OBJECT (hbox), "label",      label);
    }

  if (area)
    gimp_color_area_set_color (GIMP_COLOR_AREA (area), color);

  if (label && set_label)
    {
      gchar *str;

      str = g_strdup_printf (_("RGBA (%0.3f, %0.3f, %0.3f, %0.3f)"),
                             color->r, color->g, color->b, color->a);

      gtk_label_set_text (GTK_LABEL (label), str);

      g_free (str);
    }
}

void
gimp_item_factories_set_color (const gchar   *factory_path,
                               const gchar   *path,
                               const GimpRGB *color,
                               gboolean       set_label)
{
  GList *list;

  g_return_if_fail (factory_path != NULL);
  g_return_if_fail (path != NULL);
  g_return_if_fail (color != NULL);

  for (list = gimp_item_factories_from_path (factory_path);
       list;
       list = g_list_next (list))
    {
      gimp_item_factory_set_color (GTK_ITEM_FACTORY (list->data),
                                   path, color, set_label);
    }
}

void
gimp_item_factory_set_label (GtkItemFactory *factory,
                             const gchar    *path,
                             const gchar    *label)
{
  GtkWidget *widget;

  g_return_if_fail (GTK_IS_ITEM_FACTORY (factory));
  g_return_if_fail (path != NULL);
  g_return_if_fail (label != NULL);

  widget = gtk_item_factory_get_widget (factory, path);

  if (widget)
    {
      if (GTK_IS_MENU (widget))
        widget = gtk_menu_get_attach_widget (GTK_MENU (widget));

      if (GTK_IS_LABEL (GTK_BIN (widget)->child))
        gtk_label_set_text_with_mnemonic (GTK_LABEL (GTK_BIN (widget)->child),
                                          label);
    }
  else
    {
      g_warning ("%s: Unable to set label of menu item "
                 "which doesn't exist: %s",
                 G_STRLOC, path);
    }
}

void
gimp_item_factories_set_label (const gchar *factory_path,
                               const gchar *path,
                               const gchar *label)
{
  GList *list;

  g_return_if_fail (factory_path != NULL);
  g_return_if_fail (path != NULL);
  g_return_if_fail (label != NULL);

  for (list = gimp_item_factories_from_path (factory_path);
       list;
       list = g_list_next (list))
    {
      gimp_item_factory_set_label (GTK_ITEM_FACTORY (list->data),
                                   path, label);
    }
}

void
gimp_item_factory_set_sensitive (GtkItemFactory *factory,
                                 const gchar    *path,
                                 gboolean        sensitive)
{
  GtkWidget *widget;

  g_return_if_fail (GTK_IS_ITEM_FACTORY (factory));
  g_return_if_fail (path != NULL);

  widget = gtk_item_factory_get_widget (factory, path);

  if (widget)
    {
      gtk_widget_set_sensitive (widget, sensitive);
    }
  else if (! strstr (path, "Script-Fu"))
    {
      g_warning ("%s: Unable to set sensitivity of menu item "
                 "which doesn't exist: %s",
                 G_STRLOC, path);
    }
}

void
gimp_item_factories_set_sensitive (const gchar *factory_path,
                                   const gchar *path,
                                   gboolean     sensitive)
{
  GList *list;

  g_return_if_fail (factory_path != NULL);
  g_return_if_fail (path != NULL);

  for (list = gimp_item_factories_from_path (factory_path);
       list;
       list = g_list_next (list))
    {
      gimp_item_factory_set_sensitive (GTK_ITEM_FACTORY (list->data),
                                       path, sensitive);
    }
}

void
gimp_item_factory_set_visible (GtkItemFactory *factory,
                               const gchar    *path,
                               gboolean        visible)
{
  GtkWidget *widget;

  g_return_if_fail (GTK_IS_ITEM_FACTORY (factory));
  g_return_if_fail (path != NULL);

  widget = gtk_item_factory_get_widget (factory, path);

  if (widget)
    {
      if (GTK_IS_MENU (widget))
        widget = gtk_menu_get_attach_widget (GTK_MENU (widget));

      if (visible)
        gtk_widget_show (widget);
      else
        gtk_widget_hide (widget);
    }
  else
    {
      g_warning ("%s: Unable to set visibility of menu item "
                 "which doesn't exist: %s",
                 G_STRLOC, path);
    }
}

void
gimp_item_factories_set_visible (const gchar *factory_path,
                                 const gchar *path,
                                 gboolean     visible)
{
  GList *list;

  g_return_if_fail (factory_path != NULL);
  g_return_if_fail (path != NULL);

  for (list = gimp_item_factories_from_path (factory_path);
       list;
       list = g_list_next (list))
    {
      gimp_item_factory_set_visible (GTK_ITEM_FACTORY (list->data),
                                     path, visible);
    }
}

void
gimp_item_factory_tearoff_callback (GtkWidget *widget,
                                    gpointer   data,
                                    guint      action)
{
  if (GTK_IS_TEAROFF_MENU_ITEM (widget))
    {
      GtkTearoffMenuItem *tomi = (GtkTearoffMenuItem *) widget;

      if (tomi->torn_off)
	{
	  GtkWidget *toplevel;

	  toplevel = gtk_widget_get_toplevel (widget);

	  if (! GTK_IS_WINDOW (toplevel))
	    {
	      g_warning ("%s: tearoff menu not in top level window",
                         G_STRLOC);
	    }
	  else
	    {
#ifdef __GNUC__
#warning FIXME: register tearoffs
#endif
	      g_object_set_data (G_OBJECT (widget), "tearoff-menu-toplevel",
                                 toplevel);
	    }
	}
      else
	{
	  GtkWidget *toplevel;

	  toplevel = (GtkWidget *) g_object_get_data (G_OBJECT (widget),
                                                      "tearoff-menu-toplevel");

	  if (! toplevel)
	    {
	      g_warning ("%s: can't unregister tearoff menu top level window",
                         G_STRLOC);
	    }
	  else
	    {
#ifdef __GNUC__
#warning FIXME: unregister tearoffs
#endif
	    }
	}
    }
}


/*  private functions  */

static void
gimp_item_factory_create_branches (GimpItemFactory      *factory,
                                   GimpItemFactoryEntry *entry,
                                   const gchar          *textdomain)
{
  GString *tearoff_path;
  gint     factory_length;
  gchar   *p;

  if (! entry->entry.path)
    return;

  tearoff_path = g_string_new ("");

  p = strchr (entry->entry.path, '/');
  factory_length = p - entry->entry.path;

  /*  skip the first slash  */
  if (p)
    p = strchr (p + 1, '/');

  while (p)
    {
      g_string_assign (tearoff_path, entry->entry.path + factory_length);
      g_string_truncate (tearoff_path, p - entry->entry.path - factory_length);

      if (! gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                         tearoff_path->str))
	{
	  GimpItemFactoryEntry branch_entry =
	  {
	    { NULL, NULL, NULL, 0, "<Branch>" },
	    NULL,
	    NULL
	  };

          branch_entry.entry.path = tearoff_path->str;

          g_object_set_data (G_OBJECT (factory), "complete", entry->entry.path);

	  gimp_item_factory_create_item (factory,
                                         &branch_entry,
                                         textdomain,
                                         NULL, 2, TRUE, FALSE);

	  g_object_set_data (G_OBJECT (factory), "complete", NULL);
	}

      g_string_append (tearoff_path, "/tearoff");

      if (! gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                         tearoff_path->str))
	{
	  GimpItemFactoryEntry tearoff_entry =
	  {
	    { NULL, NULL,
              gimp_item_factory_tearoff_callback, 0, "<Tearoff>" },
	    NULL,
	    NULL, NULL
	  };

	  tearoff_entry.entry.path = tearoff_path->str;

          gimp_item_factory_create_item (factory,
                                         &tearoff_entry,
                                         textdomain,
                                         NULL, 2, TRUE, FALSE);
	}

      p = strchr (p + 1, '/');
    }

  g_string_free (tearoff_path, TRUE);
}

static void
gimp_item_factory_item_realize (GtkWidget       *widget,
                                GimpItemFactory *item_factory)
{
  if (GTK_IS_MENU_SHELL (widget->parent))
    {
      static GQuark quark_key_press_connected = 0;

      if (! quark_key_press_connected)
        quark_key_press_connected =
          g_quark_from_static_string ("gimp-menu-item-key-press-connected");

      if (! GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (widget->parent),
                                                 quark_key_press_connected)))
	{
	  g_signal_connect (widget->parent, "key_press_event",
                            G_CALLBACK (gimp_item_factory_item_key_press),
                            item_factory);

	  g_object_set_qdata (G_OBJECT (widget->parent),
                              quark_key_press_connected,
                              GINT_TO_POINTER (TRUE));
	}
    }
}

static gboolean
gimp_item_factory_item_key_press (GtkWidget       *widget,
                                  GdkEventKey     *kevent,
                                  GimpItemFactory *item_factory)
{
  GtkWidget *active_menu_item;
  gchar     *help_id = NULL;

  active_menu_item = GTK_MENU_SHELL (widget)->active_menu_item;

  /*  first, get the help page from the item
   */
  if (active_menu_item)
    {
      help_id = g_object_get_data (G_OBJECT (active_menu_item), "gimp-help-id");

      if (help_id && ! strlen (help_id))
        help_id = NULL;
    }

  /*  For any valid accelerator key except F1, continue with the
   *  standard GtkItemFactory callback and assign a new shortcut, but
   *  don't assign a shortcut to the help menu entries ...
   */
  if (kevent->keyval != GDK_F1)
    {
      if (help_id                                   &&
          gtk_accelerator_valid (kevent->keyval, 0) &&
	  (strcmp (help_id, GIMP_HELP_HELP)         == 0 ||
	   strcmp (help_id, GIMP_HELP_HELP_CONTEXT) == 0))
	{
	  return TRUE;
	}

      return FALSE;
    }

  /*  ...finally, if F1 was pressed over any menu, show it's help page...  */

  if (! help_id)
    help_id = item_factory->help_id;

  {
    gchar *help_domain = NULL;
    gchar *help_string = NULL;
    gchar *domain_separator;

    help_id = g_strdup (help_id);

    domain_separator = strchr (help_id, '?');

    if (domain_separator)
      {
        *domain_separator = '\0';

        help_domain = g_strdup (help_id);
        help_string = g_strdup (domain_separator + 1);
      }
    else
      {
        help_string = g_strdup (help_id);
      }

    gimp_help (item_factory->gimp, help_domain, help_string);

    g_free (help_domain);
    g_free (help_string);
    g_free (help_id);
  }

  return TRUE;
}

static gchar *
gimp_item_factory_translate_func (const gchar *path,
                                  gpointer     data)
{
  GtkItemFactory  *item_factory;
  GimpItemFactory *gimp_factory;
  const gchar     *retval;
  gchar           *translation;
  gchar           *domain   = NULL;
  gchar           *complete = NULL;
  gchar           *p, *t;

  item_factory = GTK_ITEM_FACTORY (data);
  gimp_factory = GIMP_ITEM_FACTORY (data);

  if (strstr (path, "tearoff") ||
      strstr (path, "/---")    ||
      strstr (path, "/MRU"))
    {
      return (gchar *) path;
    }

  domain   = g_object_get_data (G_OBJECT (item_factory), "textdomain");
  complete = g_object_get_data (G_OBJECT (item_factory), "complete");

  if (domain)  /*  use the plugin textdomain  */
    {
      gchar *full_path;

      full_path = g_strconcat (item_factory->path, path, NULL);

      if (complete)
	{
          /*  This is a branch, use the complete path for translation,
	   *  then strip off entries from the end until it matches.
	   */
	  complete    = g_strconcat (item_factory->path, complete, NULL);
	  translation = g_strdup (dgettext (domain, complete));

	  while (*complete && *translation && strcmp (complete, full_path))
	    {
	      p = strrchr (complete, '/');
	      t = strrchr (translation, '/');
	      if (p && t)
		{
		  *p = '\0';
		  *t = '\0';
		}
	      else
		break;
	    }

	  g_free (complete);
          /* DON'T set complete to NULL here */
	}
      else
	{
	  translation = g_strdup (dgettext (domain, full_path));
	}

      gimp_factory->translation_trash =
        g_list_prepend (gimp_factory->translation_trash, translation);

      if (strncmp (item_factory->path, translation,
                   strlen (item_factory->path)) == 0)
	{
	  retval = translation + strlen (item_factory->path);
	}
      else
	{
	  g_warning ("%s: bad translation for menupath: %s",
                     G_STRLOC, full_path);

	  retval = path;
	}

      g_free (full_path);
    }
  else  /*  use the gimp textdomain  */
    {
      if (complete)
	{
          /*  This is a branch, use the complete path for translation,
	   *  then strip off entries from the end until it matches.
	   */
	  complete    = g_strdup (complete);
	  translation = g_strdup (gettext (complete));

          gimp_factory->translation_trash =
            g_list_prepend (gimp_factory->translation_trash, translation);

	  while (*complete && *translation && strcmp (complete, path))
	    {
	      p = strrchr (complete, '/');
	      t = strrchr (translation, '/');
	      if (p && t)
		{
		  *p = '\0';
		  *t = '\0';
		}
	      else
		break;
	    }

	  g_free (complete);
          /* DON'T set complete to NULL here */
	}
      else
        {
          translation = gettext (path);
        }

      if (*translation == '/')
	{
	  retval = translation;
	}
      else
	{
	  g_warning ("%s: bad translation for menupath: %s",
                     G_STRLOC, path);

          retval = path;
	}
    }

  return (gchar *) retval;
}
