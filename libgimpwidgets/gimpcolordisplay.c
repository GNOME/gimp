/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolordisplay.c
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "ligmawidgetstypes.h"

#include "ligmacolordisplay.h"
#include "ligmaicons.h"


/**
 * SECTION: ligmacolordisplay
 * @title: LigmaColorDisplay
 * @short_description: Pluggable LIGMA display color correction modules.
 * @see_also: #GModule, #GTypeModule, #LigmaModule
 *
 * Functions and definitions for creating pluggable LIGMA
 * display color correction modules.
 **/


enum
{
  PROP_0,
  PROP_ENABLED,
  PROP_COLOR_CONFIG,
  PROP_COLOR_MANAGED
};

enum
{
  CHANGED,
  LAST_SIGNAL
};


struct _LigmaColorDisplayPrivate
{
  gboolean          enabled;
  LigmaColorConfig  *config;
  LigmaColorManaged *managed;
};

#define GET_PRIVATE(obj) (((LigmaColorDisplay *) (obj))->priv)



static void       ligma_color_display_constructed (GObject       *object);
static void       ligma_color_display_dispose      (GObject      *object);
static void       ligma_color_display_set_property (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                  GParamSpec    *pspec);
static void       ligma_color_display_get_property (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);

static void  ligma_color_display_set_color_config  (LigmaColorDisplay *display,
                                                   LigmaColorConfig  *config);
static void  ligma_color_display_set_color_managed (LigmaColorDisplay *display,
                                                   LigmaColorManaged *managed);


G_DEFINE_TYPE_WITH_CODE (LigmaColorDisplay, ligma_color_display, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (LigmaColorDisplay)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG, NULL))

#define parent_class ligma_color_display_parent_class

static guint display_signals[LAST_SIGNAL] = { 0 };


static void
ligma_color_display_class_init (LigmaColorDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_color_display_constructed;
  object_class->dispose      = ligma_color_display_dispose;
  object_class->set_property = ligma_color_display_set_property;
  object_class->get_property = ligma_color_display_get_property;

  g_object_class_install_property (object_class, PROP_ENABLED,
                                   g_param_spec_boolean ("enabled",
                                                         "Enabled",
                                                         "Whether this display filter is enabled",
                                                         TRUE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_COLOR_CONFIG,
                                   g_param_spec_object ("color-config",
                                                        "Color Config",
                                                        "The color config used for this filter",
                                                        LIGMA_TYPE_COLOR_CONFIG,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_COLOR_MANAGED,
                                   g_param_spec_object ("color-managed",
                                                        "Color Managed",
                                                        "The color managed pixel source that is filtered",
                                                        LIGMA_TYPE_COLOR_MANAGED,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  display_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColorDisplayClass, changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  klass->name            = "Unnamed";
  klass->help_id         = NULL;
  klass->icon_name       = LIGMA_ICON_DISPLAY_FILTER;

  klass->convert_buffer  = NULL;
  klass->configure       = NULL;

  klass->changed         = NULL;
}

static void
ligma_color_display_init (LigmaColorDisplay *display)
{
  display->priv = ligma_color_display_get_instance_private (display);
}

static void
ligma_color_display_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* emit an initial "changed" signal after all construct properties are set */
  ligma_color_display_changed (LIGMA_COLOR_DISPLAY (object));
}

static void
ligma_color_display_dispose (GObject *object)
{
  LigmaColorDisplayPrivate *private = GET_PRIVATE (object);

  if (private->config)
    {
      g_signal_handlers_disconnect_by_func (private->config,
                                            ligma_color_display_changed,
                                            object);
      g_object_unref (private->config);
      private->config = NULL;
    }

  if (private->managed)
    {
      g_signal_handlers_disconnect_by_func (private->managed,
                                            ligma_color_display_changed,
                                            object);
      g_object_unref (private->managed);
      private->managed = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_color_display_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaColorDisplay        *display = LIGMA_COLOR_DISPLAY (object);
  LigmaColorDisplayPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ENABLED:
      private->enabled = g_value_get_boolean (value);
      break;

    case PROP_COLOR_CONFIG:
      ligma_color_display_set_color_config (display,
                                           g_value_get_object (value));
      break;

    case PROP_COLOR_MANAGED:
      ligma_color_display_set_color_managed (display,
                                            g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_display_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LigmaColorDisplayPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ENABLED:
      g_value_set_boolean (value, private->enabled);
      break;

    case PROP_COLOR_CONFIG:
      g_value_set_object (value, private->config);
      break;

    case PROP_COLOR_MANAGED:
      g_value_set_object (value, private->managed);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_display_set_color_config (LigmaColorDisplay *display,
                                     LigmaColorConfig  *config)
{
  LigmaColorDisplayPrivate *private = GET_PRIVATE (display);

  g_return_if_fail (private->config == NULL);

  if (config)
    {
      private->config = g_object_ref (config);

      g_signal_connect_swapped (private->config, "notify",
                                G_CALLBACK (ligma_color_display_changed),
                                display);
    }
}

static void
ligma_color_display_set_color_managed (LigmaColorDisplay *display,
                                      LigmaColorManaged *managed)
{
  LigmaColorDisplayPrivate *private = GET_PRIVATE (display);

  g_return_if_fail (private->managed == NULL);

  if (managed)
    {
      private->managed = g_object_ref (managed);

      g_signal_connect_swapped (private->managed, "profile-changed",
                                G_CALLBACK (ligma_color_display_changed),
                                display);
    }
}

/**
 * ligma_color_display_clone:
 * @display: a #LigmaColorDisplay
 *
 * Creates a copy of @display.
 *
 * Returns: (transfer full): a duplicate of @display.
 *
 * Since: 2.0
 **/
LigmaColorDisplay *
ligma_color_display_clone (LigmaColorDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_DISPLAY (display), NULL);

  return LIGMA_COLOR_DISPLAY (ligma_config_duplicate (LIGMA_CONFIG (display)));
}

/**
 * ligma_color_display_convert_buffer:
 * @display: a #LigmaColorDisplay
 * @buffer:  a #GeglBuffer
 * @area:    area in @buffer to convert
 *
 * Converts all pixels in @area of @buffer.
 *
 * Since: 2.10
 **/
void
ligma_color_display_convert_buffer (LigmaColorDisplay *display,
                                   GeglBuffer       *buffer,
                                   GeglRectangle    *area)
{
  LigmaColorDisplayPrivate *private;

  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY (display));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  private = GET_PRIVATE (display);

  if (private->enabled &&
      LIGMA_COLOR_DISPLAY_GET_CLASS (display)->convert_buffer)
    {
      LIGMA_COLOR_DISPLAY_GET_CLASS (display)->convert_buffer (display, buffer,
                                                              area);
    }
}

/**
 * ligma_color_display_load_state:
 * @display: a #LigmaColorDisplay
 * @state:   a #LigmaParasite
 *
 * Configures @display from the contents of the parasite @state.
 * @state must be a properly serialized configuration for a
 * #LigmaColorDisplay, such as saved by ligma_color_display_save_state().
 *
 * Since: 2.0
 **/
void
ligma_color_display_load_state (LigmaColorDisplay *display,
                               LigmaParasite     *state)
{
  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY (display));
  g_return_if_fail (state != NULL);

  ligma_config_deserialize_parasite (LIGMA_CONFIG (display),
                                    state,
                                    NULL, NULL);
}

/**
 * ligma_color_display_save_state:
 * @display: a #LigmaColorDisplay
 *
 * Saves the configuration state of @display as a new parasite.
 *
 * Returns: (transfer full): a #LigmaParasite
 *
 * Since: 2.0
 **/
LigmaParasite *
ligma_color_display_save_state (LigmaColorDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_DISPLAY (display), NULL);

  return ligma_config_serialize_to_parasite (LIGMA_CONFIG (display),
                                            "Display/Proof",
                                            LIGMA_PARASITE_PERSISTENT,
                                            NULL);
}

/**
 * ligma_color_display_configure:
 * @display: a #LigmaColorDisplay
 *
 * Creates a configuration widget for @display which can be added to a
 * container widget.
 *
 * Returns: (transfer full): a new configuration widget for @display, or
 *          %NULL if no specific widget exists.
 *
 * Since: 2.0
 **/
GtkWidget *
ligma_color_display_configure (LigmaColorDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_DISPLAY (display), NULL);

  if (LIGMA_COLOR_DISPLAY_GET_CLASS (display)->configure)
    return LIGMA_COLOR_DISPLAY_GET_CLASS (display)->configure (display);

  return NULL;
}

void
ligma_color_display_configure_reset (LigmaColorDisplay *display)
{
  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY (display));

  ligma_config_reset (LIGMA_CONFIG (display));
}

void
ligma_color_display_changed (LigmaColorDisplay *display)
{
  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY (display));

  g_signal_emit (display, display_signals[CHANGED], 0);
}

void
ligma_color_display_set_enabled (LigmaColorDisplay *display,
                                gboolean          enabled)
{
  LigmaColorDisplayPrivate *private;

  g_return_if_fail (LIGMA_IS_COLOR_DISPLAY (display));

  private = GET_PRIVATE (display);

  if (enabled != private->enabled)
    {
      g_object_set (display,
                    "enabled", enabled,
                    NULL);
    }
}

gboolean
ligma_color_display_get_enabled (LigmaColorDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_DISPLAY (display), FALSE);

  return GET_PRIVATE (display)->enabled;
}

/**
 * ligma_color_display_get_config:
 * @display:
 *
 * Returns: (transfer none): a pointer to the #LigmaColorConfig
 *               object or %NULL.
 *
 * Since: 2.4
 **/
LigmaColorConfig *
ligma_color_display_get_config (LigmaColorDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_DISPLAY (display), NULL);

  return GET_PRIVATE (display)->config;
}

/**
 * ligma_color_display_get_managed:
 * @display:
 *
 * Returns: (transfer none): a pointer to the #LigmaColorManaged
 *               object or %NULL.
 *
 * Since: 2.4
 **/
LigmaColorManaged *
ligma_color_display_get_managed (LigmaColorDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_DISPLAY (display), NULL);

  return GET_PRIVATE (display)->managed;
}
