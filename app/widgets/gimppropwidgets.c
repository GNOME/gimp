/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropwidgets.c
 * Copyright (C) 2002-2004  Michael Natterer <mitch@gimp.org>
 *                          Sven Neumann <sven@gimp.org>
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

#include <string.h>
#include <stdlib.h>

#include <gegl.h>
#include <gegl-paramspecs.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpviewable.h"

#include "gimpcheckexpander.h"
#include "gimpcolorbar.h"
#include "gimpcolorpanel.h"
#include "gimpcompressioncombobox.h"
#include "gimpdial.h"
#include "gimpdnd.h"
#include "gimphandlebar.h"
#include "gimpiconpicker.h"
#include "gimplanguagecombobox.h"
#include "gimplanguageentry.h"
#include "gimplayermodebox.h"
#include "gimpview.h"
#include "gimppolar.h"
#include "gimppropwidgets.h"
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
                                        GType        type,
                                        const gchar *strloc);
static GParamSpec * check_param_spec_w (GObject     *object,
                                        const gchar *property_name,
                                        GType        type,
                                        const gchar *strloc);

static void         connect_notify     (GObject     *config,
                                        const gchar *property_name,
                                        GCallback    callback,
                                        gpointer     callback_data);


/*********************/
/*  expanding frame  */
/*********************/

/**
 * gimp_prop_expanding_frame_new:
 * @config:        #GimpConfig object to which property is attached.
 * @property_name: Name of boolean property.
 * @button_label:  Toggle widget title appearing as a frame title.
 * @child:         Child #GtkWidget of the returned frame.
 * @button:        Pointer to the #GtkCheckButton used as frame title
 *                 if not %NULL.
 *
 * Creates a #GimpFrame containing @child, using a #GtkCheckButton as a
 * title whose value is tied to the boolean @property_name.
 * @child will be visible when @property_name is %TRUE, hidden otherwise.
 * If @button_label is %NULL, the @property_name's nick will be used as
 * label of the #GtkCheckButton title.
 *
 * Returns:  A new #GimpFrame widget.
 *
 * Since GIMP 2.4
 */
GtkWidget *
gimp_prop_expanding_frame_new (GObject      *config,
                               const gchar  *property_name,
                               const gchar  *button_label,
                               GtkWidget    *child,
                               GtkWidget   **button)
{
  GParamSpec *param_spec;
  GtkWidget  *frame;
  GtkWidget  *toggle;

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_BOOLEAN, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! button_label)
    button_label = g_param_spec_get_nick (param_spec);

  frame = gimp_frame_new (NULL);

  toggle = gimp_prop_check_button_new (config, property_name, button_label);
  gtk_frame_set_label_widget (GTK_FRAME (frame), toggle);
  gtk_widget_show (toggle);

  gtk_container_add (GTK_CONTAINER (frame), child);

  g_object_bind_property (G_OBJECT (config), property_name,
                          G_OBJECT (child),  "visible",
                          G_BINDING_SYNC_CREATE);

  if (button)
    *button = toggle;

  gimp_widget_set_bound_property (frame, config, property_name);
  gtk_widget_show (frame);

  return frame;
}

/**
 * gimp_prop_check_expander_new:
 * @config:               #GimpConfig object to which properties are attached.
 * @property_name:        Name of boolean property controlling the checkbox.
 * @expand_property_name: Name of boolean property controlling the expansion.
 * @child:                Child #GtkWidget being expanded.
 *
 * Creates a #GimpCheckExpander expanding @child.
 *
 * Returns:  A new #GimpCheckExpander widget.
 *
 * Since: 3.2
 */
GtkWidget *
gimp_prop_check_expander_new (GObject     *config,
                              const gchar *property_name,
                              const gchar *expand_property_name,
                              GtkWidget   *child)
{
  GParamSpec  *param_spec;
  GtkWidget   *frame;
  const gchar *check_label;

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_BOOLEAN, G_STRFUNC);
  if (! param_spec)
    return NULL;

  check_label = g_param_spec_get_nick (param_spec);

  param_spec = check_param_spec_w (config, expand_property_name,
                                   G_TYPE_PARAM_BOOLEAN, G_STRFUNC);
  if (! param_spec)
    return NULL;

  frame = gimp_check_expander_new (check_label, child);

  g_object_bind_property (G_OBJECT (config), property_name,
                          G_OBJECT (frame),  "checked",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (G_OBJECT (config), expand_property_name,
                          G_OBJECT (frame),  "expanded",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);

  gimp_widget_set_bound_property (frame, config, property_name);
  gtk_widget_set_visible (frame, TRUE);

  return frame;
}


/**********************/
/*  boolean icon box  */
/**********************/

static void  gimp_prop_radio_button_callback (GtkWidget  *widget,
                                              GObject    *config);
static void  gimp_prop_radio_button_notify   (GObject    *config,
                                              GParamSpec *param_spec,
                                              GtkWidget  *button);

GtkWidget *
gimp_prop_boolean_icon_box_new (GObject     *config,
                                const gchar *property_name,
                                const gchar *true_icon,
                                const gchar *false_icon,
                                const gchar *true_tooltip,
                                const gchar *false_tooltip)
{
  GParamSpec *param_spec;
  GtkWidget  *box;
  GtkWidget  *button;
  GtkWidget  *first_button;
  GtkWidget  *image;
  GSList     *group = NULL;
  gboolean    value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_BOOLEAN, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  button = first_button = gtk_radio_button_new (group);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_icon_name (true_icon, GTK_ICON_SIZE_MENU);
  if (image)
    {
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);
    }

  gimp_help_set_help_data (button, true_tooltip, NULL);


  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (TRUE));

  set_param_spec (G_OBJECT (button), NULL, param_spec);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_prop_radio_button_callback),
                    config);

  button = gtk_radio_button_new (group);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_icon_name (false_icon, GTK_ICON_SIZE_MENU);
  if (image)
    {
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);
    }

  gimp_help_set_help_data (button, false_tooltip, NULL);

  g_object_set_data (G_OBJECT (button), "gimp-item-data",
                     GINT_TO_POINTER (FALSE));

  set_param_spec (G_OBJECT (button), NULL, param_spec);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_prop_radio_button_callback),
                    config);

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (first_button), value);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_radio_button_notify),
                  button);

  gimp_widget_set_bound_property (box, config, property_name);
  gtk_widget_show (box);

  return box;
}

static void
gimp_prop_radio_button_callback (GtkWidget *widget,
                                 GObject   *config)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GParamSpec *param_spec;
      gint        value;
      gint        v;

      param_spec = get_param_spec (G_OBJECT (widget));
      if (! param_spec)
        return;

      value = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "gimp-item-data"));

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != value)
        g_object_set (config, param_spec->name, value, NULL);
    }
}

static void
gimp_prop_radio_button_notify (GObject    *config,
                               GParamSpec *param_spec,
                               GtkWidget  *button)
{
  gint value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (button), value);
}


/********************/
/*  layer mode box  */
/********************/

/**
 * gimp_prop_layer_mode_box_new:
 * @config:        #GimpConfig object to which property is attached.
 * @property_name: Name of Enum property.
 * @context:       A context mask, determining the set of modes to
 *                 include in the menu.
 *
 * Creates a #GimpLayerModeBox widget to display and set the specified
 * Enum property, for which the enum must be #GimpLayerMode.
 *
 * Returns: The newly created #GimpLayerModeBox widget.
 *
 * Since GIMP 2.10
 */
GtkWidget *
gimp_prop_layer_mode_box_new (GObject              *config,
                              const gchar          *property_name,
                              GimpLayerModeContext  context)
{
  GParamSpec *param_spec;
  GtkWidget  *box;

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_ENUM, G_STRFUNC);
  if (! param_spec)
    return NULL;

  box = gimp_layer_mode_box_new (context);

  g_object_bind_property (config,          property_name,
                          G_OBJECT (box), "layer-mode",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);

  gimp_widget_set_bound_property (box, config, property_name);
  gtk_widget_show (box);

  return box;
}


/******************/
/*  color button  */
/******************/

static void   gimp_prop_gegl_color_button_callback (GtkWidget  *widget,
                                                    GObject    *config);
static void   gimp_prop_gegl_color_button_notify   (GObject    *config,
                                                    GParamSpec *param_spec,
                                                    GtkWidget  *button);

/**
 * gimp_prop_color_button_new:
 * @config:        #GimpConfig object to which property is attached.
 * @property_name: Name of #GeglColor property.
 * @title:         Title of the #GimpColorPanel that is to be created
 * @width:         Width of color button.
 * @height:        Height of color button.
 * @type:          How transparency is represented.
 *
 * Creates a #GimpColorPanel to set and display the value of a #GeglColor
 * property.  Pressing the button brings up a color selector dialog.
 * If @title is %NULL, the @property_name's nick will be used as label
 * of the returned widget.
 *
 * Returns:  A new #GimpColorPanel widget.
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
  GeglColor  *color = NULL;

  param_spec = check_param_spec_w (config, property_name,
                                   GIMP_TYPE_PARAM_COLOR, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! title)
    title = g_param_spec_get_nick (param_spec);

  g_object_get (config,
                property_name, &color,
                NULL);

  button = gimp_color_panel_new (title, color, type, width, height);
  g_clear_object (&color);

  set_param_spec (G_OBJECT (button), button, param_spec);

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (gimp_prop_gegl_color_button_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_gegl_color_button_notify),
                  button);

  gimp_widget_set_bound_property (button, config, property_name);
  gtk_widget_show (button);

  return button;
}

static void
gimp_prop_gegl_color_button_callback (GtkWidget *button,
                                      GObject   *config)
{
  GParamSpec *param_spec;
  GeglColor  *color;

  param_spec = get_param_spec (G_OBJECT (button));
  if (! param_spec)
    return;

  g_signal_handlers_block_by_func (config,
                                   gimp_prop_gegl_color_button_notify,
                                   button);

  color = gimp_color_button_get_color (GIMP_COLOR_BUTTON (button));
  g_object_set (config,
                param_spec->name, color,
                NULL);
  g_object_unref (color);

  g_signal_handlers_unblock_by_func (config,
                                     gimp_prop_gegl_color_button_notify,
                                     button);
}

static void
gimp_prop_gegl_color_button_notify (GObject    *config,
                                    GParamSpec *param_spec,
                                    GtkWidget  *button)
{
  GeglColor *color;

  g_object_get (config,
                param_spec->name, &color,
                NULL);

  g_signal_handlers_block_by_func (button,
                                   gimp_prop_gegl_color_button_callback,
                                   config);

  gimp_color_button_set_color (GIMP_COLOR_BUTTON (button), color);
  g_clear_object (&color);

  g_signal_handlers_unblock_by_func (button,
                                     gimp_prop_gegl_color_button_callback,
                                     config);
}


/************/
/*  angles  */
/************/

static gboolean
deg_to_rad (GBinding     *binding,
            const GValue *from_value,
            GValue       *to_value,
            gpointer      user_data)
{
  gdouble *lower = user_data;
  gdouble  value = g_value_get_double (from_value);

  if (lower && *lower != 0.0)
    {
      if (value < 0.0)
        value += 360.0;
    }

  value *= G_PI / 180.0;

  g_value_set_double (to_value, value);

  return TRUE;
}

static gboolean
rad_to_deg (GBinding     *binding,
            const GValue *from_value,
            GValue       *to_value,
            gpointer      user_data)
{
  gdouble *lower = user_data;
  gdouble  value = g_value_get_double (from_value);

  value *= 180.0 / G_PI;

  if (lower && *lower != 0.0)
    {
      if (value > (*lower + 360.0))
        value -= 360.0;
    }

  g_value_set_double (to_value, value);

  return TRUE;
}

/**
 * gimp_prop_angle_dial_new:
 * @config:        #GimpConfig object to which property is attached.
 * @property_name: Name of gdouble property
 *
 * Creates a #GimpDial to set and display the value of a
 * gdouble property that represents an angle.
 *
 * Returns:  A new #GimpDial widget.
 *
 * Since GIMP 2.10
 */
GtkWidget *
gimp_prop_angle_dial_new (GObject     *config,
                          const gchar *property_name)
{
  GParamSpec *param_spec;
  GtkWidget  *dial;
  gdouble     value;
  gdouble     lower;
  gdouble     upper;

  param_spec = find_param_spec (config, property_name, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! _gimp_prop_widgets_get_numeric_values (config, param_spec,
                                               &value, &lower, &upper,
                                               G_STRFUNC))
    return NULL;

  dial = gimp_dial_new ();

  g_object_set (dial,
                "size",       32,
                "background", GIMP_CIRCLE_BACKGROUND_PLAIN,
                "draw-beta",  FALSE,
                NULL);

  set_param_spec (G_OBJECT (dial), dial, param_spec);

  if (lower == 0.0 && upper == 2 * G_PI)
    {
      g_object_bind_property (config, property_name,
                              dial,   "alpha",
                              G_BINDING_BIDIRECTIONAL |
                              G_BINDING_SYNC_CREATE);
    }
  else if ((upper - lower) == 360.0)
    {
      gdouble *l = g_new0 (gdouble, 1);

      *l = lower;

      g_object_bind_property_full (config, property_name,
                                   dial,   "alpha",
                                   G_BINDING_BIDIRECTIONAL |
                                   G_BINDING_SYNC_CREATE,
                                   deg_to_rad,
                                   rad_to_deg,
                                   l, (GDestroyNotify) g_free);
    }

  gimp_widget_set_bound_property (dial, config, property_name);
  gtk_widget_show (dial);

  return dial;
}

GtkWidget *
gimp_prop_angle_range_dial_new (GObject     *config,
                                const gchar *alpha_property_name,
                                const gchar *beta_property_name,
                                const gchar *clockwise_property_name)
{
  GParamSpec *alpha_param_spec;
  GParamSpec *beta_param_spec;
  GParamSpec *clockwise_param_spec;
  GtkWidget  *dial;

  alpha_param_spec = find_param_spec (config, alpha_property_name, G_STRFUNC);
  if (! alpha_param_spec)
    return NULL;

  beta_param_spec = find_param_spec (config, beta_property_name, G_STRFUNC);
  if (! beta_param_spec)
    return NULL;

  clockwise_param_spec = find_param_spec (config, clockwise_property_name, G_STRFUNC);
  if (! clockwise_param_spec)
    return NULL;

  dial = gimp_dial_new ();

  g_object_set (dial,
                "size",         96,
                "border-width", 0,
                "background",   GIMP_CIRCLE_BACKGROUND_HSV,
                NULL);

  g_object_bind_property_full (config, alpha_property_name,
                               dial,   "alpha",
                               G_BINDING_BIDIRECTIONAL |
                               G_BINDING_SYNC_CREATE,
                               deg_to_rad,
                               rad_to_deg,
                               NULL, NULL);

  g_object_bind_property_full (config, beta_property_name,
                               dial,   "beta",
                               G_BINDING_BIDIRECTIONAL |
                               G_BINDING_SYNC_CREATE,
                               deg_to_rad,
                               rad_to_deg,
                               NULL, NULL);

  g_object_bind_property (config, clockwise_property_name,
                          dial,   "clockwise-delta",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);

  gimp_widget_set_bound_property (dial, config, alpha_property_name);
  gtk_widget_show (dial);

  return dial;
}

GtkWidget *
gimp_prop_polar_new (GObject     *config,
                     const gchar *angle_property_name,
                     const gchar *radius_property_name)
{
  GParamSpec *angle_param_spec;
  GParamSpec *radius_param_spec;
  GtkWidget  *polar;

  angle_param_spec = find_param_spec (config, angle_property_name, G_STRFUNC);
  if (! angle_param_spec)
    return NULL;

  radius_param_spec = find_param_spec (config, radius_property_name, G_STRFUNC);
  if (! radius_param_spec)
    return NULL;

  polar = gimp_polar_new ();

  g_object_set (polar,
                "size",         90,
                "border-width", 3,
                "background",   GIMP_CIRCLE_BACKGROUND_HSV,
                NULL);

  g_object_bind_property_full (config, angle_property_name,
                               polar,  "angle",
                               G_BINDING_BIDIRECTIONAL |
                               G_BINDING_SYNC_CREATE,
                               deg_to_rad,
                               rad_to_deg,
                               NULL, NULL);

  g_object_bind_property (config, radius_property_name,
                          polar, "radius",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);

  gimp_widget_set_bound_property (polar, config, angle_property_name);
  gtk_widget_show (polar);

  return polar;
}


/************/
/*  ranges  */
/************/

#define RANGE_GRADIENT_HEIGHT 12
#define RANGE_CONTROL_HEIGHT  10

GtkWidget *
gimp_prop_range_new (GObject     *config,
                     const gchar *lower_property_name,
                     const gchar *upper_property_name,
                     gdouble      step_increment,
                     gdouble      page_increment,
                     gint         digits,
                     gboolean     sorted)
{
  GtkWidget     *vbox;
  GtkWidget     *color_bar;
  GtkWidget     *handle_bar;
  GtkWidget     *hbox;
  GtkWidget     *spin_button;
  GtkAdjustment *adjustment1;
  GtkAdjustment *adjustment2;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  color_bar = gimp_color_bar_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_size_request (color_bar, -1, RANGE_GRADIENT_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), color_bar, FALSE, FALSE, 0);
  gtk_widget_show (color_bar);

  handle_bar = gimp_handle_bar_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_size_request (handle_bar, -1, RANGE_CONTROL_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), handle_bar, FALSE, FALSE, 0);
  gtk_widget_show (handle_bar);

  gimp_handle_bar_connect_events (GIMP_HANDLE_BAR (handle_bar), color_bar);

  g_object_set_data (G_OBJECT (vbox), "gimp-range-handle-bar", handle_bar);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  spin_button = gimp_prop_spin_button_new (config, lower_property_name,
                                           step_increment, page_increment,
                                           digits);
  adjustment1 = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spin_button));
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin_button), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);
  gtk_widget_show (spin_button);

  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (handle_bar), 0, adjustment1);

  spin_button = gimp_prop_spin_button_new (config, upper_property_name,
                                           step_increment, page_increment,
                                           digits);
  adjustment2 = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spin_button));
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin_button), TRUE);
  gtk_box_pack_end (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);
  gtk_widget_show (spin_button);

  gimp_handle_bar_set_adjustment (GIMP_HANDLE_BAR (handle_bar), 2, adjustment2);

  if (sorted)
    gimp_gtk_adjustment_chain (adjustment1, adjustment2);

  gimp_widget_set_bound_property (vbox, config, lower_property_name);
  gtk_widget_show (vbox);

  return vbox;
}

void
gimp_prop_range_set_ui_limits (GtkWidget *widget,
                               gdouble    lower,
                               gdouble    upper)
{
  GimpHandleBar *handle_bar;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  handle_bar = g_object_get_data (G_OBJECT (widget), "gimp-range-handle-bar");

  gimp_handle_bar_set_limits (handle_bar, lower, upper);
}


/**********/
/*  view  */
/**********/

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
 * Returns:  A new #GimpView widget.
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

  gimp_widget_set_bound_property (view, config, property_name);
  gtk_widget_show (view);

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


static void  gimp_prop_number_pair_entry_config_notify   (GObject                     *config,
                                                          GParamSpec                  *param_spec,
                                                          GtkEntry                    *entry);
static void  gimp_prop_number_pair_entry_number_pair_numbers_changed
                                                         (GtkWidget                   *widget,
                                                          GimpPropNumberPairEntryData *data);
static void  gimp_prop_number_pair_entry_number_pair_user_override_notify
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
 * @connect_ratio_changed:         %TRUE to connect to the widgets "ratio-changed"
 *                                 signal, %FALSE to not connect.
 * @separators:
 * @allow_simplification:
 * @min_valid_value:
 * @max_valid_value:         What to pass to gimp_number_pair_entry_new ().
 *
 * Returns: A #GimpNumberPairEntry widget.
 */
GtkWidget *
gimp_prop_number_pair_entry_new (GObject     *config,
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
                    G_CALLBACK (gimp_prop_number_pair_entry_number_pair_user_override_notify),
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

  gimp_widget_set_bound_property (number_pair_entry, config, left_number_property);
  gtk_widget_show (number_pair_entry);

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

  if (strcmp (param_spec->name, data->left_number_property)  == 0 ||
      strcmp (param_spec->name, data->right_number_property) == 0)
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
  else if (strcmp (param_spec->name, data->default_left_number_property)  == 0 ||
           strcmp (param_spec->name, data->default_right_number_property) == 0)
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
  else if (strcmp (param_spec->name, data->user_override_property)  == 0)
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
gimp_prop_number_pair_entry_number_pair_user_override_notify (GtkWidget                   *entry,
                                                              GParamSpec                  *param_spec,
                                                              GimpPropNumberPairEntryData *data)

{
  gboolean old_config_user_override;
  gboolean new_config_user_override;

  g_object_get (data->config,
                data->user_override_property, &old_config_user_override,
                NULL);

  new_config_user_override =
    gimp_number_pair_entry_get_user_override (GIMP_NUMBER_PAIR_ENTRY (entry));

  /* Only set when property changed, to avoid deadlocks */
  if (new_config_user_override != old_config_user_override)
    g_object_set (data->config,
                  data->user_override_property, new_config_user_override,
                  NULL);
}


/************************/
/*  language combo-box  */
/************************/

static void   gimp_prop_language_combo_box_callback (GtkWidget  *combo,
                                                     GObject    *config);
static void   gimp_prop_language_combo_box_notify   (GObject    *config,
                                                     GParamSpec *param_spec,
                                                     GtkWidget  *combo);

GtkWidget *
gimp_prop_language_combo_box_new (GObject     *config,
                                  const gchar *property_name)
{
  GParamSpec *param_spec;
  GtkWidget  *combo;
  gchar      *value;

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_STRING, G_STRFUNC);
  if (! param_spec)
    return NULL;

  combo = gimp_language_combo_box_new (FALSE, NULL);

  g_object_get (config,
                property_name, &value,
                NULL);

  gimp_language_combo_box_set_code (GIMP_LANGUAGE_COMBO_BOX (combo), value);
  g_free (value);

  set_param_spec (G_OBJECT (combo), combo, param_spec);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_prop_language_combo_box_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_language_combo_box_notify),
                  combo);

  gimp_widget_set_bound_property (combo, config, property_name);
  gtk_widget_show (combo);

  return combo;
}

static void
gimp_prop_language_combo_box_callback (GtkWidget *combo,
                                       GObject   *config)
{
  GParamSpec *param_spec;
  gchar      *code;

  param_spec = get_param_spec (G_OBJECT (combo));
  if (! param_spec)
    return;

  code = gimp_language_combo_box_get_code (GIMP_LANGUAGE_COMBO_BOX (combo));

  g_signal_handlers_block_by_func (config,
                                   gimp_prop_language_combo_box_notify,
                                   combo);

  g_object_set (config,
                param_spec->name, code,
                NULL);

  g_signal_handlers_unblock_by_func (config,
                                     gimp_prop_language_combo_box_notify,
                                     combo);

  g_free (code);
}

static void
gimp_prop_language_combo_box_notify (GObject    *config,
                                     GParamSpec *param_spec,
                                     GtkWidget  *combo)
{
  gchar *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (combo,
                                   gimp_prop_language_combo_box_callback,
                                   config);

  gimp_language_combo_box_set_code (GIMP_LANGUAGE_COMBO_BOX (combo), value);

  g_signal_handlers_unblock_by_func (combo,
                                     gimp_prop_language_combo_box_callback,
                                     config);

  g_free (value);
}


/********************/
/*  language entry  */
/********************/

static void   gimp_prop_language_entry_callback (GtkWidget  *entry,
                                                 GObject    *config);
static void   gimp_prop_language_entry_notify   (GObject    *config,
                                                 GParamSpec *param_spec,
                                                 GtkWidget  *entry);

GtkWidget *
gimp_prop_language_entry_new (GObject     *config,
                              const gchar *property_name)
{
  GParamSpec *param_spec;
  GtkWidget  *entry;
  gchar      *value;

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_STRING, G_STRFUNC);
  if (! param_spec)
    return NULL;

  entry = gimp_language_entry_new ();

  g_object_get (config,
                property_name, &value,
                NULL);

  gimp_language_entry_set_code (GIMP_LANGUAGE_ENTRY (entry), value);
  g_free (value);

  set_param_spec (G_OBJECT (entry), entry, param_spec);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_prop_language_entry_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_language_entry_notify),
                  entry);

  gimp_widget_set_bound_property (entry, config, property_name);
  gtk_widget_show (entry);

  return entry;
}

static void
gimp_prop_language_entry_callback (GtkWidget *entry,
                                   GObject   *config)
{
  GParamSpec  *param_spec;
  const gchar *code;

  param_spec = get_param_spec (G_OBJECT (entry));
  if (! param_spec)
    return;

  code = gimp_language_entry_get_code (GIMP_LANGUAGE_ENTRY (entry));

  g_signal_handlers_block_by_func (config,
                                   gimp_prop_language_entry_notify,
                                   entry);

  g_object_set (config,
                param_spec->name, code,
                NULL);

  g_signal_handlers_unblock_by_func (config,
                                     gimp_prop_language_entry_notify,
                                     entry);
}

static void
gimp_prop_language_entry_notify (GObject    *config,
                                 GParamSpec *param_spec,
                                 GtkWidget  *entry)
{
  gchar *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (entry,
                                   gimp_prop_language_entry_callback,
                                   config);

  gimp_language_entry_set_code (GIMP_LANGUAGE_ENTRY (entry), value);

  g_signal_handlers_unblock_by_func (entry,
                                     gimp_prop_language_entry_callback,
                                     config);

  g_free (value);
}


/***********************/
/*  profile combo box  */
/***********************/

static void   gimp_prop_profile_combo_callback (GimpColorProfileComboBox *combo,
                                                GObject                  *config);
static void   gimp_prop_profile_combo_notify   (GObject                  *config,
                                                const GParamSpec         *param_spec,
                                                GimpColorProfileComboBox *combo);

GtkWidget *
gimp_prop_profile_combo_box_new (GObject      *config,
                                 const gchar  *property_name,
                                 GtkListStore *profile_store,
                                 const gchar  *dialog_title,
                                 GObject      *profile_path_config,
                                 const gchar  *profile_path_property_name)
{
  GParamSpec *param_spec;
  GtkWidget  *dialog;
  GtkWidget  *combo;
  GFile      *file = NULL;

  param_spec = find_param_spec (config, property_name, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (G_IS_PARAM_SPEC_STRING (param_spec))
    {
      param_spec = check_param_spec_w (config, property_name,
                                       G_TYPE_PARAM_STRING, G_STRFUNC);
    }
  else
    {
      param_spec = check_param_spec_w (config, property_name,
                                       G_TYPE_PARAM_OBJECT, G_STRFUNC);
    }

  if (! param_spec)
    return NULL;

  dialog = gimp_color_profile_chooser_dialog_new (dialog_title, NULL,
                                                  GTK_FILE_CHOOSER_ACTION_OPEN);

  if (profile_path_config && profile_path_property_name)
    gimp_color_profile_chooser_dialog_connect_path (dialog,
                                                    profile_path_config,
                                                    profile_path_property_name);

  if (G_IS_PARAM_SPEC_STRING (param_spec))
    {
      gchar *path;

      g_object_get (config,
                    property_name, &path,
                    NULL);

      if (path)
        {
          file = gimp_file_new_for_config_path (path, NULL);
          g_free (path);
        }
    }
  else
    {
      g_object_get (config, property_name, &file, NULL);
    }

  if (profile_store)
    {
      combo = gimp_color_profile_combo_box_new_with_model (dialog,
                                                           GTK_TREE_MODEL (profile_store));
    }
  else
    {
      GFile *file;

      file = gimp_directory_file ("profilerc", NULL);
      combo = gimp_color_profile_combo_box_new (dialog, file);
      g_object_unref (file);
    }

  gimp_color_profile_combo_box_set_active_file (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
                                                file, NULL);

  if (file)
    g_object_unref (file);

  set_param_spec (G_OBJECT (combo), combo, param_spec);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_prop_profile_combo_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_profile_combo_notify),
                  combo);

  gimp_widget_set_bound_property (combo, config, property_name);
  gtk_widget_show (combo);

  return combo;
}

static void
gimp_prop_profile_combo_callback (GimpColorProfileComboBox *combo,
                                  GObject                  *config)
{
  GParamSpec *param_spec;
  GFile      *file;

  param_spec = get_param_spec (G_OBJECT (combo));
  if (! param_spec)
    return;

  file = gimp_color_profile_combo_box_get_active_file (combo);

  if (! file)
    g_signal_handlers_block_by_func (config,
                                     gimp_prop_profile_combo_notify,
                                     combo);

  if (G_IS_PARAM_SPEC_STRING (param_spec))
    {
      gchar *path = NULL;

      if (file)
        path = gimp_file_get_config_path (file, NULL);

      g_object_set (config,
                    param_spec->name, path,
                    NULL);

      g_free (path);
    }
  else
    {
      g_object_set (config,
                    param_spec->name, file,
                    NULL);
    }

  if (! file)
    g_signal_handlers_unblock_by_func (config,
                                       gimp_prop_profile_combo_notify,
                                       combo);

  if (file)
    g_object_unref (file);
}

static void
gimp_prop_profile_combo_notify (GObject                  *config,
                                const GParamSpec         *param_spec,
                                GimpColorProfileComboBox *combo)
{
  GFile *file = NULL;

  if (G_IS_PARAM_SPEC_STRING (param_spec))
    {
      gchar *path;

      g_object_get (config,
                    param_spec->name, &path,
                    NULL);

      if (path)
        {
          file = gimp_file_new_for_config_path (path, NULL);
          g_free (path);
        }
    }
  else
    {
      g_object_get (config,
                    param_spec->name, &file,
                    NULL);

    }

  g_signal_handlers_block_by_func (combo,
                                   gimp_prop_profile_combo_callback,
                                   config);

  gimp_color_profile_combo_box_set_active_file (combo, file, NULL);

  g_signal_handlers_unblock_by_func (combo,
                                     gimp_prop_profile_combo_callback,
                                     config);

  if (file)
    g_object_unref (file);
}


/***************************/
/*  compression combo box  */
/***************************/

static void   gimp_prop_compression_combo_box_callback (GtkWidget  *combo,
                                                        GObject    *config);
static void   gimp_prop_compression_combo_box_notify   (GObject    *config,
                                                        GParamSpec *param_spec,
                                                        GtkWidget  *combo);

GtkWidget *
gimp_prop_compression_combo_box_new (GObject     *config,
                                     const gchar *property_name)
{
  GParamSpec *param_spec;
  GtkWidget  *combo;
  gchar      *value;

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_STRING, G_STRFUNC);
  if (! param_spec)
    return NULL;

  combo = gimp_compression_combo_box_new ();

  g_object_get (config,
                property_name, &value,
                NULL);

  gimp_compression_combo_box_set_compression (
    GIMP_COMPRESSION_COMBO_BOX (combo), value);
  g_free (value);

  set_param_spec (G_OBJECT (combo), combo, param_spec);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_prop_compression_combo_box_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_compression_combo_box_notify),
                  combo);

  gimp_widget_set_bound_property (combo, config, property_name);
  gtk_widget_show (combo);

  return combo;
}

static void
gimp_prop_compression_combo_box_callback (GtkWidget *combo,
                                          GObject   *config)
{
  GParamSpec *param_spec;
  gchar      *compression;

  param_spec = get_param_spec (G_OBJECT (combo));
  if (! param_spec)
    return;

  compression = gimp_compression_combo_box_get_compression (
    GIMP_COMPRESSION_COMBO_BOX (combo));

  g_signal_handlers_block_by_func (config,
                                   gimp_prop_compression_combo_box_notify,
                                   combo);

  g_object_set (config,
                param_spec->name, compression,
                NULL);

  g_signal_handlers_unblock_by_func (config,
                                     gimp_prop_compression_combo_box_notify,
                                     combo);

  g_free (compression);
}

static void
gimp_prop_compression_combo_box_notify (GObject    *config,
                                        GParamSpec *param_spec,
                                        GtkWidget  *combo)
{
  gchar *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (combo,
                                   gimp_prop_compression_combo_box_callback,
                                   config);

  gimp_compression_combo_box_set_compression (
    GIMP_COMPRESSION_COMBO_BOX (combo), value);

  g_signal_handlers_unblock_by_func (combo,
                                     gimp_prop_compression_combo_box_callback,
                                     config);

  g_free (value);
}


/*****************/
/*  icon picker  */
/*****************/

static void   gimp_prop_icon_picker_callback (GtkWidget  *picker,
                                              GParamSpec *param_spec,
                                              GObject    *config);
static void   gimp_prop_icon_picker_notify   (GObject    *config,
                                              GParamSpec *param_spec,
                                              GtkWidget  *picker);

GtkWidget *
gimp_prop_icon_picker_new (GimpViewable *viewable,
                           Gimp         *gimp)
{
  GObject     *object          = G_OBJECT (viewable);
  GtkWidget   *picker          = NULL;
  GdkPixbuf   *pixbuf_value    = NULL;
  gchar       *icon_name_value = NULL;

  picker = gimp_icon_picker_new (gimp);

  g_object_get (object,
                "icon-name",   &icon_name_value,
                "icon-pixbuf", &pixbuf_value,
                NULL);

  gimp_icon_picker_set_icon_name (GIMP_ICON_PICKER (picker), icon_name_value);
  gimp_icon_picker_set_icon_pixbuf (GIMP_ICON_PICKER (picker), pixbuf_value);

  g_signal_connect (picker, "notify::icon-pixbuf",
                    G_CALLBACK (gimp_prop_icon_picker_callback),
                    object);

  g_signal_connect (picker, "notify::icon-name",
                    G_CALLBACK (gimp_prop_icon_picker_callback),
                    object);

  connect_notify (object, "icon-name",
                  G_CALLBACK (gimp_prop_icon_picker_notify),
                  picker);

  connect_notify (object, "icon-pixbuf",
                  G_CALLBACK (gimp_prop_icon_picker_notify),
                  picker);

  if (icon_name_value)
    g_free (icon_name_value);
  if (pixbuf_value)
    g_object_unref (pixbuf_value);

  gtk_widget_show (picker);

  return picker;
}

static void
gimp_prop_icon_picker_callback (GtkWidget  *picker,
                                GParamSpec *param_spec,
                                GObject    *config)
{
  g_signal_handlers_block_by_func (config,
                                   gimp_prop_icon_picker_notify,
                                   picker);

  if (! strcmp (param_spec->name, "icon-name"))
    {
      const gchar *value = gimp_icon_picker_get_icon_name (GIMP_ICON_PICKER (picker));

      g_object_set (config,
                    "icon-name", value,
                    NULL);

    }
  else if (! strcmp (param_spec->name, "icon-pixbuf"))
    {
      GdkPixbuf *value = gimp_icon_picker_get_icon_pixbuf (GIMP_ICON_PICKER (picker));

      g_object_set (config,
                    "icon-pixbuf", value,
                    NULL);
    }


  g_signal_handlers_unblock_by_func (config,
                                     gimp_prop_icon_picker_notify,
                                     picker);
}

static void
gimp_prop_icon_picker_notify (GObject    *config,
                              GParamSpec *param_spec,
                              GtkWidget  *picker)
{
  g_signal_handlers_block_by_func (picker,
                                   gimp_prop_icon_picker_callback,
                                   config);

  if (!strcmp (param_spec->name, "icon-name"))
    {
      gchar *value = NULL;

      g_object_get (config,
                    "icon-name", &value,
                    NULL);

      gimp_icon_picker_set_icon_name (GIMP_ICON_PICKER (picker), value);

      if (value)
        g_free (value);
    }
  else if (!strcmp (param_spec->name, "icon-pixbuf"))
    {
      GdkPixbuf *value = NULL;

      g_object_get (config,
                    "icon-pixbuf", &value,
                    NULL);

      gimp_icon_picker_set_icon_pixbuf (GIMP_ICON_PICKER (picker), value);

      if (value)
        g_object_unref (value);
    }

  g_signal_handlers_unblock_by_func (picker,
                                     gimp_prop_icon_picker_callback,
                                     config);
}


/******************************/
/*  public utility functions  */
/******************************/

gboolean
_gimp_prop_widgets_get_numeric_values (GObject     *object,
                                       GParamSpec  *param_spec,
                                       gdouble     *value,
                                       gdouble     *lower,
                                       gdouble     *upper,
                                       const gchar *strloc)
{
  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      GParamSpecInt *int_spec = G_PARAM_SPEC_INT (param_spec);
      gint           int_value;

      g_object_get (object, param_spec->name, &int_value, NULL);

      *value = int_value;
      *lower = int_spec->minimum;
      *upper = int_spec->maximum;
    }
  else if (G_IS_PARAM_SPEC_UINT (param_spec))
    {
      GParamSpecUInt *uint_spec = G_PARAM_SPEC_UINT (param_spec);
      guint           uint_value;

      g_object_get (object, param_spec->name, &uint_value, NULL);

      *value = uint_value;
      *lower = uint_spec->minimum;
      *upper = uint_spec->maximum;
    }
  else if (G_IS_PARAM_SPEC_DOUBLE (param_spec))
    {
      GParamSpecDouble *double_spec = G_PARAM_SPEC_DOUBLE (param_spec);

      g_object_get (object, param_spec->name, value, NULL);

      *lower = double_spec->minimum;
      *upper = double_spec->maximum;
    }
  else
    {
      g_warning ("%s: property '%s' of %s is not numeric",
                 strloc,
                 param_spec->name,
                 g_type_name (G_TYPE_FROM_INSTANCE (object)));
      return FALSE;
    }

  return TRUE;
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
        gimp_help_set_help_data (widget, blurb, NULL);
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
      g_warning ("%s: property '%s' of %s is not writable",
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
