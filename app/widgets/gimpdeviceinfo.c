/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#undef GSEAL_ENABLE

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"
#include "core/gimpcurve.h"
#include "core/gimpcurve-map.h"
#include "core/gimpdatafactory.h"
#include "core/gimpmarshal.h"
#include "core/gimpparamspecs.h"
#include "core/gimptoolinfo.h"

#include "gimpdeviceinfo.h"

#include "gimp-intl.h"


#define GIMP_DEVICE_INFO_DATA_KEY "gimp-device-info"


enum
{
  PROP_0,
  PROP_DEVICE,
  PROP_DISPLAY,
  PROP_MODE,
  PROP_AXES,
  PROP_KEYS,
  PROP_PRESSURE_CURVE
};


/*  local function prototypes  */

static void   gimp_device_info_constructed  (GObject        *object);
static void   gimp_device_info_finalize     (GObject        *object);
static void   gimp_device_info_set_property (GObject        *object,
                                             guint           property_id,
                                             const GValue   *value,
                                             GParamSpec     *pspec);
static void   gimp_device_info_get_property (GObject        *object,
                                             guint           property_id,
                                             GValue         *value,
                                             GParamSpec     *pspec);

static void   gimp_device_info_guess_icon   (GimpDeviceInfo *info);


G_DEFINE_TYPE (GimpDeviceInfo, gimp_device_info, GIMP_TYPE_TOOL_PRESET)

#define parent_class gimp_device_info_parent_class


static void
gimp_device_info_class_init (GimpDeviceInfoClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);
  GParamSpec        *param_spec;

  object_class->constructed         = gimp_device_info_constructed;
  object_class->finalize            = gimp_device_info_finalize;
  object_class->set_property        = gimp_device_info_set_property;
  object_class->get_property        = gimp_device_info_get_property;

  viewable_class->default_icon_name = GIMP_ICON_INPUT_DEVICE;

  g_object_class_install_property (object_class, PROP_DEVICE,
                                   g_param_spec_object ("device",
                                                        NULL, NULL,
                                                        GDK_TYPE_DEVICE,
                                                        GIMP_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DISPLAY,
                                   g_param_spec_object ("display",
                                                        NULL, NULL,
                                                        GDK_TYPE_DISPLAY,
                                                        GIMP_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_MODE,
                         "mode",
                         _("Mode"),
                         NULL,
                         GDK_TYPE_INPUT_MODE,
                         GDK_MODE_DISABLED,
                         GIMP_PARAM_STATIC_STRINGS);

  param_spec = g_param_spec_enum ("axis",
                                  NULL, NULL,
                                  GDK_TYPE_AXIS_USE,
                                  GDK_AXIS_IGNORE,
                                  GIMP_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_AXES,
                                   gimp_param_spec_value_array ("axes",
                                                                NULL, NULL,
                                                                param_spec,
                                                                GIMP_PARAM_STATIC_STRINGS |
                                                                GIMP_CONFIG_PARAM_FLAGS));

  param_spec = g_param_spec_string ("key",
                                    NULL, NULL,
                                    NULL,
                                    GIMP_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_KEYS,
                                   gimp_param_spec_value_array ("keys",
                                                                NULL, NULL,
                                                                param_spec,
                                                                GIMP_PARAM_STATIC_STRINGS |
                                                                GIMP_CONFIG_PARAM_FLAGS));

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_PRESSURE_CURVE,
                           "pressure-curve",
                           _("Pressure curve"),
                           NULL,
                           GIMP_TYPE_CURVE,
                           GIMP_CONFIG_PARAM_AGGREGATE);
}

static void
gimp_device_info_init (GimpDeviceInfo *info)
{
  gimp_data_make_internal (GIMP_DATA (info), NULL);

  info->mode           = GDK_MODE_DISABLED;
  info->pressure_curve = GIMP_CURVE (gimp_curve_new ("pressure curve"));

  g_signal_connect (info, "notify::name",
                    G_CALLBACK (gimp_device_info_guess_icon),
                    NULL);
}

static void
gimp_device_info_constructed (GObject *object)
{
  GimpDeviceInfo *info = GIMP_DEVICE_INFO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert ((info->device == NULL         && info->display == NULL) ||
               (GDK_IS_DEVICE (info->device) && GDK_IS_DISPLAY (info->display)));

  if (info->device)
    {
      gint i;

      g_object_set_data (G_OBJECT (info->device), GIMP_DEVICE_INFO_DATA_KEY,
                         info);

      gimp_object_set_name (GIMP_OBJECT (info), info->device->name);

      info->mode = gdk_device_get_mode (info->device);

      info->n_axes = gdk_device_get_n_axes (info->device);
      info->axes = g_new0 (GdkAxisUse, info->n_axes);
      for (i = 0; i < info->n_axes; i++)
        info->axes[i] = gdk_device_get_axis_use (info->device, i);

      info->n_keys = gdk_device_get_n_keys (info->device);
      info->keys = g_new0 (GdkDeviceKey, info->n_keys);
      for (i = 0; i < info->n_keys; i++)
        gdk_device_get_key (info->device, i,
                            &info->keys[i].keyval,
                            &info->keys[i].modifiers);
    }
}

static void
gimp_device_info_finalize (GObject *object)
{
  GimpDeviceInfo *info = GIMP_DEVICE_INFO (object);

  g_clear_pointer (&info->axes, g_free);
  g_clear_pointer (&info->keys, g_free);

  g_clear_object (&info->pressure_curve);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_device_info_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpDeviceInfo *info       = GIMP_DEVICE_INFO (object);
  GdkDevice      *device     = info->device;
  GimpCurve      *src_curve  = NULL;
  GimpCurve      *dest_curve = NULL;

  switch (property_id)
    {
    case PROP_DEVICE:
      info->device = g_value_get_object (value);
      break;

    case PROP_DISPLAY:
      info->display = g_value_get_object (value);
      break;

    case PROP_MODE:
      gimp_device_info_set_mode (info, g_value_get_enum (value));
      break;

    case PROP_AXES:
      {
        GimpValueArray *array = g_value_get_boxed (value);

        if (array)
          {
            gint i;
            gint n_device_values = gimp_value_array_length (array);

            if (device)
              n_device_values = MIN (n_device_values,
                                     gdk_device_get_n_axes (device));

            info->n_axes = n_device_values;
            info->axes   = g_renew (GdkAxisUse, info->axes, info->n_axes);
            memset (info->axes, 0, info->n_axes * sizeof (GdkAxisUse));

            for (i = 0; i < n_device_values; i++)
              {
                GdkAxisUse axis_use;

                axis_use = g_value_get_enum (gimp_value_array_index (array, i));

                gimp_device_info_set_axis_use (info, i, axis_use);
              }
          }
      }
      break;

    case PROP_KEYS:
      {
        GimpValueArray *array = g_value_get_boxed (value);

        if (array)
          {
            gint i;
            gint n_device_values = gimp_value_array_length (array);

            if (device)
              n_device_values = MIN (n_device_values,
                                     gdk_device_get_n_keys (device));

            info->n_keys = n_device_values;
            info->keys   = g_renew (GdkDeviceKey, info->keys, info->n_keys);
            memset (info->keys, 0, info->n_keys * sizeof (GdkDeviceKey));

            for (i = 0; i < n_device_values; i++)
              {
                const gchar     *accel;
                guint            keyval;
                GdkModifierType  modifiers;

                accel = g_value_get_string (gimp_value_array_index (array, i));

                gtk_accelerator_parse (accel, &keyval, &modifiers);

                gimp_device_info_set_key (info, i, keyval, modifiers);
              }
          }
      }
      break;

    case PROP_PRESSURE_CURVE:
      src_curve  = g_value_get_object (value);
      dest_curve = info->pressure_curve;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  if (src_curve && dest_curve)
    {
      gimp_config_copy (GIMP_CONFIG (src_curve),
                        GIMP_CONFIG (dest_curve),
                        GIMP_CONFIG_PARAM_SERIALIZE);
    }
}

static void
gimp_device_info_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpDeviceInfo *info = GIMP_DEVICE_INFO (object);

  switch (property_id)
    {
    case PROP_DEVICE:
      g_value_set_object (value, info->device);
      break;

    case PROP_DISPLAY:
      g_value_set_object (value, info->display);
      break;

    case PROP_MODE:
      g_value_set_enum (value, gimp_device_info_get_mode (info));
      break;

    case PROP_AXES:
      {
        GimpValueArray *array;
        GValue          enum_value = G_VALUE_INIT;
        gint            n_axes;
        gint            i;

        array = gimp_value_array_new (6);
        g_value_init (&enum_value, GDK_TYPE_AXIS_USE);

        n_axes = gimp_device_info_get_n_axes (info);

        for (i = 0; i < n_axes; i++)
          {
            g_value_set_enum (&enum_value,
                              gimp_device_info_get_axis_use (info, i));

            gimp_value_array_append (array, &enum_value);
          }

        g_value_unset (&enum_value);

        g_value_take_boxed (value, array);
      }
      break;

    case PROP_KEYS:
      {
        GimpValueArray *array;
        GValue          string_value = G_VALUE_INIT;
        gint            n_keys;
        gint            i;

        array = gimp_value_array_new (32);
        g_value_init (&string_value, G_TYPE_STRING);

        n_keys = gimp_device_info_get_n_keys (info);

        for (i = 0; i < n_keys; i++)
          {
            guint           keyval;
            GdkModifierType modifiers;

            gimp_device_info_get_key (info, i, &keyval, &modifiers);

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

            gimp_value_array_append (array, &string_value);
          }

        g_value_unset (&string_value);

        g_value_take_boxed (value, array);
      }
      break;

    case PROP_PRESSURE_CURVE:
      g_value_set_object (value, info->pressure_curve);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_device_info_guess_icon (GimpDeviceInfo *info)
{
  GimpViewable *viewable = GIMP_VIEWABLE (info);

  if (gimp_object_get_name (viewable) &&
      ! strcmp (gimp_viewable_get_icon_name (viewable),
                GIMP_VIEWABLE_GET_CLASS (viewable)->default_icon_name))
    {
      const gchar *icon_name = NULL;
      gchar       *down      = g_ascii_strdown (gimp_object_get_name (viewable),
                                                -1);

      if (strstr (down, "eraser"))
        {
          icon_name = GIMP_ICON_TOOL_ERASER;
        }
      else if (strstr (down, "pen"))
        {
          icon_name = GIMP_ICON_TOOL_PAINTBRUSH;
        }
      else if (strstr (down, "airbrush"))
        {
          icon_name = GIMP_ICON_TOOL_AIRBRUSH;
        }
      else if (strstr (down, "cursor")   ||
               strstr (down, "mouse")    ||
               strstr (down, "pointer")  ||
               strstr (down, "touchpad") ||
               strstr (down, "trackpoint"))
        {
          icon_name = GIMP_ICON_CURSOR;
        }

      g_free (down);

      if (icon_name)
        gimp_viewable_set_icon_name (viewable, icon_name);
    }
}


/*  public functions  */

GimpDeviceInfo *
gimp_device_info_new (Gimp       *gimp,
                      GdkDevice  *device,
                      GdkDisplay *display)
{
  GimpContext  *context;
  GimpToolInfo *tool_info;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GDK_IS_DEVICE (device), NULL);
  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);

  context   = gimp_get_user_context (gimp);
  tool_info = gimp_context_get_tool (context);

  g_return_val_if_fail (tool_info != NULL, NULL);

  return g_object_new (GIMP_TYPE_DEVICE_INFO,
                       "gimp",         gimp,
                       "device",       device,
                       "display",      display,
                       "tool-options", tool_info->tool_options,
                       NULL);
}

GdkDevice *
gimp_device_info_get_device (GimpDeviceInfo  *info,
                             GdkDisplay     **display)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), NULL);

  if (display)
    *display = info->display;

  return info->device;
}

gboolean
gimp_device_info_set_device (GimpDeviceInfo *info,
                             GdkDevice      *device,
                             GdkDisplay     *display)
{
  gint i;

  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), FALSE);
  g_return_val_if_fail ((device == NULL && display == NULL) ||
                        (GDK_IS_DEVICE (device) && GDK_IS_DISPLAY (display)),
                        FALSE);

  if (device && info->device)
    {
      g_printerr ("%s: trying to set GdkDevice '%s' on GimpDeviceInfo "
                  "which already has a device\n",
                  G_STRFUNC, gdk_device_get_name (device));

#ifdef G_OS_WIN32
      /*  This is a very weird/dirty difference we make between Win32 and
       *  Linux. On Linux, we had breakage because of duplicate devices,
       *  fixed by overwriting the info's old device (assuming it to be
       *  dead) with the new one. Unfortunately doing this on Windows
       *  too broke a lot of devices (which used to work with the old
       *  way). See the regression bug #2495.
       *
       *  NOTE that this only happens if something is wrong on the USB
       *  or udev or libinput or whatever side and the same device is
       *  present multiple times. Therefore there doesn't seem to be an
       *  absolute single "solution" to this problem (well there is, but
       *  probably not in GIMP, where we can only react). This is more
       *  of an experimenting-in-real-life kind of bug.
       *  Also we had no clear report on macOS or BSD (AFAIK) of broken
       *  tablets with any of the version of the code. So let's keep
       *  these similar to Linux for now.
       */
      return FALSE;
#endif /* G_OS_WIN32 */
    }
  else if (! device && ! info->device)
    {
      g_printerr ("%s: trying to unset GdkDevice of GimpDeviceInfo '%s'"
                  "which has no device\n",
                  G_STRFUNC, gimp_object_get_name (info));

      /*  bail out, unsetting twice makes no sense  */
      return FALSE;
    }

  g_return_val_if_fail (device == NULL ||
                        strcmp (gdk_device_get_name (device),
                                gimp_object_get_name (info)) == 0, FALSE);

  if (device)
    {
      info->device  = device;
      info->display = display;

      g_object_set_data (G_OBJECT (device), GIMP_DEVICE_INFO_DATA_KEY, info);

      gimp_device_info_set_mode (info, info->mode);

      if (info->n_axes != device->num_axes)
        g_printerr ("%s: stored 'num-axes' for device '%s' doesn't match "
                    "number of axes present in device\n",
                    G_STRFUNC, device->name);

      for (i = 0; i < MIN (info->n_axes, device->num_axes); i++)
        gimp_device_info_set_axis_use (info, i,
                                       info->axes[i]);

      if (info->n_keys != device->num_keys)
        g_printerr ("%s: stored 'num-keys' for device '%s' doesn't match "
                    "number of keys present in device\n",
                    G_STRFUNC, device->name);

      for (i = 0; i < MIN (info->n_keys, device->num_keys); i++)
        gimp_device_info_set_key (info, i,
                                  info->keys[i].keyval,
                                  info->keys[i].modifiers);
    }
  else
    {
      device  = info->device;
      display = info->display;

      info->device  = NULL;
      info->display = NULL;

      g_object_set_data (G_OBJECT (device), GIMP_DEVICE_INFO_DATA_KEY, NULL);

      gimp_device_info_set_mode (info, device->mode);

      info->n_axes = device->num_axes;
      info->axes   = g_renew (GdkAxisUse, info->axes, info->n_axes);
      memset (info->axes, 0, info->n_axes * sizeof (GdkAxisUse));

      for (i = 0; i < info->n_axes; i++)
        gimp_device_info_set_axis_use (info, i,
                                       device->axes[i].use);

      info->n_keys = device->num_keys;
      info->keys   = g_renew (GdkDeviceKey, info->keys, info->n_keys);
      memset (info->keys, 0, info->n_keys * sizeof (GdkDeviceKey));

      for (i = 0; i < MIN (info->n_keys, device->num_keys); i++)
        gimp_device_info_set_key (info, i,
                                  device->keys[i].keyval,
                                  device->keys[i].modifiers);
    }

  /*  sort order depends on device presence  */
  gimp_object_name_changed (GIMP_OBJECT (info));

  g_object_notify (G_OBJECT (info), "device");

  return TRUE;
}

void
gimp_device_info_set_default_tool (GimpDeviceInfo *info)
{
  g_return_if_fail (GIMP_IS_DEVICE_INFO (info));

  if (info->device &&
      gdk_device_get_source (info->device) == GDK_SOURCE_ERASER)
    {
      GimpContainer *tools = GIMP_TOOL_PRESET (info)->gimp->tool_info_list;
      GimpToolInfo  *eraser;

      eraser =
        GIMP_TOOL_INFO (gimp_container_get_child_by_name (tools,
                                                          "gimp-eraser-tool"));

      if (eraser)
        g_object_set (info,
                      "tool-options", eraser->tool_options,
                      NULL);
    }
}

void
gimp_device_info_save_tool (GimpDeviceInfo *info)
{
  GimpToolPreset      *preset = GIMP_TOOL_PRESET (info);
  GimpContext         *user_context;
  GimpToolInfo        *tool_info;
  GimpContextPropMask  serialize_props;

  g_return_if_fail (GIMP_IS_DEVICE_INFO (info));

  user_context = gimp_get_user_context (GIMP_TOOL_PRESET (info)->gimp);

  tool_info = gimp_context_get_tool (user_context);

  g_object_set (info,
                "tool-options", tool_info->tool_options,
                NULL);

  serialize_props =
    gimp_context_get_serialize_properties (GIMP_CONTEXT (preset->tool_options));

  g_object_set (preset,
                "use-fg-bg",
                (serialize_props & GIMP_CONTEXT_PROP_MASK_FOREGROUND) ||
                (serialize_props & GIMP_CONTEXT_PROP_MASK_BACKGROUND),

                "use-brush",
                (serialize_props & GIMP_CONTEXT_PROP_MASK_BRUSH) != 0,

                "use-dynamics",
                (serialize_props & GIMP_CONTEXT_PROP_MASK_DYNAMICS) != 0,

                "use-mypaint-brush",
                (serialize_props & GIMP_CONTEXT_PROP_MASK_MYBRUSH) != 0,

                "use-gradient",
                (serialize_props & GIMP_CONTEXT_PROP_MASK_GRADIENT) != 0,

                "use-pattern",
                (serialize_props & GIMP_CONTEXT_PROP_MASK_PATTERN) != 0,

                "use-palette",
                (serialize_props & GIMP_CONTEXT_PROP_MASK_PALETTE) != 0,

                "use-font",
                (serialize_props & GIMP_CONTEXT_PROP_MASK_FONT) != 0,

                NULL);
}

void
gimp_device_info_restore_tool (GimpDeviceInfo *info)
{
  GimpToolPreset *preset;
  GimpContext    *user_context;

  g_return_if_fail (GIMP_IS_DEVICE_INFO (info));

  preset = GIMP_TOOL_PRESET (info);

  user_context = gimp_get_user_context (GIMP_TOOL_PRESET (info)->gimp);

  if (preset->tool_options)
    {
      if (gimp_context_get_tool_preset (user_context) != preset)
        {
          gimp_context_set_tool_preset (user_context, preset);
        }
      else
        {
          gimp_context_tool_preset_changed (user_context);
        }
    }
}

GdkInputMode
gimp_device_info_get_mode (GimpDeviceInfo *info)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), GDK_MODE_DISABLED);

  if (info->device)
    return info->device->mode;
  else
    return info->mode;
}

void
gimp_device_info_set_mode (GimpDeviceInfo *info,
                           GdkInputMode    mode)
{
  g_return_if_fail (GIMP_IS_DEVICE_INFO (info));

  if (mode != gimp_device_info_get_mode (info))
    {
      if (info->device)
        gdk_device_set_mode (info->device, mode);
      else
        info->mode = mode;

      g_object_notify (G_OBJECT (info), "mode");
    }
}

gboolean
gimp_device_info_has_cursor (GimpDeviceInfo *info)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), FALSE);

  if (info->device)
    return info->device->has_cursor;

  return FALSE;
}

gint
gimp_device_info_get_n_axes (GimpDeviceInfo *info)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), 0);

  if (info->device)
    return info->device->num_axes;
  else
    return info->n_axes;
}

GdkAxisUse
gimp_device_info_get_axis_use (GimpDeviceInfo *info,
                               gint            axis)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), GDK_AXIS_IGNORE);
  g_return_val_if_fail (axis >= 0 && axis < gimp_device_info_get_n_axes (info),
                        GDK_AXIS_IGNORE);

  if (info->device)
    return info->device->axes[axis].use;
  else
    return info->axes[axis];
}

void
gimp_device_info_set_axis_use (GimpDeviceInfo *info,
                               gint            axis,
                               GdkAxisUse      use)
{
  g_return_if_fail (GIMP_IS_DEVICE_INFO (info));
  g_return_if_fail (axis >= 0 && axis < gimp_device_info_get_n_axes (info));

  if (use != gimp_device_info_get_axis_use (info, axis))
    {
      if (info->device)
        gdk_device_set_axis_use (info->device, axis, use);

      info->axes[axis] = use;

      g_object_notify (G_OBJECT (info), "axes");
    }
}

gint
gimp_device_info_get_n_keys (GimpDeviceInfo *info)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), 0);

  if (info->device)
    return info->device->num_keys;
  else
    return info->n_keys;
}

void
gimp_device_info_get_key (GimpDeviceInfo  *info,
                          gint             key,
                          guint           *keyval,
                          GdkModifierType *modifiers)
{
  g_return_if_fail (GIMP_IS_DEVICE_INFO (info));
  g_return_if_fail (key >= 0 && key < gimp_device_info_get_n_keys (info));
  g_return_if_fail (keyval != NULL);
  g_return_if_fail (modifiers != NULL);

  if (info->device)
    {
      *keyval    = info->device->keys[key].keyval;
      *modifiers = info->device->keys[key].modifiers;
    }
  else
    {
      *keyval    = info->keys[key].keyval;
      *modifiers = info->keys[key].modifiers;
    }
}

void
gimp_device_info_set_key (GimpDeviceInfo *info,
                          gint             key,
                          guint            keyval,
                          GdkModifierType  modifiers)
{
  guint           old_keyval;
  GdkModifierType old_modifiers;

  g_return_if_fail (GIMP_IS_DEVICE_INFO (info));
  g_return_if_fail (key >= 0 && key < gimp_device_info_get_n_keys (info));

  gimp_device_info_get_key (info, key, &old_keyval, &old_modifiers);

  if (keyval    != old_keyval ||
      modifiers != old_modifiers)
    {
      if (info->device)
        gdk_device_set_key (info->device, key, keyval, modifiers);

      info->keys[key].keyval    = keyval;
      info->keys[key].modifiers = modifiers;

      g_object_notify (G_OBJECT (info), "keys");
    }
}

GimpCurve *
gimp_device_info_get_curve (GimpDeviceInfo *info,
                            GdkAxisUse      use)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), NULL);

  switch (use)
    {
    case GDK_AXIS_PRESSURE:
      return info->pressure_curve;
      break;

    default:
      return NULL;
    }
}

gdouble
gimp_device_info_map_axis (GimpDeviceInfo *info,
                           GdkAxisUse      use,
                           gdouble         value)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), value);

  /* CLAMP() the return values be safe against buggy XInput drivers */

  switch (use)
    {
    case GDK_AXIS_PRESSURE:
      return gimp_curve_map_value (info->pressure_curve, value);

    case GDK_AXIS_XTILT:
      return CLAMP (value, GIMP_COORDS_MIN_TILT, GIMP_COORDS_MAX_TILT);

    case GDK_AXIS_YTILT:
      return CLAMP (value, GIMP_COORDS_MIN_TILT, GIMP_COORDS_MAX_TILT);

    case GDK_AXIS_WHEEL:
      return CLAMP (value, GIMP_COORDS_MIN_WHEEL, GIMP_COORDS_MAX_WHEEL);

    default:
      break;
    }

  return value;
}

GimpDeviceInfo *
gimp_device_info_get_by_device (GdkDevice *device)
{
  g_return_val_if_fail (GDK_IS_DEVICE (device), NULL);

  return g_object_get_data (G_OBJECT (device), GIMP_DEVICE_INFO_DATA_KEY);
}

gint
gimp_device_info_compare (GimpDeviceInfo *a,
                          GimpDeviceInfo *b)
{
  if (a->device && a->display &&
      a->device == gdk_display_get_core_pointer (a->display))
    {
      return -1;
    }
  else if (b->device && b->display &&
           b->device == gdk_display_get_core_pointer (b->display))
    {
      return 1;
    }
  else if (a->device && ! b->device)
    {
      return -1;
    }
  else if (! a->device && b->device)
    {
      return 1;
    }
  else
    {
      return gimp_object_name_collate ((GimpObject *) a,
                                       (GimpObject *) b);
    }
}
