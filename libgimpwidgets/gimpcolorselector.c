/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorselector.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on:
 * Colour selector module
 * Copyright (C) 1999 Austin Donnelly <austin@greenend.org.uk>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorselector.h"
#include "gimpicons.h"
#include "gimpwidgetsmarshal.h"


/**
 * SECTION: gimpcolorselector
 * @title: GimpColorSelector
 * @short_description: Pluggable GIMP color selector modules.
 * @see_also: #GModule, #GTypeModule, #GimpModule
 *
 * Functions and definitions for creating pluggable GIMP color
 * selector modules.
 **/


enum
{
  COLOR_CHANGED,
  CHANNEL_CHANGED,
  MODEL_VISIBLE_CHANGED,
  LAST_SIGNAL
};


struct _GimpColorSelectorPrivate
{
  gboolean model_visible[3];
};

#define GET_PRIVATE(obj) (((GimpColorSelector *) (obj))->priv)


static void   gimp_color_selector_dispose (GObject *object);


G_DEFINE_TYPE (GimpColorSelector, gimp_color_selector, GTK_TYPE_BOX)

#define parent_class gimp_color_selector_parent_class

static guint selector_signals[LAST_SIGNAL] = { 0 };


static void
gimp_color_selector_class_init (GimpColorSelectorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gimp_color_selector_dispose;

  selector_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorSelectorClass, color_changed),
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__POINTER_POINTER,
                  G_TYPE_NONE, 2,
                  G_TYPE_POINTER,
                  G_TYPE_POINTER);

  selector_signals[CHANNEL_CHANGED] =
    g_signal_new ("channel-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorSelectorClass, channel_changed),
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_COLOR_SELECTOR_CHANNEL);

  selector_signals[MODEL_VISIBLE_CHANGED] =
    g_signal_new ("model-visible-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorSelectorClass, model_visible_changed),
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__ENUM_BOOLEAN,
                  G_TYPE_NONE, 2,
                  GIMP_TYPE_COLOR_SELECTOR_MODEL,
                  G_TYPE_BOOLEAN);

  klass->name                  = "Unnamed";
  klass->help_id               = NULL;
  klass->icon_name             = GIMP_ICON_PALETTE;

  klass->set_toggles_visible   = NULL;
  klass->set_toggles_sensitive = NULL;
  klass->set_show_alpha        = NULL;
  klass->set_color             = NULL;
  klass->set_channel           = NULL;
  klass->set_model_visible     = NULL;
  klass->color_changed         = NULL;
  klass->channel_changed       = NULL;
  klass->model_visible_changed = NULL;
  klass->set_config            = NULL;

  g_type_class_add_private (object_class, sizeof (GimpColorSelectorPrivate));
}

static void
gimp_color_selector_init (GimpColorSelector *selector)
{
  GimpColorSelectorPrivate *priv;

  selector->priv = G_TYPE_INSTANCE_GET_PRIVATE (selector,
                                                GIMP_TYPE_COLOR_SELECTOR,
                                                GimpColorSelectorPrivate);

  priv = GET_PRIVATE (selector);

  selector->toggles_visible   = TRUE;
  selector->toggles_sensitive = TRUE;
  selector->show_alpha        = TRUE;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (selector),
                                  GTK_ORIENTATION_VERTICAL);

  gimp_rgba_set (&selector->rgb, 0.0, 0.0, 0.0, 1.0);
  gimp_rgb_to_hsv (&selector->rgb, &selector->hsv);

  selector->channel = GIMP_COLOR_SELECTOR_RED;

  priv->model_visible[GIMP_COLOR_SELECTOR_MODEL_RGB] = TRUE;
  priv->model_visible[GIMP_COLOR_SELECTOR_MODEL_LCH] = TRUE;
  priv->model_visible[GIMP_COLOR_SELECTOR_MODEL_HSV] = FALSE;
}

static void
gimp_color_selector_dispose (GObject *object)
{
  gimp_color_selector_set_config (GIMP_COLOR_SELECTOR (object), NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */

/**
 * gimp_color_selector_new:
 * @selector_type: The #GType of the selector to create.
 * @rgb:           The initial color to be edited.
 * @hsv:           The same color in HSV.
 * @channel:       The selector's initial channel.
 *
 * Creates a new #GimpColorSelector widget of type @selector_type.
 *
 * Note that this is mostly internal API to be used by other widgets.
 *
 * Please use gimp_color_selection_new() for the "GIMP-typical" color
 * selection widget. Also see gimp_color_button_new().
 *
 * Retunn value: the new #GimpColorSelector widget.
 **/
GtkWidget *
gimp_color_selector_new (GType                     selector_type,
                         const GimpRGB            *rgb,
                         const GimpHSV            *hsv,
                         GimpColorSelectorChannel  channel)
{
  GimpColorSelector *selector;

  g_return_val_if_fail (g_type_is_a (selector_type, GIMP_TYPE_COLOR_SELECTOR),
                        NULL);
  g_return_val_if_fail (rgb != NULL, NULL);
  g_return_val_if_fail (hsv != NULL, NULL);

  selector = g_object_new (selector_type, NULL);

  gimp_color_selector_set_color (selector, rgb, hsv);
  gimp_color_selector_set_channel (selector, channel);

  return GTK_WIDGET (selector);
}

/**
 * gimp_color_selector_set_toggles_visible:
 * @selector: A #GimpColorSelector widget.
 * @visible:  The new @visible setting.
 *
 * Sets the @visible property of the @selector's toggles.
 *
 * This function has no effect if this @selector instance has no
 * toggles to switch channels.
 **/
void
gimp_color_selector_set_toggles_visible (GimpColorSelector *selector,
                                         gboolean           visible)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  if (selector->toggles_visible != visible)
    {
      GimpColorSelectorClass *selector_class;

      selector->toggles_visible = visible ? TRUE : FALSE;

      selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_toggles_visible)
        selector_class->set_toggles_visible (selector, visible);
    }
}

/**
 * gimp_color_selector_get_toggles_visible:
 * @selector: A #GimpColorSelector widget.
 *
 * Returns the @visible property of the @selector's toggles.
 *
 * Return value: #TRUE if the #GimpColorSelector's toggles are visible.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_selector_get_toggles_visible (GimpColorSelector *selector)
{
  g_return_val_if_fail (GIMP_IS_COLOR_SELECTOR (selector), FALSE);

  return selector->toggles_visible;
}

/**
 * gimp_color_selector_set_toggles_sensitive:
 * @selector:  A #GimpColorSelector widget.
 * @sensitive: The new @sensitive setting.
 *
 * Sets the @sensitive property of the @selector's toggles.
 *
 * This function has no effect if this @selector instance has no
 * toggles to switch channels.
 **/
void
gimp_color_selector_set_toggles_sensitive (GimpColorSelector *selector,
                                           gboolean           sensitive)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  if (selector->toggles_sensitive != sensitive)
    {
      GimpColorSelectorClass *selector_class;

      selector->toggles_sensitive = sensitive ? TRUE : FALSE;

      selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_toggles_sensitive)
        selector_class->set_toggles_sensitive (selector, sensitive);
    }
}

/**
 * gimp_color_selector_get_toggles_sensitive:
 * @selector: A #GimpColorSelector widget.
 *
 * Returns the @sensitive property of the @selector's toggles.
 *
 * Return value: #TRUE if the #GimpColorSelector's toggles are sensitive.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_selector_get_toggles_sensitive (GimpColorSelector *selector)
{
  g_return_val_if_fail (GIMP_IS_COLOR_SELECTOR (selector), FALSE);

  return selector->toggles_sensitive;
}

/**
 * gimp_color_selector_set_show_alpha:
 * @selector:   A #GimpColorSelector widget.
 * @show_alpha: The new @show_alpha setting.
 *
 * Sets the @show_alpha property of the @selector widget.
 **/
void
gimp_color_selector_set_show_alpha (GimpColorSelector *selector,
                                    gboolean           show_alpha)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  if (show_alpha != selector->show_alpha)
    {
      GimpColorSelectorClass *selector_class;

      selector->show_alpha = show_alpha ? TRUE : FALSE;

      selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_show_alpha)
        selector_class->set_show_alpha (selector, show_alpha);
    }
}

/**
 * gimp_color_selector_get_show_alpha:
 * @selector: A #GimpColorSelector widget.
 *
 * Returns the @selector's @show_alpha property.
 *
 * Return value: #TRUE if the #GimpColorSelector has alpha controls.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_selector_get_show_alpha (GimpColorSelector *selector)
{
  g_return_val_if_fail (GIMP_IS_COLOR_SELECTOR (selector), FALSE);

  return selector->show_alpha;
}

/**
 * gimp_color_selector_set_color:
 * @selector: A #GimpColorSelector widget.
 * @rgb:      The new color.
 * @hsv:      The same color in HSV.
 *
 * Sets the color shown in the @selector widget.
 **/
void
gimp_color_selector_set_color (GimpColorSelector *selector,
                               const GimpRGB     *rgb,
                               const GimpHSV     *hsv)
{
  GimpColorSelectorClass *selector_class;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  selector->rgb = *rgb;
  selector->hsv = *hsv;

  selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

  if (selector_class->set_color)
    selector_class->set_color (selector, rgb, hsv);

  gimp_color_selector_color_changed (selector);
}

/**
 * gimp_color_selector_get_color:
 * @selector: A #GimpColorSelector widget.
 * @rgb:      Return location for the color.
 * @hsv:      Return location for the same same color in HSV.
 *
 * Retrieves the color shown in the @selector widget.
 *
 * Since: 2.10
 **/
void
gimp_color_selector_get_color (GimpColorSelector *selector,
                               GimpRGB           *rgb,
                               GimpHSV           *hsv)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  *rgb = selector->rgb;
  *hsv = selector->hsv;
}

/**
 * gimp_color_selector_set_channel:
 * @selector: A #GimpColorSelector widget.
 * @channel:  The new @channel setting.
 *
 * Sets the @channel property of the @selector widget.
 *
 * Changes between displayed channels if this @selector instance has
 * the ability to show different channels.
 * This will also update the color model if needed.
 **/
void
gimp_color_selector_set_channel (GimpColorSelector        *selector,
                                 GimpColorSelectorChannel  channel)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  if (channel != selector->channel)
    {
      GimpColorSelectorClass *selector_class;
      GimpColorSelectorModel  model = -1;

      selector->channel = channel;

      switch (channel)
        {
        case GIMP_COLOR_SELECTOR_RED:
        case GIMP_COLOR_SELECTOR_GREEN:
        case GIMP_COLOR_SELECTOR_BLUE:
          model = GIMP_COLOR_SELECTOR_MODEL_RGB;
          break;

        case GIMP_COLOR_SELECTOR_HUE:
        case GIMP_COLOR_SELECTOR_SATURATION:
        case GIMP_COLOR_SELECTOR_VALUE:
          model = GIMP_COLOR_SELECTOR_MODEL_HSV;
          break;

        case GIMP_COLOR_SELECTOR_LCH_LIGHTNESS:
        case GIMP_COLOR_SELECTOR_LCH_CHROMA:
        case GIMP_COLOR_SELECTOR_LCH_HUE:
          model = GIMP_COLOR_SELECTOR_MODEL_LCH;
          break;

        case GIMP_COLOR_SELECTOR_ALPHA:
          /* Alpha channel does not change the color model. */
          break;

        default:
          /* Should not happen. */
          g_return_if_reached ();
          break;
        }

      selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_channel)
        selector_class->set_channel (selector, channel);

      gimp_color_selector_channel_changed (selector);

      if (model != -1)
        {
          /*  make visibility of LCH and HSV mutuallky exclusive  */
          if (model == GIMP_COLOR_SELECTOR_MODEL_HSV)
            {
              gimp_color_selector_set_model_visible (selector,
                                                     GIMP_COLOR_SELECTOR_MODEL_LCH,
                                                     FALSE);
            }
          else if (model == GIMP_COLOR_SELECTOR_MODEL_LCH)
            {
              gimp_color_selector_set_model_visible (selector,
                                                     GIMP_COLOR_SELECTOR_MODEL_HSV,
                                                     FALSE);
            }

          gimp_color_selector_set_model_visible (selector, model, TRUE);
        }
    }
}

/**
 * gimp_color_selector_get_channel:
 * @selector: A #GimpColorSelector widget.
 *
 * Returns the @selector's current channel.
 *
 * Return value: The #GimpColorSelectorChannel currently shown by the
 * @selector.
 *
 * Since: 2.10
 **/
GimpColorSelectorChannel
gimp_color_selector_get_channel (GimpColorSelector *selector)
{
  g_return_val_if_fail (GIMP_IS_COLOR_SELECTOR (selector),
                        GIMP_COLOR_SELECTOR_RED);

  return selector->channel;
}

/**
 * gimp_color_selector_set_model_visible:
 * @selector: A #GimpColorSelector widget.
 * @model:    The affected #GimpColorSelectorModel.
 * @visible:  The new visible setting.
 *
 * Sets the @model visible/invisible on the @selector widget.
 *
 * Toggles visibility of displayed models if this @selector instance
 * has the ability to show different color models.
 *
 * Since: 2.10
 **/
void
gimp_color_selector_set_model_visible (GimpColorSelector      *selector,
                                       GimpColorSelectorModel  model,
                                       gboolean                visible)
{
  GimpColorSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  priv = GET_PRIVATE (selector);

  visible = visible ? TRUE : FALSE;

  if (visible != priv->model_visible[model])
    {
      GimpColorSelectorClass *selector_class;

      priv->model_visible[model] = visible;

      selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_model_visible)
        selector_class->set_model_visible (selector, model, visible);

      gimp_color_selector_model_visible_changed (selector, model);
    }
}

/**
 * gimp_color_selector_get_model_visible:
 * @selector: A #GimpColorSelector widget.
 * @model:    The #GimpColorSelectorModel.
 *
 * Return value: whether @model is visible in @selector.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_selector_get_model_visible (GimpColorSelector      *selector,
                                       GimpColorSelectorModel  model)
{
  GimpColorSelectorPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_SELECTOR (selector), FALSE);

  priv = GET_PRIVATE (selector);

  return priv->model_visible[model];
}

/**
 * gimp_color_selector_color_changed:
 * @selector: A #GimpColorSelector widget.
 *
 * Emits the "color-changed" signal.
 **/
void
gimp_color_selector_color_changed (GimpColorSelector *selector)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  g_signal_emit (selector, selector_signals[COLOR_CHANGED], 0,
                 &selector->rgb, &selector->hsv);
}

/**
 * gimp_color_selector_channel_changed:
 * @selector: A #GimpColorSelector widget.
 *
 * Emits the "channel-changed" signal.
 **/
void
gimp_color_selector_channel_changed (GimpColorSelector *selector)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  g_signal_emit (selector, selector_signals[CHANNEL_CHANGED], 0,
                 selector->channel);
}

/**
 * gimp_color_selector_model_visible_changed:
 * @selector: A #GimpColorSelector widget.
 *
 * Emits the "model-visible-changed" signal.
 *
 * Since: 2.10
 **/
void
gimp_color_selector_model_visible_changed (GimpColorSelector      *selector,
                                           GimpColorSelectorModel  model)
{
  GimpColorSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  priv = GET_PRIVATE (selector);

  g_signal_emit (selector, selector_signals[MODEL_VISIBLE_CHANGED], 0,
                 model, priv->model_visible[model]);
}

/**
 * gimp_color_selector_set_config:
 * @selector: a #GimpColorSelector widget.
 * @config:   a #GimpColorConfig object.
 *
 * Sets the color management configuration to use with this color selector.
 *
 * Since: 2.4
 */
void
gimp_color_selector_set_config (GimpColorSelector *selector,
                                GimpColorConfig   *config)
{
  GimpColorSelectorClass *selector_class;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (config == NULL || GIMP_IS_COLOR_CONFIG (config));

  selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

  if (selector_class->set_config)
    selector_class->set_config (selector, config);
}
