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

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"

#include "gimpitemfactory.h"
#include "gimpwidgets-utils.h"

#include "gimphelp.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void     gimp_item_factory_class_init      (GimpItemFactoryClass *klass);
static void     gimp_item_factory_init            (GimpItemFactory      *factory);

static void     gimp_item_factory_finalize        (GObject              *object);

static void     gimp_item_factory_create_branches (GimpItemFactory      *factory,
                                                   GimpItemFactoryEntry *entry);
static void     gimp_item_factory_item_realize    (GtkWidget            *widget,
                                                   gpointer              data);
static gboolean gimp_item_factory_item_key_press  (GtkWidget            *widget,
                                                   GdkEventKey          *kevent,
                                                   gpointer              data);
#ifdef ENABLE_NLS
static gchar *  gimp_item_factory_translate_func  (const gchar          *path,
                                                   gpointer              data);
#else
#define         gimp_item_factory_translate_func  (NULL)
#endif


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
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_item_factory_finalize;

  klass->factories = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
gimp_item_factory_init (GimpItemFactory *factory)
{
  factory->factory_path = NULL;
}

static void
gimp_item_factory_finalize (GObject *object)
{
  GimpItemFactory *factory;

  factory = GIMP_ITEM_FACTORY (object);

  if (factory->factory_path)
    {
      g_hash_table_remove (GIMP_ITEM_FACTORY_GET_CLASS (object)->factories,
                           factory->factory_path);

      g_free (factory->factory_path);
      factory->factory_path = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

GimpItemFactory *
gimp_item_factory_new (Gimp                      *gimp,
                       GType                      container_type,
                       const gchar               *path,
                       const gchar               *factory_path,
                       GimpItemFactoryUpdateFunc  update_func,
                       guint                      n_entries,
                       GimpItemFactoryEntry      *entries,
                       gpointer                   callback_data,
                       gboolean                   create_tearoff)
{
  GimpItemFactoryClass *factory_class;
  GimpItemFactory      *factory;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (factory_path != NULL, NULL);

  factory_class = g_type_class_ref (GIMP_TYPE_ITEM_FACTORY);

  if (g_hash_table_lookup (factory_class->factories, path))
    {
      g_warning ("%s: item factory with path \"%s\" already exists",
                 G_STRLOC, path);

      g_type_class_unref (factory_class);
      return NULL;
    }

  factory = g_object_new (GIMP_TYPE_ITEM_FACTORY, NULL);

  gtk_item_factory_construct (GTK_ITEM_FACTORY (factory),
                              container_type,
                              path,
                              NULL);

  gtk_item_factory_set_translate_func (GTK_ITEM_FACTORY (factory),
				       gimp_item_factory_translate_func,
				       (gpointer) path,
				       NULL);

  factory->gimp = gimp;

  /*  this is correct!  <mitch>  */
  factory->factory_path = g_strdup (path);
  factory->update_func  = update_func;

  g_hash_table_insert (factory_class->factories,
                       factory->factory_path,
                       factory);

  /*  this will go away  <mitch>  */
  g_object_set_data (G_OBJECT (factory), "factory_path",
                     (gpointer) factory_path);

  gimp_item_factory_create_items (factory,
                                  n_entries,
                                  entries,
                                  callback_data,
                                  2,
                                  create_tearoff,
                                  TRUE);

  g_type_class_unref (factory_class);

  return factory;
}

GimpItemFactory *
gimp_item_factory_from_path (const gchar *path)
{
  GimpItemFactoryClass *factory_class;
  GimpItemFactory      *factory;
  gchar                *base_path;
  gchar                *p;

  g_return_val_if_fail (path != NULL, NULL);

  base_path = g_strdup (path);

  p = strchr (base_path, '>');

  if (p)
    p[1] = '\0';

  factory_class = g_type_class_ref (GIMP_TYPE_ITEM_FACTORY);

  factory = g_hash_table_lookup (factory_class->factories, base_path);

  g_type_class_unref (factory_class);

  g_free (base_path);

  return factory;
}

void
gimp_item_factory_create_item (GimpItemFactory       *item_factory,
                               GimpItemFactoryEntry  *entry,
                               gpointer               callback_data,
                               guint                  callback_type,
                               gboolean               create_tearoff,
                               gboolean               static_entry)
{
  GtkWidget *menu_item;

  g_return_if_fail (GIMP_IS_ITEM_FACTORY (item_factory));
  g_return_if_fail (entry != NULL);

  if (! (strstr (entry->entry.path, "tearoff1")))
    {
      if (! gimprc.disable_tearoff_menus && create_tearoff)
	{
	  gimp_item_factory_create_branches (item_factory, entry);
	}
    }
  else if (gimprc.disable_tearoff_menus || ! create_tearoff)
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

  gtk_item_factory_create_item (GTK_ITEM_FACTORY (item_factory),
				(GtkItemFactoryEntry *) entry,
				callback_data,
				callback_type);

  menu_item = gtk_item_factory_get_item (GTK_ITEM_FACTORY (item_factory),
					 ((GtkItemFactoryEntry *) entry)->path);

  if (menu_item)
    {
      g_signal_connect_after (G_OBJECT (menu_item), "realize",
			      G_CALLBACK (gimp_item_factory_item_realize),
			      item_factory);

      if (entry->help_page)
        {
          if (static_entry)
            g_object_set_data (G_OBJECT (menu_item), "help_page",
                               (gpointer) entry->help_page);
          else
            g_object_set_data_full (G_OBJECT (menu_item), "help_page",
                                    g_strdup (entry->help_page),
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
                                     callback_data,
                                     callback_type,
                                     create_tearoff,
                                     static_entries);
    }
}

void
gimp_item_factory_popup_with_data (GimpItemFactory  *item_factory,
				   gpointer          data,
                                   GtkDestroyNotify  popdown_func)
{
  gint    x, y;
  guint   button;
  guint32 activate_time;

  g_return_if_fail (GIMP_IS_ITEM_FACTORY (item_factory));

  if (item_factory->update_func)
    item_factory->update_func (GTK_ITEM_FACTORY (item_factory), data);

  gimp_menu_position (GTK_MENU (GTK_ITEM_FACTORY (item_factory)->widget),
		      &x, &y,
		      &button,
		      &activate_time);

  gtk_item_factory_popup_with_data (GTK_ITEM_FACTORY (item_factory),
				    data,
				    popdown_func,
				    x, y,
				    button,
				    activate_time);
}

void
gimp_item_factory_set_active (GtkItemFactory *factory,
                              gchar          *path,
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
gimp_item_factory_set_color (GtkItemFactory *factory,
                             gchar          *path,
                             const GimpRGB  *color,
                             gboolean        set_label)
{
  GtkWidget *widget;
  GtkWidget *preview = NULL;
  GtkWidget *label   = NULL;

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

#define COLOR_BOX_WIDTH  16
#define COLOR_BOX_HEIGHT 16

  if (GTK_IS_HBOX (GTK_BIN (widget)->child))
    {
      preview = g_object_get_data (G_OBJECT (GTK_BIN (widget)->child),
                                   "preview");
      label = g_object_get_data (G_OBJECT (GTK_BIN (widget)->child),
                                 "label");
    }
  else if (GTK_IS_LABEL (GTK_BIN (widget)->child))
    {
      GtkWidget *hbox;

      label = GTK_BIN (widget)->child;

      g_object_ref (G_OBJECT (label));

      gtk_container_remove (GTK_CONTAINER (widget), label);

      hbox = gtk_hbox_new (FALSE, 4);
      gtk_container_add (GTK_CONTAINER (widget), hbox);
      gtk_widget_show (hbox);

      preview = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_preview_size (GTK_PREVIEW (preview),
                        COLOR_BOX_WIDTH, COLOR_BOX_HEIGHT);
      gtk_box_pack_start (GTK_BOX (hbox), preview, FALSE, FALSE, 0);
      gtk_widget_show (preview);

      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
      gtk_widget_show (label);

      g_object_unref (G_OBJECT (label));

      g_object_set_data (G_OBJECT (hbox), "preview", preview);
      g_object_set_data (G_OBJECT (hbox), "label",   label);
    }

  if (preview)
    {
      guchar  rows[3][COLOR_BOX_WIDTH * 3];
      gint    x, y;
      gint    r0, g0, b0;
      gint    r1, g1, b1;
      guchar *p0, *p1, *p2;

      /* Fill rows */

      r0 = (GIMP_CHECK_DARK + (color->r - GIMP_CHECK_DARK) * color->a) * 255.0;
      r1 = (GIMP_CHECK_LIGHT + (color->r - GIMP_CHECK_LIGHT) * color->a) * 255.0;

      g0 = (GIMP_CHECK_DARK + (color->g - GIMP_CHECK_DARK) * color->a) * 255.0;
      g1 = (GIMP_CHECK_LIGHT + (color->g - GIMP_CHECK_LIGHT) * color->a) * 255.0;

      b0 = (GIMP_CHECK_DARK + (color->b - GIMP_CHECK_DARK) * color->a) * 255.0;
      b1 = (GIMP_CHECK_LIGHT + (color->b - GIMP_CHECK_LIGHT) * color->a) * 255.0;

      p0 = rows[0];
      p1 = rows[1];
      p2 = rows[2];

      for (x = 0; x < COLOR_BOX_WIDTH; x++)
        {
          if ((x == 0) || (x == (COLOR_BOX_WIDTH - 1)))
            {
              *p0++ = 0;
              *p0++ = 0;
              *p0++ = 0;

              *p1++ = 0;
              *p1++ = 0;
              *p1++ = 0;
            }
          else if ((x / GIMP_CHECK_SIZE) & 1)
            {
              *p0++ = r1;
              *p0++ = g1;
              *p0++ = b1;

              *p1++ = r0;
              *p1++ = g0;
              *p1++ = b0;
            }
          else
            {
              *p0++ = r0;
              *p0++ = g0;
              *p0++ = b0;

              *p1++ = r1;
              *p1++ = g1;
              *p1++ = b1;
            }

          *p2++ = 0;
          *p2++ = 0;
          *p2++ = 0;
        }

      /* Fill preview */

      gtk_preview_draw_row (GTK_PREVIEW (preview), rows[2],
                            0, 0, COLOR_BOX_WIDTH);

      for (y = 1; y < (COLOR_BOX_HEIGHT - 1); y++)
        if ((y / GIMP_CHECK_SIZE) & 1)
          gtk_preview_draw_row (GTK_PREVIEW (preview), rows[1],
                                0, y, COLOR_BOX_WIDTH);
        else
          gtk_preview_draw_row (GTK_PREVIEW (preview), rows[0],
                                0, y, COLOR_BOX_WIDTH);

      gtk_preview_draw_row (GTK_PREVIEW (preview), rows[2],
                            0, y, COLOR_BOX_WIDTH);

      gtk_widget_queue_draw (preview);
    }

  if (label && set_label)
    {
      gchar *str;

      str = g_strdup_printf (_("RGBA (%0.3f, %0.3f, %0.3f, %0.3f)"),
                             color->r, color->g, color->b, color->a);

      gtk_label_set_text (GTK_LABEL (label), str);

      g_free (str);
    }

#undef COLOR_BOX_WIDTH
#undef COLOR_BOX_HEIGHT
}

void
gimp_item_factory_set_label (GtkItemFactory *factory,
                             gchar          *path,
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
        gtk_label_set_text (GTK_LABEL (GTK_BIN (widget)->child), label);
    }
  else
    {
      g_warning ("%s: Unable to set label of menu item "
                 "which doesn't exist: %s",
                 G_STRLOC, path);
    }
}

void
gimp_item_factory_set_sensitive (GtkItemFactory *factory,
                                 gchar          *path,
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
      g_warning ("%s: Unable to set sensitivity of menu item"
                 "which doesn't exist: %s",
                 G_STRLOC, path);
    }
}

void
gimp_item_factory_set_visible (GtkItemFactory *factory,
                               gchar          *path,
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
      g_warning ("%s: Unable to set visibility of menu item"
                 "which doesn't exist: %s",
                 G_STRLOC, path);
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
                                   GimpItemFactoryEntry *entry)
{
  GString *tearoff_path;
  gint     factory_length;
  gchar   *p;

  if (! entry->entry.path)
    return;

  tearoff_path = g_string_new ("");

  p              = strchr (entry->entry.path, '/');
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
	    { tearoff_path->str, NULL, NULL, 0, "<Branch>" },
	    NULL,
	    NULL
	  };

	  g_object_set_data (G_OBJECT (factory), "complete", entry->entry.path);

	  gimp_item_factory_create_item (factory,
                                         &branch_entry,
                                         NULL, 2, TRUE, FALSE);

	  g_object_set_data (G_OBJECT (factory), "complete", NULL);
	}

      g_string_append (tearoff_path, "/tearoff1");

      if (! gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                         tearoff_path->str))
	{
	  GimpItemFactoryEntry tearoff_entry =
	  {
	    { tearoff_path->str, NULL,
              gimp_item_factory_tearoff_callback, 0, "<Tearoff>" },
	    NULL,
	    NULL, NULL
	  };

	  gimp_item_factory_create_item (factory,
                                         &tearoff_entry,
                                         NULL, 2, TRUE, FALSE);
	}

      p = strchr (p + 1, '/');
    }

  g_string_free (tearoff_path, TRUE);
}

static void
gimp_item_factory_item_realize (GtkWidget *widget,
                                gpointer   data)
{
  if (GTK_IS_MENU_SHELL (widget->parent))
    {
      if (! g_object_get_data (G_OBJECT (widget->parent),
                               "menus_key_press_connected"))
	{
	  g_signal_connect (G_OBJECT (widget->parent), "key_press_event",
                            G_CALLBACK (gimp_item_factory_item_key_press),
                            data);

	  g_object_set_data (G_OBJECT (widget->parent),
                             "menus_key_press_connected",
                             (gpointer) TRUE);
	}
    }
}

static gboolean
gimp_item_factory_item_key_press (GtkWidget   *widget,
                                  GdkEventKey *kevent,
                                  gpointer     data)
{
  GtkItemFactory *item_factory     = NULL;
  GtkWidget      *active_menu_item = NULL;
  gchar          *factory_path     = NULL;
  gchar          *help_path        = NULL;
  gchar          *help_page        = NULL;

  item_factory     = (GtkItemFactory *) data;
  active_menu_item = GTK_MENU_SHELL (widget)->active_menu_item;

  /*  first, get the help page from the item
   */
  if (active_menu_item)
    {
      help_page = (gchar *) g_object_get_data (G_OBJECT (active_menu_item),
                                               "help_page");
    }

  /*  For any key except F1, continue with the standard
   *  GtkItemFactory callback and assign a new shortcut, but don't
   *  assign a shortcut to the help menu entries...
   */
  if (kevent->keyval != GDK_F1)
    {
      if (help_page &&
	  *help_page &&
	  item_factory == (GtkItemFactory *) gimp_item_factory_from_path ("<Toolbox>") &&
	  (strcmp (help_page, "help/dialogs/help.html") == 0 ||
	   strcmp (help_page, "help/context_help.html") == 0))
	{
	  return TRUE;
	}
      else
	{
	  return FALSE;
	}
    }

  /*  ...finally, if F1 was pressed over any menu, show it's help page...  */

  factory_path = (gchar *) g_object_get_data (G_OBJECT (item_factory),
                                              "factory_path");

  if (! help_page ||
      ! *help_page)
    help_page = "index.html";

  if (factory_path && help_page)
    {
      gchar *help_string;
      gchar *at;

      help_page = g_strdup (help_page);

      at = strchr (help_page, '@');  /* HACK: locale subdir */

      if (at)
	{
	  *at = '\0';
	  help_path   = g_strdup (help_page);
	  help_string = g_strdup (at + 1);
	}
      else
	{
	  help_string = g_strdup_printf ("%s/%s", factory_path, help_page);
	}

      gimp_help (GIMP_ITEM_FACTORY (item_factory)->gimp,
                 help_path, help_string);

      g_free (help_string);
      g_free (help_page);
    }
  else
    {
      gimp_standard_help_func (NULL);
    }

  return TRUE;
}

#ifdef ENABLE_NLS

static gchar *
gimp_item_factory_translate_func (const gchar *path,
                                  gpointer     data)
{
  static gchar   *menupath = NULL;

  GtkItemFactory *item_factory = NULL;
  gchar          *retval;
  gchar          *factory;
  gchar          *translation;
  gchar          *domain   = NULL;
  gchar          *complete = NULL;
  gchar          *p, *t;

  factory = (gchar *) data;

  if (menupath)
    g_free (menupath);

  retval = menupath = g_strdup (path);

  if ((strstr (path, "/tearoff1") != NULL) ||
      (strstr (path, "/---")      != NULL) ||
      (strstr (path, "/MRU")      != NULL))
    return retval;

  if (factory)
    item_factory = (GtkItemFactory *) gimp_item_factory_from_path (factory);

  if (item_factory)
    {
      domain   = g_object_get_data (G_OBJECT (item_factory), "textdomain");
      complete = g_object_get_data (G_OBJECT (item_factory), "complete");
    }

  if (domain)  /*  use the plugin textdomain  */
    {
      g_free (menupath);
      menupath = g_strconcat (factory, path, NULL);

      if (complete)
	{
          /*  This is a branch, use the complete path for translation, 
	   *  then strip off entries from the end until it matches. 
	   */
	  complete    = g_strconcat (factory, complete, NULL);
	  translation = g_strdup (dgettext (domain, complete));

	  while (*complete && *translation && strcmp (complete, menupath))
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
	}
      else
	{
	  translation = dgettext (domain, menupath);
	}

      /* 
       * Work around a bug in GTK+ prior to 1.2.7 (similar workaround below)
       */
      if (strncmp (factory, translation, strlen (factory)) == 0)
	{
	  retval = translation + strlen (factory);
	  if (complete)
	    {
	      g_free (menupath);
	      menupath = translation;
	    }
	}
      else
	{
	  g_warning ("%s: bad translation for menupath: %s",
                     G_STRLOC, menupath);

	  retval = menupath + strlen (factory);
	  if (complete)
	    g_free (translation);
	}
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
	  
	  while (*complete && *translation && strcmp (complete, menupath))
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
	}
      else
	translation = gettext (menupath);

      if (*translation == '/')
	{
	  retval = translation;
	  if (complete)
	    {
	      g_free (menupath);
	      menupath = translation;
	    }
	}
      else
	{
	  g_warning ("%s: bad translation for menupath: %s",
                     G_STRLOC, menupath);

	  if (complete)
	    g_free (translation);
	}
    }
  
  return retval;
}

#endif  /*  ENABLE_NLS  */
