/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpeditor.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

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
#define DEFAULT_BUTTON_RELIEF    GTK_RELIEF_NONE


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


struct _GimpEditorPrivate
{
  GimpMenuFactory *menu_factory;
  gchar           *menu_identifier;
  GimpUIManager   *ui_manager;
  gchar           *ui_path;
  gpointer         popup_data;

  gboolean         show_button_bar;
  GtkWidget       *name_label;
  GtkWidget       *button_box;
};


static void         gimp_editor_docked_iface_init (GimpDockedInterface *iface);

static void            gimp_editor_constructed         (GObject        *object);
static void            gimp_editor_dispose             (GObject        *object);
static void            gimp_editor_set_property        (GObject        *object,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);
static void            gimp_editor_get_property        (GObject        *object,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);

static void            gimp_editor_style_updated       (GtkWidget      *widget);

static GimpUIManager * gimp_editor_get_menu            (GimpDocked     *docked,
                                                        const gchar   **ui_path,
                                                        gpointer       *popup_data);
static gboolean        gimp_editor_has_button_bar      (GimpDocked     *docked);
static void            gimp_editor_set_show_button_bar (GimpDocked     *docked,
                                                        gboolean        show);
static gboolean        gimp_editor_get_show_button_bar (GimpDocked     *docked);

static GtkIconSize     gimp_editor_ensure_button_box   (GimpEditor     *editor,
                                                        GtkReliefStyle *button_relief);

static void            gimp_editor_get_styling         (GimpEditor     *editor,
                                                        GimpGuiConfig  *config,
                                                        gint           *content_spacing,
                                                        GtkIconSize    *button_icon_size,
                                                        gint           *button_spacing,
                                                        GtkReliefStyle *button_relief);
static void            gimp_editor_config_size_changed (GimpGuiConfig   *config,
                                                        GimpEditor      *editor);


G_DEFINE_TYPE_WITH_CODE (GimpEditor, gimp_editor, GTK_TYPE_BOX,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_editor_docked_iface_init))

#define parent_class gimp_editor_parent_class


static void
gimp_editor_class_init (GimpEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = gimp_editor_constructed;
  object_class->dispose       = gimp_editor_dispose;
  object_class->set_property  = gimp_editor_set_property;
  object_class->get_property  = gimp_editor_get_property;

  widget_class->style_updated = gimp_editor_style_updated;

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

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("button-relief",
                                                              NULL, NULL,
                                                              GTK_TYPE_RELIEF_STYLE,
                                                              DEFAULT_BUTTON_RELIEF,
                                                              GIMP_PARAM_READABLE));

  g_type_class_add_private (klass, sizeof (GimpEditorPrivate));
}

static void
gimp_editor_docked_iface_init (GimpDockedInterface *iface)
{
  iface->get_menu            = gimp_editor_get_menu;
  iface->has_button_bar      = gimp_editor_has_button_bar;
  iface->set_show_button_bar = gimp_editor_set_show_button_bar;
  iface->get_show_button_bar = gimp_editor_get_show_button_bar;
}

static void
gimp_editor_init (GimpEditor *editor)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  editor->priv            = G_TYPE_INSTANCE_GET_PRIVATE (editor,
                                                         GIMP_TYPE_EDITOR,
                                                         GimpEditorPrivate);
  editor->priv->popup_data      = editor;
  editor->priv->show_button_bar = TRUE;

  editor->priv->name_label = g_object_new (GTK_TYPE_LABEL,
                                     "xalign",    0.0,
                                     "yalign",    0.5,
                                     "ellipsize", PANGO_ELLIPSIZE_END,
                                     NULL);
  gimp_label_set_attributes (GTK_LABEL (editor->priv->name_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (editor), editor->priv->name_label,
                      FALSE, FALSE, 0);
}

static void
gimp_editor_constructed (GObject *object)
{
  GimpEditor *editor = GIMP_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  if (! editor->priv->popup_data)
    editor->priv->popup_data = editor;

  if (editor->priv->menu_factory && editor->priv->menu_identifier)
    {
      editor->priv->ui_manager =
        gimp_menu_factory_manager_new (editor->priv->menu_factory,
                                       editor->priv->menu_identifier,
                                       editor->priv->popup_data);
      g_signal_connect (editor->priv->ui_manager->gimp->config,
                        "size-changed",
                        G_CALLBACK (gimp_editor_config_size_changed),
                        editor);
    }
}

static void
gimp_editor_dispose (GObject *object)
{
  GimpEditor *editor = GIMP_EDITOR (object);

  g_clear_object (&editor->priv->menu_factory);

  g_clear_pointer (&editor->priv->menu_identifier, g_free);

  if (editor->priv->ui_manager)
    {
      g_signal_handlers_disconnect_by_func (editor->priv->ui_manager->gimp->config,
                                            G_CALLBACK (gimp_editor_config_size_changed),
                                            editor);
      g_clear_object (&editor->priv->ui_manager);
    }

  g_clear_pointer (&editor->priv->ui_path, g_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
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
      editor->priv->menu_factory = g_value_dup_object (value);
      break;

    case PROP_MENU_IDENTIFIER:
      editor->priv->menu_identifier = g_value_dup_string (value);
      break;

    case PROP_UI_PATH:
      editor->priv->ui_path = g_value_dup_string (value);
      break;

    case PROP_POPUP_DATA:
      editor->priv->popup_data = g_value_get_pointer (value);
      break;

    case PROP_SHOW_NAME:
      g_object_set_property (G_OBJECT (editor->priv->name_label),
                             "visible", value);
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
      g_value_set_object (value, editor->priv->menu_factory);
      break;

    case PROP_MENU_IDENTIFIER:
      g_value_set_string (value, editor->priv->menu_identifier);
      break;

    case PROP_UI_PATH:
      g_value_set_string (value, editor->priv->ui_path);
      break;

    case PROP_POPUP_DATA:
      g_value_set_pointer (value, editor->priv->popup_data);
      break;

    case PROP_SHOW_NAME:
      g_object_get_property (G_OBJECT (editor->priv->name_label),
                             "visible", value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_editor_style_updated (GtkWidget *widget)
{
  GimpEditor    *editor = GIMP_EDITOR (widget);
  GimpGuiConfig *config = NULL;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (editor->priv->ui_manager)
    config = GIMP_GUI_CONFIG (editor->priv->ui_manager->gimp->config);
  gimp_editor_config_size_changed (config, editor);
}

static GimpUIManager *
gimp_editor_get_menu (GimpDocked   *docked,
                      const gchar **ui_path,
                      gpointer     *popup_data)
{
  GimpEditor *editor = GIMP_EDITOR (docked);

  *ui_path    = editor->priv->ui_path;
  *popup_data = editor->priv->popup_data;

  return editor->priv->ui_manager;
}


static gboolean
gimp_editor_has_button_bar (GimpDocked *docked)
{
  GimpEditor *editor = GIMP_EDITOR (docked);

  return editor->priv->button_box != NULL;
}

static void
gimp_editor_set_show_button_bar (GimpDocked *docked,
                                 gboolean    show)
{
  GimpEditor *editor = GIMP_EDITOR (docked);

  if (show != editor->priv->show_button_bar)
    {
      editor->priv->show_button_bar = show;

      if (editor->priv->button_box)
        gtk_widget_set_visible (editor->priv->button_box, show);
    }
}

static gboolean
gimp_editor_get_show_button_bar (GimpDocked *docked)
{
  GimpEditor *editor = GIMP_EDITOR (docked);

  return editor->priv->show_button_bar;
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

  if (editor->priv->menu_factory)
    g_object_unref (editor->priv->menu_factory);

  editor->priv->menu_factory = g_object_ref (menu_factory);

  if (editor->priv->ui_manager)
    {
      g_signal_handlers_disconnect_by_func (editor->priv->ui_manager->gimp->config,
                                            G_CALLBACK (gimp_editor_config_size_changed),
                                            editor);
      g_object_unref (editor->priv->ui_manager);
    }

  editor->priv->ui_manager = gimp_menu_factory_manager_new (menu_factory,
                                                            menu_identifier,
                                                            popup_data);
  g_signal_connect (editor->priv->ui_manager->gimp->config,
                    "size-changed",
                    G_CALLBACK (gimp_editor_config_size_changed),
                    editor);

  if (editor->priv->ui_path)
    g_free (editor->priv->ui_path);

  editor->priv->ui_path = g_strdup (ui_path);

  editor->priv->popup_data = popup_data;
}

gboolean
gimp_editor_popup_menu (GimpEditor           *editor,
                        GimpMenuPositionFunc  position_func,
                        gpointer              position_data)
{
  g_return_val_if_fail (GIMP_IS_EDITOR (editor), FALSE);

  if (editor->priv->ui_manager && editor->priv->ui_path)
    {
      gimp_ui_manager_update (editor->priv->ui_manager, editor->priv->popup_data);
      gimp_ui_manager_ui_popup (editor->priv->ui_manager, editor->priv->ui_path,
                                GTK_WIDGET (editor),
                                position_func, position_data,
                                NULL, NULL);
      return TRUE;
    }

  return FALSE;
}

GtkWidget *
gimp_editor_add_button (GimpEditor  *editor,
                        const gchar *icon_name,
                        const gchar *tooltip,
                        const gchar *help_id,
                        GCallback    callback,
                        GCallback    extended_callback,
                        gpointer     callback_data)
{
  GtkWidget      *button;
  GtkWidget      *image;
  GtkIconSize     button_icon_size;
  GtkReliefStyle  button_relief;

  g_return_val_if_fail (GIMP_IS_EDITOR (editor), NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);

  button_icon_size = gimp_editor_ensure_button_box (editor, &button_relief);

  button = gimp_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), button_relief);
  gtk_box_pack_start (GTK_BOX (editor->priv->button_box), button, TRUE, TRUE, 0);
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

  image = gtk_image_new_from_icon_name (icon_name, button_icon_size);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  return button;
}

GtkWidget *
gimp_editor_add_icon_box (GimpEditor  *editor,
                          GType        enum_type,
                          const gchar *icon_prefix,
                          GCallback    callback,
                          gpointer     callback_data)
{
  GtkWidget      *hbox;
  GtkWidget      *first_button;
  GtkIconSize     button_icon_size;
  GtkReliefStyle  button_relief;
  GList          *children;
  GList          *list;

  g_return_val_if_fail (GIMP_IS_EDITOR (editor), NULL);
  g_return_val_if_fail (g_type_is_a (enum_type, G_TYPE_ENUM), NULL);
  g_return_val_if_fail (icon_prefix != NULL, NULL);

  button_icon_size = gimp_editor_ensure_button_box (editor, &button_relief);

  hbox = gimp_enum_icon_box_new (enum_type, icon_prefix, button_icon_size,
                                 callback, callback_data,
                                 &first_button);

  children = gtk_container_get_children (GTK_CONTAINER (hbox));

  for (list = children; list; list = g_list_next (list))
    {
      GtkWidget *button = list->data;

      g_object_ref (button);

      gtk_button_set_relief (GTK_BUTTON (button), button_relief);

      gtk_container_remove (GTK_CONTAINER (hbox), button);
      gtk_box_pack_start (GTK_BOX (editor->priv->button_box), button,
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
  GtkReliefStyle   button_relief;
  const gchar     *icon_name;
  gchar           *tooltip;
  const gchar     *help_id;
  GList           *extended = NULL;
  va_list          args;

  g_return_val_if_fail (GIMP_IS_EDITOR (editor), NULL);
  g_return_val_if_fail (action_name != NULL, NULL);
  g_return_val_if_fail (editor->priv->ui_manager != NULL, NULL);

  group = gimp_ui_manager_get_action_group (editor->priv->ui_manager,
                                            group_name);

  g_return_val_if_fail (group != NULL, NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                        action_name);

  g_return_val_if_fail (action != NULL, NULL);

  button_icon_size = gimp_editor_ensure_button_box (editor, &button_relief);

  if (GTK_IS_TOGGLE_ACTION (action))
    button = gtk_toggle_button_new ();
  else
    button = gimp_button_new ();

  gtk_button_set_relief (GTK_BUTTON (button), button_relief);

  icon_name = gtk_action_get_icon_name (action);
  tooltip   = g_strdup (gtk_action_get_tooltip (action));
  help_id   = g_object_get_qdata (G_OBJECT (action), GIMP_HELP_ID);

  old_child = gtk_bin_get_child (GTK_BIN (button));

  if (old_child)
    gtk_widget_destroy (old_child);

  image = gtk_image_new_from_icon_name (icon_name, button_icon_size);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  gtk_activatable_set_related_action (GTK_ACTIVATABLE (button), action);
  gtk_box_pack_start (GTK_BOX (editor->priv->button_box), button,
                      TRUE, TRUE, 0);
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
              const gchar *ext_tooltip = gtk_action_get_tooltip (action);

              if (ext_tooltip)
                {
                  gchar *tmp = g_strconcat (tooltip, "\n<b>",
                                            gimp_get_mod_string (ext->mod_mask),
                                            "</b>  ", ext_tooltip, NULL);
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

  if (tooltip || help_id)
    gimp_help_set_help_data_with_markup (button, tooltip, help_id);

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

  gtk_label_set_text (GTK_LABEL (editor->priv->name_label),
                      name ? name : _("(None)"));
}

void
gimp_editor_set_box_style (GimpEditor *editor,
                           GtkBox     *box)
{
  GimpGuiConfig  *config = NULL;
  GList          *children;
  GList          *list;
  gint            content_spacing;
  GtkIconSize     button_icon_size;
  gint            button_spacing;
  GtkReliefStyle  button_relief;

  g_return_if_fail (GIMP_IS_EDITOR (editor));
  g_return_if_fail (GTK_IS_BOX (box));

  if (editor->priv->ui_manager)
    config = GIMP_GUI_CONFIG (editor->priv->ui_manager->gimp->config);

  gimp_editor_get_styling (editor, config,
                           &content_spacing,
                           &button_icon_size,
                           &button_spacing,
                           &button_relief);

  gtk_box_set_spacing (box, button_spacing);

  children = gtk_container_get_children (GTK_CONTAINER (box));
  for (list = children; list; list = g_list_next (list))
    {
      if (GTK_IS_BUTTON (list->data))
        {
          GtkWidget *child;

          gtk_button_set_relief (GTK_BUTTON (list->data), button_relief);

          child = gtk_bin_get_child (GTK_BIN (list->data));

          if (GTK_IS_IMAGE (child))
            {
              GtkIconSize  old_size;
              const gchar *icon_name;

              gtk_image_get_icon_name (GTK_IMAGE (child), &icon_name, &old_size);

              if (button_icon_size != old_size)
                gtk_image_set_from_icon_name (GTK_IMAGE (child),
                                              icon_name, button_icon_size);
            }
        }
    }

  g_list_free (children);
}

GimpUIManager *
gimp_editor_get_ui_manager (GimpEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_EDITOR (editor), NULL);

  return editor->priv->ui_manager;
}

GtkBox *
gimp_editor_get_button_box (GimpEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_EDITOR (editor), NULL);

  return GTK_BOX (editor->priv->button_box);
}

GimpMenuFactory *

gimp_editor_get_menu_factory (GimpEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_EDITOR (editor), NULL);

  return editor->priv->menu_factory;
}

gpointer *
gimp_editor_get_popup_data (GimpEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_EDITOR (editor), NULL);

  return editor->priv->popup_data;
}

gchar *
gimp_editor_get_ui_path (GimpEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_EDITOR (editor), NULL);

  return editor->priv->ui_path;
}


/*  private functions  */

static GtkIconSize
gimp_editor_ensure_button_box (GimpEditor     *editor,
                               GtkReliefStyle *button_relief)
{
  GimpGuiConfig *config = NULL;
  GtkIconSize    button_icon_size;
  gint           button_spacing;
  gint           content_spacing;

  if (editor->priv->ui_manager)
    {
      Gimp *gimp;

      gimp = editor->priv->ui_manager->gimp;
      config = GIMP_GUI_CONFIG (gimp->config);
    }
  gimp_editor_get_styling (editor, config,
                           &content_spacing,
                           &button_icon_size,
                           &button_spacing,
                           button_relief);

  if (! editor->priv->button_box)
    {
      editor->priv->button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
                                              button_spacing);
      gtk_box_set_homogeneous (GTK_BOX (editor->priv->button_box), TRUE);
      gtk_box_pack_end (GTK_BOX (editor), editor->priv->button_box, FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (editor), editor->priv->button_box, 0);

      if (editor->priv->show_button_bar)
        gtk_widget_show (editor->priv->button_box);
    }

  return button_icon_size;
}

static void
gimp_editor_get_styling (GimpEditor     *editor,
                         GimpGuiConfig  *config,
                         gint           *content_spacing,
                         GtkIconSize    *button_icon_size,
                         gint           *button_spacing,
                         GtkReliefStyle *button_relief)
{
  GimpIconSize size;

  /* Get the theme styling. */
  gtk_widget_style_get (GTK_WIDGET (editor),
                        "content-spacing",  content_spacing,
                        "button-icon-size", button_icon_size,
                        "button-spacing",   button_spacing,
                        "button-relief",    button_relief,
                        NULL);

  /* Check if we should override theme styling. */
  if (config)
    {
      size = gimp_gui_config_detect_icon_size (config);
      switch (size)
        {
        case GIMP_ICON_SIZE_SMALL:
          *button_spacing  = MIN (*button_spacing / 2, 1);
          *content_spacing = MIN (*content_spacing / 2, 1);
        case GIMP_ICON_SIZE_MEDIUM:
          *button_icon_size = GTK_ICON_SIZE_MENU;
          break;
        case GIMP_ICON_SIZE_LARGE:
          *button_icon_size = GTK_ICON_SIZE_LARGE_TOOLBAR;
          *button_spacing  *= 2;
          *content_spacing *= 2;
          break;
        case GIMP_ICON_SIZE_HUGE:
          *button_icon_size = GTK_ICON_SIZE_DND;
          *button_spacing  *= 3;
          *content_spacing *= 3;
          break;
        default:
          /* GIMP_ICON_SIZE_DEFAULT:
           * let's use the sizes set by the theme. */
          break;
        }
    }
}

static void
gimp_editor_config_size_changed (GimpGuiConfig *config,
                                 GimpEditor    *editor)
{
  gint            content_spacing;
  GtkIconSize     button_icon_size;
  gint            button_spacing;
  GtkReliefStyle  button_relief;

  gimp_editor_get_styling (editor, config,
                           &content_spacing,
                           &button_icon_size,
                           &button_spacing,
                           &button_relief);

  /* Editor styling. */
  gtk_box_set_spacing (GTK_BOX (editor), content_spacing);

  /* Button box styling. */
  if (editor->priv->button_box)
    gimp_editor_set_box_style (editor,
                               GTK_BOX (editor->priv->button_box));
}
