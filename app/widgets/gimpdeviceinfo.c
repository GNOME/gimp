/* The GIMP -- an image manipulation program
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


/*  local function prototypes  */

static void   gimp_device_info_class_init (GimpDeviceInfoClass *klass);
static void   gimp_device_info_init       (GimpDeviceInfo      *device_info);

static void   gimp_device_info_finalize   (GObject           *object);


static GimpContextClass *parent_class = NULL;

static guint device_info_signals[LAST_SIGNAL] = { 0 };


GType
gimp_device_info_get_type (void)
{
  static GType device_info_type = 0;

  if (! device_info_type)
    {
      static const GTypeInfo device_info_info =
      {
        sizeof (GimpDeviceInfoClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_device_info_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpDeviceInfo),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_device_info_init,
      };

      device_info_type = g_type_register_static (GIMP_TYPE_CONTEXT,
                                                 "GimpDeviceInfo",
                                                 &device_info_info, 0);
    }

  return device_info_type;
}

static void
gimp_device_info_class_init (GimpDeviceInfoClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  device_info_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDeviceInfoClass, changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize = gimp_device_info_finalize;
}

static void
gimp_device_info_init (GimpDeviceInfo *device_info)
{
  device_info->device   = NULL;
  device_info->mode     = GDK_MODE_DISABLED;
  device_info->num_axes = 0;
  device_info->axes     = NULL;
  device_info->num_keys = 0;
  device_info->keys     = NULL;
}

static void
gimp_device_info_finalize (GObject *object)
{
  GimpDeviceInfo *device_info;

  device_info = GIMP_DEVICE_INFO (object);

  if (device_info->axes)
    g_free (device_info->axes);

  if (device_info->keys)
    g_free (device_info->keys);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpDeviceInfo *
gimp_device_info_new (Gimp        *gimp,
                      const gchar *name)
{
  GimpDeviceInfo *device_info;
  GimpContext    *context;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  device_info = g_object_new (GIMP_TYPE_DEVICE_INFO,
                              "name", name,
                              "gimp", gimp,
                              NULL);

  context = GIMP_CONTEXT (device_info);

  gimp_context_define_properties (context,
                                  GIMP_DEVICE_INFO_CONTEXT_MASK,
                                  FALSE);
  gimp_context_copy_properties (gimp_get_user_context (gimp),
                                context,
                                GIMP_DEVICE_INFO_CONTEXT_MASK);

  /*
   * FIXME: this is ugly and needs to be done via "notify" once
   * the contexts' properties are dynamic.
   */
  g_signal_connect_swapped (G_OBJECT (context), "foreground_changed",
                            G_CALLBACK (gimp_device_info_changed),
                            device_info);
  g_signal_connect_swapped (G_OBJECT (context), "background_changed",
                            G_CALLBACK (gimp_device_info_changed),
                            device_info);
  g_signal_connect_swapped (G_OBJECT (context), "tool_changed",
                            G_CALLBACK (gimp_device_info_changed),
                            device_info);
  g_signal_connect_swapped (G_OBJECT (context), "brush_changed",
                            G_CALLBACK (gimp_device_info_changed),
                            device_info);
  g_signal_connect_swapped (G_OBJECT (context), "pattern_changed",
                            G_CALLBACK (gimp_device_info_changed),
                            device_info);
  g_signal_connect_swapped (G_OBJECT (context), "gradient_changed",
                            G_CALLBACK (gimp_device_info_changed),
                            device_info);

  return device_info;
}

GimpDeviceInfo *
gimp_device_info_set_from_device (GimpDeviceInfo *device_info,
                                  GdkDevice      *device)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (device_info), NULL);
  g_return_val_if_fail (GDK_IS_DEVICE (device), NULL);

  g_object_set_data (G_OBJECT (device), GIMP_DEVICE_INFO_DATA_KEY, device_info);

  device_info->device     = device;

  device_info->mode       = device->mode;

  device_info->num_axes   = device->num_axes;
  device_info->axes       = NULL;

  device_info->num_keys   = device->num_keys;
  device_info->keys       = NULL;

  return device_info;
}

GimpDeviceInfo *
gimp_device_info_set_from_rc (GimpDeviceInfo     *device_info,
                              GimpDeviceValues    values,
                              GdkInputMode        mode,
                              gint                num_axes,
                              const GdkAxisUse   *axes,
                              gint                num_keys,
                              const GdkDeviceKey *keys,
                              const gchar        *tool_name,
                              const GimpRGB      *foreground,
                              const GimpRGB      *background,
                              const gchar        *brush_name,
                              const gchar        *pattern_name,
                              const gchar        *gradient_name)
{
  GimpContext *context;

  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (device_info), NULL);

  context = GIMP_CONTEXT (device_info);

  if (values & GIMP_DEVICE_VALUE_MODE)
    {
      device_info->mode = mode;

      if (device_info->device)
        {
          gdk_device_set_mode (device_info->device, mode);
        }
    }

  if (values & GIMP_DEVICE_VALUE_AXES)
    {
      device_info->num_axes = num_axes;
      device_info->axes     = g_new (GdkAxisUse, num_axes);
      memcpy (device_info->axes, axes, num_axes * sizeof (GdkAxisUse));

      if (device_info->device && (num_axes >= device_info->device->num_axes))
        {
          gint i;

          for (i = 0; i < MIN (num_axes, device_info->device->num_axes); i++)
            {
              gdk_device_set_axis_use (device_info->device, i, axes[i]);
            }
        }
    }

  if (values & GIMP_DEVICE_VALUE_KEYS)
    {
      device_info->num_keys = num_keys;
      device_info->keys = g_new (GdkDeviceKey, num_keys);
      memcpy (device_info->keys, axes, num_keys * sizeof (GdkDeviceKey));

      if (device_info->device && (num_keys >= device_info->device->num_keys))
        {
          gint i;

          for (i = 0; i < MIN (num_keys, device_info->device->num_keys); i++)
            {
              gdk_device_set_key (device_info->device, i,
                                  keys[i].keyval,
                                  keys[i].modifiers);
            }
        }
    }

  if (values & GIMP_DEVICE_VALUE_TOOL)
    {
      GimpToolInfo *tool_info;

      tool_info = (GimpToolInfo *)
	gimp_container_get_child_by_name (context->gimp->tool_info_list,
					  tool_name);

      if (tool_info)
	{
	  gimp_context_set_tool (context, tool_info);
	}
      else
	{
	  g_free (context->tool_name);
	  context->tool_name = g_strdup (tool_name);
	}
    }

  if (values & GIMP_DEVICE_VALUE_FOREGROUND)
    {
      gimp_context_set_foreground (context, foreground);
    }

  if (values & GIMP_DEVICE_VALUE_BACKGROUND)
    {
      gimp_context_set_background (context, background);
    }

  if (values & GIMP_DEVICE_VALUE_BRUSH)
    {
      GimpBrush *brush;

      brush = (GimpBrush *)
	gimp_container_get_child_by_name (context->gimp->brush_factory->container,
					  brush_name);

      if (brush)
	{
	  gimp_context_set_brush (context, brush);
	}
      else if (context->gimp->no_data)
	{
	  g_free (context->brush_name);
	  context->brush_name = g_strdup (brush_name);
	}
    }

  if (values & GIMP_DEVICE_VALUE_PATTERN)
    {
      GimpPattern *pattern;

      pattern = (GimpPattern *)
	gimp_container_get_child_by_name (context->gimp->pattern_factory->container,
					  pattern_name);

      if (pattern)
	{
	  gimp_context_set_pattern (context, pattern);
	}
      else if (context->gimp->no_data)
	{
	  g_free (context->pattern_name);
	  context->pattern_name = g_strdup (pattern_name);
	}
    }

  if (values & GIMP_DEVICE_VALUE_GRADIENT)
    {
      GimpGradient *gradient;

      gradient = (GimpGradient *)
	gimp_container_get_child_by_name (context->gimp->gradient_factory->container,
					  gradient_name);

      if (gradient)
	{
	  gimp_context_set_gradient (context, gradient);
	}
      else if (context->gimp->no_data)
	{
	  g_free (context->gradient_name);
	  context->gradient_name = g_strdup (gradient_name);
	}
    }

  return device_info;
}

void
gimp_device_info_changed (GimpDeviceInfo *device_info)
{
  g_return_if_fail (GIMP_IS_DEVICE_INFO (device_info));

  g_signal_emit (G_OBJECT (device_info), device_info_signals[CHANGED], 0);
}

void
gimp_device_info_save (GimpDeviceInfo *device_info,
                       FILE           *fp)
{
  GimpContext *context;
  GdkDevice   *device;
  gchar       *mode   = NULL;
  gint         i;

  g_return_if_fail (GIMP_IS_DEVICE_INFO (device_info));
  g_return_if_fail (fp != NULL);

  context = GIMP_CONTEXT (device_info);

  device = device_info->device;

  fprintf (fp, "(device \"%s\"", GIMP_OBJECT (device_info)->name);

  switch (device ? device->mode : device_info->mode)
    {
    case GDK_MODE_DISABLED:
      mode = "disabled";
      break;
    case GDK_MODE_SCREEN:
      mode = "screen";
      break;
    case GDK_MODE_WINDOW:
      mode = "window";
      break;
    }
  
  fprintf (fp, "\n    (mode %s)", mode);

  fprintf (fp, "\n    (axes %d",
	   device ? device->num_axes : device_info->num_axes);

  for (i = 0; i < (device ? device->num_axes : device_info->num_axes); i++)
    {
      gchar *axis_type = NULL;

      switch (device ? device->axes[i].use : device_info->axes[i])
	{
	case GDK_AXIS_IGNORE:
	  axis_type = "ignore";
	  break;
	case GDK_AXIS_X:
	  axis_type = "x";
	  break;
	case GDK_AXIS_Y:
	  axis_type = "y";
	  break;
	case GDK_AXIS_PRESSURE:
	  axis_type = "pressure";
	  break;
	case GDK_AXIS_XTILT:
	  axis_type = "xtilt";
	  break;
	case GDK_AXIS_YTILT:
	  axis_type = "ytilt";
	  break;
	case GDK_AXIS_WHEEL:
	  axis_type = "wheel";
	  break;
	}
      fprintf (fp, " %s",axis_type);
    }
  fprintf (fp,")");

  fprintf (fp, "\n    (keys %d",
	   device ? device->num_keys : device_info->num_keys);

  for (i = 0; i < (device ? device->num_keys : device_info->num_keys); i++)
    {
      GdkModifierType modifiers = (device ? device->keys[i].modifiers :
				   device_info->keys[i].modifiers);
      guint           keyval    = (device ? device->keys[i].keyval :
				   device_info->keys[i].keyval);

      if (keyval)
	{
	  /* FIXME: integrate this back with menus_install_accelerator */
	  gchar accel[64];
	  gchar t2[2];

	  accel[0] = '\0';
	  if (modifiers & GDK_CONTROL_MASK)
	    strcat (accel, "<control>");
	  if (modifiers & GDK_SHIFT_MASK)
	    strcat (accel, "<shift>");
	  if (modifiers & GDK_MOD1_MASK)
	    strcat (accel, "<alt>");

	  t2[0] = keyval;
	  t2[1] = '\0';
	  strcat (accel, t2);
	  fprintf (fp, " \"%s\"",accel);
	}
      else
        {
          fprintf (fp, " \"\"");
        }
    }

  fprintf (fp,")");

  if (gimp_context_get_tool (context))
    {
      fprintf (fp, "\n    (tool \"%s\")",
	       GIMP_OBJECT (gimp_context_get_tool (context))->name);
    }

  {
    GimpRGB color;
    gchar buf[3][G_ASCII_DTOSTR_BUF_SIZE];

    gimp_context_get_foreground (context, &color);

    g_ascii_formatd (buf[0], G_ASCII_DTOSTR_BUF_SIZE, "%f", color.r);
    g_ascii_formatd (buf[1], G_ASCII_DTOSTR_BUF_SIZE, "%f", color.g);
    g_ascii_formatd (buf[2], G_ASCII_DTOSTR_BUF_SIZE, "%f", color.b);

    fprintf (fp, "\n    (foreground (color-rgb %s %s %s))",
	     buf[0], buf[1], buf[2]);

    gimp_context_get_background (context, &color);

    g_ascii_formatd (buf[0], G_ASCII_DTOSTR_BUF_SIZE, "%f", color.r);
    g_ascii_formatd (buf[1], G_ASCII_DTOSTR_BUF_SIZE, "%f", color.g);
    g_ascii_formatd (buf[2], G_ASCII_DTOSTR_BUF_SIZE, "%f", color.b);

    fprintf (fp, "\n    (background (color-rgb %s %s %s))",
	     buf[0], buf[1], buf[2]);
  }

  if (gimp_context_get_brush (context))
    {
      fprintf (fp, "\n    (brush \"%s\")",
	       GIMP_OBJECT (gimp_context_get_brush (context))->name);
    }

  if (gimp_context_get_pattern (context))
    {
      fprintf (fp, "\n    (pattern \"%s\")",
	       GIMP_OBJECT (gimp_context_get_pattern (context))->name);
    }

  if (gimp_context_get_gradient (context))
    {
      fprintf (fp, "\n    (gradient \"%s\")",
	       GIMP_OBJECT (gimp_context_get_gradient (context))->name);
    }

  fprintf(fp,")\n");
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

  device_info = g_object_get_data (G_OBJECT (device), GIMP_DEVICE_INFO_DATA_KEY);

  if (device_info)
    gimp_device_info_changed (device_info);
}
