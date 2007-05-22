/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpeditor.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpdocked.h"
#include "gimpeditor.h"
#include "gimpdnd.h"
#include "gimpmenufactory.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define DEFAULT_CONTENT_SPACING  2
#define DEFAULT_BUTTON_SPACING   2
#define DEFAULT_BUTTON_ICON_SIZE GTK_ICON_SIZE_MENU


enum
{
  PROP_0,
  PROP_MENU_FACTORY,
  PROP_MENU_IDENTIFIER,
  PROP_UI_PATH,
  PROP_POPUP_DATA,
  PROP_SHOW_NAME,
  PROP_NAME
};


static void        gimp_editor_docked_iface_init (GimpDockedInterface *iface);

static GObject   * gimp_editor_constructor       (GType            type,
                                                  guint            n_params,
                                                  GObjectConstructParam *params);
static void        gimp_editor_set_property      (GObject         *object,
                                                  guint            property_id,
                                                  const GValue    *value,
                                                  GParamSpec      *pspec);
static void        gimp_editor_get_property      (GObject         *object,
                                                  guint            property_id,
                                                  GValue          *value,
                                                  GParamSpec      *pspec);
static void        gimp_editor_destroy           (GtkObject       *object);
static void        gimp_editor_style_set         (GtkWidget       *widget,
                                                  GtkStyle        *prev_style);

static GimpUIManager * gimp_editor_get_menu        (GimpDocked      *docked,
                                                    const gchar    **ui_path,
                                                    gpointer        *popup_data);
static gboolean    gimp_editor_has_button_bar      (GimpDocked      *docked);
static void        gimp_editor_set_show_button_bar (GimpDocked      *docked,
                                                    gboolean         show);
static gboolean    gimp_editor_get_show_button_bar (GimpDocked      *docked);

static GtkIconSize gimp_editor_ensure_button_box   (GimpEditor      *editor);


G_DEFINE_TYPE_WITH_CODE (GimpEditor, gimp_editor, GTK_TYPE_VBOX,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_editor_docked_iface_init))

#define parent_class gimp_editor_parent_class


static void
gimp_editor_class_init (GimpEditorClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class     = GTK_WIDGET_CLASS (klass);

  object_class->constructor  = gimp_editor_constructor;
  object_class->set_property = gimp_editor_set_property;
  object_class->get_property = gimp_editor_get_property;

  gtk_object_class->destroy  = gimp_editor_destroy;

  widget_class->style_set    = gimp_editor_style_set;

  g_object_class_install_property (object_class, PROP_MENU_FACTORY,
                                   g_param_spec_object ("menu-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_MENU_FACTORY,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_MENU_IDENTIFIER,
                                   g_param_spec_string ("menu-identifier",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UI_PATH,
                                   g_param_spec_string ("ui-path",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_POPUP_DATA,
                                   g_param_spec_pointer ("popup-data",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SHOW_NAME,
                                   g_param_spec_boolean ("show-name",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-spacing",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_CONTENT_SPACING,
                                                             GIMP_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("button-spacing",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_BUTTON_SPACING,
                                                             GIMP_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("button-icon-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_BUTTON_ICON_SIZE,
                                                              GIMP_PARAM_READABLE));
}

static void
gimp_editor_init (GimpEditor *editor)
{
  editor->menu_factory    = NULL;
  editor->menu_identifier = NULL;
  editor->ui_manager      = NULL;
  editor->ui_path         = NULL;
  editor->popup_data      = editor;
  editor->button_box      = NULL;
  editor->show_button_bar = TRUE;

  editor->name_label = g_object_new (GTK_TYPE_LABEL,
                                     "xalign",    0.0,
                                     "yalign",    0.5,
                                     "ellipsize", PANGO_ELLIPSIZE_END,
                                     NULL);
  gimp_label_set_attributes (GTK_LABEL (editor->name_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (editor), editor->name_label, FALSE, FALSE, 0);
}

static void
gimp_editor_docked_iface_init (GimpDockedInterface *iface)
{
  iface->get_menu            = gimp_editor_get_menu;
  iface->has_button_bar      = gimp_editor_has_button_bar;
  iface->set_show_button_bar = gimp_editor_set_show_button_bar;
  iface->get_show_button_bar = gimp_editor_get_show_button_bar;
}

static GObject *
gimp_editor_constructor (GType                  type,
                         guint                  n_params,
                         GObjectConstructParam *params)
{
  GObject    *object;
  GimpEditor *editor;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_EDITOR (object);

  if (! editor->popup_data)
    editor->popup_data = editor;

  if (editor->menu_factory && editor->menu_identifier)
    {
      editor->ui_manager =
        gimp_menu_factory_manager_new (editor->menu_factory,
                                       editor->menu_identifier,
                                       editor->popup_data,
                                       FALSE);
    }

  return object;
}

static void
gimp_editor_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GimpEditor *editor = GIMP_EDITOR (object);

  switch (property_id)
    {
    case PROP_MENU_FACTORY:
      editor->menu_factory = (GimpMenuFactory *) g_value_dup_object (value);
      break;
    case PROP_MENU_IDENTIFIER:
      editor->menu_identifier = g_value_dup_string (value);
      break;
    case PROP_UI_PATH:
      editor->ui_path = g_value_dup_string (value);
      break;
    case PROP_POPUP_DATA:
      editor->popup_data = g_value_get_pointer (value);
      break;
    case PROP_SHOW_NAME:
      g_object_set_property (G_OBJECT (editor->name_label), "visible", value);
      break;
    case PROP_NAME:
      gimp_editor_set_name (editor, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_editor_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GimpEditor *editor = GIMP_EDITOR (object);

  switch (property_id)
    {
    case PROP_MENU_FACTORY:
      g_value_set_object (value, editor->menu_factory);
      break;
    case PROP_MENU_IDENTIFIER:
      g_value_set_string (value, editor->menu_identifier);
      break;
    case PROP_UI_PATH:
      g_value_set_string (value, editor->ui_path);
      break;
    case PROP_POPUP_DATA:
      g_value_set_pointer (value, editor->popup_data);
      break;
    case PROP_SHOW_NAME:
      g_object_get_property (G_OBJECT (editor->name_label), "visible", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_editor_destroy (GtkObject *object)
{
  GimpEditor *editor = GIMP_EDITOR (object);

  if (editor->menu_factory)
    {
      g_object_unref (editor->menu_factory);
      editor->menu_factory = NULL;
    }

  if (editor->menu_identifier)
    {
      g_free (editor->menu_identifier);
      editor->menu_identifier = NULL;
    }

  if (editor->ui_manager)
    {
      g_object_unref (editor->ui_manager);
      editor->ui_manager = NULL;
    }

  if (editor->ui_path)
    {
      g_free (editor->ui_path);
      editor->ui_path = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_editor_style_set (GtkWidget *widget,
                       GtkStyle  *prev_style)
{
  GimpEditor  *editor = GIMP_EDITOR (widget);
  gint         content_spacing;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget, "content-spacing",  &content_spacing, NULL);

  gtk_box_set_spacing (GTK_BOX (widget), content_spacing);

  if (editor->button_box)
    gimp_editor_set_box_style (editor, GTK_BOX (editor->button_box));
}

static GimpUIManager *
gimp_editor_get_menu (GimpDocked   *docked,
                      const gchar **ui_path,
                      gpointer     *popup_data)
{
  GimpEditor *editor = GIMP_EDITOR (docked);

  *ui_path    = editor->ui_path;
  *popup_data = editor->popup_data;

  return editor->ui_manager;
}


static gboolean
gimp_editor_has_button_bar (GimpDocked *docked)
{
  GimpEditor *editor = GIMP_EDITOR (docked);

  return editor->button_box != NULL;
}

static void
gimp_editor_set_show_button_bar (GimpDocked *docked,
                                 gboolean    show)
{
  GimpEditor *editor = GIMP_EDITOR (docked);

  if (show != editor->show_button_bar)
    {
      editor->show_button_bar = show;

      if (editor->button_box)
        {
          if (show)
            gtk_widget_show (editor->button_box);
          else
            gtk_widget_hide (editor->button_box);
        }
    }
}

static gboolean
gimp_editor_get_show_button_bar (GimpDocked *docked)
{
  GimpEditor *editor = GIMP_EDITOR (docked);

  return editor->show_button_bar;
}

GtkWidget *
gimp_editor_new (void)
{
  return g_object_new (GIMP_TYPE_EDITOR, NULL);
}

void
gimp_editor_create_menu (GimpEditor      *editor,
                         GimpMenuFactory *menu_factory,
                         const gchar     *menu_identifier,
                         const gchar     *ui_path,
                         gpointer         popup_data)
{
  g_return_if_fail (GIMP_IS_EDITOR (editor));
  g_return_if_fail (GIMP_IS_MENU_FACTORY (menu_factory));
  g_return_if_fail (menu_identifier != NULL);
  g_return_if_fail (ui_path != NULL);

  if (editor->menu_factory)
    g_object_unref (editor->menu_factory);

  editor->menu_factory = g_object_ref (menu_factory);

  if (editor->ui_manager)
    g_object_unref (editor->ui_manager);

  editor->ui_manager = gimp_menu_factory_manager_new (menu_factory,
                                                      menu_identifier,
                                                      popup_data,
                                                      FALSE);

  if (editor->ui_path)
    g_free (editor->ui_path);

  editor->ui_path = g_strdup (ui_path);

  editor->popup_data = popup_data;
}

gboolean
gimp_editor_popup_menu (GimpEditor           *editor,
                        GimpMenuPositionFunc  position_func,
                        gpointer              position_data)
{
  g_return_val_if_fail (GIMP_IS_EDITOR (editor), FALSE);

  if (editor->ui_manager && editor->ui_path)
    {
      gimp_ui_manager_update (editor->ui_manager, editor->popup_data);
      gimp_ui_manager_ui_popup (editor->ui_manager, editor->ui_path,
                                GTK_WIDGET (editor),
                                position_func, position_data,
                                NULL, NULL);
      return TRUE;
    }

  return FALSE;
}

GtkWidget *
gimp_editor_add_button (GimpEditor  *editor,
                        const gchar *stock_id,
                        const gchar *tooltip,
                        const gchar *help_id,
                        GCallback    callback,
                        GCallback    extended_callback,
                        gpointer     callback_data)
{
  GtkWidget   *button;
  GtkWidget   *image;
  GtkIconSize  button_icon_size;

  g_return_val_if_fail (GIMP_IS_EDITOR (editor), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);

  button_icon_size = gimp_editor_ensure_button_box (editor);

  button = g_object_new (GIMP_TYPE_BUTTON,
                         "use-stock", TRUE,
                         NULL);
  gtk_box_pack_start (GTK_BOX (editor->button_box), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  if (tooltip || help_id)
    gimp_help_set_help_data (button, tooltip, help_id);

  if (callback)
    g_signal_connect (button, "clicked",
                      callback,
                      callback_data);

  if (extended_callback)
    g_signal_connect (button, "extended-clicked",
                      extended_callback,
                      callback_data);

  image = gtk_image_new_from_stock (stock_id, button_icon_size);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  return button;
}

GtkWidget *
gimp_editor_add_stock_box (GimpEditor  *editor,
                           GType        enum_type,
                           const gchar *stock_prefix,
                           GCallback    callback,
                           gpointer     callback_data)
{
  GtkWidget   *hbox;
  GtkWidget   *first_button;
  GtkIconSize  button_icon_size;
  GList       *children;
  GList       *list;

  g_return_val_if_fail (GIMP_IS_EDITOR (editor), NULL);
  g_return_val_if_fail (g_type_is_a (enum_type, G_TYPE_ENUM), NULL);
  g_return_val_if_fail (stock_prefix != NULL, NULL);

  button_icon_size = gimp_editor_ensure_button_box (editor);

  hbox = gimp_enum_stock_box_new (enum_type, stock_prefix, button_icon_size,
                                  callback, callback_data,
                                  &first_button);

  children = gtk_container_get_children (GTK_CONTAINER (hbox));

  for (list = children; list; list = g_list_next (list))
    {
      GtkWidget *button = list->data;

      g_object_ref (button);

      gtk_container_remove (GTK_CONTAINER (hbox), button);
      gtk_box_pack_start (GTK_BOX (editor->button_box), button,
                          TRUE, TRUE, 0);

      g_object_unref (button);
    }

  g_list_free (children);

  g_object_ref_sink (hbox);
  g_object_unref (hbox);

  return first_button;
}


typedef struct
{
  GdkModifierType  mod_mask;
  GtkAction       *action;
} ExtendedAction;

static void
gimp_editor_button_extended_actions_free (GList *actions)
{
  GList *list;

  for (list = actions; list; list = list->next)
    g_slice_free (ExtendedAction, list->data);

  g_list_free (actions);
}

static void
gimp_editor_button_extended_clicked (GtkWidget       *button,
                                     GdkModifierType  mask,
                                     gpointer         data)
{
  GList *extended = g_object_get_data (G_OBJECT (button), "extended-actions");
  GList *list;

  for (list = extended; list; list = g_list_next (list))
    {
      ExtendedAction *ext = list->data;

      if ((ext->mod_mask & mask) == ext->mod_mask &&
          gtk_action_get_sensitive (ext->action))
        {
          gtk_action_activate (ext->action);
          break;
        }
    }
}

GtkWidget *
gimp_editor_add_action_button (GimpEditor  *editor,
                               const gchar *group_name,
                               const gchar *action_name,
                               ...)
{
  GimpActionGroup *group;
  GtkAction       *action;
  GtkWidget       *button;
  GtkWidget       *old_child;
  GtkWidget       *image;
  GtkIconSize      button_icon_size;
  gchar           *stock_id;
  gchar           *tooltip;
  const gchar     *help_id;
  GList           *extended = NULL;
  va_list          args;

  g_return_val_if_fail (GIMP_IS_EDITOR (editor), NULL);
  g_return_val_if_fail (action_name != NULL, NULL);
  g_return_val_if_fail (editor->ui_manager != NULL, NULL);

  group = gimp_ui_manager_get_action_group (editor->ui_manager, group_name);

  g_return_val_if_fail (group != NULL, NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                        action_name);

  g_return_val_if_fail (action != NULL, NULL);

  button_icon_size = gimp_editor_ensure_button_box (editor);

  if (GTK_IS_TOGGLE_ACTION (action))
    {
      button = gtk_toggle_button_new ();
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
    }
  else
    {
      button = gimp_button_new ();
    }

  g_object_get (action,
                "stock-id", &stock_id,
                "tooltip",  &tooltip,
                NULL);

  old_child = gtk_bin_get_child (GTK_BIN (button));

  if (old_child)
    gtk_container_remove (GTK_CONTAINER (button), old_child);

  image = gtk_image_new_from_stock (stock_id, button_icon_size);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_free (stock_id);

  gtk_action_connect_proxy (action, button);
  gtk_box_pack_start (GTK_BOX (editor->button_box), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  va_start (args, action_name);

  action_name = va_arg (args, const gchar *);

  while (action_name)
    {
      GdkModifierType mod_mask;

      mod_mask = va_arg (args, GdkModifierType);

      action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                            action_name);

      if (action && mod_mask)
        {
          ExtendedAction *ext = g_slice_new (ExtendedAction);

          ext->mod_mask = mod_mask;
          ext->action   = action;

          extended = g_list_prepend (extended, ext);

          if (tooltip)
            {
              gchar *ext_tooltip;

              g_object_get (action, "tooltip", &ext_tooltip, NULL);

              if (ext_tooltip)
                {
                  gchar *tmp = g_strconcat (tooltip, "\n(",
                                            gimp_get_mod_string (mod_mask),
                                            ")  ", ext_tooltip, NULL);

                  g_free (ext_tooltip);
                  g_free (tooltip);
                  tooltip = tmp;
                }
            }
        }

      action_name = va_arg (args, const gchar *);
    }

  va_end (args);

  if (extended)
    {
      g_object_set_data_full (G_OBJECT (button), "extended-actions", extended,
                              (GDestroyNotify) gimp_editor_button_extended_actions_free);

      g_signal_connect (button, "extended-clicked",
                        G_CALLBACK (gimp_editor_button_extended_clicked),
                        NULL);
    }

  help_id = g_object_get_qdata (G_OBJECT (action), GIMP_HELP_ID);

  if (tooltip || help_id)
    gimp_help_set_help_data (button, tooltip, help_id);

  g_free (tooltip);

  return button;
}

void
gimp_editor_set_show_name (GimpEditor *editor,
                           gboolean    show)
{
  g_return_if_fail (GIMP_IS_EDITOR (editor));

  g_object_set (editor, "show-name", show, NULL);
}

void
gimp_editor_set_name (GimpEditor  *editor,
                      const gchar *name)
{
  g_return_if_fail (GIMP_IS_EDITOR (editor));

  gtk_label_set_text (GTK_LABEL (editor->name_label),
                      name ? name : _("(None)"));
}

void
gimp_editor_set_box_style (GimpEditor *editor,
                           GtkBox     *box)
{
  GtkIconSize  button_icon_size;
  gint         button_spacing;
  GList       *children;
  GList       *list;

  g_return_if_fail (GIMP_IS_EDITOR (editor));
  g_return_if_fail (GTK_IS_BOX (box));

  gtk_widget_style_get (GTK_WIDGET (editor),
                        "button-icon-size", &button_icon_size,
                        "button-spacing",   &button_spacing,
                        NULL);

  gtk_box_set_spacing (box, button_spacing);

  children = gtk_container_get_children (GTK_CONTAINER (box));

  for (list = children; list; list = g_list_next (list))
    {
      if (GTK_IS_BUTTON (list->data))
        {
          GtkWidget *child;

          child = gtk_bin_get_child (GTK_BIN (list->data));

          if (GTK_IS_IMAGE (child))
            {
              gchar *stock_id;

              gtk_image_get_stock (GTK_IMAGE (child), &stock_id, NULL);
              gtk_image_set_from_stock (GTK_IMAGE (child),
                                        stock_id, button_icon_size);
            }
        }
    }

  g_list_free (children);
}


/*  private functions  */

static GtkIconSize
gimp_editor_ensure_button_box (GimpEditor *editor)
{
  GtkIconSize  button_icon_size;
  gint         button_spacing;

  gtk_widget_style_get (GTK_WIDGET (editor),
                        "button-icon-size", &button_icon_size,
                        "button-spacing",   &button_spacing,
                        NULL);

  if (! editor->button_box)
    {
      editor->button_box = gtk_hbox_new (TRUE, button_spacing);
      gtk_box_pack_end (GTK_BOX (editor), editor->button_box, FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (editor), editor->button_box, 0);

      if (editor->show_button_bar)
        gtk_widget_show (editor->button_box);
    }

  return button_icon_size;
}
