/* GTK - The GIMP Toolkit
 * Copyright (C) 2005 Ronald S. Bultje
 * Copyright (C) 2006, 2007 Christian Persch
 * Copyright (C) 2006 Jan Arne Petersen
 * Copyright (C) 2005-2007 Red Hat, Inc.
 *
 * Authors:
 * - Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * - Bastien Nocera <bnocera@redhat.com>
 * - Jan Arne Petersen <jpetersen@jpetersen.org>
 * - Christian Persch <chpe@svn.gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 2007.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "config.h"

#define _GNU_SOURCE
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>

#define __GTK_SCALE_BUTTON_H__
#define __GTK_VOLUME_BUTTON_H__

#define P_(string) string
#define I_(string) string
#define GTK_PARAM_READWRITE (G_PARAM_READWRITE)
#define gtk_scale_button_parent_class gimp_gtk_scale_button_parent_class
#define gtk_scale_button_set_orientation gimp_gtk_scale_button_set_orientation
#define gtk_scale_button_get_value gimp_gtk_scale_button_get_value
#define gtk_scale_button_set_value gimp_gtk_scale_button_set_value
#define gtk_scale_button_get_adjustment gimp_gtk_scale_button_get_adjustment
#define gtk_scale_button_set_adjustment gimp_gtk_scale_button_set_adjustment
#define gtk_scale_button_set_icons gimp_gtk_scale_button_set_icons

#include "gtkscalebutton.h"
#include "core/gimpmarshal.h"

#define SCALE_SIZE 100
#define CLICK_TIMEOUT 250

enum
{
  VALUE_CHANGED,
  POPUP,
  POPDOWN,

  LAST_SIGNAL
};

enum
{
  PROP_0,

  PROP_ORIENTATION,
  PROP_VALUE,
  PROP_SIZE,
  PROP_ADJUSTMENT,
  PROP_ICONS
};

#define GET_PRIVATE(obj)        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_SCALE_BUTTON, GtkScaleButtonPrivate))

struct _GtkScaleButtonPrivate
{
  GtkWidget *dock;
  GtkWidget *scale;
  GtkWidget *image;

  GtkIconSize size;
  GtkOrientation orientation;

  guint click_id;
  gint click_timeout;
  guint timeout : 1;
  gdouble direction;
  guint32 pop_time;

  gchar **icon_list;

  GtkAdjustment *adjustment; /* needed because it must be settable in init() */
};

static GObject* gtk_scale_button_constructor    (GType                  type,
                                                 guint                  n_construct_properties,
                                                 GObjectConstructParam *construct_params);
static void	gtk_scale_button_dispose	(GObject             *object);
static void     gtk_scale_button_finalize       (GObject             *object);
static void	gtk_scale_button_set_property	(GObject             *object,
						 guint                prop_id,
						 const GValue        *value,
						 GParamSpec          *pspec);
static void	gtk_scale_button_get_property	(GObject             *object,
						 guint                prop_id,
						 GValue              *value,
						 GParamSpec          *pspec);
static gboolean	gtk_scale_button_scroll		(GtkWidget           *widget,
						 GdkEventScroll      *event);
static void gtk_scale_button_screen_changed	(GtkWidget           *widget,
						 GdkScreen           *previous_screen);
static gboolean	gtk_scale_button_press		(GtkWidget           *widget,
						 GdkEventButton      *event);
static gboolean gtk_scale_button_key_release	(GtkWidget           *widget,
    						 GdkEventKey         *event);
static void gtk_scale_button_popup_from_bindings (GtkWidget *widget);
static void gtk_scale_button_popdown_from_bindings (GtkWidget *widget);
static gboolean cb_dock_button_press		(GtkWidget           *widget,
						 GdkEventButton      *event,
						 gpointer             user_data);
static gboolean cb_dock_key_release		(GtkWidget           *widget,
						 GdkEventKey         *event,
						 gpointer             user_data);
static gboolean cb_button_press			(GtkWidget           *widget,
						 GdkEventButton      *event,
						 gpointer             user_data);
static gboolean cb_button_release		(GtkWidget           *widget,
						 GdkEventButton      *event,
						 gpointer             user_data);
static void cb_dock_grab_notify			(GtkWidget           *widget,
						 gboolean             was_grabbed,
						 gpointer             user_data);
static gboolean cb_dock_grab_broken_event	(GtkWidget           *widget,
						 gboolean             was_grabbed,
						 gpointer             user_data);
static void cb_scale_grab_notify		(GtkWidget           *widget,
						 gboolean             was_grabbed,
						 gpointer             user_data);
static void gtk_scale_button_update_icon	(GtkScaleButton      *button);
static void gtk_scale_button_scale_value_changed(GtkRange            *range);

/* see below for scale definitions */
static GtkWidget *gtk_scale_button_scale_box_new(GtkScaleButton      *button);

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (GtkScaleButton, gimp_gtk_scale_button, GTK_TYPE_BUTTON)

static guint
gimp_gtk_binding_signal_new (const gchar        *signal_name,
                         GType               itype,
                         GSignalFlags        signal_flags,
                         GCallback           handler,
                         GSignalAccumulator  accumulator,
                         gpointer            accu_data,
                         GSignalCMarshaller  c_marshaller,
                         GType               return_type,
                         guint               n_params,
                         ...)
{
  va_list args;
  guint signal_id;

  g_return_val_if_fail (signal_name != NULL, 0);

  va_start (args, n_params);

  signal_id = g_signal_new_valist (signal_name, itype, signal_flags,
                                   g_cclosure_new (handler, NULL, NULL),
                                   accumulator, accu_data, c_marshaller,
                                   return_type, n_params, args);

  va_end (args);

  return signal_id;
}

static void
gimp_gtk_scale_button_class_init (GtkScaleButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet *binding_set;

  g_type_class_add_private (klass, sizeof (GtkScaleButtonPrivate));

  gobject_class->constructor = gtk_scale_button_constructor;
  gobject_class->finalize = gtk_scale_button_finalize;
  gobject_class->dispose = gtk_scale_button_dispose;
  gobject_class->set_property = gtk_scale_button_set_property;
  gobject_class->get_property = gtk_scale_button_get_property;

  widget_class->button_press_event = gtk_scale_button_press;
  widget_class->key_release_event = gtk_scale_button_key_release;
  widget_class->scroll_event = gtk_scale_button_scroll;
  widget_class->screen_changed = gtk_scale_button_screen_changed;

  /**
   * GtkScaleButton:orientation:
   *
   * The orientation of the #GtkScaleButton's popup window.
   *
   * Since: 2.14
   **/
  g_object_class_install_property (gobject_class,
				   PROP_ORIENTATION,
				   g_param_spec_enum ("orientation",
                                                      P_("Orientation"),
                                                      P_("The orientation of the scale"),
                                                      GTK_TYPE_ORIENTATION,
                                                      GTK_ORIENTATION_VERTICAL,
                                                      GTK_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
				   PROP_VALUE,
				   g_param_spec_double ("value",
							P_("Value"),
							P_("The value of the scale"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							0,
							GTK_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
				   PROP_SIZE,
				   g_param_spec_enum ("size",
						      P_("Icon size"),
						      P_("The icon size"),
						      GTK_TYPE_ICON_SIZE,
						      GTK_ICON_SIZE_SMALL_TOOLBAR,
						      GTK_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_ADJUSTMENT,
                                   g_param_spec_object ("adjustment",
							P_("Adjustment"),
							P_("The GtkAdjustment that contains the current value of this scale button object"),
                                                        GTK_TYPE_ADJUSTMENT,
                                                        GTK_PARAM_READWRITE));

  /**
   * GtkScaleButton:icons:
   *
   * The names of the icons to be used by the scale button.
   * The first item in the array will be used in the button
   * when the current value is the lowest value, the second
   * item for the highest value. All the subsequent icons will
   * be used for all the other values, spread evenly over the
   * range of values.
   *
   * If there's only one icon name in the @icons array, it will
   * be used for all the values. If only two icon names are in
   * the @icons array, the first one will be used for the bottom
   * 50% of the scale, and the second one for the top 50%.
   *
   * It is recommended to use at least 3 icons so that the
   * #GtkScaleButton reflects the current value of the scale
   * better for the users.
   *
   * Since: 2.12
   */
  g_object_class_install_property (gobject_class,
                                   PROP_ICONS,
                                   g_param_spec_boxed ("icons",
                                                       P_("Icons"),
                                                       P_("List of icon names"),
                                                       G_TYPE_STRV,
                                                       GTK_PARAM_READWRITE));

  /**
   * GtkScaleButton::value-changed:
   * @button: the object which received the signal
   * @value: the new value
   *
   * The ::value-changed signal is emitted when the value field has
   * changed.
   *
   * Since: 2.12
   */
  signals[VALUE_CHANGED] =
    g_signal_new (I_("value-changed"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GtkScaleButtonClass, value_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__DOUBLE,
		  G_TYPE_NONE, 1, G_TYPE_DOUBLE);

  /**
   * GtkScaleButton::popup:
   * @button: the object which received the signal
   *
   * The ::popup signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted to popup the scale widget.
   *
   * The default bindings for this signal are Space, Enter and Return.
   *
   * Since: 2.12
   */
  signals[POPUP] =
    gimp_gtk_binding_signal_new (I_("popup"),
			     G_OBJECT_CLASS_TYPE (klass),
			     G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			     G_CALLBACK (gtk_scale_button_popup_from_bindings),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

  /**
   * GtkScaleButton::popdown:
   * @button: the object which received the signal
   *
   * The ::popdown signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted to popdown the scale widget.
   *
   * The default binding for this signal is Escape.
   *
   * Since: 2.12
   */
  signals[POPDOWN] =
    gimp_gtk_binding_signal_new (I_("popdown"),
			     G_OBJECT_CLASS_TYPE (klass),
			     G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			     G_CALLBACK (gtk_scale_button_popdown_from_bindings),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

  /* Key bindings */
  binding_set = gtk_binding_set_by_class (widget_class);

  gtk_binding_entry_add_signal (binding_set, GDK_space, 0,
				"popup", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Space, 0,
				"popup", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_Return, 0,
				"popup", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_ISO_Enter, 0,
				"popup", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Enter, 0,
				"popup", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_Escape, 0,
				"popdown", 0);
}

static void
gimp_gtk_scale_button_init (GtkScaleButton *button)
{
  GtkScaleButtonPrivate *priv;

  button->priv = priv = GET_PRIVATE (button);

  priv->timeout = FALSE;
  priv->click_id = 0;
  priv->click_timeout = CLICK_TIMEOUT;
  priv->orientation = GTK_ORIENTATION_VERTICAL;

  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);

  /* image */
  priv->image = gtk_image_new ();
  gtk_container_add (GTK_CONTAINER (button), priv->image);
  gtk_widget_show_all (priv->image);

  /* window */
  priv->dock = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_set_name (priv->dock, "gtk-scalebutton-popup-window");
  g_signal_connect (priv->dock, "button-press-event",
		    G_CALLBACK (cb_dock_button_press), button);
  g_signal_connect (priv->dock, "key-release-event",
		    G_CALLBACK (cb_dock_key_release), button);
  g_signal_connect (priv->dock, "grab-notify",
		    G_CALLBACK (cb_dock_grab_notify), button);
  g_signal_connect (priv->dock, "grab-broken-event",
		    G_CALLBACK (cb_dock_grab_broken_event), button);
  gtk_window_set_decorated (GTK_WINDOW (priv->dock), FALSE);

  /* + */
  button->plus_button = gtk_button_new_with_label ("+");
  gtk_button_set_relief (GTK_BUTTON (button->plus_button), GTK_RELIEF_NONE);
  g_signal_connect (button->plus_button, "button-press-event",
		    G_CALLBACK (cb_button_press), button);
  g_signal_connect (button->plus_button, "button-release-event",
		    G_CALLBACK (cb_button_release), button);

  /* - */
  button->minus_button = gtk_button_new_with_label ("-");
  gtk_button_set_relief (GTK_BUTTON (button->minus_button), GTK_RELIEF_NONE);
  g_signal_connect (button->minus_button, "button-press-event",
		   G_CALLBACK (cb_button_press), button);
  g_signal_connect (button->minus_button, "button-release-event",
		    G_CALLBACK (cb_button_release), button);

  priv->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 100.0, 2, 20, 0));
  g_object_ref_sink (priv->adjustment);
}

static GObject *
gtk_scale_button_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_params)
{
  GObject *object;
  GtkScaleButton *button;
  GtkWidget *frame, *box;
  GtkScaleButtonPrivate *priv;

  object = G_OBJECT_CLASS (gtk_scale_button_parent_class)->constructor (type, n_construct_properties, construct_params);

  button = GTK_SCALE_BUTTON (object);

  priv = button->priv;

  /* frame */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (priv->dock), frame);

  /* box with scale and +/- buttons */
  box = gtk_scale_button_scale_box_new (button);
  gtk_container_add (GTK_CONTAINER (frame), box);

  /* set button text and size */
  priv->size = GTK_ICON_SIZE_SMALL_TOOLBAR;
  gtk_scale_button_update_icon (button);

  return object;
}

static void
gtk_scale_button_set_property (GObject       *object,
			       guint          prop_id,
			       const GValue  *value,
			       GParamSpec    *pspec)
{
  GtkScaleButton *button = GTK_SCALE_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      gtk_scale_button_set_orientation (button, g_value_get_enum (value));
      break;
    case PROP_VALUE:
      gtk_scale_button_set_value (button, g_value_get_double (value));
      break;
    case PROP_SIZE:
      {
	GtkIconSize size;
	size = g_value_get_enum (value);
	if (button->priv->size != size)
	  {
	    button->priv->size = size;
	    gtk_scale_button_update_icon (button);
	  }
      }
      break;
    case PROP_ADJUSTMENT:
      gtk_scale_button_set_adjustment (button, g_value_get_object (value));
      break;
    case PROP_ICONS:
      gtk_scale_button_set_icons (button,
                                  (const gchar **)g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_scale_button_get_property (GObject     *object,
			       guint        prop_id,
			       GValue      *value,
			       GParamSpec  *pspec)
{
  GtkScaleButton *button = GTK_SCALE_BUTTON (object);
  GtkScaleButtonPrivate *priv = button->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_VALUE:
      g_value_set_double (value, gtk_scale_button_get_value (button));
      break;
    case PROP_SIZE:
      g_value_set_enum (value, priv->size);
      break;
    case PROP_ADJUSTMENT:
      g_value_set_object (value, gtk_scale_button_get_adjustment (button));
      break;
    case PROP_ICONS:
      g_value_set_boxed (value, priv->icon_list);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_scale_button_finalize (GObject *object)
{
  GtkScaleButton *button = GTK_SCALE_BUTTON (object);
  GtkScaleButtonPrivate *priv = button->priv;

  if (priv->icon_list)
    {
      g_strfreev (priv->icon_list);
      priv->icon_list = NULL;
    }

  if (priv->adjustment)
    {
      g_object_unref (priv->adjustment);
      priv->adjustment = NULL;
    }

  G_OBJECT_CLASS (gtk_scale_button_parent_class)->finalize (object);
}

static void
gtk_scale_button_dispose (GObject *object)
{
  GtkScaleButton *button = GTK_SCALE_BUTTON (object);
  GtkScaleButtonPrivate *priv = button->priv;

  if (priv->dock)
    {
      gtk_widget_destroy (priv->dock);
      priv->dock = NULL;
    }

  if (priv->click_id != 0)
    {
      g_source_remove (priv->click_id);
      priv->click_id = 0;
    }

  G_OBJECT_CLASS (gtk_scale_button_parent_class)->dispose (object);
}

/**
 * gtk_scale_button_new:
 * @size: a stock icon size
 * @min: the minimum value of the scale (usually 0)
 * @max: the maximum value of the scale (usually 100)
 * @step: the stepping of value when a scroll-wheel event,
 *        or up/down arrow event occurs (usually 2)
 * @icons: a %NULL-terminated array of icon names, or %NULL if
 *         you want to set the list later with gtk_scale_button_set_icons()
 *
 * Creates a #GtkScaleButton, with a range between @min and @max, with
 * a stepping of @step.
 *
 * Return value: a new #GtkScaleButton
 *
 * Since: 2.12
 */
GtkWidget *
gimp_gtk_scale_button_new (GtkIconSize   size,
		      gdouble       min,
		      gdouble       max,
		      gdouble       step,
		      const gchar **icons)
{
  GtkScaleButton *button;
  GtkObject *adj;

  adj = gtk_adjustment_new (min, min, max, step, 10 * step, 0);

  button = g_object_new (GTK_TYPE_SCALE_BUTTON,
                         "adjustment", adj,
                         "icons", icons,
                         "size", size,
                         NULL);

  return GTK_WIDGET (button);
}

/**
 * gtk_scale_button_get_value:
 * @button: a #GtkScaleButton
 *
 * Gets the current value of the scale button.
 *
 * Return value: current value of the scale button
 *
 * Since: 2.12
 */
gdouble
gimp_gtk_scale_button_get_value (GtkScaleButton * button)
{
  GtkScaleButtonPrivate *priv;

  g_return_val_if_fail (GTK_IS_SCALE_BUTTON (button), 0);

  priv = button->priv;

  return gtk_adjustment_get_value (priv->adjustment);
}

/**
 * gtk_scale_button_set_value:
 * @button: a #GtkScaleButton
 * @value: new value of the scale button
 *
 * Sets the current value of the scale; if the value is outside
 * the minimum or maximum range values, it will be clamped to fit
 * inside them. The scale button emits the #GtkScaleButton::value-changed
 * signal if the value changes.
 *
 * Since: 2.12
 */
void
gimp_gtk_scale_button_set_value (GtkScaleButton *button,
			    gdouble         value)
{
  GtkScaleButtonPrivate *priv;

  g_return_if_fail (GTK_IS_SCALE_BUTTON (button));

  priv = button->priv;

  gtk_range_set_value (GTK_RANGE (priv->scale), value);
}

/**
 * gtk_scale_button_set_icons:
 * @button: a #GtkScaleButton
 * @icons: a %NULL-terminated array of icon names
 *
 * Sets the icons to be used by the scale button.
 * For details, see the #GtkScaleButton:icons property.
 *
 * Since: 2.12
 */
void
gimp_gtk_scale_button_set_icons (GtkScaleButton  *button,
			    const gchar    **icons)
{
  GtkScaleButtonPrivate *priv;
  gchar **tmp;

  g_return_if_fail (GTK_IS_SCALE_BUTTON (button));

  priv = button->priv;

  tmp = priv->icon_list;
  priv->icon_list = g_strdupv ((gchar **) icons);
  g_strfreev (tmp);
  gtk_scale_button_update_icon (button);

  g_object_notify (G_OBJECT (button), "icons");
}

/**
 * gtk_scale_button_get_adjustment:
 * @button: a #GtkScaleButton
 *
 * Gets the #GtkAdjustment associated with the #GtkScaleButton's scale.
 * See gtk_range_get_adjustment() for details.
 *
 * Returns: the adjustment associated with the scale
 *
 * Since: 2.12
 */
GtkAdjustment*
gimp_gtk_scale_button_get_adjustment	(GtkScaleButton *button)
{
  g_return_val_if_fail (GTK_IS_SCALE_BUTTON (button), NULL);

  return button->priv->adjustment;
}

/**
 * gtk_scale_button_set_adjustment:
 * @button: a #GtkScaleButton
 * @adjustment: a #GtkAdjustment
 *
 * Sets the #GtkAdjustment to be used as a model
 * for the #GtkScaleButton's scale.
 * See gtk_range_set_adjustment() for details.
 *
 * Since: 2.12
 */
void
gimp_gtk_scale_button_set_adjustment	(GtkScaleButton *button,
				 GtkAdjustment  *adjustment)
{
  g_return_if_fail (GTK_IS_SCALE_BUTTON (button));
  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  if (button->priv->adjustment)
    g_object_unref (button->priv->adjustment);
  button->priv->adjustment = g_object_ref_sink (adjustment);

  if (button->priv->scale)
    gtk_range_set_adjustment (GTK_RANGE (button->priv->scale), adjustment);

  g_object_notify (G_OBJECT (button), "adjustment");
}

/**
 * gtk_scale_button_get_orientation:
 * @button: a #GtkScaleButton
 *
 * Gets the orientation of the #GtkScaleButton's popup window.
 *
 * Returns: the #GtkScaleButton's orientation.
 *
 * Since: 2.14
 **/
GtkOrientation
gimp_gtk_scale_button_get_orientation (GtkScaleButton *button)
{
  g_return_val_if_fail (GTK_IS_SCALE_BUTTON (button), GTK_ORIENTATION_VERTICAL);

  return button->priv->orientation;
}

/**
 * gtk_scale_button_set_orientation:
 * @button: a #GtkScaleButton
 * @orientation: the new orientation
 *
 * Sets the orientation of the #GtkScaleButton's popup window.
 *
 * Since: 2.14
 **/
void
gimp_gtk_scale_button_set_orientation (GtkScaleButton *button,
                                  GtkOrientation  orientation)
{
  GtkScaleButtonPrivate *priv;

  g_return_if_fail (GTK_IS_SCALE_BUTTON (button));

  priv = button->priv;

  if (orientation != priv->orientation)
    {
      priv->orientation = orientation;

      if (priv->scale)
        {
          GtkWidget *box = priv->scale->parent;
          GtkWidget *frame = box->parent;

          g_object_ref (button->plus_button);
          g_object_ref (button->minus_button);

          gtk_container_remove (GTK_CONTAINER (box), button->plus_button);
          gtk_container_remove (GTK_CONTAINER (box), button->minus_button);
          gtk_container_remove (GTK_CONTAINER (box), priv->scale);
          gtk_container_remove (GTK_CONTAINER (frame), box);

          box = gtk_scale_button_scale_box_new (button);
          gtk_container_add (GTK_CONTAINER (frame), box);

          /* FIXME: without this, the popup window appears as a square
           * after changing the orientation
           */
          gtk_window_resize (GTK_WINDOW (priv->dock), 1, 1);

          g_object_unref (button->plus_button);
          g_object_unref (button->minus_button);
        }

      g_object_notify (G_OBJECT (button), "orientation");
    }
}

/*
 * button callbacks.
 */

static gboolean
gtk_scale_button_scroll (GtkWidget      *widget,
			 GdkEventScroll *event)
{
  GtkScaleButton *button;
  GtkScaleButtonPrivate *priv;
  GtkAdjustment *adj;
  gdouble d;

  button = GTK_SCALE_BUTTON (widget);
  priv = button->priv;
  adj = priv->adjustment;

  if (event->type != GDK_SCROLL)
    return FALSE;

  d = gtk_scale_button_get_value (button);
  if (event->direction == GDK_SCROLL_UP)
    {
      d += adj->step_increment;
      if (d > adj->upper)
	d = adj->upper;
    }
  else
    {
      d -= adj->step_increment;
      if (d < adj->lower)
	d = adj->lower;
    }
  gtk_scale_button_set_value (button, d);

  return TRUE;
}

static void
gtk_scale_button_screen_changed (GtkWidget *widget,
				 GdkScreen *previous_screen)
{
  GtkScaleButton *button = (GtkScaleButton *) widget;
  GtkScaleButtonPrivate *priv;
  GdkScreen *screen;
  GValue value = { 0, };

  if (gtk_widget_has_screen (widget) == FALSE)
    return;

  priv = button->priv;

  screen = gtk_widget_get_screen (widget);
  g_value_init (&value, G_TYPE_INT);
  if (gdk_screen_get_setting (screen,
			      "gtk-double-click-time",
			      &value) == FALSE)
    {
      priv->click_timeout = CLICK_TIMEOUT;
      return;
    }

  priv->click_timeout = g_value_get_int (&value);
}

static gboolean
gtk_scale_popup (GtkWidget *widget,
		 GdkEvent  *event,
		 guint32    time)
{
  GtkScaleButton *button;
  GtkScaleButtonPrivate *priv;
  GtkAdjustment *adj;
  gint x, y, m, dx, dy, sx, sy, startoff;
  gdouble v;
  GdkDisplay *display;
  GdkScreen *screen;

  button = GTK_SCALE_BUTTON (widget);
  priv = button->priv;
  adj = priv->adjustment;

  display = gtk_widget_get_display (widget);
  screen = gtk_widget_get_screen (widget);

  /* position roughly */
  gtk_window_set_screen (GTK_WINDOW (priv->dock), screen);

  gdk_window_get_origin (widget->window, &x, &y);
  x += widget->allocation.x;
  y += widget->allocation.y;

  if (priv->orientation == GTK_ORIENTATION_VERTICAL)
    gtk_window_move (GTK_WINDOW (priv->dock), x, y - (SCALE_SIZE / 2));
  else
    gtk_window_move (GTK_WINDOW (priv->dock), x - (SCALE_SIZE / 2), y);

  gtk_widget_show_all (priv->dock);

  gdk_window_get_origin (priv->dock->window, &dx, &dy);
  dx += priv->dock->allocation.x;
  dy += priv->dock->allocation.y;

  gdk_window_get_origin (priv->scale->window, &sx, &sy);
  sx += priv->scale->allocation.x;
  sy += priv->scale->allocation.y;

  priv->timeout = TRUE;

  /* position (needs widget to be shown already) */
  v = gtk_scale_button_get_value (button) / (adj->upper - adj->lower);

  if (priv->orientation == GTK_ORIENTATION_VERTICAL)
    {
      startoff = sy - dy;

      x += (widget->allocation.width - priv->dock->allocation.width) / 2;
      y -= startoff;
      y -= GTK_RANGE (priv->scale)->min_slider_size / 2;
      m = priv->scale->allocation.height -
          GTK_RANGE (priv->scale)->min_slider_size;
      y -= m * (1.0 - v);
    }
  else
    {
      startoff = sx - dx;

      x -= startoff;
      y += (widget->allocation.height - priv->dock->allocation.height) / 2;
      x -= GTK_RANGE (priv->scale)->min_slider_size / 2;
      m = priv->scale->allocation.width -
          GTK_RANGE (priv->scale)->min_slider_size;
      x -= m * v;
    }

  /* Make sure the dock stays inside the monitor */
  if (event->type == GDK_BUTTON_PRESS)
    {
      int monitor;
      GdkEventButton *button_event = (GdkEventButton *) event;
      GdkRectangle rect;
      GtkWidget *d;

      d = GTK_WIDGET (priv->dock);
      monitor = gdk_screen_get_monitor_at_point (screen,
						 button_event->x_root,
						 button_event->y_root);
      gdk_screen_get_monitor_geometry (screen, monitor, &rect);

      if (priv->orientation == GTK_ORIENTATION_VERTICAL)
        y += button_event->y;
      else
        x += button_event->x;

      if (y < rect.y)
	y = rect.y;
      else if (y + d->allocation.height > rect.height + rect.y)
	y = rect.y + rect.height - d->allocation.height;

      if (x < rect.x)
	x = rect.x;
      else if (x + d->allocation.width > rect.width + rect.x)
	x = rect.x + rect.width - d->allocation.width;
    }

  gtk_window_move (GTK_WINDOW (priv->dock), x, y);

  if (event->type == GDK_BUTTON_PRESS)
    GTK_WIDGET_CLASS (gtk_scale_button_parent_class)->button_press_event (widget, (GdkEventButton *) event);

  /* grab focus */
  gtk_grab_add (priv->dock);

  if (gdk_pointer_grab (priv->dock->window, TRUE,
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			GDK_POINTER_MOTION_MASK, NULL, NULL, time)
      != GDK_GRAB_SUCCESS)
    {
      gtk_grab_remove (priv->dock);
      gtk_widget_hide (priv->dock);
      return FALSE;
    }

  if (gdk_keyboard_grab (priv->dock->window, TRUE, time) != GDK_GRAB_SUCCESS)
    {
      gdk_display_pointer_ungrab (display, time);
      gtk_grab_remove (priv->dock);
      gtk_widget_hide (priv->dock);
      return FALSE;
    }

  gtk_widget_grab_focus (priv->dock);

  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *e;
      GdkEventButton *button_event = (GdkEventButton *) event;

      /* forward event to the slider */
      e = (GdkEventButton *) gdk_event_copy ((GdkEvent *) event);
      e->window = priv->scale->window;

      /* position: the X position isn't relevant, halfway will work just fine.
       * The vertical position should be *exactly* in the middle of the slider
       * of the scale; if we don't do that correctly, it'll move from its current
       * position, which means a position change on-click, which is bad.
       */
      if (priv->orientation == GTK_ORIENTATION_VERTICAL)
        {
          e->x = priv->scale->allocation.width / 2;
          m = priv->scale->allocation.height -
              GTK_RANGE (priv->scale)->min_slider_size;
          e->y = ((1.0 - v) * m) + GTK_RANGE (priv->scale)->min_slider_size / 2;
        }
      else
        {
          e->y = priv->scale->allocation.height / 2;
          m = priv->scale->allocation.width -
              GTK_RANGE (priv->scale)->min_slider_size;
          e->x = (v * m) + GTK_RANGE (priv->scale)->min_slider_size / 2;
        }

      gtk_widget_event (priv->scale, (GdkEvent *) e);
      e->window = button_event->window;
      gdk_event_free ((GdkEvent *) e);
    }

  gtk_widget_grab_focus (priv->scale);

  priv->pop_time = time;

  return TRUE;
}

static gboolean
gtk_scale_button_press (GtkWidget      *widget,
			GdkEventButton *event)
{
  return gtk_scale_popup (widget, (GdkEvent *) event, event->time);
}

static void
gtk_scale_button_popup_from_bindings (GtkWidget *widget)
{
  GdkEvent *ev;

  ev = gdk_event_new (GDK_KEY_RELEASE);
  gtk_scale_popup (widget, ev, GDK_CURRENT_TIME);
  gdk_event_free (ev);
}

static gboolean
gtk_scale_button_key_release (GtkWidget   *widget,
			      GdkEventKey *event)
{
  return gtk_bindings_activate_event (GTK_OBJECT (widget), event);
}

/* This is called when the grab is broken for
 * either the dock, or the scale itself */
static void
gtk_scale_button_grab_notify (GtkScaleButton *button,
			      gboolean        was_grabbed)
{
  GdkDisplay *display;
  GtkScaleButtonPrivate *priv;

  if (was_grabbed != FALSE)
    return;

  priv = button->priv;

  if (!GTK_WIDGET_HAS_GRAB (priv->dock))
    return;

  if (gtk_widget_is_ancestor (gtk_grab_get_current (), priv->dock))
    return;

  display = gtk_widget_get_display (priv->dock);
  gdk_display_keyboard_ungrab (display, GDK_CURRENT_TIME);
  gdk_display_pointer_ungrab (display, GDK_CURRENT_TIME);
  gtk_grab_remove (priv->dock);

  /* hide again */
  gtk_widget_hide (priv->dock);
  priv->timeout = FALSE;
}

/*
 * +/- button callbacks.
 */

static gboolean
cb_button_timeout (gpointer user_data)
{
  GtkScaleButton *button;
  GtkScaleButtonPrivate *priv;
  GtkAdjustment *adj;
  gdouble val;
  gboolean res = TRUE;

  button = GTK_SCALE_BUTTON (user_data);
  priv = button->priv;

  if (priv->click_id == 0)
    return FALSE;

  adj = priv->adjustment;

  val = gtk_scale_button_get_value (button);
  val += priv->direction;
  if (val <= adj->lower)
    {
      res = FALSE;
      val = adj->lower;
    }
  else if (val > adj->upper)
    {
      res = FALSE;
      val = adj->upper;
    }
  gtk_scale_button_set_value (button, val);

  if (!res)
    {
      g_source_remove (priv->click_id);
      priv->click_id = 0;
    }

  return res;
}

static gboolean
cb_button_press (GtkWidget      *widget,
		 GdkEventButton *event,
		 gpointer        user_data)
{
  GtkScaleButton *button;
  GtkScaleButtonPrivate *priv;
  GtkAdjustment *adj;

  button = GTK_SCALE_BUTTON (user_data);
  priv = button->priv;
  adj = priv->adjustment;

  if (priv->click_id != 0)
    g_source_remove (priv->click_id);

  if (widget == button->plus_button)
    priv->direction = fabs (adj->page_increment);
  else
    priv->direction = - fabs (adj->page_increment);

  priv->click_id = gdk_threads_add_timeout (priv->click_timeout,
                                            cb_button_timeout,
                                            button);
  cb_button_timeout (button);

  return TRUE;
}

static gboolean
cb_button_release (GtkWidget      *widget,
		   GdkEventButton *event,
		   gpointer        user_data)
{
  GtkScaleButton *button;
  GtkScaleButtonPrivate *priv;

  button = GTK_SCALE_BUTTON (user_data);
  priv = button->priv;

  if (priv->click_id != 0)
    {
      g_source_remove (priv->click_id);
      priv->click_id = 0;
    }

  return TRUE;
}

static void
cb_dock_grab_notify (GtkWidget *widget,
		     gboolean   was_grabbed,
		     gpointer   user_data)
{
  GtkScaleButton *button = (GtkScaleButton *) user_data;

  gtk_scale_button_grab_notify (button, was_grabbed);
}

static gboolean
cb_dock_grab_broken_event (GtkWidget *widget,
			   gboolean   was_grabbed,
			   gpointer   user_data)
{
  GtkScaleButton *button = (GtkScaleButton *) user_data;

  gtk_scale_button_grab_notify (button, FALSE);

  return FALSE;
}

/*
 * Scale callbacks.
 */

static void
gtk_scale_button_release_grab (GtkScaleButton *button,
			       GdkEventButton *event)
{
  GdkEventButton *e;
  GdkDisplay *display;
  GtkScaleButtonPrivate *priv;

  priv = button->priv;

  /* ungrab focus */
  display = gtk_widget_get_display (GTK_WIDGET (button));
  gdk_display_keyboard_ungrab (display, event->time);
  gdk_display_pointer_ungrab (display, event->time);
  gtk_grab_remove (priv->dock);

  /* hide again */
  gtk_widget_hide (priv->dock);
  priv->timeout = FALSE;

  e = (GdkEventButton *) gdk_event_copy ((GdkEvent *) event);
  e->window = GTK_WIDGET (button)->window;
  e->type = GDK_BUTTON_RELEASE;
  gtk_widget_event (GTK_WIDGET (button), (GdkEvent *) e);
  e->window = event->window;
  gdk_event_free ((GdkEvent *) e);
}

static gboolean
cb_dock_button_press (GtkWidget      *widget,
		      GdkEventButton *event,
		      gpointer        user_data)
{
  GtkScaleButton *button = GTK_SCALE_BUTTON (user_data);

  if (event->type == GDK_BUTTON_PRESS)
    {
      gtk_scale_button_release_grab (button, event);
      return TRUE;
    }

  return FALSE;
}

static void
gtk_scale_button_popdown_from_bindings (GtkWidget *widget)
{
  GtkScaleButton *button;
  GtkScaleButtonPrivate *priv;
  GdkDisplay *display;

  button = GTK_SCALE_BUTTON (widget);
  priv = button->priv;

  /* ungrab focus */
  display = gtk_widget_get_display (widget);
  gdk_display_keyboard_ungrab (display, GDK_CURRENT_TIME);
  gdk_display_pointer_ungrab (display, GDK_CURRENT_TIME);
  gtk_grab_remove (priv->dock);

  /* hide again */
  gtk_widget_hide (priv->dock);
  priv->timeout = FALSE;
}

static gboolean
cb_dock_key_release (GtkWidget   *widget,
		     GdkEventKey *event,
		     gpointer     user_data)
{
  if (event->keyval == GDK_Escape)
    {
      gtk_scale_button_popdown_from_bindings (GTK_WIDGET (user_data));
      return TRUE;
    }

  if (!gtk_bindings_activate_event (GTK_OBJECT (widget), event))
    {
      /* The popup hasn't managed the event, pass onto the button */
      gtk_bindings_activate_event (GTK_OBJECT (user_data), event);
    }

  return TRUE;
}

static void
cb_scale_grab_notify (GtkWidget *widget,
		      gboolean   was_grabbed,
		      gpointer   user_data)
{
  GtkScaleButton *button = (GtkScaleButton *) user_data;

  gtk_scale_button_grab_notify (button, was_grabbed);
}

/*
 * Scale stuff.
 */

static GType gtk_scale_button_hscale_get_type (void) G_GNUC_CONST;
static GType gtk_scale_button_vscale_get_type (void) G_GNUC_CONST;


#define GTK_TYPE_SCALE_BUTTON_VSCALE    (gtk_scale_button_vscale_get_type ())
#define GTK_SCALE_BUTTON_VSCALE(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_SCALE_BUTTON_VSCALE, GtkScaleButtonVScale))
#define GTK_IS_SCALE_BUTTON_VSCALE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_SCALE_BUTTON_VSCALE))

typedef struct _GtkScaleButtonVScale {
  GtkVScale parent;
  GtkScaleButton *button;
} GtkScaleButtonVScale;

typedef struct _GtkScaleButtonVScaleClass {
  GtkVScaleClass parent_class;
} GtkScaleButtonVScaleClass;

#define GTK_TYPE_SCALE_BUTTON_HSCALE    (gtk_scale_button_hscale_get_type ())
#define GTK_SCALE_BUTTON_HSCALE(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_SCALE_BUTTON_HSCALE, GtkScaleButtonHScale))
#define GTK_IS_SCALE_BUTTON_HSCALE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_SCALE_BUTTON_HSCALE))

typedef struct _GtkScaleButtonHScale {
  GtkVScale parent;
  GtkScaleButton *button;
} GtkScaleButtonHScale;

typedef struct _GtkScaleButtonHScaleClass {
  GtkVScaleClass parent_class;
} GtkScaleButtonHScaleClass;

static gboolean	gtk_scale_button_scale_press   (GtkWidget      *widget,
                                                GdkEventButton *event);
static gboolean gtk_scale_button_scale_release (GtkWidget      *widget,
                                                GdkEventButton *event);

G_DEFINE_TYPE (GtkScaleButtonVScale, gtk_scale_button_vscale, GTK_TYPE_VSCALE)

G_DEFINE_TYPE (GtkScaleButtonHScale, gtk_scale_button_hscale, GTK_TYPE_HSCALE)

static void
gtk_scale_button_vscale_class_init (GtkScaleButtonVScaleClass *klass)
{
  GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
  GtkRangeClass *gtkrange_class = GTK_RANGE_CLASS (klass);

  gtkwidget_class->button_press_event = gtk_scale_button_scale_press;
  gtkwidget_class->button_release_event = gtk_scale_button_scale_release;
  gtkrange_class->value_changed = gtk_scale_button_scale_value_changed;
}

static void
gtk_scale_button_hscale_class_init (GtkScaleButtonHScaleClass *klass)
{
  GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
  GtkRangeClass *gtkrange_class = GTK_RANGE_CLASS (klass);

  gtkwidget_class->button_press_event = gtk_scale_button_scale_press;
  gtkwidget_class->button_release_event = gtk_scale_button_scale_release;
  gtkrange_class->value_changed = gtk_scale_button_scale_value_changed;
}

static void
gtk_scale_button_vscale_init (GtkScaleButtonVScale *scale)
{
}

static void
gtk_scale_button_hscale_init (GtkScaleButtonHScale *scale)
{
}

static GtkWidget *
gtk_scale_button_scale_box_new (GtkScaleButton *button)
{
  GtkScaleButtonPrivate *priv = button->priv;
  GtkScaleButtonVScale *scale;
  GtkWidget *box;

  if (priv->orientation == GTK_ORIENTATION_VERTICAL)
    {
      box = gtk_vbox_new (FALSE, 0);

      scale = g_object_new (GTK_TYPE_SCALE_BUTTON_VSCALE, NULL);
      priv->scale = GTK_WIDGET (scale);

      gtk_widget_set_size_request (priv->scale, -1, SCALE_SIZE);
      gtk_range_set_inverted (GTK_RANGE (priv->scale), TRUE);

      gtk_box_pack_start (GTK_BOX (box), button->plus_button, TRUE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (box), priv->scale, TRUE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (box), button->minus_button, TRUE, FALSE, 0);
    }
  else
    {
      box = gtk_hbox_new (FALSE, 0);

      scale = g_object_new (GTK_TYPE_SCALE_BUTTON_HSCALE, NULL);
      priv->scale = GTK_WIDGET (scale);

      gtk_widget_set_size_request (priv->scale, SCALE_SIZE, -1);

      gtk_box_pack_start (GTK_BOX (box), button->minus_button, TRUE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (box), priv->scale, TRUE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (box), button->plus_button, TRUE, FALSE, 0);
    }

  gtk_scale_set_draw_value (GTK_SCALE (priv->scale), FALSE);
  gtk_range_set_adjustment (GTK_RANGE (scale),
                            GTK_ADJUSTMENT (priv->adjustment));
  scale->button = button;

  g_signal_connect (priv->scale, "grab-notify",
                    G_CALLBACK (cb_scale_grab_notify), button);

  /* FIXME: without this, the popup window appears as a square
   * after changing the orientation
   */
  gtk_window_resize (GTK_WINDOW (priv->dock), 1, 1);

  return box;
}

static gboolean
gtk_scale_button_scale_press (GtkWidget      *widget,
			      GdkEventButton *event)
{
  GtkScaleButtonPrivate *priv;
  GtkWidgetClass *widget_class;

  if (GTK_IS_SCALE_BUTTON_VSCALE (widget))
    {
      priv = GTK_SCALE_BUTTON_VSCALE (widget)->button->priv;
      widget_class = GTK_WIDGET_CLASS (gtk_scale_button_vscale_parent_class);
    }
  else
    {
      priv = GTK_SCALE_BUTTON_HSCALE (widget)->button->priv;
      widget_class = GTK_WIDGET_CLASS (gtk_scale_button_hscale_parent_class);
    }

  /* the scale will grab input; if we have input grabbed, all goes
   * horribly wrong, so let's not do that. */
  gtk_grab_remove (priv->dock);

  return widget_class->button_press_event (widget, event);
}

static gboolean
gtk_scale_button_scale_release (GtkWidget      *widget,
				GdkEventButton *event)
{
  GtkScaleButton *button;
  GtkWidgetClass *widget_class;
  gboolean res;

  if (GTK_IS_SCALE_BUTTON_VSCALE (widget))
    {
      button = GTK_SCALE_BUTTON_VSCALE (widget)->button;
      widget_class = GTK_WIDGET_CLASS (gtk_scale_button_vscale_parent_class);
    }
  else
    {
      button = GTK_SCALE_BUTTON_HSCALE (widget)->button;
      widget_class = GTK_WIDGET_CLASS (gtk_scale_button_hscale_parent_class);
    }

  if (button->priv->timeout)
    {
      /* if we did a quick click, leave the window open; else, hide it */
      if (event->time > button->priv->pop_time + button->priv->click_timeout)
        {

	  gtk_scale_button_release_grab (button, event);
	  widget_class->button_release_event (widget, event);

	  return TRUE;
	}

      button->priv->timeout = FALSE;
    }

  res = widget_class->button_release_event (widget, event);

  /* the scale will release input; right after that, we *have to* grab
   * it back so we can catch out-of-scale clicks and hide the popup,
   * so I basically want a g_signal_connect_after_always(), but I can't
   * find that, so we do this complex 'first-call-parent-then-do-actual-
   * action' thingy...
   */
  gtk_grab_add (button->priv->dock);

  return res;
}

static void
gtk_scale_button_update_icon (GtkScaleButton *button)
{
  GtkScaleButtonPrivate *priv;
  GtkRange *range;
  GtkAdjustment *adj;
  gdouble value;
  const gchar *name;
  guint num_icons;

  priv = button->priv;

  if (!priv->icon_list || priv->icon_list[0] == '\0')
    {
      gtk_image_set_from_stock (GTK_IMAGE (priv->image),
				GTK_STOCK_MISSING_IMAGE,
				priv->size);
      return;
    }

  num_icons = g_strv_length (priv->icon_list);

  /* The 1-icon special case */
  if (num_icons == 1)
    {
      gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
				    priv->icon_list[0],
				    priv->size);
      return;
    }

  range = GTK_RANGE (priv->scale);
  adj = priv->adjustment;
  value = gtk_scale_button_get_value (button);

  /* The 2-icons special case */
  if (num_icons == 2)
    {
      gdouble limit;
      limit = (adj->upper - adj->lower) / 2 + adj->lower;
      if (value < limit)
	name = priv->icon_list[0];
      else
	name = priv->icon_list[1];

      gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
				    name,
				    priv->size);
      return;
    }

  /* With 3 or more icons */
  if (value == adj->lower)
    {
      name = priv->icon_list[0];
    }
  else if (value == adj->upper)
    {
      name = priv->icon_list[1];
    }
  else
    {
      gdouble step;
      guint i;

      step = (adj->upper - adj->lower) / (num_icons - 2);
      i = (guint) ((value - adj->lower) / step) + 2;
      g_assert (i < num_icons);
      name = priv->icon_list[i];
    }

  gtk_image_set_from_icon_name (GTK_IMAGE (priv->image),
				name,
				priv->size);
}

static void
gtk_scale_button_scale_value_changed (GtkRange *range)
{
  GtkScaleButton *button;
  gdouble value;

  if (GTK_IS_SCALE_BUTTON_VSCALE (range))
    button = GTK_SCALE_BUTTON_VSCALE (range)->button;
  else
    button = GTK_SCALE_BUTTON_HSCALE (range)->button;

  value = gtk_range_get_value (range);

  gtk_scale_button_update_icon (button);

  g_signal_emit (button, signals[VALUE_CHANGED], 0, value);
  g_object_notify (G_OBJECT (button), "value");
}
