/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpdatafactory.h"
#include "core/gimpmarshal.h"

#include "gimpdeviceinfo.h"


#define GIMP_DEVICE_INFO_DATA_KEY "gimp-device-info"


enum
{
  CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_MODE,
  PROP_AXES,
  PROP_KEYS
};


/*  local function prototypes  */

static GObject * gimp_device_info_constructor  (GType                  type,
                                                guint                  n_params,
                                                GObjectConstructParam *params);
static void      gimp_device_info_finalize     (GObject               *object);
static void      gimp_device_info_set_property (GObject               *object,
                                                guint                  property_id,
                                                const GValue          *value,
                                                GParamSpec            *pspec);
static void      gimp_device_info_get_property (GObject               *object,
                                                guint                  property_id,
                                                GValue                *value,
                                                GParamSpec            *pspec);


G_DEFINE_TYPE (GimpDeviceInfo, gimp_device_info, GIMP_TYPE_CONTEXT)

#define parent_class gimp_device_info_parent_class

static guint device_info_signals[LAST_SIGNAL] = { 0 };


static void
gimp_device_info_class_init (GimpDeviceInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *param_spec;

  device_info_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDeviceInfoClass, changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->constructor  = gimp_device_info_constructor;
  object_class->finalize     = gimp_device_info_finalize;
  object_class->set_property = gimp_device_info_set_property;
  object_class->get_property = gimp_device_info_get_property;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_MODE, "mode", NULL,
                                 GDK_TYPE_INPUT_MODE,
                                 GDK_MODE_DISABLED,
                                 GIMP_PARAM_STATIC_STRINGS);

  param_spec = g_param_spec_enum ("axis",
                                  NULL, NULL,
                                  GDK_TYPE_AXIS_USE,
                                  GDK_AXIS_IGNORE,
                                  GIMP_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_AXES,
                                   g_param_spec_value_array ("axes",
                                                             NULL, NULL,
                                                             param_spec,
                                                             GIMP_PARAM_STATIC_STRINGS |
                                                             GIMP_CONFIG_PARAM_FLAGS));

  param_spec = g_param_spec_string ("key",
                                    NULL, NULL,
                                    NULL,
                                    GIMP_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_KEYS,
                                   g_param_spec_value_array ("keys",
                                                             NULL, NULL,
                                                             param_spec,
                                                             GIMP_PARAM_STATIC_STRINGS |
                                                             GIMP_CONFIG_PARAM_FLAGS));
}

static void
gimp_device_info_init (GimpDeviceInfo *device_info)
{
  device_info->device   = NULL;
  device_info->display  = NULL;
  device_info->mode     = GDK_MODE_DISABLED;
  device_info->num_axes = 0;
  device_info->axes     = NULL;
  device_info->num_keys = 0;
  device_info->keys     = NULL;
}

static GObject *
gimp_device_info_constructor (GType                  type,
                              guint                  n_params,
                              GObjectConstructParam *params)
{
  GObject *object;
  Gimp    *gimp;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimp = GIMP_CONTEXT (object)->gimp;

  g_assert (GIMP_IS_GIMP (gimp));

  gimp_context_define_properties (GIMP_CONTEXT (object),
                                  GIMP_DEVICE_INFO_CONTEXT_MASK,
                                  FALSE);
  gimp_context_copy_properties (gimp_get_user_context (gimp),
                                GIMP_CONTEXT (object),
                                GIMP_DEVICE_INFO_CONTEXT_MASK);

  gimp_context_set_serialize_properties (GIMP_CONTEXT (object),
                                         GIMP_DEVICE_INFO_CONTEXT_MASK);

  /*  FIXME: this is ugly and needs to be done via "notify" once
   *  the contexts' properties are dynamic.
   */
  g_signal_connect (object, "foreground-changed",
                    G_CALLBACK (gimp_device_info_changed),
                    NULL);
  g_signal_connect (object, "background-changed",
                    G_CALLBACK (gimp_device_info_changed),
                    NULL);
  g_signal_connect (object, "tool-changed",
                    G_CALLBACK (gimp_device_info_changed),
                    NULL);
  g_signal_connect (object, "brush-changed",
                    G_CALLBACK (gimp_device_info_changed),
                    NULL);
  g_signal_connect (object, "pattern-changed",
                    G_CALLBACK (gimp_device_info_changed),
                    NULL);
  g_signal_connect (object, "gradient-changed",
                    G_CALLBACK (gimp_device_info_changed),
                    NULL);

  return object;
}

static void
gimp_device_info_finalize (GObject *object)
{
  GimpDeviceInfo *device_info = GIMP_DEVICE_INFO (object);

  if (device_info->axes)
    g_free (device_info->axes);

  if (device_info->keys)
    g_free (device_info->keys);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_device_info_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpDeviceInfo *device_info = GIMP_DEVICE_INFO (object);
  GdkDevice      *device      = device_info->device;

  switch (property_id)
    {
    case PROP_MODE:
      if (device)
        gdk_device_set_mode (device, g_value_get_enum (value));
      else
        device_info->mode = g_value_get_enum (value);
      break;

    case PROP_AXES:
      {
        GValueArray *array = g_value_get_boxed (value);

        if (array)
          {
            gint i;
            gint n_device_values;

            if (device)
              {
                n_device_values = MIN (array->n_values, device->num_axes);
              }
            else
              {
                n_device_values = array->n_values;

                device_info->num_axes = n_device_values;
                device_info->axes     = g_new0 (GdkAxisUse, n_device_values);
              }

            for (i = 0; i < n_device_values; i++)
              {
                GdkAxisUse axis_use;

                axis_use = g_value_get_enum (g_value_array_get_nth (array, i));

                if (device)
                  gdk_device_set_axis_use (device, i, axis_use);
                else
                  device_info->axes[i] = axis_use;
              }
          }
      }
      break;

    case PROP_KEYS:
      {
        GValueArray *array = g_value_get_boxed (value);

        if (array)
          {
            gint i;
            gint n_device_values;

            if (device)
              {
                n_device_values = MIN (array->n_values, device->num_keys);
              }
            else
              {
                n_device_values = array->n_values;

                device_info->num_keys = n_device_values;
                device_info->keys     = g_new0 (GdkDeviceKey, n_device_values);
              }

            for (i = 0; i < n_device_values; i++)
              {
                const gchar     *accel;
                guint            keyval;
                GdkModifierType  modifiers;

                accel = g_value_get_string (g_value_array_get_nth (array, i));

                gtk_accelerator_parse (accel, &keyval, &modifiers);

                if (device)
                  {
                    gdk_device_set_key (device, i, keyval, modifiers);
                  }
                else
                  {
                    device_info->keys[i].keyval    = keyval;
                    device_info->keys[i].modifiers = modifiers;
                  }
              }
          }
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_device_info_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpDeviceInfo *device_info = GIMP_DEVICE_INFO (object);
  GdkDevice      *device      = device_info->device;

  switch (property_id)
    {
    case PROP_MODE:
      if (device)
        g_value_set_enum (value, device->mode);
      else
        g_value_set_enum (value, device_info->mode);
      break;

    case PROP_AXES:
      {
        GValueArray *array;
        GValue       enum_value = { 0, };
        gint         i;

        array = g_value_array_new (6);
        g_value_init (&enum_value, GDK_TYPE_AXIS_USE);

        for (i = 0; i < (device ? device->num_axes : device_info->num_axes); i++)
          {
            g_value_set_enum (&enum_value,
                              device ?
                              device->axes[i].use : device_info->axes[i]);
            g_value_array_append (array, &enum_value);
          }

        g_value_unset (&enum_value);

        g_value_take_boxed (value, array);
      }
      break;

    case PROP_KEYS:
      {
        GValueArray *array;
        GValue       string_value = { 0, };
        gint         i;

        array = g_value_array_new (32);
        g_value_init (&string_value, G_TYPE_STRING);

        for (i = 0; i < (device ? device->num_keys : device_info->num_keys); i++)
          {
            GdkModifierType modifiers = (device ? device->keys[i].modifiers :
                                         device_info->keys[i].modifiers);
            guint           keyval    = (device ? device->keys[i].keyval :
                                         device_info->keys[i].keyval);

            if (keyval)
              {
                gchar *accel;
                gchar *escaped;

                accel = gtk_accelerator_name (keyval, modifiers);
                escaped = g_strescape (accel, NULL);
                g_free (accel);

                g_value_set_string (&string_value, escaped);
                g_free (escaped);
              }
            else
              {
                g_value_set_string (&string_value, "");
              }

            g_value_array_append (array, &string_value);
          }

        g_value_unset (&string_value);

        g_value_take_boxed (value, array);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GimpDeviceInfo *
gimp_device_info_new (Gimp        *gimp,
                      const gchar *name)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (GIMP_TYPE_DEVICE_INFO,
                       "name", name,
                       "gimp", gimp,
                       NULL);
}

GimpDeviceInfo *
gimp_device_info_set_from_device (GimpDeviceInfo *device_info,
                                  GdkDevice      *device,
                                  GdkDisplay     *display)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (device_info), NULL);
  g_return_val_if_fail (GDK_IS_DEVICE (device), NULL);
  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);

  g_object_set_data (G_OBJECT (device), GIMP_DEVICE_INFO_DATA_KEY, device_info);

  device_info->device     = device;
  device_info->display    = display;

  device_info->mode       = device->mode;

  device_info->num_axes   = device->num_axes;
  device_info->axes       = NULL;

  device_info->num_keys   = device->num_keys;
  device_info->keys       = NULL;

  return device_info;
}

void
gimp_device_info_changed (GimpDeviceInfo *device_info)
{
  g_return_if_fail (GIMP_IS_DEVICE_INFO (device_info));

  g_signal_emit (device_info, device_info_signals[CHANGED], 0);
}

GimpDeviceInfo *
gimp_device_info_get_by_device (GdkDevice *device)
{
  g_return_val_if_fail (GDK_IS_DEVICE (device), NULL);

  return g_object_get_data (G_OBJECT (device), GIMP_DEVICE_INFO_DATA_KEY);
}

void
gimp_device_info_changed_by_device (GdkDevice *device)
{
  GimpDeviceInfo *device_info;

  g_return_if_fail (GDK_IS_DEVICE (device));

  device_info = gimp_device_info_get_by_device (device);

  if (device_info)
    gimp_device_info_changed (device_info);
}
