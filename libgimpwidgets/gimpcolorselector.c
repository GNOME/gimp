/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorselector.c
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "ligmawidgetstypes.h"

#include "ligmacolorselector.h"
#include "ligmaicons.h"
#include "ligmawidgetsmarshal.h"


/**
 * SECTION: ligmacolorselector
 * @title: LigmaColorSelector
 * @short_description: Pluggable LIGMA color selector modules.
 * @see_also: #GModule, #GTypeModule, #LigmaModule
 *
 * Functions and definitions for creating pluggable LIGMA color
 * selector modules.
 **/


enum
{
  COLOR_CHANGED,
  CHANNEL_CHANGED,
  MODEL_VISIBLE_CHANGED,
  LAST_SIGNAL
};


struct _LigmaColorSelectorPrivate
{
  gboolean model_visible[3];
};

#define GET_PRIVATE(obj) (((LigmaColorSelector *) (obj))->priv)


static void   ligma_color_selector_dispose (GObject *object);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaColorSelector, ligma_color_selector,
                            GTK_TYPE_BOX)

#define parent_class ligma_color_selector_parent_class

static guint selector_signals[LAST_SIGNAL] = { 0 };


static void
ligma_color_selector_class_init (LigmaColorSelectorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ligma_color_selector_dispose;

  selector_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColorSelectorClass, color_changed),
                  NULL, NULL,
                  _ligma_widgets_marshal_VOID__BOXED_BOXED,
                  G_TYPE_NONE, 2,
                  LIGMA_TYPE_RGB,
                  LIGMA_TYPE_RGB);

  selector_signals[CHANNEL_CHANGED] =
    g_signal_new ("channel-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColorSelectorClass, channel_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_COLOR_SELECTOR_CHANNEL);

  selector_signals[MODEL_VISIBLE_CHANGED] =
    g_signal_new ("model-visible-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColorSelectorClass, model_visible_changed),
                  NULL, NULL,
                  _ligma_widgets_marshal_VOID__ENUM_BOOLEAN,
                  G_TYPE_NONE, 2,
                  LIGMA_TYPE_COLOR_SELECTOR_MODEL,
                  G_TYPE_BOOLEAN);

  klass->name                  = "Unnamed";
  klass->help_id               = NULL;
  klass->icon_name             = LIGMA_ICON_PALETTE;

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
}

static void
ligma_color_selector_init (LigmaColorSelector *selector)
{
  LigmaColorSelectorPrivate *priv;

  selector->priv = ligma_color_selector_get_instance_private (selector);

  priv = GET_PRIVATE (selector);

  selector->toggles_visible   = TRUE;
  selector->toggles_sensitive = TRUE;
  selector->show_alpha        = TRUE;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (selector),
                                  GTK_ORIENTATION_VERTICAL);

  ligma_rgba_set (&selector->rgb, 0.0, 0.0, 0.0, 1.0);
  ligma_rgb_to_hsv (&selector->rgb, &selector->hsv);

  selector->channel = LIGMA_COLOR_SELECTOR_RED;

  priv->model_visible[LIGMA_COLOR_SELECTOR_MODEL_RGB] = TRUE;
  priv->model_visible[LIGMA_COLOR_SELECTOR_MODEL_LCH] = TRUE;
  priv->model_visible[LIGMA_COLOR_SELECTOR_MODEL_HSV] = FALSE;
}

static void
ligma_color_selector_dispose (GObject *object)
{
  ligma_color_selector_set_config (LIGMA_COLOR_SELECTOR (object), NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */

/**
 * ligma_color_selector_new:
 * @selector_type: The #GType of the selector to create.
 * @rgb:           The initial color to be edited.
 * @hsv:           The same color in HSV.
 * @channel:       The selector's initial channel.
 *
 * Creates a new #LigmaColorSelector widget of type @selector_type.
 *
 * Note that this is mostly internal API to be used by other widgets.
 *
 * Please use ligma_color_selection_new() for the "LIGMA-typical" color
 * selection widget. Also see ligma_color_button_new().
 *
 * Retunn value: the new #LigmaColorSelector widget.
 **/
GtkWidget *
ligma_color_selector_new (GType                     selector_type,
                         const LigmaRGB            *rgb,
                         const LigmaHSV            *hsv,
                         LigmaColorSelectorChannel  channel)
{
  LigmaColorSelector *selector;

  g_return_val_if_fail (g_type_is_a (selector_type, LIGMA_TYPE_COLOR_SELECTOR),
                        NULL);
  g_return_val_if_fail (rgb != NULL, NULL);
  g_return_val_if_fail (hsv != NULL, NULL);

  selector = g_object_new (selector_type, NULL);

  ligma_color_selector_set_color (selector, rgb, hsv);
  ligma_color_selector_set_channel (selector, channel);

  return GTK_WIDGET (selector);
}

/**
 * ligma_color_selector_set_toggles_visible:
 * @selector: A #LigmaColorSelector widget.
 * @visible:  The new @visible setting.
 *
 * Sets the @visible property of the @selector's toggles.
 *
 * This function has no effect if this @selector instance has no
 * toggles to switch channels.
 **/
void
ligma_color_selector_set_toggles_visible (LigmaColorSelector *selector,
                                         gboolean           visible)
{
  g_return_if_fail (LIGMA_IS_COLOR_SELECTOR (selector));

  if (selector->toggles_visible != visible)
    {
      LigmaColorSelectorClass *selector_class;

      selector->toggles_visible = visible ? TRUE : FALSE;

      selector_class = LIGMA_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_toggles_visible)
        selector_class->set_toggles_visible (selector, visible);
    }
}

/**
 * ligma_color_selector_get_toggles_visible:
 * @selector: A #LigmaColorSelector widget.
 *
 * Returns the @visible property of the @selector's toggles.
 *
 * Returns: %TRUE if the #LigmaColorSelector's toggles are visible.
 *
 * Since: 2.10
 **/
gboolean
ligma_color_selector_get_toggles_visible (LigmaColorSelector *selector)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_SELECTOR (selector), FALSE);

  return selector->toggles_visible;
}

/**
 * ligma_color_selector_set_toggles_sensitive:
 * @selector:  A #LigmaColorSelector widget.
 * @sensitive: The new @sensitive setting.
 *
 * Sets the @sensitive property of the @selector's toggles.
 *
 * This function has no effect if this @selector instance has no
 * toggles to switch channels.
 **/
void
ligma_color_selector_set_toggles_sensitive (LigmaColorSelector *selector,
                                           gboolean           sensitive)
{
  g_return_if_fail (LIGMA_IS_COLOR_SELECTOR (selector));

  if (selector->toggles_sensitive != sensitive)
    {
      LigmaColorSelectorClass *selector_class;

      selector->toggles_sensitive = sensitive ? TRUE : FALSE;

      selector_class = LIGMA_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_toggles_sensitive)
        selector_class->set_toggles_sensitive (selector, sensitive);
    }
}

/**
 * ligma_color_selector_get_toggles_sensitive:
 * @selector: A #LigmaColorSelector widget.
 *
 * Returns the @sensitive property of the @selector's toggles.
 *
 * Returns: %TRUE if the #LigmaColorSelector's toggles are sensitive.
 *
 * Since: 2.10
 **/
gboolean
ligma_color_selector_get_toggles_sensitive (LigmaColorSelector *selector)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_SELECTOR (selector), FALSE);

  return selector->toggles_sensitive;
}

/**
 * ligma_color_selector_set_show_alpha:
 * @selector:   A #LigmaColorSelector widget.
 * @show_alpha: The new @show_alpha setting.
 *
 * Sets the @show_alpha property of the @selector widget.
 **/
void
ligma_color_selector_set_show_alpha (LigmaColorSelector *selector,
                                    gboolean           show_alpha)
{
  g_return_if_fail (LIGMA_IS_COLOR_SELECTOR (selector));

  if (show_alpha != selector->show_alpha)
    {
      LigmaColorSelectorClass *selector_class;

      selector->show_alpha = show_alpha ? TRUE : FALSE;

      selector_class = LIGMA_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_show_alpha)
        selector_class->set_show_alpha (selector, show_alpha);
    }
}

/**
 * ligma_color_selector_get_show_alpha:
 * @selector: A #LigmaColorSelector widget.
 *
 * Returns the @selector's @show_alpha property.
 *
 * Returns: %TRUE if the #LigmaColorSelector has alpha controls.
 *
 * Since: 2.10
 **/
gboolean
ligma_color_selector_get_show_alpha (LigmaColorSelector *selector)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_SELECTOR (selector), FALSE);

  return selector->show_alpha;
}

/**
 * ligma_color_selector_set_color:
 * @selector: A #LigmaColorSelector widget.
 * @rgb:      The new color.
 * @hsv:      The same color in HSV.
 *
 * Sets the color shown in the @selector widget.
 **/
void
ligma_color_selector_set_color (LigmaColorSelector *selector,
                               const LigmaRGB     *rgb,
                               const LigmaHSV     *hsv)
{
  LigmaColorSelectorClass *selector_class;

  g_return_if_fail (LIGMA_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  selector->rgb = *rgb;
  selector->hsv = *hsv;

  selector_class = LIGMA_COLOR_SELECTOR_GET_CLASS (selector);

  if (selector_class->set_color)
    selector_class->set_color (selector, rgb, hsv);

  ligma_color_selector_emit_color_changed (selector);
}

/**
 * ligma_color_selector_get_color:
 * @selector: A #LigmaColorSelector widget.
 * @rgb:      (out caller-allocates): Return location for the color.
 * @hsv:      (out caller-allocates): Return location for the same same
 *            color in HSV.
 *
 * Retrieves the color shown in the @selector widget.
 *
 * Since: 2.10
 **/
void
ligma_color_selector_get_color (LigmaColorSelector *selector,
                               LigmaRGB           *rgb,
                               LigmaHSV           *hsv)
{
  g_return_if_fail (LIGMA_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  *rgb = selector->rgb;
  *hsv = selector->hsv;
}

/**
 * ligma_color_selector_set_channel:
 * @selector: A #LigmaColorSelector widget.
 * @channel:  The new @channel setting.
 *
 * Sets the @channel property of the @selector widget.
 *
 * Changes between displayed channels if this @selector instance has
 * the ability to show different channels.
 * This will also update the color model if needed.
 **/
void
ligma_color_selector_set_channel (LigmaColorSelector        *selector,
                                 LigmaColorSelectorChannel  channel)
{
  g_return_if_fail (LIGMA_IS_COLOR_SELECTOR (selector));

  if (channel != selector->channel)
    {
      LigmaColorSelectorClass *selector_class;
      LigmaColorSelectorModel  model = -1;

      selector->channel = channel;

      switch (channel)
        {
        case LIGMA_COLOR_SELECTOR_RED:
        case LIGMA_COLOR_SELECTOR_GREEN:
        case LIGMA_COLOR_SELECTOR_BLUE:
          model = LIGMA_COLOR_SELECTOR_MODEL_RGB;
          break;

        case LIGMA_COLOR_SELECTOR_HUE:
        case LIGMA_COLOR_SELECTOR_SATURATION:
        case LIGMA_COLOR_SELECTOR_VALUE:
          model = LIGMA_COLOR_SELECTOR_MODEL_HSV;
          break;

        case LIGMA_COLOR_SELECTOR_LCH_LIGHTNESS:
        case LIGMA_COLOR_SELECTOR_LCH_CHROMA:
        case LIGMA_COLOR_SELECTOR_LCH_HUE:
          model = LIGMA_COLOR_SELECTOR_MODEL_LCH;
          break;

        case LIGMA_COLOR_SELECTOR_ALPHA:
          /* Alpha channel does not change the color model. */
          break;

        default:
          /* Should not happen. */
          g_return_if_reached ();
          break;
        }

      selector_class = LIGMA_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_channel)
        selector_class->set_channel (selector, channel);

      ligma_color_selector_emit_channel_changed (selector);

      if (model != -1)
        {
          /*  make visibility of LCH and HSV mutuallky exclusive  */
          if (model == LIGMA_COLOR_SELECTOR_MODEL_HSV)
            {
              ligma_color_selector_set_model_visible (selector,
                                                     LIGMA_COLOR_SELECTOR_MODEL_LCH,
                                                     FALSE);
            }
          else if (model == LIGMA_COLOR_SELECTOR_MODEL_LCH)
            {
              ligma_color_selector_set_model_visible (selector,
                                                     LIGMA_COLOR_SELECTOR_MODEL_HSV,
                                                     FALSE);
            }

          ligma_color_selector_set_model_visible (selector, model, TRUE);
        }
    }
}

/**
 * ligma_color_selector_get_channel:
 * @selector: A #LigmaColorSelector widget.
 *
 * Returns the @selector's current channel.
 *
 * Returns: The #LigmaColorSelectorChannel currently shown by the
 * @selector.
 *
 * Since: 2.10
 **/
LigmaColorSelectorChannel
ligma_color_selector_get_channel (LigmaColorSelector *selector)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_SELECTOR (selector),
                        LIGMA_COLOR_SELECTOR_RED);

  return selector->channel;
}

/**
 * ligma_color_selector_set_model_visible:
 * @selector: A #LigmaColorSelector widget.
 * @model:    The affected #LigmaColorSelectorModel.
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
ligma_color_selector_set_model_visible (LigmaColorSelector      *selector,
                                       LigmaColorSelectorModel  model,
                                       gboolean                visible)
{
  LigmaColorSelectorPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_SELECTOR (selector));

  priv = GET_PRIVATE (selector);

  visible = visible ? TRUE : FALSE;

  if (visible != priv->model_visible[model])
    {
      LigmaColorSelectorClass *selector_class;

      priv->model_visible[model] = visible;

      selector_class = LIGMA_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_model_visible)
        selector_class->set_model_visible (selector, model, visible);

      ligma_color_selector_emit_model_visible_changed (selector, model);
    }
}

/**
 * ligma_color_selector_get_model_visible:
 * @selector: A #LigmaColorSelector widget.
 * @model:    The #LigmaColorSelectorModel.
 *
 * Returns: whether @model is visible in @selector.
 *
 * Since: 2.10
 **/
gboolean
ligma_color_selector_get_model_visible (LigmaColorSelector      *selector,
                                       LigmaColorSelectorModel  model)
{
  LigmaColorSelectorPrivate *priv;

  g_return_val_if_fail (LIGMA_IS_COLOR_SELECTOR (selector), FALSE);

  priv = GET_PRIVATE (selector);

  return priv->model_visible[model];
}

/**
 * ligma_color_selector_emit_color_changed:
 * @selector: A #LigmaColorSelector widget.
 *
 * Emits the "color-changed" signal.
 */
void
ligma_color_selector_emit_color_changed (LigmaColorSelector *selector)
{
  g_return_if_fail (LIGMA_IS_COLOR_SELECTOR (selector));

  g_signal_emit (selector, selector_signals[COLOR_CHANGED], 0,
                 &selector->rgb, &selector->hsv);
}

/**
 * ligma_color_selector_emit_channel_changed:
 * @selector: A #LigmaColorSelector widget.
 *
 * Emits the "channel-changed" signal.
 */
void
ligma_color_selector_emit_channel_changed (LigmaColorSelector *selector)
{
  g_return_if_fail (LIGMA_IS_COLOR_SELECTOR (selector));

  g_signal_emit (selector, selector_signals[CHANNEL_CHANGED], 0,
                 selector->channel);
}

/**
 * ligma_color_selector_emit_model_visible_changed:
 * @selector: A #LigmaColorSelector widget.
 * @model:    The #LigmaColorSelectorModel where visibility changed.
 *
 * Emits the "model-visible-changed" signal.
 *
 * Since: 2.10
 */
void
ligma_color_selector_emit_model_visible_changed (LigmaColorSelector      *selector,
                                                LigmaColorSelectorModel  model)
{
  LigmaColorSelectorPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_SELECTOR (selector));

  priv = GET_PRIVATE (selector);

  g_signal_emit (selector, selector_signals[MODEL_VISIBLE_CHANGED], 0,
                 model, priv->model_visible[model]);
}

/**
 * ligma_color_selector_set_config:
 * @selector: a #LigmaColorSelector widget.
 * @config:   a #LigmaColorConfig object.
 *
 * Sets the color management configuration to use with this color selector.
 *
 * Since: 2.4
 */
void
ligma_color_selector_set_config (LigmaColorSelector *selector,
                                LigmaColorConfig   *config)
{
  LigmaColorSelectorClass *selector_class;

  g_return_if_fail (LIGMA_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (config == NULL || LIGMA_IS_COLOR_CONFIG (config));

  selector_class = LIGMA_COLOR_SELECTOR_GET_CLASS (selector);

  if (selector_class->set_config)
    selector_class->set_config (selector, config);
}

/**
 * ligma_color_selector_set_simulation
 * @selector: a #LigmaColorSelector widget.
 * @profile:  a #LigmaColorProfile object.
 * @intent:   a #LigmaColorRenderingIntent enum.
 * @bpc:      a gboolean.
 *
 * Sets the simulation options to use with this color selector.
 *
 * Since: 3.0
 */
void
ligma_color_selector_set_simulation (LigmaColorSelector *selector,
                                    LigmaColorProfile  *profile,
                                    LigmaColorRenderingIntent intent,
                                    gboolean           bpc)
{
  LigmaColorSelectorClass *selector_class;

  g_return_if_fail (LIGMA_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (profile == NULL || LIGMA_IS_COLOR_PROFILE (profile));

  selector_class = LIGMA_COLOR_SELECTOR_GET_CLASS (selector);

  if (selector_class->set_simulation)
    selector_class->set_simulation (selector, profile, intent, bpc);
}
