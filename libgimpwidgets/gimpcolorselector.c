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
 * <https://www.gnu.org/licenses/>.
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
  SIMULATION,
  LAST_SIGNAL
};


typedef struct _GimpColorSelectorPrivate
{
  gboolean                  toggles_visible;
  gboolean                  toggles_sensitive;
  gboolean                  show_alpha;
  gboolean                  model_visible[3];

  GimpColorSelectorChannel  channel;

  GeglColor                *color;

  gboolean                  simulation;
  GimpColorProfile         *simulation_profile;
  GimpColorRenderingIntent  simulation_intent;
  gboolean                  simulation_bpc;
} GimpColorSelectorPrivate;


static void   gimp_color_selector_dispose                    (GObject           *object);

static void   gimp_color_selector_emit_color_changed         (GimpColorSelector *selector);
static void   gimp_color_selector_emit_channel_changed       (GimpColorSelector *selector);
static void   gimp_color_selector_emit_model_visible_changed (GimpColorSelector *selector,
                                                              GimpColorSelectorModel model);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GimpColorSelector, gimp_color_selector, GTK_TYPE_BOX)

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
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_COLOR);

  selector_signals[CHANNEL_CHANGED] =
    g_signal_new ("channel-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorSelectorClass, channel_changed),
                  NULL, NULL, NULL,
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

  selector_signals[SIMULATION] =
    g_signal_new ("simulation",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorSelectorClass, simulation),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
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
}

static void
gimp_color_selector_init (GimpColorSelector *selector)
{
  GimpColorSelectorPrivate *priv;

  priv = gimp_color_selector_get_instance_private (selector);

  priv->toggles_visible   = TRUE;
  priv->toggles_sensitive = TRUE;
  priv->show_alpha        = TRUE;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (selector),
                                  GTK_ORIENTATION_VERTICAL);

  priv->channel = GIMP_COLOR_SELECTOR_RED;

  priv->color = gegl_color_new ("black");
  priv->model_visible[GIMP_COLOR_SELECTOR_MODEL_RGB] = TRUE;
  priv->model_visible[GIMP_COLOR_SELECTOR_MODEL_LCH] = TRUE;
  priv->model_visible[GIMP_COLOR_SELECTOR_MODEL_HSV] = FALSE;

  priv->simulation         = FALSE;
  priv->simulation_profile = NULL;
  priv->simulation_intent  = GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  priv->simulation_bpc     = FALSE;
}

static void
gimp_color_selector_dispose (GObject *object)
{
  GimpColorSelector        *selector = GIMP_COLOR_SELECTOR (object);
  GimpColorSelectorPrivate *priv;

  priv = gimp_color_selector_get_instance_private (selector);

  gimp_color_selector_set_config (selector, NULL);
  g_clear_object (&priv->color);
  g_clear_object (&priv->simulation_profile);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */

/**
 * gimp_color_selector_new: (skip)
 * @selector_type: The #GType of the selector to create.
 * @color:         The initial color to be edited.
 * @channel:       The selector's initial channel.
 *
 * Creates a new #GimpColorSelector widget of type @selector_type.
 *
 * Note that this is mostly internal API to be used by other widgets.
 *
 * Please use gimp_color_selection_new() for the "GIMP-typical" color
 * selection widget. Also see gimp_color_button_new().
 *
 * Returns: the new #GimpColorSelector widget.
 **/
GtkWidget *
gimp_color_selector_new (GType                     selector_type,
                         GeglColor                *color,
                         GimpColorSelectorChannel  channel)
{
  GimpColorSelector *selector;

  g_return_val_if_fail (g_type_is_a (selector_type, GIMP_TYPE_COLOR_SELECTOR), NULL);
  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);

  selector = g_object_new (selector_type, NULL);

  gimp_color_selector_set_color (selector, color);
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
  GimpColorSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  priv = gimp_color_selector_get_instance_private (selector);

  if (priv->toggles_visible != visible)
    {
      GimpColorSelectorClass *selector_class;

      priv->toggles_visible = visible ? TRUE : FALSE;

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
 * Returns: %TRUE if the #GimpColorSelector's toggles are visible.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_selector_get_toggles_visible (GimpColorSelector *selector)
{
  GimpColorSelectorPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_SELECTOR (selector), FALSE);

  priv = gimp_color_selector_get_instance_private (selector);

  return priv->toggles_visible;
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
  GimpColorSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  priv = gimp_color_selector_get_instance_private (selector);

  if (priv->toggles_sensitive != sensitive)
    {
      GimpColorSelectorClass *selector_class;

      priv->toggles_sensitive = sensitive ? TRUE : FALSE;

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
 * Returns: %TRUE if the #GimpColorSelector's toggles are sensitive.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_selector_get_toggles_sensitive (GimpColorSelector *selector)
{
  GimpColorSelectorPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_SELECTOR (selector), FALSE);

  priv = gimp_color_selector_get_instance_private (selector);

  return priv->toggles_sensitive;
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
  GimpColorSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  priv = gimp_color_selector_get_instance_private (selector);

  if (show_alpha != priv->show_alpha)
    {
      GimpColorSelectorClass *selector_class;

      priv->show_alpha = show_alpha ? TRUE : FALSE;

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
 * Returns: %TRUE if the #GimpColorSelector has alpha controls.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_selector_get_show_alpha (GimpColorSelector *selector)
{
  GimpColorSelectorPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_SELECTOR (selector), FALSE);

  priv = gimp_color_selector_get_instance_private (selector);

  return priv->show_alpha;
}

/**
 * gimp_color_selector_set_color:
 * @selector: A #GimpColorSelector widget.
 * @color:    The new color.
 *
 * Sets the color shown in the @selector widget.
 **/
void
gimp_color_selector_set_color (GimpColorSelector *selector,
                               GeglColor         *color)
{
  GimpColorSelectorClass   *selector_class;
  GimpColorSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (GEGL_IS_COLOR (color));

  priv = gimp_color_selector_get_instance_private (selector);

  g_object_unref (priv->color);
  priv->color = gegl_color_duplicate (color);

  selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

  if (selector_class->set_color)
    selector_class->set_color (selector, priv->color);

  gimp_color_selector_emit_color_changed (selector);
}

/**
 * gimp_color_selector_get_color:
 * @selector: A #GimpColorSelector widget.
 *
 * Retrieves the color shown in the @selector widget.
 *
 * Returns: (transfer full): a copy of the selected color.
 * Since: 2.10
 **/
GeglColor *
gimp_color_selector_get_color (GimpColorSelector *selector)
{
  GimpColorSelectorPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_SELECTOR (selector), NULL);

  priv = gimp_color_selector_get_instance_private (selector);

  return gegl_color_duplicate (priv->color);
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
  GimpColorSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  priv = gimp_color_selector_get_instance_private (selector);

  if (channel != priv->channel)
    {
      GimpColorSelectorClass *selector_class;
      GimpColorSelectorModel  model = -1;

      priv->channel = channel;

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

      gimp_color_selector_emit_channel_changed (selector);

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
 * Returns: The #GimpColorSelectorChannel currently shown by the
 * @selector.
 *
 * Since: 2.10
 **/
GimpColorSelectorChannel
gimp_color_selector_get_channel (GimpColorSelector *selector)
{
  GimpColorSelectorPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_SELECTOR (selector),
                        GIMP_COLOR_SELECTOR_RED);

  priv = gimp_color_selector_get_instance_private (selector);

  return priv->channel;
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

  priv = gimp_color_selector_get_instance_private (selector);

  visible = visible ? TRUE : FALSE;

  if (visible != priv->model_visible[model])
    {
      GimpColorSelectorClass *selector_class;

      priv->model_visible[model] = visible;

      selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_model_visible)
        selector_class->set_model_visible (selector, model, visible);

      gimp_color_selector_emit_model_visible_changed (selector, model);
    }
}

/**
 * gimp_color_selector_get_model_visible:
 * @selector: A #GimpColorSelector widget.
 * @model:    The #GimpColorSelectorModel.
 *
 * Returns: whether @model is visible in @selector.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_selector_get_model_visible (GimpColorSelector      *selector,
                                       GimpColorSelectorModel  model)
{
  GimpColorSelectorPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_SELECTOR (selector), FALSE);

  priv = gimp_color_selector_get_instance_private (selector);

  return priv->model_visible[model];
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

/**
 * gimp_color_selector_set_format
 * @selector: a #GimpColorSelector widget.
 * @format:   a Babl format, with space.
 *
 * Sets the babl format representing the color model and the space this
 * @selector is supposed to display values for. Depending on the type of color
 * selector, it may trigger various UX changes, or none at all.
 *
 * Since: 3.0
 */
void
gimp_color_selector_set_format (GimpColorSelector *selector,
                                const Babl        *format)
{
  GimpColorSelectorClass *selector_class;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

  if (selector_class->set_format)
    selector_class->set_format (selector, format);
}

/**
 * gimp_color_selector_set_simulation
 * @selector: a #GimpColorSelector widget.
 * @profile:  a #GimpColorProfile object.
 * @intent:   a #GimpColorRenderingIntent enum.
 * @bpc:      a gboolean.
 *
 * Sets the simulation options to use with this color selector.
 *
 * Since: 3.0
 */
void
gimp_color_selector_set_simulation (GimpColorSelector        *selector,
                                    GimpColorProfile         *profile,
                                    GimpColorRenderingIntent  intent,
                                    gboolean                  bpc)
{
  GimpColorSelectorClass   *selector_class;
  GimpColorSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (profile == NULL || GIMP_IS_COLOR_PROFILE (profile));

  selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);
  priv           = gimp_color_selector_get_instance_private (selector);

  if ((profile && ! priv->simulation_profile)                                        ||
      (! profile && priv->simulation_profile)                                        ||
      (profile && ! gimp_color_profile_is_equal (profile, priv->simulation_profile)) ||
       intent != priv->simulation_intent                                             ||
       bpc    != priv->simulation_bpc)
    {
      g_set_object (&priv->simulation_profile, profile);
      priv->simulation_intent = intent;
      priv->simulation_bpc    = bpc;

      if (selector_class->set_simulation)
        selector_class->set_simulation (selector, profile, intent, bpc);
    }
}

gboolean
gimp_color_selector_get_simulation (GimpColorSelector         *selector,
                                    GimpColorProfile         **profile,
                                    GimpColorRenderingIntent *intent,
                                    gboolean                 *bpc)
{
  GimpColorSelectorPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_SELECTOR (selector), FALSE);

  priv = gimp_color_selector_get_instance_private (selector);

  if (profile)
    *profile = priv->simulation_profile;
  if (intent)
    *intent = priv->simulation_intent;
  if (bpc)
    *bpc = priv->simulation_bpc;

  return priv->simulation;
}

gboolean
gimp_color_selector_enable_simulation (GimpColorSelector *selector,
                                       gboolean           enabled)
{
  GimpColorSelectorPrivate *priv;

  g_return_val_if_fail (GIMP_IS_COLOR_SELECTOR (selector), FALSE);

  priv = gimp_color_selector_get_instance_private (selector);
  if (priv->simulation != enabled)
    {
      if (! enabled || priv->simulation_profile)
        {
          priv->simulation = enabled;
          g_signal_emit (selector, selector_signals[SIMULATION], 0, enabled);
        }
    }

  return priv->simulation;
}


/* Private functions. */

/**
 * gimp_color_selector_emit_color_changed:
 * @selector: A #GimpColorSelector widget.
 *
 * Emits the "color-changed" signal.
 */
static void
gimp_color_selector_emit_color_changed (GimpColorSelector *selector)
{
  GimpColorSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  priv = gimp_color_selector_get_instance_private (selector);

  g_signal_emit (selector, selector_signals[COLOR_CHANGED], 0, priv->color);
}

/**
 * gimp_color_selector_emit_channel_changed:
 * @selector: A #GimpColorSelector widget.
 *
 * Emits the "channel-changed" signal.
 */
static void
gimp_color_selector_emit_channel_changed (GimpColorSelector *selector)
{
  GimpColorSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  priv = gimp_color_selector_get_instance_private (selector);

  g_signal_emit (selector, selector_signals[CHANNEL_CHANGED], 0,
                 priv->channel);
}

/**
 * gimp_color_selector_emit_model_visible_changed:
 * @selector: A #GimpColorSelector widget.
 * @model:    The #GimpColorSelectorModel where visibility changed.
 *
 * Emits the "model-visible-changed" signal.
 *
 * Since: 2.10
 */
static void
gimp_color_selector_emit_model_visible_changed (GimpColorSelector      *selector,
                                                GimpColorSelectorModel  model)
{
  GimpColorSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  priv = gimp_color_selector_get_instance_private (selector);

  g_signal_emit (selector, selector_signals[MODEL_VISIBLE_CHANGED], 0,
                 model, priv->model_visible[model]);
}
