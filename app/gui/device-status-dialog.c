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

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontainer.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpdnd.h"
#include "widgets/gimpdeviceinfo.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimppreview.h"

#include "device-status-dialog.h"

#include "gimp-intl.h"


#define CELL_SIZE 20 /* The size of the preview cells */


typedef struct _DeviceStatusDialog DeviceStatusDialog;

struct _DeviceStatusDialog
{
  gint        num_devices;

  GdkDevice  *current;
  GdkDevice **devices;

  GtkWidget  *shell;
  GtkWidget  *table;

  GtkWidget **frames;
  GtkWidget **tools;
  GtkWidget **foregrounds;
  GtkWidget **backgrounds;
  GtkWidget **brushes;
  GtkWidget **patterns;
  GtkWidget **gradients;
};


/*  local functions */

static void     device_status_destroy_callback   (void);

static void     device_status_dialog_update      (GimpDeviceInfo     *device_info,
                                                  DeviceStatusDialog *dialog);
static void     device_status_drop_tool          (GtkWidget          *widget,
						  GimpViewable       *viewable,
						  gpointer            data);
static void     device_status_foreground_changed (GtkWidget          *widget,
						  gpointer            data);
static void     device_status_background_changed (GtkWidget          *widget,
						  gpointer            data);
static void     device_status_drop_brush         (GtkWidget          *widget,
						  GimpViewable       *viewable,
						  gpointer            data);
static void     device_status_drop_pattern       (GtkWidget          *widget,
						  GimpViewable       *viewable,
						  gpointer            data);
static void     device_status_drop_gradient      (GtkWidget          *widget,
						  GimpViewable       *viewable,
						  gpointer            data);


/*  local data  */

static DeviceStatusDialog *deviceD = NULL;


/*  public functions  */

GtkWidget *
device_status_dialog_create (Gimp *gimp)
{
  GimpDeviceInfo *device_info;
  GimpContext    *context;
  GtkWidget      *label;
  GimpRGB         color;
  GList          *list;
  gint            i;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (deviceD)
    return deviceD->shell;

  deviceD = g_new0 (DeviceStatusDialog, 1);

  deviceD->shell = gimp_dialog_new (_("Device Status"), "device_status",
				    gimp_standard_help_func,
				    "dialogs/device_status.html",
				    GTK_WIN_POS_NONE,
				    FALSE, FALSE, TRUE,

				    GTK_STOCK_CLOSE, gtk_widget_hide,
				    NULL, 1, NULL, TRUE, TRUE,

				    GTK_STOCK_SAVE, gimp_devices_save,
				    NULL, gimp, NULL, FALSE, FALSE,

				    NULL);

  deviceD->num_devices = g_list_length (gdk_devices_list ());

  /*  devices table  */
  deviceD->table = gtk_table_new (deviceD->num_devices, 7, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (deviceD->table), 3);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (deviceD->shell)->vbox),
		     deviceD->table);
  gtk_widget_realize (deviceD->table);
  gtk_widget_show (deviceD->table);

  deviceD->devices     = g_new (GdkDevice *, deviceD->num_devices);
  deviceD->frames      = g_new (GtkWidget *, deviceD->num_devices);
  deviceD->tools       = g_new (GtkWidget *, deviceD->num_devices);
  deviceD->foregrounds = g_new (GtkWidget *, deviceD->num_devices);
  deviceD->backgrounds = g_new (GtkWidget *, deviceD->num_devices);
  deviceD->brushes     = g_new (GtkWidget *, deviceD->num_devices);
  deviceD->patterns    = g_new (GtkWidget *, deviceD->num_devices);
  deviceD->gradients   = g_new (GtkWidget *, deviceD->num_devices);

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);

  for (list = gdk_devices_list (), i = 0;
       list;
       list = g_list_next (list), i++)
    {
      device_info = gimp_device_info_get_by_device (GDK_DEVICE (list->data));

      context = GIMP_CONTEXT (device_info);

      g_signal_connect (device_info, "changed",
                        G_CALLBACK (device_status_dialog_update),
                        deviceD);

      deviceD->devices[i] = device_info->device;

      /*  the device name  */

      deviceD->frames[i] = gtk_frame_new (NULL);

      gtk_frame_set_shadow_type (GTK_FRAME(deviceD->frames[i]), GTK_SHADOW_OUT);
      gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->frames[i],
			0, 1, i, i+1,
			GTK_FILL, GTK_FILL, 2, 0);

      label = gtk_label_new (GIMP_OBJECT (device_info)->name);
      gtk_misc_set_padding (GTK_MISC(label), 2, 0);
      gtk_container_add (GTK_CONTAINER(deviceD->frames[i]), label);
      gtk_widget_show(label);

      /*  the tool  */

      deviceD->tools[i] =
	gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_tool (context)),
			       CELL_SIZE, CELL_SIZE, 0,
			       FALSE, FALSE, TRUE);
      g_signal_connect_object (context, "tool_changed",
                               G_CALLBACK (gimp_preview_set_viewable),
                               deviceD->tools[i],
                               G_CONNECT_SWAPPED);
      gimp_dnd_viewable_dest_add (deviceD->tools[i],
				  GIMP_TYPE_TOOL_INFO,
				  device_status_drop_tool,
				  device_info);
      gtk_table_attach (GTK_TABLE (deviceD->table), deviceD->tools[i],
			1, 2, i, i+1,
			0, 0, 2, 2);

      /*  the foreground color  */

      deviceD->foregrounds[i] = 
	gimp_color_area_new (&color,
			     GIMP_COLOR_AREA_FLAT,
			     GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);
      gtk_widget_set_size_request (deviceD->foregrounds[i],
                                   CELL_SIZE, CELL_SIZE);
      g_signal_connect (deviceD->foregrounds[i], "color_changed",
                        G_CALLBACK (device_status_foreground_changed),
                        device_info);
      gtk_table_attach (GTK_TABLE (deviceD->table), 
			deviceD->foregrounds[i],
			2, 3, i, i+1,
			0, 0, 2, 2);

      /*  the background color  */

      deviceD->backgrounds[i] = 
	gimp_color_area_new (&color,
			     GIMP_COLOR_AREA_FLAT,
			     GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);
      gtk_widget_set_size_request (deviceD->backgrounds[i],
                                   CELL_SIZE, CELL_SIZE);
      g_signal_connect (deviceD->backgrounds[i], "color_changed",
                        G_CALLBACK (device_status_background_changed),
                        device_info);
      gtk_table_attach (GTK_TABLE (deviceD->table), 
			deviceD->backgrounds[i],
			3, 4, i, i+1,
			0, 0, 2, 2);

      /*  the brush  */

      deviceD->brushes[i] =
	gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_brush (context)),
			       CELL_SIZE, CELL_SIZE, 0,
			       FALSE, FALSE, TRUE);
      g_signal_connect_object (context,"brush_changed",
                               G_CALLBACK (gimp_preview_set_viewable),
                               deviceD->brushes[i],
                               G_CONNECT_SWAPPED);
      gimp_dnd_viewable_dest_add (deviceD->brushes[i],
				  GIMP_TYPE_BRUSH,
				  device_status_drop_brush,
				  device_info);
      gtk_table_attach (GTK_TABLE (deviceD->table), deviceD->brushes[i],
			4, 5, i, i+1,
			0, 0, 2, 2);

      /*  the pattern  */

      deviceD->patterns[i] =
	gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_pattern (context)),
			       CELL_SIZE, CELL_SIZE, 0,
			       FALSE, FALSE, TRUE);
      g_signal_connect_object (context, "pattern_changed",
                               G_CALLBACK (gimp_preview_set_viewable),
                               deviceD->patterns[i],
                               G_CONNECT_SWAPPED);
      gimp_dnd_viewable_dest_add (deviceD->patterns[i],
				  GIMP_TYPE_PATTERN,
				  device_status_drop_pattern,
				  device_info);
      gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->patterns[i],
			5, 6, i, i+1,
			0, 0, 2, 2);

      /*  the gradient  */

      deviceD->gradients[i] =
	gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_gradient (context)),
			       CELL_SIZE * 2, CELL_SIZE, 0,
			       FALSE, FALSE, TRUE);
      g_signal_connect_object (context, "gradient_changed",
                               G_CALLBACK (gimp_preview_set_viewable),
                               deviceD->gradients[i],
                               G_CONNECT_SWAPPED);
      gimp_dnd_viewable_dest_add (deviceD->gradients[i],
				  GIMP_TYPE_GRADIENT,
				  device_status_drop_gradient,
				  device_info);
      gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->gradients[i],
			6, 7, i, i+1,
			0, 0, 2, 2);

      device_status_dialog_update (device_info, deviceD);
    }

  deviceD->current = NULL;

  device_status_dialog_update_current (gimp);

  g_signal_connect (deviceD->shell, "destroy",
                    G_CALLBACK (device_status_destroy_callback),
                    NULL);

  return deviceD->shell;
}

void
device_status_dialog_update_current (Gimp *gimp)
{
  gint i;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (deviceD)
    {
      for (i = 0; i < deviceD->num_devices; i++)
	{
	  if (deviceD->devices[i] == deviceD->current)
            {
              gtk_frame_set_shadow_type (GTK_FRAME(deviceD->frames[i]), 
                                         GTK_SHADOW_OUT);
            }
	  else if (deviceD->devices[i] == gimp_devices_get_current (gimp))
            {
              gtk_frame_set_shadow_type (GTK_FRAME(deviceD->frames[i]), 
                                         GTK_SHADOW_IN);
            }
	}

      deviceD->current = gimp_devices_get_current (gimp);
    }
}


/*  private functions  */

static void
device_status_destroy_callback (void)
{
  g_free (deviceD->devices);
  g_free (deviceD->frames);
  g_free (deviceD->tools);
  g_free (deviceD->foregrounds);
  g_free (deviceD->backgrounds);
  g_free (deviceD->brushes);
  g_free (deviceD->patterns);
  g_free (deviceD->gradients);

  g_free (deviceD);
  deviceD = NULL;
}

static void 
device_status_dialog_update (GimpDeviceInfo     *device_info,
                             DeviceStatusDialog *dialog)
{
  GimpContext *context;
  GimpRGB      color;
  guchar       red, green, blue;
  gchar        ttbuf[64];
  gint         i;

  context = GIMP_CONTEXT (device_info);

  for (i = 0; i < dialog->num_devices; i++)
    {
      if (dialog->devices[i] == device_info->device)
	break;
    }

  g_return_if_fail (i < dialog->num_devices);

  if (device_info->device->mode == GDK_MODE_DISABLED)
    {
      gtk_widget_hide (dialog->frames[i]);
      gtk_widget_hide (dialog->tools[i]);
      gtk_widget_hide (dialog->foregrounds[i]);
      gtk_widget_hide (dialog->backgrounds[i]);
      gtk_widget_hide (dialog->brushes[i]);
      gtk_widget_hide (dialog->patterns[i]);
      gtk_widget_hide (dialog->gradients[i]);
    }
  else
    {
      gtk_widget_show (dialog->frames[i]);

      if (gimp_context_get_tool (context))
	{
	  gtk_widget_show (dialog->tools[i]);
	}

      /*  foreground color  */
      gimp_context_get_foreground (context, &color);
      gimp_color_area_set_color (GIMP_COLOR_AREA (dialog->foregrounds[i]), 
				 &color);
      gtk_widget_show (dialog->foregrounds[i]);

      /*  Set the tip to be the RGB value  */
      gimp_rgb_get_uchar (&color, &red, &green, &blue);
      g_snprintf (ttbuf, sizeof (ttbuf), _("Foreground: %d, %d, %d"),
		  red, green, blue);
      gtk_widget_add_events (dialog->foregrounds[i],
			     GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
      gimp_help_set_help_data (dialog->foregrounds[i], ttbuf, NULL);

      /*  background color  */
      gimp_context_get_background (context, &color);
      gimp_color_area_set_color (GIMP_COLOR_AREA (dialog->backgrounds[i]), 
				 &color);
      gtk_widget_show (dialog->backgrounds[i]);

      /*  Set the tip to be the RGB value  */
      gimp_rgb_get_uchar (&color, &red, &green, &blue);
      g_snprintf (ttbuf, sizeof (ttbuf), _("Background: %d, %d, %d"),
		  red, green, blue);
      gtk_widget_add_events (dialog->backgrounds[i],
			     GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
      gimp_help_set_help_data (dialog->backgrounds[i], ttbuf, NULL);

      if (gimp_context_get_brush (context))
	{
	  gtk_widget_show (dialog->brushes[i]);
	}

      if (gimp_context_get_pattern (context))
	{
	  gtk_widget_show (dialog->patterns[i]);
	}

      if (gimp_context_get_gradient (context))
	{
	  gtk_widget_show (dialog->gradients[i]);
	}
    }
}

static void
device_status_drop_tool (GtkWidget    *widget,
			 GimpViewable *viewable,
			 gpointer      data)
{
  GimpDeviceInfo *device_info;

  device_info = (GimpDeviceInfo *) data;

  if (device_info && device_info->device)
    {
      gimp_context_set_tool (GIMP_CONTEXT (device_info),
                             GIMP_TOOL_INFO (viewable));
    }
}

static void
device_status_foreground_changed (GtkWidget *widget,
				  gpointer   data)
{
  GimpDeviceInfo *device_info;
  GimpRGB         color;

  device_info = (GimpDeviceInfo *) data;

  if (device_info && device_info->device)
    {
      gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &color);
      gimp_context_set_foreground (GIMP_CONTEXT (device_info), &color);
    }
}

static void
device_status_background_changed (GtkWidget *widget,
				  gpointer   data)
{
  GimpDeviceInfo *device_info;
  GimpRGB         color;

  device_info = (GimpDeviceInfo *) data;

  if (device_info && device_info->device)
    {
      gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &color);
      gimp_context_set_background (GIMP_CONTEXT (device_info), &color);
    }
}

static void
device_status_drop_brush (GtkWidget    *widget,
			  GimpViewable *viewable,
			  gpointer      data)
{
  GimpDeviceInfo *device_info;

  device_info = (GimpDeviceInfo *) data;

  if (device_info && device_info->device)
    {
      gimp_context_set_brush (GIMP_CONTEXT (device_info),
                              GIMP_BRUSH (viewable));
    }
}

static void
device_status_drop_pattern (GtkWidget    *widget,
			    GimpViewable *viewable,
			    gpointer      data)
{
  GimpDeviceInfo *device_info;
  
  device_info = (GimpDeviceInfo *) data;

  if (device_info && device_info->device)
    {
      gimp_context_set_pattern (GIMP_CONTEXT (device_info),
                                GIMP_PATTERN (viewable));
    }
}

static void
device_status_drop_gradient (GtkWidget    *widget,
			     GimpViewable *viewable,
			     gpointer      data)
{
  GimpDeviceInfo *device_info;
  
  device_info = (GimpDeviceInfo *) data;

  if (device_info && device_info->device)
    {
      gimp_context_set_gradient (GIMP_CONTEXT (device_info),
                                 GIMP_GRADIENT (viewable));
    }
}
