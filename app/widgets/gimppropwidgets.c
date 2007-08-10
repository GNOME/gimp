/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropwidgets.c
 * Copyright (C) 2002-2004  Michael Natterer <mitch@gimp.org>
 *                          Sven Neumann <sven@gimp.org>
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
#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpviewable.h"

#include "gimpcolorpanel.h"
#include "gimpdnd.h"
#include "gimpview.h"
#include "gimppropwidgets.h"
#include "gimpwidgets-constructors.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


/*  utility function prototypes  */

static void         set_param_spec     (GObject     *object,
                                        GtkWidget   *widget,
                                        GParamSpec  *param_spec);
static GParamSpec * get_param_spec     (GObject     *object);

static GParamSpec * find_param_spec    (GObject     *object,
                                        const gchar *property_name,
                                        const gchar *strloc);
static GParamSpec * check_param_spec   (GObject     *object,
                                        const gchar *property_name,
                                        GType         type,
                                        const gchar *strloc);
static GParamSpec * check_param_spec_w (GObject     *object,
                                        const gchar *property_name,
                                        GType         type,
                                        const gchar *strloc);

static void         connect_notify     (GObject     *config,
                                        const gchar *property_name,
                                        GCallback    callback,
                                        gpointer     callback_data);


/*********************/
/*  expanding frame  */
/*********************/

GtkWidget *
gimp_prop_expanding_frame_new (GObject      *config,
                               const gchar  *property_name,
                               const gchar  *button_label,
                               GtkWidget    *child,
                               GtkWidget   **button)
{
  GtkWidget *frame;
  GtkWidget *toggle;
  gboolean   value;

  if (! check_param_spec_w (config, property_name,
                            G_TYPE_PARAM_BOOLEAN, G_STRFUNC))
    return NULL;

  frame = gimp_frame_new (NULL);

  toggle = gimp_prop_check_button_new (config, property_name, button_label);
  gtk_frame_set_label_widget (GTK_FRAME (frame), toggle);
  gtk_widget_show (toggle);

  gtk_container_add (GTK_CONTAINER (frame), child);

  g_object_get (config,
                property_name, &value,
                NULL);

  if (value)
    gtk_widget_show (child);

  g_signal_connect_object (toggle, "toggled",
                           G_CALLBACK (gimp_toggle_button_set_visible),
                           child, 0);

  if (button)
    *button = toggle;

  return frame;
}


/****************/
/*  paint menu  */
/****************/

static void   gimp_prop_paint_menu_callback (GtkWidget   *widget,
                                             GObject     *config);
static void   gimp_prop_paint_menu_notify   (GObject     *config,
                                             GParamSpec  *param_spec,
                                             GtkWidget   *menu);

/**
 * gimp_prop_paint_mode_menu_new:
 * @config:             #GimpConfig object to which property is attached.
 * @property_name:      Name of Enum property.
 * @with_behind_mode:   Whether to include "Behind" mode in the menu.
 * @with_replace_modes: Whether to include the "Replace", "Erase" and
 *                      "Anti Erase" modes in the menu.
 *
 * Creates a #GimpPaintModeMenu widget to display and set the specified
 * Enum property, for which the enum must be #GimpLayerModeEffects.
 *
 * Return value: The newly created #GimpPaintModeMenu widget.
 *
 * Since GIMP 2.4
 */
GtkWidget *
gimp_prop_paint_mode_menu_new (GObject     *config,
                               const gchar *property_name,
                               gboolean     with_behind_mode,
                               gboolean     with_replace_modes)
{
  GParamSpec *param_spec;
  GtkWidget  *menu;
  gint        value;

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_ENUM, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  menu = gimp_paint_mode_menu_new (with_behind_mode, with_replace_modes);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (menu),
                              value,
                              G_CALLBACK (gimp_prop_paint_menu_callback),
                              config);

  set_param_spec (G_OBJECT (menu), menu, param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_paint_menu_notify),
                  menu);

  return menu;
}

static void
gimp_prop_paint_menu_callback (GtkWidget *widget,
                               GObject   *config)
{
  GParamSpec *param_spec;
  gint        value;

  param_spec = get_param_spec (G_OBJECT (widget));
  if (! param_spec)
    return;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value))
    {
      g_object_set (config,
                    param_spec->name, value,
                    NULL);
    }
}

static void
gimp_prop_paint_menu_notify (GObject    *config,
                             GParamSpec *param_spec,
                             GtkWidget  *menu)
{
  gint value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (menu,
                                   gimp_prop_paint_menu_callback,
                                   config);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (menu), value);

  g_signal_handlers_unblock_by_func (menu,
                                     gimp_prop_paint_menu_callback,
                                     config);

}


/******************/
/*  color button  */
/******************/

static void   gimp_prop_color_button_callback (GtkWidget  *widget,
                                               GObject    *config);
static void   gimp_prop_color_button_notify   (GObject    *config,
                                               GParamSpec *param_spec,
                                               GtkWidget  *button);

/**
 * gimp_prop_color_button_new:
 * @config:        #GimpConfig object to which property is attached.
 * @property_name: Name of #GimpRGB property.
 * @title:         Title of the #GimpColorPanel that is to be created
 * @width:         Width of color button.
 * @height:        Height of color button.
 * @type:          How transparency is represented.
 *
 * Creates a #GimpColorPanel to set and display the value of a #GimpRGB
 * property.  Pressing the button brings up a color selector dialog.
 *
 * Return value:  A new #GimpColorPanel widget.
 *
 * Since GIMP 2.4
 */
GtkWidget *
gimp_prop_color_button_new (GObject           *config,
                            const gchar       *property_name,
                            const gchar       *title,
                            gint               width,
                            gint               height,
                            GimpColorAreaType  type)
{
  GParamSpec *param_spec;
  GtkWidget  *button;
  GimpRGB    *value;

  param_spec = check_param_spec_w (config, property_name,
                                   GIMP_TYPE_PARAM_RGB, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  button = gimp_color_panel_new (title, value, type, width, height);
  g_free (value);

  set_param_spec (G_OBJECT (button), button, param_spec);

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (gimp_prop_color_button_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_color_button_notify),
                  button);

  return button;
}

static void
gimp_prop_color_button_callback (GtkWidget *button,
                                 GObject   *config)
{
  GParamSpec *param_spec;
  GimpRGB     value;

  param_spec = get_param_spec (G_OBJECT (button));
  if (! param_spec)
    return;

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (button), &value);

  g_signal_handlers_block_by_func (config,
                                   gimp_prop_color_button_notify,
                                   button);

  g_object_set (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_unblock_by_func (config,
                                     gimp_prop_color_button_notify,
                                     button);
}

static void
gimp_prop_color_button_notify (GObject    *config,
                               GParamSpec *param_spec,
                               GtkWidget  *button)
{
  GimpRGB *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (button,
                                   gimp_prop_color_button_callback,
                                   config);

  gimp_color_button_set_color (GIMP_COLOR_BUTTON (button), value);

  g_free (value);

  g_signal_handlers_unblock_by_func (button,
                                     gimp_prop_color_button_callback,
                                     config);
}


/*************/
/*  view  */
/*************/

static void   gimp_prop_view_drop   (GtkWidget    *menu,
                                     gint          x,
                                     gint          y,
                                     GimpViewable *viewable,
                                     gpointer      data);
static void   gimp_prop_view_notify (GObject      *config,
                                     GParamSpec   *param_spec,
                                     GtkWidget    *view);

/**
 * gimp_prop_view_new:
 * @config:        #GimpConfig object to which property is attached.
 * @context:       a #Gimpcontext.
 * @property_name: Name of #GimpViewable property.
 * @size:          Width and height of preview display.
 *
 * Creates a widget to display the value of a #GimpViewable property.
 *
 * Return value:  A new #GimpView widget.
 *
 * Since GIMP 2.4
 */
GtkWidget *
gimp_prop_view_new (GObject     *config,
                    const gchar *property_name,
                    GimpContext *context,
                    gint         size)
{
  GParamSpec   *param_spec;
  GtkWidget    *view;
  GimpViewable *viewable;

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_OBJECT, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! g_type_is_a (param_spec->value_type, GIMP_TYPE_VIEWABLE))
    {
      g_warning ("%s: property '%s' of %s is not a GimpViewable",
                 G_STRFUNC, property_name,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)));
      return NULL;
    }

  view = gimp_view_new_by_types (context,
                                 GIMP_TYPE_VIEW,
                                 param_spec->value_type,
                                 size, 0, FALSE);

  if (! view)
    {
      g_warning ("%s: cannot create view for type '%s'",
                 G_STRFUNC, g_type_name (param_spec->value_type));
      return NULL;
    }

  g_object_get (config,
                property_name, &viewable,
                NULL);

  if (viewable)
    {
      gimp_view_set_viewable (GIMP_VIEW (view), viewable);
      g_object_unref (viewable);
    }

  set_param_spec (G_OBJECT (view), view, param_spec);

  gimp_dnd_viewable_dest_add (view, param_spec->value_type,
                              gimp_prop_view_drop,
                              config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_view_notify),
                  view);

  return view;
}

static void
gimp_prop_view_drop (GtkWidget    *view,
                     gint          x,
                     gint          y,
                     GimpViewable *viewable,
                     gpointer      data)
{
  GObject    *config;
  GParamSpec *param_spec;

  param_spec = get_param_spec (G_OBJECT (view));
  if (! param_spec)
    return;

  config = G_OBJECT (data);

  g_object_set (config,
                param_spec->name, viewable,
                NULL);
}

static void
gimp_prop_view_notify (GObject      *config,
                       GParamSpec   *param_spec,
                       GtkWidget    *view)
{
  GimpViewable *viewable;

  g_object_get (config,
                param_spec->name, &viewable,
                NULL);

  gimp_view_set_viewable (GIMP_VIEW (view), viewable);

  if (viewable)
    g_object_unref (viewable);
}


/***********************
 *  number pair entry  *
 ***********************/

typedef struct
{
  GObject     *config;
  const gchar *left_number_property;
  const gchar *right_number_property;
  const gchar *default_left_number_property;
  const gchar *default_right_number_property;
  const gchar *user_override_property;
} GimpPropNumberPairEntryData;

static void
gimp_prop_number_pair_entry_data_free (GimpPropNumberPairEntryData *data)
{
  g_slice_free (GimpPropNumberPairEntryData, data);
}


static void  gimp_prop_number_pair_entry_config_notify
                                                         (GObject                     *config,
                                                          GParamSpec                  *param_spec,
                                                          GtkEntry                    *entry);
static void  gimp_prop_number_pair_entry_number_pair_numbers_changed
                                                         (GtkWidget                   *widget,
                                                          GimpPropNumberPairEntryData *data);
static void  gimp_prop_number_pair_entry_number_pair_user_override_changed
                                                         (GtkWidget                   *entry,
                                                          GParamSpec                  *param_spec,
                                                          GimpPropNumberPairEntryData *data);


/**
 * gimp_prop_number_pair_entry_new:
 * @config:                        Object to which properties are attached.
 * @left_number_property:          Name of double property for left number.
 * @right_number_property:         Name of double property for right number.
 * @default_left_number_property:  Name of double property for default left number.
 * @default_right_number_property: Name of double property for default right number.
 * @user_override_property:        Name of boolean property for user override mode.
 * @connect_numbers_changed:       %TRUE to connect to the widgets "numbers-changed"
 *                                 signal, %FALSE to not connect.
 * @connect_numbers_changed:       %TRUE to connect to the widgets "ratio-changed"
 *                                 signal, %FALSE to not connect.
 * @separators:
 * @allow_simplification:
 * @min_valid_value:
 * @max_valid_value:         What to pass to gimp_number_pair_entry_new ().
 *
 * Return value: A #GimpNumberPairEntry widget.
 */
GtkWidget * gimp_prop_number_pair_entry_new (GObject     *config,
                                             const gchar *left_number_property,
                                             const gchar *right_number_property,
                                             const gchar *default_left_number_property,
                                             const gchar *default_right_number_property,
                                             const gchar *user_override_property,
                                             gboolean     connect_numbers_changed,
                                             gboolean     connect_ratio_changed,
                                             const gchar *separators,
                                             gboolean     allow_simplification,
                                             gdouble      min_valid_value,
                                             gdouble      max_valid_value)
{
  GimpPropNumberPairEntryData *data;
  GtkWidget                   *number_pair_entry;
  gdouble                      left_number;
  gdouble                      right_number;
  gdouble                      default_left_number;
  gdouble                      default_right_number;
  gboolean                     user_override;


  /* Setup config data */

  data = g_slice_new (GimpPropNumberPairEntryData);

  data->config                        = config;
  data->left_number_property          = left_number_property;
  data->right_number_property         = right_number_property;
  data->default_left_number_property  = default_left_number_property;
  data->default_right_number_property = default_right_number_property;
  data->user_override_property        = user_override_property;


  /* Read current values of config properties */

  g_object_get (config,
                left_number_property,          &left_number,
                right_number_property,         &right_number,
                default_left_number_property,  &default_left_number,
                default_right_number_property, &default_right_number,
                user_override_property,        &user_override,
                NULL);


  /* Create a GimpNumberPairEntry and setup with config property values */

  number_pair_entry = gimp_number_pair_entry_new (separators,
                                                  allow_simplification,
                                                  min_valid_value,
                                                  max_valid_value);

  g_object_set_data_full (G_OBJECT (number_pair_entry),
                          "gimp-prop-number-pair-entry-data", data,
                          (GDestroyNotify) gimp_prop_number_pair_entry_data_free);

  gtk_entry_set_width_chars (GTK_ENTRY (number_pair_entry), 7);

  gimp_number_pair_entry_set_user_override  (GIMP_NUMBER_PAIR_ENTRY (number_pair_entry),
                                             user_override);
  gimp_number_pair_entry_set_values         (GIMP_NUMBER_PAIR_ENTRY (number_pair_entry),
                                             left_number,
                                             right_number);
  gimp_number_pair_entry_set_default_values (GIMP_NUMBER_PAIR_ENTRY (number_pair_entry),
                                             default_left_number,
                                             default_right_number);


  /* Connect to GimpNumberPairEntry signals */

  if (connect_ratio_changed)
    g_signal_connect (number_pair_entry, "ratio-changed",
                      G_CALLBACK (gimp_prop_number_pair_entry_number_pair_numbers_changed),
                      data);

  if (connect_numbers_changed)
    g_signal_connect (number_pair_entry, "numbers-changed",
                      G_CALLBACK (gimp_prop_number_pair_entry_number_pair_numbers_changed),
                      data);

  g_signal_connect (number_pair_entry, "notify::user-override",
                    G_CALLBACK (gimp_prop_number_pair_entry_number_pair_user_override_changed),
                    data);


  /* Connect to connfig object signals */

  connect_notify (config, left_number_property,
                  G_CALLBACK (gimp_prop_number_pair_entry_config_notify),
                  number_pair_entry);
  connect_notify (config, right_number_property,
                  G_CALLBACK (gimp_prop_number_pair_entry_config_notify),
                  number_pair_entry);
  connect_notify (config, default_left_number_property,
                  G_CALLBACK (gimp_prop_number_pair_entry_config_notify),
                  number_pair_entry);
  connect_notify (config, default_right_number_property,
                  G_CALLBACK (gimp_prop_number_pair_entry_config_notify),
                  number_pair_entry);
  connect_notify (config, user_override_property,
                  G_CALLBACK (gimp_prop_number_pair_entry_config_notify),
                  number_pair_entry);


  /* Done */

  return number_pair_entry;
}

static void
gimp_prop_number_pair_entry_config_notify (GObject    *config,
                                           GParamSpec *param_spec,
                                           GtkEntry   *number_pair_entry)
{
  GimpPropNumberPairEntryData *data =
    g_object_get_data (G_OBJECT (number_pair_entry),
                       "gimp-prop-number-pair-entry-data");

  g_return_if_fail (data != NULL);

  if (strcmp (param_spec->name, "left-number")  == 0 ||
      strcmp (param_spec->name, "right-number") == 0)
    {
      gdouble left_number;
      gdouble right_number;

      g_object_get (config,
                    data->left_number_property,  &left_number,
                    data->right_number_property, &right_number,
                    NULL);

      gimp_number_pair_entry_set_values (GIMP_NUMBER_PAIR_ENTRY (number_pair_entry),
                                         left_number,
                                         right_number);
    }
  else if (strcmp (param_spec->name, "default-left-number")  == 0 ||
           strcmp (param_spec->name, "default-right-number") == 0)
    {
      gdouble default_left_number;
      gdouble default_right_number;

      g_object_get (config,
                    data->default_left_number_property,  &default_left_number,
                    data->default_right_number_property, &default_right_number,
                    NULL);

      gimp_number_pair_entry_set_default_values (GIMP_NUMBER_PAIR_ENTRY (number_pair_entry),
                                                 default_left_number,
                                                 default_right_number);
    }
  else if (strcmp (param_spec->name, "user-override")  == 0)
    {
      gboolean user_override;

      g_object_get (config,
                    data->user_override_property, &user_override,
                    NULL);

      gimp_number_pair_entry_set_user_override (GIMP_NUMBER_PAIR_ENTRY (number_pair_entry),
                                                user_override);
    }
}

static void
gimp_prop_number_pair_entry_number_pair_numbers_changed (GtkWidget                   *widget,
                                                         GimpPropNumberPairEntryData *data)
{
  gdouble left_number;
  gdouble right_number;

  gimp_number_pair_entry_get_values (GIMP_NUMBER_PAIR_ENTRY (widget),
                                     &left_number,
                                     &right_number);

  g_object_set (data->config,
                data->left_number_property,  left_number,
                data->right_number_property, right_number,
                NULL);
}

static void
gimp_prop_number_pair_entry_number_pair_user_override_changed (GtkWidget                   *entry,
                                                               GParamSpec                  *param_spec,
                                                               GimpPropNumberPairEntryData *data)

{
  gboolean user_override;

  user_override =
    gimp_number_pair_entry_get_user_override (GIMP_NUMBER_PAIR_ENTRY (entry));

  g_object_set (data->config,
                data->user_override_property, user_override,
                NULL);
}


/*******************************/
/*  private utility functions  */
/*******************************/

static GQuark gimp_prop_widgets_param_spec_quark (void) G_GNUC_CONST;

#define PARAM_SPEC_QUARK (gimp_prop_widgets_param_spec_quark ())

static GQuark
gimp_prop_widgets_param_spec_quark (void)
{
  static GQuark param_spec_quark = 0;

  if (! param_spec_quark)
    param_spec_quark = g_quark_from_static_string ("gimp-config-param-spec");

  return param_spec_quark;
}

static void
set_param_spec (GObject     *object,
                GtkWidget   *widget,
                GParamSpec  *param_spec)
{
  if (object)
    {
      g_object_set_qdata (object, PARAM_SPEC_QUARK, param_spec);
    }

  if (widget)
    {
      const gchar *blurb = g_param_spec_get_blurb (param_spec);

      if (blurb)
        gimp_help_set_help_data (widget, gettext (blurb), NULL);
    }
}

static GParamSpec *
get_param_spec (GObject *object)
{
  return g_object_get_qdata (object, PARAM_SPEC_QUARK);
}

static GParamSpec *
find_param_spec (GObject     *object,
                 const gchar *property_name,
                 const gchar *strloc)
{
  GParamSpec *param_spec;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                             property_name);

  if (! param_spec)
    g_warning ("%s: %s has no property named '%s'",
               strloc,
               g_type_name (G_TYPE_FROM_INSTANCE (object)),
               property_name);

  return param_spec;
}

static GParamSpec *
check_param_spec (GObject     *object,
                  const gchar *property_name,
                  GType        type,
                  const gchar *strloc)
{
  GParamSpec *param_spec;

  param_spec = find_param_spec (object, property_name, strloc);

  if (param_spec && ! g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), type))
    {
      g_warning ("%s: property '%s' of %s is not a %s",
                 strloc,
                 param_spec->name,
                 g_type_name (param_spec->owner_type),
                 g_type_name (type));
      return NULL;
    }

  return param_spec;
}

static GParamSpec *
check_param_spec_w (GObject     *object,
                    const gchar *property_name,
                    GType        type,
                    const gchar *strloc)
{
  GParamSpec *param_spec;

  param_spec = check_param_spec (object, property_name, type, strloc);

  if (param_spec &&
      (param_spec->flags & G_PARAM_WRITABLE) == 0)
    {
      g_warning ("%s: property '%s' of %s is writable",
                 strloc,
                 param_spec->name,
                 g_type_name (param_spec->owner_type));
      return NULL;
    }

  return param_spec;
}

static void
connect_notify (GObject     *config,
                const gchar *property_name,
                GCallback    callback,
                gpointer     callback_data)
{
  gchar *notify_name;

  notify_name = g_strconcat ("notify::", property_name, NULL);

  g_signal_connect_object (config, notify_name, callback, callback_data, 0);

  g_free (notify_name);
}
