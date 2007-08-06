/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolordisplay.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimpwidgetstypes.h"

#include "gimpstock.h"

#undef GIMP_DISABLE_DEPRECATED
#include "gimpcolordisplay.h"


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

typedef struct
{
  GimpColorConfig  *config;
  GimpColorManaged *managed;
} GimpColorDisplayPrivate;

#define GIMP_COLOR_DISPLAY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIMP_TYPE_COLOR_DISPLAY, GimpColorDisplayPrivate))

static GObject  * gimp_color_display_constructor (GType                  type,
                                                  guint                  n_params,
                                                  GObjectConstructParam *params);
static void       gimp_color_display_finalize     (GObject      *object);
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

  object_class->constructor  = gimp_color_display_constructor;
  object_class->finalize     = gimp_color_display_finalize;
  object_class->set_property = gimp_color_display_set_property;
  object_class->get_property = gimp_color_display_get_property;

  g_type_class_add_private (object_class, sizeof (GimpColorDisplayPrivate));

  g_object_class_install_property (object_class, PROP_ENABLED,
                                   g_param_spec_boolean ("enabled", NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_COLOR_CONFIG,
                                   g_param_spec_object ("color-config",
                                                        NULL, NULL,
                                                        GIMP_TYPE_COLOR_CONFIG,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_COLOR_MANAGED,
                                   g_param_spec_object ("color-managed",
                                                        NULL, NULL,
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
  klass->stock_id        = GIMP_STOCK_DISPLAY_FILTER;

  klass->clone           = NULL;
  klass->convert         = NULL;
  klass->load_state      = NULL;
  klass->save_state      = NULL;
  klass->configure       = NULL;
  klass->configure_reset = NULL;
  klass->changed         = NULL;
}

static void
gimp_color_display_init (GimpColorDisplay *display)
{
  display->enabled = FALSE;
}

static GObject *
gimp_color_display_constructor (GType                  type,
                                guint                  n_params,
                                GObjectConstructParam *params)
{
  GObject *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  /* emit an initial "changed" signal after all construct properties are set */
  gimp_color_display_changed (GIMP_COLOR_DISPLAY (object));

  return object;
}

static void
gimp_color_display_finalize (GObject *object)
{
  GimpColorDisplayPrivate *private = GIMP_COLOR_DISPLAY_GET_PRIVATE (object);

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

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_display_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpColorDisplay *display = GIMP_COLOR_DISPLAY (object);

  switch (property_id)
    {
    case PROP_ENABLED:
      display->enabled = g_value_get_boolean (value);
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
  GimpColorDisplay *display = GIMP_COLOR_DISPLAY (object);

  switch (property_id)
    {
    case PROP_ENABLED:
      g_value_set_boolean (value, display->enabled);
      break;

    case PROP_COLOR_CONFIG:
      g_value_set_object (value,
                          GIMP_COLOR_DISPLAY_GET_PRIVATE (display)->config);
      break;

    case PROP_COLOR_MANAGED:
      g_value_set_object (value,
                          GIMP_COLOR_DISPLAY_GET_PRIVATE (display)->managed);
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
  GimpColorDisplayPrivate *private = GIMP_COLOR_DISPLAY_GET_PRIVATE (display);

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
  GimpColorDisplayPrivate *private = GIMP_COLOR_DISPLAY_GET_PRIVATE (display);

  g_return_if_fail (private->managed == NULL);

  if (managed)
    {
      private->managed = g_object_ref (managed);

      g_signal_connect_swapped (private->managed, "profile-changed",
                                G_CALLBACK (gimp_color_display_changed),
                                display);
    }
}

/**
 * gimp_color_display_new:
 * @display_type: the GType of the GimpColorDisplay to instantiate.
 *
 * This function is deprecated. Please use g_object_new() directly.
 *
 * Return value: a new %GimpColorDisplay object.
 **/
GimpColorDisplay *
gimp_color_display_new (GType display_type)
{
  g_return_val_if_fail (g_type_is_a (display_type, GIMP_TYPE_COLOR_DISPLAY),
                        NULL);

  return g_object_new (display_type, NULL);
}

GimpColorDisplay *
gimp_color_display_clone (GimpColorDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), NULL);

  /*  implementing the clone method is deprecated
   */
  if (GIMP_COLOR_DISPLAY_GET_CLASS (display)->clone)
    {
      GimpColorDisplay *clone;

      clone = GIMP_COLOR_DISPLAY_GET_CLASS (display)->clone (display);

      if (clone)
        {
          GimpColorDisplayPrivate *private;

          private = GIMP_COLOR_DISPLAY_GET_PRIVATE (display);

          g_object_set (clone,
                        "enabled",       display->enabled,
                        "color-managed", private->managed,
                        NULL);
        }

      return clone;
    }

  return GIMP_COLOR_DISPLAY (gimp_config_duplicate (GIMP_CONFIG (display)));
}

void
gimp_color_display_convert (GimpColorDisplay *display,
                            guchar            *buf,
                            gint               width,
                            gint               height,
                            gint               bpp,
                            gint               bpl)
{
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  if (display->enabled && GIMP_COLOR_DISPLAY_GET_CLASS (display)->convert)
    GIMP_COLOR_DISPLAY_GET_CLASS (display)->convert (display, buf,
                                                     width, height,
                                                     bpp, bpl);
}

void
gimp_color_display_load_state (GimpColorDisplay *display,
                               GimpParasite     *state)
{
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));
  g_return_if_fail (state != NULL);

  /*  implementing the load_state method is deprecated
   */
  if (GIMP_COLOR_DISPLAY_GET_CLASS (display)->load_state)
    {
      GIMP_COLOR_DISPLAY_GET_CLASS (display)->load_state (display, state);
    }
  else
    {
      gimp_config_deserialize_string (GIMP_CONFIG (display),
                                      gimp_parasite_data (state),
                                      gimp_parasite_data_size (state),
                                      NULL, NULL);
    }
}

GimpParasite *
gimp_color_display_save_state (GimpColorDisplay *display)
{
  GimpParasite *parasite;
  gchar        *str;

  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), NULL);

  /*  implementing the save_state method is deprecated
   */
  if (GIMP_COLOR_DISPLAY_GET_CLASS (display)->save_state)
    {
      return GIMP_COLOR_DISPLAY_GET_CLASS (display)->save_state (display);
    }

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

  /*  implementing the configure_reset method is deprecated
   */
  if (GIMP_COLOR_DISPLAY_GET_CLASS (display)->configure_reset)
    {
      GIMP_COLOR_DISPLAY_GET_CLASS (display)->configure_reset (display);
    }
  else
    {
      gimp_config_reset (GIMP_CONFIG (display));
    }
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
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  if (enabled != display->enabled)
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

  return display->enabled;
}

/**
 * gimp_color_display_get_config:
 * @display:
 *
 * Return value: a pointer to the #GimpColorConfig object or %NULL.
 *
 * Since: GIMP 2.4
 **/
GimpColorConfig *
gimp_color_display_get_config (GimpColorDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), NULL);

  return GIMP_COLOR_DISPLAY_GET_PRIVATE (display)->config;
}

/**
 * gimp_color_display_get_managed:
 * @display:
 *
 * Return value: a pointer to the #GimpColorManaged object or %NULL.
 *
 * Since: GIMP 2.4
 **/
GimpColorManaged *
gimp_color_display_get_managed (GimpColorDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), NULL);

  return GIMP_COLOR_DISPLAY_GET_PRIVATE (display)->managed;
}
