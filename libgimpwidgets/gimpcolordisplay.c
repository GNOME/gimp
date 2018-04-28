/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolordisplay.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimpwidgetstypes.h"

#include "gimpcolordisplay.h"
#include "gimpicons.h"


/**
 * SECTION: gimpcolordisplay
 * @title: GimpColorDisplay
 * @short_description: Pluggable GIMP display color correction modules.
 * @see_also: #GModule, #GTypeModule, #GimpModule
 *
 * Functions and definitions for creating pluggable GIMP
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


typedef struct _GimpColorDisplayPrivate GimpColorDisplayPrivate;

struct _GimpColorDisplayPrivate
{
  gboolean          enabled;
  GimpColorConfig  *config;
  GimpColorManaged *managed;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                                       GIMP_TYPE_COLOR_DISPLAY, \
                                                       GimpColorDisplayPrivate))



static void       gimp_color_display_constructed (GObject       *object);
static void       gimp_color_display_dispose      (GObject      *object);
static void       gimp_color_display_set_property (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                  GParamSpec    *pspec);
static void       gimp_color_display_get_property (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);

static void  gimp_color_display_set_color_config  (GimpColorDisplay *display,
                                                   GimpColorConfig  *config);
static void  gimp_color_display_set_color_managed (GimpColorDisplay *display,
                                                   GimpColorManaged *managed);


G_DEFINE_TYPE_WITH_CODE (GimpColorDisplay, gimp_color_display, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

#define parent_class gimp_color_display_parent_class

static guint display_signals[LAST_SIGNAL] = { 0 };


static void
gimp_color_display_class_init (GimpColorDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_color_display_constructed;
  object_class->dispose      = gimp_color_display_dispose;
  object_class->set_property = gimp_color_display_set_property;
  object_class->get_property = gimp_color_display_get_property;

  g_object_class_install_property (object_class, PROP_ENABLED,
                                   g_param_spec_boolean ("enabled",
                                                         "Enabled",
                                                         "Whether this display filter is enabled",
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_COLOR_CONFIG,
                                   g_param_spec_object ("color-config",
                                                        "Color Config",
                                                        "The color config used for this filter",
                                                        GIMP_TYPE_COLOR_CONFIG,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_COLOR_MANAGED,
                                   g_param_spec_object ("color-managed",
                                                        "Color Managed",
                                                        "The color managed pixel source that is filtered",
                                                        GIMP_TYPE_COLOR_MANAGED,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  display_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorDisplayClass, changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  klass->name            = "Unnamed";
  klass->help_id         = NULL;
  klass->icon_name       = GIMP_ICON_DISPLAY_FILTER;

  klass->convert_buffer  = NULL;
  klass->configure       = NULL;

  klass->changed         = NULL;

  g_type_class_add_private (object_class, sizeof (GimpColorDisplayPrivate));
}

static void
gimp_color_display_init (GimpColorDisplay *display)
{
  GimpColorDisplayPrivate *private = GET_PRIVATE (display);

  private->enabled = FALSE;
}

static void
gimp_color_display_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* emit an initial "changed" signal after all construct properties are set */
  gimp_color_display_changed (GIMP_COLOR_DISPLAY (object));
}

static void
gimp_color_display_dispose (GObject *object)
{
  GimpColorDisplayPrivate *private = GET_PRIVATE (object);

  if (private->config)
    {
      g_signal_handlers_disconnect_by_func (private->config,
                                            gimp_color_display_changed,
                                            object);
      g_object_unref (private->config);
      private->config = NULL;
    }

  if (private->managed)
    {
      g_signal_handlers_disconnect_by_func (private->managed,
                                            gimp_color_display_changed,
                                            object);
      g_object_unref (private->managed);
      private->managed = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_color_display_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpColorDisplay        *display = GIMP_COLOR_DISPLAY (object);
  GimpColorDisplayPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ENABLED:
      private->enabled = g_value_get_boolean (value);
      break;

    case PROP_COLOR_CONFIG:
      gimp_color_display_set_color_config (display,
                                           g_value_get_object (value));
      break;

    case PROP_COLOR_MANAGED:
      gimp_color_display_set_color_managed (display,
                                            g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_display_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpColorDisplayPrivate *private = GET_PRIVATE (object);

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
gimp_color_display_set_color_config (GimpColorDisplay *display,
                                     GimpColorConfig  *config)
{
  GimpColorDisplayPrivate *private = GET_PRIVATE (display);

  g_return_if_fail (private->config == NULL);

  if (config)
    {
      private->config = g_object_ref (config);

      g_signal_connect_swapped (private->config, "notify",
                                G_CALLBACK (gimp_color_display_changed),
                                display);
    }
}

static void
gimp_color_display_set_color_managed (GimpColorDisplay *display,
                                      GimpColorManaged *managed)
{
  GimpColorDisplayPrivate *private = GET_PRIVATE (display);

  g_return_if_fail (private->managed == NULL);

  if (managed)
    {
      private->managed = g_object_ref (managed);

      g_signal_connect_swapped (private->managed, "profile-changed",
                                G_CALLBACK (gimp_color_display_changed),
                                display);
    }
}

GimpColorDisplay *
gimp_color_display_clone (GimpColorDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), NULL);

  return GIMP_COLOR_DISPLAY (gimp_config_duplicate (GIMP_CONFIG (display)));
}

/**
 * gimp_color_display_convert_buffer:
 * @display: a #GimpColorDisplay
 * @buffer:  a #GeglBuffer
 * @area:    area in @buffer to convert
 *
 * Converts all pixels in @area of @buffer.
 *
 * Since: 2.10
 **/
void
gimp_color_display_convert_buffer (GimpColorDisplay *display,
                                   GeglBuffer       *buffer,
                                   GeglRectangle    *area)
{
  GimpColorDisplayPrivate *private;

  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  private = GET_PRIVATE (display);

  if (private->enabled &&
      GIMP_COLOR_DISPLAY_GET_CLASS (display)->convert_buffer)
    {
      GIMP_COLOR_DISPLAY_GET_CLASS (display)->convert_buffer (display, buffer,
                                                              area);
    }
}

void
gimp_color_display_load_state (GimpColorDisplay *display,
                               GimpParasite     *state)
{
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));
  g_return_if_fail (state != NULL);

  gimp_config_deserialize_string (GIMP_CONFIG (display),
                                  gimp_parasite_data (state),
                                  gimp_parasite_data_size (state),
                                  NULL, NULL);
}

GimpParasite *
gimp_color_display_save_state (GimpColorDisplay *display)
{
  GimpParasite *parasite;
  gchar        *str;

  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), NULL);

  str = gimp_config_serialize_to_string (GIMP_CONFIG (display), NULL);

  parasite = gimp_parasite_new ("Display/Proof",
                                GIMP_PARASITE_PERSISTENT,
                                strlen (str) + 1, str);
  g_free (str);

  return parasite;
}

GtkWidget *
gimp_color_display_configure (GimpColorDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), NULL);

  if (GIMP_COLOR_DISPLAY_GET_CLASS (display)->configure)
    return GIMP_COLOR_DISPLAY_GET_CLASS (display)->configure (display);

  return NULL;
}

void
gimp_color_display_configure_reset (GimpColorDisplay *display)
{
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  gimp_config_reset (GIMP_CONFIG (display));
}

void
gimp_color_display_changed (GimpColorDisplay *display)
{
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  g_signal_emit (display, display_signals[CHANGED], 0);
}

void
gimp_color_display_set_enabled (GimpColorDisplay *display,
                                gboolean          enabled)
{
  GimpColorDisplayPrivate *private;

  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  private = GET_PRIVATE (display);

  if (enabled != private->enabled)
    {
      g_object_set (display,
                    "enabled", enabled,
                    NULL);
    }
}

gboolean
gimp_color_display_get_enabled (GimpColorDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), FALSE);

  return GET_PRIVATE (display)->enabled;
}

/**
 * gimp_color_display_get_config:
 * @display:
 *
 * Return value: a pointer to the #GimpColorConfig object or %NULL.
 *
 * Since: 2.4
 **/
GimpColorConfig *
gimp_color_display_get_config (GimpColorDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), NULL);

  return GET_PRIVATE (display)->config;
}

/**
 * gimp_color_display_get_managed:
 * @display:
 *
 * Return value: a pointer to the #GimpColorManaged object or %NULL.
 *
 * Since: 2.4
 **/
GimpColorManaged *
gimp_color_display_get_managed (GimpColorDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), NULL);

  return GET_PRIVATE (display)->managed;
}
