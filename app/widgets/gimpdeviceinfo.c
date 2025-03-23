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
#include "core/gimppadactions.h"
#include "core/gimpparamspecs.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpaction.h"

#include "gimpdeviceinfo.h"

#include "gimp-intl.h"


#define GIMP_DEVICE_INFO_DATA_KEY "gimp-device-info"

/**
 * Some default names for axis. We will only use them when the device is
 * not plugged (in which case, we would use names returned by GDK).
 */
static const gchar *const axis_use_strings[] =
{
  N_("X"),
  N_("Y"),
  N_("Pressure"),
  N_("X tilt"),
  N_("Y tilt"),
  /* Wheel as in mouse or input device wheel.
   * Some pens would use the same axis for their rotation feature.
   * See bug 791455.
   * Yet GTK+ has a different axis since v. 3.22.
   * TODO: this should be actually tested with a device having such
   * feature.
   */
  N_("Wheel"),
  N_("Distance"),
  N_("Rotation"),
  N_("Slider")
};


enum
{
  PROP_0,
  PROP_DEVICE,
  PROP_DISPLAY,
  PROP_MODE,
  PROP_SOURCE,
  PROP_VENDOR_ID,
  PROP_PRODUCT_ID,
  PROP_TOOL_TYPE,
  PROP_TOOL_SERIAL,
  PROP_TOOL_HARDWARE_ID,
  PROP_AXES,
  PROP_KEYS,
  PROP_PRESSURE_CURVE,
  PROP_PAD_ACTIONS
};

struct _GimpDeviceInfoPrivate
{
  GdkDevice      *device;
  GdkDisplay     *display;

  /*  either "device" or the options below are set  */

  GdkInputMode    mode;
  gint            n_axes;
  GdkAxisUse     *axes_uses;
  gchar         **axes_names;

  gint            n_keys;
  GimpDeviceKey  *keys;

  GimpPadActions *pad_actions;

  /*  curves  */

  GimpCurve      *pressure_curve;
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

static void   gimp_device_info_tool_changed (GdkDevice      *device,
                                             GdkDeviceTool  *tool,
                                             GimpDeviceInfo *info);
static void gimp_device_info_device_changed (GdkDevice      *device,
                                             GimpDeviceInfo *info);

static void   gimp_device_info_updated      (GimpDeviceInfo *info);


G_DEFINE_TYPE_WITH_PRIVATE (GimpDeviceInfo, gimp_device_info, GIMP_TYPE_TOOL_PRESET)

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
                                                        G_PARAM_READWRITE         |
                                                        G_PARAM_CONSTRUCT_ONLY    |
                                                        G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class, PROP_DISPLAY,
                                   g_param_spec_object ("display",
                                                        NULL, NULL,
                                                        GDK_TYPE_DISPLAY,
                                                        GIMP_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE         |
                                                        G_PARAM_CONSTRUCT_ONLY    |
                                                        G_PARAM_EXPLICIT_NOTIFY));

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_MODE,
                         "mode",
                         _("Mode"),
                         NULL,
                         GDK_TYPE_INPUT_MODE,
                         GDK_MODE_DISABLED,
                         GIMP_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_SOURCE,
                                   g_param_spec_enum ("source",
                                                      NULL, NULL,
                                                      GDK_TYPE_INPUT_SOURCE,
                                                      GDK_SOURCE_MOUSE,
                                                      GIMP_PARAM_STATIC_STRINGS |
                                                      G_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_VENDOR_ID,
                                   g_param_spec_string ("vendor-id",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_PRODUCT_ID,
                                   g_param_spec_string ("product-id",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_TOOL_TYPE,
                                   g_param_spec_enum ("tool-type",
                                                      NULL, NULL,
                                                      GDK_TYPE_DEVICE_TOOL_TYPE,
                                                      GDK_DEVICE_TOOL_TYPE_UNKNOWN,
                                                      GIMP_PARAM_STATIC_STRINGS |
                                                      G_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_TOOL_SERIAL,
                                   g_param_spec_uint64 ("tool-serial",
                                                        NULL, NULL,
                                                        0, G_MAXUINT64, 0,
                                                        GIMP_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_TOOL_HARDWARE_ID,
                                   g_param_spec_uint64 ("tool-hardware-id",
                                                        NULL, NULL,
                                                        0, G_MAXUINT64, 0,
                                                        GIMP_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READABLE));

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

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_PAD_ACTIONS,
                           "pad-actions",
                           _("Pad actions"),
                           NULL,
                           GIMP_TYPE_PAD_ACTIONS,
                           GIMP_CONFIG_PARAM_AGGREGATE);
}

static void
gimp_device_info_init (GimpDeviceInfo *info)
{
  gimp_data_make_internal (GIMP_DATA (info), NULL);

  info->priv                 = gimp_device_info_get_instance_private (info);
  info->priv->mode           = GDK_MODE_DISABLED;
  info->priv->pressure_curve = GIMP_CURVE (gimp_curve_new ("pressure curve"));
  info->priv->axes_names     = NULL;
  info->priv->pad_actions    = gimp_pad_actions_new ();

  g_signal_connect (info, "notify::name",
                    G_CALLBACK (gimp_device_info_guess_icon),
                    NULL);
}

static void
gimp_device_info_constructed (GObject *object)
{
  GimpDeviceInfo *info = GIMP_DEVICE_INFO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_device_info_updated (info);

  gimp_assert ((info->priv->device == NULL         && info->priv->display == NULL) ||
               (GDK_IS_DEVICE (info->priv->device) && GDK_IS_DISPLAY (info->priv->display)));
}

static void
gimp_device_info_finalize (GObject *object)
{
  GimpDeviceInfo *info = GIMP_DEVICE_INFO (object);

  g_clear_pointer (&info->priv->axes_uses, g_free);
  g_clear_pointer (&info->priv->keys, g_free);
  g_strfreev (info->priv->axes_names);

  g_clear_object (&info->priv->pressure_curve);
  g_clear_object (&info->priv->pad_actions);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_device_info_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpDeviceInfo *info       = GIMP_DEVICE_INFO (object);
  GdkDevice      *device     = info->priv->device;
  GimpCurve      *src_curve  = NULL;
  GimpCurve      *dest_curve = NULL;

  switch (property_id)
    {
    case PROP_DEVICE:
      /* Nothing to disconnect, it's G_PARAM_CONSTRUCT_ONLY. */
      info->priv->device = g_value_get_object (value);

      if (info->priv->device)
        {
          g_signal_connect_object (info->priv->device, "tool-changed",
                                   G_CALLBACK (gimp_device_info_tool_changed),
                                   G_OBJECT (info), 0);
          g_signal_connect_object (info->priv->device, "changed",
                                   G_CALLBACK (gimp_device_info_device_changed),
                                   G_OBJECT (info), 0);
        }
      break;

    case PROP_DISPLAY:
      info->priv->display = g_value_get_object (value);
      break;

    case PROP_MODE:
      gimp_device_info_set_mode (info, g_value_get_enum (value));
      break;

    case PROP_AXES:
      {
        /* Axes information as stored in devicerc. */
        GimpValueArray *array = g_value_get_boxed (value);

        if (array)
          {
            gint i;
            gint n_device_values = gimp_value_array_length (array);

            if (device)
              {
                if (info->priv->n_axes != 0                                       &&
                    gdk_device_get_device_type (device) != GDK_DEVICE_TYPE_MASTER &&
                    info->priv->n_axes != n_device_values)
                  g_printerr ("%s: stored 'num-axes' for device '%s' doesn't match "
                              "number of axes present in device\n",
                              G_STRFUNC, gdk_device_get_name (device));
                n_device_values = MIN (n_device_values,
                                       gdk_device_get_n_axes (device));
              }
            else if (! info->priv->n_axes && n_device_values > 0)
              {
                info->priv->n_axes     = n_device_values;
                info->priv->axes_uses  = g_new0 (GdkAxisUse, info->priv->n_axes);
                info->priv->axes_names = g_new0 (gchar *, info->priv->n_axes + 1);
              }

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

            info->priv->n_keys = n_device_values;
            info->priv->keys   = g_renew (GimpDeviceKey, info->priv->keys, info->priv->n_keys);
            if (info->priv->n_keys > 0)
              memset (info->priv->keys, 0, info->priv->n_keys * sizeof (GimpDeviceKey));

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
      dest_curve = info->priv->pressure_curve;
      break;

    case PROP_PAD_ACTIONS:
      g_clear_object (&info->priv->pad_actions);
      info->priv->pad_actions = g_value_get_object (value);
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
      g_value_set_object (value, info->priv->device);
      break;

    case PROP_DISPLAY:
      g_value_set_object (value, info->priv->display);
      break;

    case PROP_MODE:
      g_value_set_enum (value, gimp_device_info_get_mode (info));
      break;

    case PROP_SOURCE:
      g_value_set_enum (value, gimp_device_info_get_source (info));
      break;

    case PROP_VENDOR_ID:
      g_value_set_string (value, gimp_device_info_get_vendor_id (info));
      break;

     case PROP_PRODUCT_ID:
      g_value_set_string (value, gimp_device_info_get_product_id (info));
      break;

    case PROP_TOOL_TYPE:
      g_value_set_enum (value, gimp_device_info_get_tool_type (info));
      break;

    case PROP_TOOL_SERIAL:
      g_value_set_uint64 (value, gimp_device_info_get_tool_serial (info));
      break;

    case PROP_TOOL_HARDWARE_ID:
      g_value_set_uint64 (value, gimp_device_info_get_tool_hardware_id (info));
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
      g_value_set_object (value, info->priv->pressure_curve);
      break;

    case PROP_PAD_ACTIONS:
      g_value_set_object (value, info->priv->pad_actions);
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

static void
gimp_device_info_tool_changed (GdkDevice      *device,
                               GdkDeviceTool  *tool,
                               GimpDeviceInfo *info)
{
  g_object_freeze_notify (G_OBJECT (info));

  /* GDK docs says that the number of axes can change on "changed"
   * signal for virtual devices only, but here, I encounter a change of
   * number of axes on a physical device, the first time when a stylus
   * approached then moved away from the active surface. When the tool
   * became then a GDK_DEVICE_TOOL_TYPE_UNKNOWN, I had one more axis
   * (which created criticals later).
   * So let's check specifically for such case.
   */
  if (info->priv->n_axes != gdk_device_get_n_axes (device))
    {
      gint n_axes     = gdk_device_get_n_axes (device);
      gint old_n_axes = info->priv->n_axes;
      gint i;

      for (i = n_axes; i < info->priv->n_axes; i++)
        g_free (info->priv->axes_names[i]);

      info->priv->axes_names = g_renew (gchar *, info->priv->axes_names, n_axes + 1);
      for (i = info->priv->n_axes; i < n_axes + 1; i++)
        info->priv->axes_names[i] = NULL;

      info->priv->axes_uses  = g_renew (GdkAxisUse, info->priv->axes_uses, n_axes);
      info->priv->n_axes     = n_axes;

      for (i = old_n_axes; i < n_axes; i++)
        {
          GdkAxisUse axis_use;

          axis_use = gdk_device_get_axis_use (info->priv->device, i);
          gimp_device_info_set_axis_use (info, i, axis_use);
        }
    }

  g_object_notify (G_OBJECT (info), "tool-type");
  g_object_notify (G_OBJECT (info), "tool-serial");
  g_object_notify (G_OBJECT (info), "tool-hardware-id");

  g_object_thaw_notify (G_OBJECT (info));
}

static void
gimp_device_info_device_changed (GdkDevice      *device,
                                 GimpDeviceInfo *info)
{
  /* Number of axes or keys can change on virtual devices when the
   * physical device changes.
   */
  gimp_device_info_updated (info);
}

static void
gimp_device_info_updated (GimpDeviceInfo *info)
{
  g_return_if_fail (GIMP_IS_DEVICE_INFO (info));
  g_return_if_fail ((info->priv->device == NULL && info->priv->display == NULL) ||
                    (GDK_IS_DEVICE (info->priv->device) && GDK_IS_DISPLAY (info->priv->display)));

  g_object_freeze_notify (G_OBJECT (info));

  if (info->priv->device)
    {
      GdkAxisUse *old_uses;
      GList      *axes;
      GList      *iter;
      gint        old_n_axes;
      gint        i;

      GimpDeviceKey  *old_keys;
      gint            old_n_keys;

      g_object_set_data (G_OBJECT (info->priv->device), GIMP_DEVICE_INFO_DATA_KEY, info);
      gimp_object_set_name (GIMP_OBJECT (info),
                            gdk_device_get_name (info->priv->device));
      gimp_device_info_set_mode (info, gdk_device_get_mode (info->priv->device));

      old_n_axes = info->priv->n_axes;
      old_uses   = info->priv->axes_uses;
      g_strfreev (info->priv->axes_names);
      axes = gdk_device_list_axes (info->priv->device);
      info->priv->n_axes     = g_list_length (axes);
      info->priv->axes_uses  = g_new0 (GdkAxisUse, info->priv->n_axes);
      info->priv->axes_names = g_new0 (gchar *, info->priv->n_axes + 1);

      for (i = 0, iter = axes; i < info->priv->n_axes; i++, iter = iter->next)
        {
          GdkAxisUse use;

          /* Virtual devices are special and just reproduce their actual
           * physical device at a given moment. We should never try to
           * reuse the data because we risk to pass device configuration
           * from one physical device to another.
           */
          if (gdk_device_get_device_type (info->priv->device) == GDK_DEVICE_TYPE_MASTER ||
              i >= old_n_axes)
            use = gdk_device_get_axis_use (info->priv->device, i);
          else
            use = old_uses[i];

          gimp_device_info_set_axis_use (info, i, use);

          if (iter->data != GDK_NONE)
            info->priv->axes_names[i] = gdk_atom_name (iter->data);
          else
            info->priv->axes_names[i] = NULL;
        }
      g_list_free (axes);
      g_clear_pointer (&old_uses, g_free);

      old_n_keys   = info->priv->n_keys;
      old_keys     = info->priv->keys;
      info->priv->n_keys = gdk_device_get_n_keys (info->priv->device);
      info->priv->keys   = g_new0 (GimpDeviceKey, info->priv->n_keys);

      for (i = 0; i < info->priv->n_keys; i++)
        {
          GimpDeviceKey key;

          if (gdk_device_get_device_type (info->priv->device) == GDK_DEVICE_TYPE_MASTER ||
              i >= old_n_keys)
            gdk_device_get_key (info->priv->device, i, &key.keyval, &key.modifiers);
          else
            key = old_keys[i];

          gimp_device_info_set_key (info, i, key.keyval, key.modifiers);
        }
      g_clear_pointer (&old_keys, g_free);
    }

  /*  sort order depends on device presence  */
  gimp_object_name_changed (GIMP_OBJECT (info));

  g_object_notify (G_OBJECT (info), "source");
  g_object_notify (G_OBJECT (info), "vendor-id");
  g_object_notify (G_OBJECT (info), "product-id");
  g_object_notify (G_OBJECT (info), "tool-type");
  g_object_notify (G_OBJECT (info), "tool-serial");
  g_object_notify (G_OBJECT (info), "tool-hardware-id");
  g_object_notify (G_OBJECT (info), "device");
  g_object_notify (G_OBJECT (info), "display");

  g_object_thaw_notify (G_OBJECT (info));
}

static void
gimp_device_info_pad_action_map_foreach (GimpPadActions    *pad_actions,
                                         GimpPadActionType  action_type,
                                         guint              number,
                                         guint              mode,
                                         const gchar       *action_name,
                                         gpointer           data)
{
  GtkPadController *controller = data;
  GimpDeviceInfo   *info;
  Gimp             *gimp;
  GimpAction       *action;
  gchar            *label;
  gchar            *accel_pos;

  info = g_object_get_data (G_OBJECT (controller), "device-info");
  if (!info)
    return;

  gimp = GIMP_TOOL_PRESET (info)->gimp;

  action = GIMP_ACTION (g_action_map_lookup_action (G_ACTION_MAP (gimp->app),
                                                    action_name));
  if (!action)
    return;

  /* Trim the accelerator from the feedback string */
  label = g_strdup (gimp_action_get_label (action));
  accel_pos = g_utf8_strchr (label, -1, '_');
  if (accel_pos)
    strcpy (accel_pos, accel_pos + 1);

  gtk_pad_controller_set_action (controller,
                                 /* Action type enums are binary compatible */
                                 (GtkPadActionType) action_type,
                                 number, mode,
                                 label,
                                 gimp_action_get_name (action));
  g_free (label);
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
                       "mode",         gdk_device_get_mode (device),
                       "tool-options", tool_info->tool_options,
                       NULL);
}

GdkDevice *
gimp_device_info_get_device (GimpDeviceInfo  *info,
                             GdkDisplay     **display)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), NULL);

  if (display)
    *display = info->priv->display;

  return info->priv->device;
}

gboolean
gimp_device_info_set_device (GimpDeviceInfo *info,
                             GdkDevice      *device,
                             GdkDisplay     *display)
{
  GdkInputMode mode;

  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), FALSE);
  g_return_val_if_fail ((device == NULL && display == NULL) ||
                        (GDK_IS_DEVICE (device) && GDK_IS_DISPLAY (display)),
                        FALSE);
  g_return_val_if_fail (device == NULL ||
                        strcmp (gdk_device_get_name (device),
                                gimp_object_get_name (info)) == 0, FALSE);

  if (device && info->priv->device)
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
  else if (! device && ! info->priv->device)
    {
      g_printerr ("%s: trying to unset GdkDevice of GimpDeviceInfo '%s'"
                  "which has no device\n",
                  G_STRFUNC, gimp_object_get_name (info));

      /*  bail out, unsetting twice makes no sense  */
      return FALSE;
    }
  else if (info->priv->device)
    {
      if (info->priv->n_axes != gdk_device_get_n_axes (info->priv->device))
        g_printerr ("%s: stored 'num-axes' for device '%s' doesn't match "
                    "number of axes present in device\n",
                    G_STRFUNC, gdk_device_get_name (info->priv->device));

      if (info->priv->n_keys != gdk_device_get_n_keys (info->priv->device))
        g_printerr ("%s: stored 'num-keys' for device '%s' doesn't match "
                    "number of keys present in device\n",
                    G_STRFUNC, gdk_device_get_name (info->priv->device));
    }

  if (info->priv->device)
    {
      g_object_set_data (G_OBJECT (info->priv->device),
                         GIMP_DEVICE_INFO_DATA_KEY, NULL);
      g_signal_handlers_disconnect_by_func (info->priv->device,
                                            gimp_device_info_tool_changed,
                                            info);
      g_signal_handlers_disconnect_by_func (info->priv->device,
                                            gimp_device_info_device_changed,
                                            info);
    }
  mode = gimp_device_info_get_mode (info);

  info->priv->device  = device;
  info->priv->display = display;
  if (info->priv->device)
    {
      g_signal_connect_object (info->priv->device, "tool-changed",
                               G_CALLBACK (gimp_device_info_tool_changed),
                               G_OBJECT (info), 0);
      g_signal_connect_object (info->priv->device, "changed",
                               G_CALLBACK (gimp_device_info_device_changed),
                               G_OBJECT (info), 0);
    }

  gimp_device_info_updated (info);
  /* The info existed from a previous run. Restore its mode. */
  gimp_device_info_set_mode (info, mode);

  return TRUE;
}

void
gimp_device_info_set_default_tool (GimpDeviceInfo *info)
{
  GimpContainer *tools;
  GimpToolInfo  *tool_info = NULL;

  g_return_if_fail (GIMP_IS_DEVICE_INFO (info));

  tools     = GIMP_TOOL_PRESET (info)->gimp->tool_info_list;
  tool_info =
    GIMP_TOOL_INFO (gimp_container_get_child_by_name (tools,
                                                      "gimp-paintbrush-tool"));

  if (info->priv->device)
    {
      switch (gdk_device_get_source (info->priv->device))
        {
        case GDK_SOURCE_ERASER:
          tool_info =
            GIMP_TOOL_INFO (gimp_container_get_child_by_name (tools,
                                                              "gimp-eraser-tool"));
          break;
        case GDK_SOURCE_PEN:
          tool_info =
            GIMP_TOOL_INFO (gimp_container_get_child_by_name (tools,
                                                              "gimp-paintbrush-tool"));
          break;
        case GDK_SOURCE_TOUCHSCREEN:
          tool_info =
            GIMP_TOOL_INFO (gimp_container_get_child_by_name (tools,
                                                              "gimp-smudge-tool"));
          break;
        default:
          break;
        }
    }

  if (tool_info)
    {
      g_object_set (info,
                    "tool-options", tool_info->tool_options,
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

  if (info->priv->device)
    return gdk_device_get_mode (info->priv->device);
  else
    return info->priv->mode;
}

void
gimp_device_info_set_mode (GimpDeviceInfo *info,
                           GdkInputMode    mode)
{
  g_return_if_fail (GIMP_IS_DEVICE_INFO (info));

  if (mode != gimp_device_info_get_mode (info))
    {
      if (info->priv->device)
        gdk_device_set_mode (info->priv->device, mode);
      else
        info->priv->mode = mode;

      g_object_notify (G_OBJECT (info), "mode");
    }
}

gboolean
gimp_device_info_has_cursor (GimpDeviceInfo *info)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), FALSE);

  if (info->priv->device)
    {
      /*  this should really be
       *
       *  "is slave, *and* the associated master is gdk_seat_get_pointer()"
       *
       *  but who knows if future multiple masters will all have their
       *  own visible pointer or not, and what the API for figuring
       *  that will be, so for now let's simply assume that all
       *  devices except floating ones move the pointer on screen.
       */
      return gdk_device_get_device_type (info->priv->device) !=
             GDK_DEVICE_TYPE_FLOATING;
    }

  return FALSE;
}

GdkInputSource
gimp_device_info_get_source (GimpDeviceInfo *info)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), GDK_SOURCE_MOUSE);

  if (info->priv->device)
    return gdk_device_get_source (info->priv->device);

  if (info->priv->pad_actions &&
      gimp_pad_actions_get_n_actions (info->priv->pad_actions) > 0)
    return GDK_SOURCE_TABLET_PAD;

  return GDK_SOURCE_MOUSE;
}

const gchar *
gimp_device_info_get_vendor_id (GimpDeviceInfo  *info)
{
  const gchar *id = _("(Device not present)");

  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), NULL);

  if (info->priv->device)
    {
      if (gdk_device_get_device_type (info->priv->device) == GDK_DEVICE_TYPE_MASTER)
        {
          id = _("(Virtual device)");
        }
      else
        {
          id = gdk_device_get_vendor_id (info->priv->device);

          if (! (id && strlen (id)))
            id = _("(none)");
        }
    }

  return id;
}

const gchar *
gimp_device_info_get_product_id (GimpDeviceInfo  *info)
{
  const gchar *id = _("(Device not present)");

  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), NULL);

  if (info->priv->device)
    {
      if (gdk_device_get_device_type (info->priv->device) == GDK_DEVICE_TYPE_MASTER)
        {
          return _("(Virtual device)");
        }
      else
        {
          id = gdk_device_get_product_id (info->priv->device);

          if (! (id && strlen (id)))
            id = _("(none)");
        }
    }

  return id;
}

GdkDeviceToolType
gimp_device_info_get_tool_type (GimpDeviceInfo *info)
{
  GdkDeviceToolType type = GDK_DEVICE_TOOL_TYPE_UNKNOWN;

  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), type);

  if (info->priv->device)
    {
      GdkDeviceTool *tool;

      g_object_get (info->priv->device, "tool", &tool, NULL);

      if (tool)
        {
          type = gdk_device_tool_get_tool_type (tool);
          g_object_unref (tool);
        }
    }

  return type;
}

guint64
gimp_device_info_get_tool_serial (GimpDeviceInfo *info)
{
  guint64 serial = 0;

  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), serial);

  if (info->priv->device)
    {
      GdkDeviceTool *tool;

      g_object_get (info->priv->device, "tool", &tool, NULL);

      if (tool)
        {
          serial = gdk_device_tool_get_serial (tool);
          g_object_unref (tool);
        }
    }

  return serial;
}

guint64
gimp_device_info_get_tool_hardware_id (GimpDeviceInfo *info)
{
  guint64 id = 0;

  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), id);

  if (info->priv->device)
    {
      GdkDeviceTool *tool;

      g_object_get (info->priv->device, "tool", &tool, NULL);

      if (tool)
        {
          id = gdk_device_tool_get_hardware_id (tool);
          g_object_unref (tool);
        }
    }

  return id;
}

gint
gimp_device_info_get_n_axes (GimpDeviceInfo *info)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), 0);

  if (info->priv->device)
    return gdk_device_get_n_axes (info->priv->device);
  else
    return info->priv->n_axes;
}

gboolean
gimp_device_info_ignore_axis (GimpDeviceInfo *info,
                              gint            axis)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), TRUE);
  g_return_val_if_fail (axis >= 0 && axis < info->priv->n_axes, TRUE);

  return (info->priv->axes_names[axis] == NULL);
}

const gchar *
gimp_device_info_get_axis_name (GimpDeviceInfo *info,
                                gint            axis)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), NULL);
  g_return_val_if_fail (axis >= 0 && axis < GDK_AXIS_LAST, NULL);

  if (info->priv->device && axis < info->priv->n_axes &&
      info->priv->axes_names[axis] != NULL)
    return info->priv->axes_names[axis];
  else
    return axis_use_strings[axis];
}

GdkAxisUse
gimp_device_info_get_axis_use (GimpDeviceInfo *info,
                               gint            axis)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), GDK_AXIS_IGNORE);
  g_return_val_if_fail (axis >= 0 && axis < gimp_device_info_get_n_axes (info),
                        GDK_AXIS_IGNORE);

  if (info->priv->device)
    return gdk_device_get_axis_use (info->priv->device, axis);
  else
    return info->priv->axes_uses[axis];
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
      if (info->priv->device)
        gdk_device_set_axis_use (info->priv->device, axis, use);

      info->priv->axes_uses[axis] = use;

      g_object_notify (G_OBJECT (info), "axes");
    }
}

gint
gimp_device_info_get_n_keys (GimpDeviceInfo *info)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), 0);

  if (info->priv->device)
    return gdk_device_get_n_keys (info->priv->device);
  else
    return info->priv->n_keys;
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

  if (info->priv->device)
    {
      *keyval    = 0;
      *modifiers = 0;

      gdk_device_get_key (info->priv->device, key,
                          keyval,
                          modifiers);
    }
  else
    {
      *keyval    = info->priv->keys[key].keyval;
      *modifiers = info->priv->keys[key].modifiers;
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
      if (info->priv->device)
        gdk_device_set_key (info->priv->device, key, keyval, modifiers);

      info->priv->keys[key].keyval    = keyval;
      info->priv->keys[key].modifiers = modifiers;

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
      return info->priv->pressure_curve;
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
      return gimp_curve_map_value (info->priv->pressure_curve, value);

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
  GdkSeat *seat;

  if (a->priv->device && a->priv->display &&
      (seat = gdk_display_get_default_seat (a->priv->display)) &&
      a->priv->device == gdk_seat_get_pointer (seat))
    {
      return -1;
    }
  else if (b->priv->device && b->priv->display &&
           (seat = gdk_display_get_default_seat (b->priv->display)) &&
           b->priv->device == gdk_seat_get_pointer (seat))
    {
      return 1;
    }
  else if (a->priv->device && ! b->priv->device)
    {
      return -1;
    }
  else if (! a->priv->device && b->priv->device)
    {
      return 1;
    }
  else
    {
      return gimp_object_name_collate ((GimpObject *) a,
                                       (GimpObject *) b);
    }
}

GimpPadActions *
gimp_device_info_get_pad_actions (GimpDeviceInfo *info)
{
  return info->priv->pad_actions;
}

GtkPadController *
gimp_device_info_create_pad_controller (GimpDeviceInfo *info,
                                        GimpWindow     *window)
{
  Gimp             *gimp;
  GimpPadActions   *pad_actions;
  GtkPadController *controller;

  gimp = GIMP_TOOL_PRESET (info)->gimp;

  pad_actions = gimp_device_info_get_pad_actions (info);

  if (gimp_pad_actions_get_n_actions (pad_actions) == 0)
    return NULL;

  controller = gtk_pad_controller_new (GTK_WINDOW (window),
                                       G_ACTION_GROUP (gimp->app),
                                       info->priv->device);
  g_object_set_data (G_OBJECT (controller), "device-info", info);
  gimp_pad_actions_foreach (pad_actions,
                            gimp_device_info_pad_action_map_foreach,
                            controller);

  return controller;
}
