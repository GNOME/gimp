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

#include "widgets/widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpdnd.h"
#include "widgets/gimppreview.h"

#include "appenv.h"
#include "app_procs.h"
#include "devices.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


#define CELL_SIZE 20 /* The size of the preview cells */

#define DEVICE_CONTEXT_MASK (GIMP_CONTEXT_TOOL_MASK       | \
                             GIMP_CONTEXT_FOREGROUND_MASK | \
                             GIMP_CONTEXT_BACKGROUND_MASK | \
			     GIMP_CONTEXT_BRUSH_MASK      | \
			     GIMP_CONTEXT_PATTERN_MASK    | \
                             GIMP_CONTEXT_GRADIENT_MASK)


typedef struct _DeviceInfo DeviceInfo;

struct _DeviceInfo
{
  GdkDevice     *device;
  gchar         *name;

  gshort         is_present;  /*  is the device currently present  */

  /*  gdk_input options - for not present devices  */

  GdkInputMode   mode;
  gint           num_axes;
  GdkAxisUse    *axes;
  gint           num_keys;
  GdkDeviceKey  *keys;

  GimpContext   *context;
};

typedef struct _DeviceInfoDialog DeviceInfoDialog;

struct _DeviceInfoDialog
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
static void     input_dialog_able_callback       (GtkWidget    *widget,
						  GdkDevice    *device,
						  gpointer      data);

static void     devices_write_rc_device          (DeviceInfo   *device_info,
						  FILE         *fp);
static void     devices_write_rc                 (void);

static void     device_status_destroy_callback   (void);
static void     devices_close_callback           (GtkWidget    *widget,
						  gpointer      data);

static void     device_status_update             (GdkDevice    *device);
static void     device_status_update_current     (void);

static void     device_status_drop_tool          (GtkWidget    *widget,
						  GimpViewable *viewable,
						  gpointer      data);
static void     device_status_foreground_changed (GtkWidget    *widget,
						  gpointer      data);
static void     device_status_background_changed (GtkWidget    *widget,
						  gpointer      data);
static void     device_status_drop_brush         (GtkWidget    *widget,
						  GimpViewable *viewable,
						  gpointer      data);
static void     device_status_drop_pattern       (GtkWidget    *widget,
						  GimpViewable *viewable,
						  gpointer      data);
static void     device_status_drop_gradient      (GtkWidget    *widget,
						  GimpViewable *viewable,
						  gpointer      data);

static void     device_status_data_changed       (GimpContext  *context,
						  gpointer      dummy,
						  gpointer      data);

static void     device_status_context_connect    (GimpContext  *context,
						  GdkDevice    *device);


/*  global data  */
GdkDevice *current_device = NULL;

/*  local data  */
static GList            *device_info_list = NULL;
static DeviceInfoDialog *deviceD          = NULL;

/*  if true, don't update device information dialog */
static gboolean          suppress_update  = FALSE;


/*  utility functions for the device_info_list  */

static DeviceInfo *
device_info_get_by_device (GdkDevice *device)
{
  DeviceInfo *info;
  GList      *list;

  for (list = device_info_list; list; list = g_list_next (list))
    {
      info = (DeviceInfo *) list->data;

      if (info->device == device)
	return info;
    }

  return NULL;
}

static DeviceInfo *
device_info_get_by_name (gchar *name)
{
  DeviceInfo *info;
  GList      *list;

  for (list = device_info_list; list; list = g_list_next (list))
    {
      info = (DeviceInfo *) list->data;

      if (!strcmp (info->name, name))
	return info;
    }

  return NULL;
}

/*  the gtk input dialog  */

GtkWidget *
input_dialog_create (void)
{
  static GtkWidget *inputd = NULL;
  GtkWidget        *hbbox;

  if (inputd)
    return inputd;

  inputd = gtk_input_dialog_new ();

  gtk_container_set_border_width
    (GTK_CONTAINER (GTK_DIALOG (inputd)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (inputd)->action_area),
			   FALSE);

  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);

  gtk_widget_reparent (GTK_INPUT_DIALOG (inputd)->save_button, hbbox);
  GTK_WIDGET_SET_FLAGS (GTK_INPUT_DIALOG (inputd)->save_button,
			GTK_CAN_DEFAULT);
  gtk_widget_reparent (GTK_INPUT_DIALOG (inputd)->close_button, hbbox);
  GTK_WIDGET_SET_FLAGS (GTK_INPUT_DIALOG (inputd)->close_button,
			GTK_CAN_DEFAULT);

  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (inputd)->action_area), hbbox,
		    FALSE, FALSE, 0);
  gtk_widget_grab_default (GTK_INPUT_DIALOG (inputd)->close_button);
  gtk_widget_show(hbbox);

  g_signal_connect (G_OBJECT (GTK_INPUT_DIALOG (inputd)->save_button),
                    "clicked",
                    G_CALLBACK (devices_write_rc),
		      NULL);
  g_signal_connect (G_OBJECT (GTK_INPUT_DIALOG (inputd)->close_button),
                    "clicked",
                    G_CALLBACK (devices_close_callback),
                    inputd);

  g_signal_connect (G_OBJECT (inputd), "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &inputd);

  g_signal_connect (G_OBJECT (inputd), "enable_device",
                    G_CALLBACK (input_dialog_able_callback),
                    NULL);
  g_signal_connect (G_OBJECT (inputd), "disable_device",
                    G_CALLBACK (input_dialog_able_callback),
                    NULL);

  /*  Connect the "F1" help key  */
  gimp_help_connect_help_accel (inputd,
				gimp_standard_help_func,
				"dialogs/input_devices.html");

  return inputd;
}

static void
input_dialog_able_callback (GtkWidget *widget,
			    GdkDevice *device,
			    gpointer   data)
{
  device_status_update (device);
}

void 
devices_init (void)
{
  GdkDevice  *device;
  DeviceInfo *device_info;
  GList      *list;

  current_device = gdk_core_pointer;

  /*  create device info structures for present devices */
  for (list = gdk_devices_list (); list; list = g_list_next (list))
    {
      device = (GdkDevice *) list->data;

      device_info = g_new (DeviceInfo, 1);

      device_info->device     = device;
      device_info->name       = g_strdup (device->name);
      device_info->mode       = device->mode;

      device_info->is_present = TRUE;

      device_info->num_axes   = device->num_axes;
      device_info->axes       = NULL;

      device_info->num_keys   = device->num_keys;
      device_info->keys       = NULL;

      device_info->context    = gimp_create_context (the_gimp,
						     device_info->name, NULL);
      gimp_context_define_args (device_info->context,
				DEVICE_CONTEXT_MASK,
				FALSE);
      gimp_context_copy_args (gimp_get_user_context (the_gimp),
			      device_info->context,
			      DEVICE_CONTEXT_MASK);
      device_status_context_connect (device_info->context,
				     device_info->device);

      device_info_list = g_list_append (device_info_list, device_info);
    }
}

void
devices_restore (void)
{
  DeviceInfo  *device_info;
  GimpContext *context;
  gchar       *filename;

  /* Augment with information from rc file */
  filename = gimp_personal_rc_file ("devicerc");
  gimprc_parse_file (filename);
  g_free (filename);

  if ((device_info = device_info_get_by_device (current_device)) == NULL)
    return;

  suppress_update = TRUE;

  context = gimp_get_user_context (the_gimp);

  gimp_context_copy_args (device_info->context, context, DEVICE_CONTEXT_MASK);
  gimp_context_set_parent (device_info->context, context);

  suppress_update = FALSE;
}

void
devices_rc_update (gchar        *name, 
		   DeviceValues  values,
		   GdkInputMode  mode, 
		   gint          num_axes,
		   GdkAxisUse   *axes, 
		   gint          num_keys, 
		   GdkDeviceKey *keys,
		   const gchar  *tool_name,
		   GimpRGB      *foreground,
		   GimpRGB      *background,
		   const gchar  *brush_name, 
		   const gchar  *pattern_name,
		   const gchar  *gradient_name)
{
  DeviceInfo *device_info;

  /*  Find device if we have it  */
  device_info = device_info_get_by_name (name);

  if (! device_info)
    {
      device_info = g_new (DeviceInfo, 1);

      device_info->device     = NULL;
      device_info->name       = g_strdup (name);
      device_info->is_present = FALSE;

      if (values & DEVICE_AXES)
	{
	  device_info->num_axes = num_axes;
	  device_info->axes = g_new (GdkAxisUse, num_axes);
	  memcpy (device_info->axes, axes, num_axes * sizeof (GdkAxisUse));
	}
      else
	{
	  device_info->num_axes = 0;
	  device_info->axes     = NULL;
	}

      if (values & DEVICE_KEYS)
	{
	  device_info->num_keys = num_keys;
	  device_info->keys = g_new (GdkDeviceKey, num_keys);
	  memcpy (device_info->keys, axes, num_keys * sizeof (GdkDeviceKey));
	}
      else
	{
	  device_info->num_keys = 0;
	  device_info->keys     = NULL;
	}

      if (values & DEVICE_MODE)
	device_info->mode = mode;
      else
	device_info->mode = GDK_MODE_DISABLED;

      device_info->context = gimp_create_context (the_gimp,
						  device_info->name, NULL);
      gimp_context_define_args (device_info->context,
				DEVICE_CONTEXT_MASK,
				FALSE);
      gimp_context_copy_args (gimp_get_user_context (the_gimp),
			      device_info->context,
			      DEVICE_CONTEXT_MASK);
      device_status_context_connect (device_info->context,
				     device_info->device);

      device_info_list = g_list_append (device_info_list, device_info);
    }
  else
    {
      GdkDevice *device;

      device = device_info->device;

      if (device)
	{
	  if (values & DEVICE_MODE)
	    {
	      gdk_device_set_mode (device, mode);
	    }
	  
	  if ((values & DEVICE_AXES) && num_axes >= device->num_axes)
	    {
	      gint i;

	      for (i = 0; i < MIN (num_axes, device->num_axes); i++)
		{
		  gdk_device_set_axis_use (device, i, axes[i]);
		}
	    }

	  if ((values & DEVICE_KEYS) && num_keys >= device->num_keys)
	    {
	      gint i;

	      for (i = 0; i < MIN (num_keys, device->num_keys); i++)
		{
		  gdk_device_set_key (device, i,
				      keys[i].keyval,
				      keys[i].modifiers);
		}
	    }
	}
      else
	{
	  g_warning ("devices_rc_update called multiple times "
		     "for not present device\n");
	  return;
	}
    }

  if (values & DEVICE_TOOL)
    {
      GimpToolInfo *tool_info;

      tool_info = (GimpToolInfo *)
	gimp_container_get_child_by_name (the_gimp->tool_info_list,
					  tool_name);

      if (tool_info)
	{
	  gimp_context_set_tool (device_info->context, tool_info);
	}
      else
	{
	  g_free (device_info->context->tool_name);
	  device_info->context->tool_name = g_strdup (tool_name);
	}
    }

  if (values & DEVICE_FOREGROUND)
    {
      gimp_context_set_foreground (device_info->context, foreground);
    }

  if (values & DEVICE_BACKGROUND)
    {
      gimp_context_set_background (device_info->context, background);
    }

  if (values & DEVICE_BRUSH)
    {
      GimpBrush *brush;

      brush = (GimpBrush *)
	gimp_container_get_child_by_name (the_gimp->brush_factory->container,
					  brush_name);

      if (brush)
	{
	  gimp_context_set_brush (device_info->context, brush);
	}
      else if (no_data)
	{
	  g_free (device_info->context->brush_name);
	  device_info->context->brush_name = g_strdup (brush_name);
	}
    }

  if (values & DEVICE_PATTERN)
    {
      GimpPattern *pattern;

      pattern = (GimpPattern *)
	gimp_container_get_child_by_name (the_gimp->pattern_factory->container,
					  pattern_name);

      if (pattern)
	{
	  gimp_context_set_pattern (device_info->context, pattern);
	}
      else if (no_data)
	{
	  g_free (device_info->context->pattern_name);
	  device_info->context->pattern_name = g_strdup (pattern_name);
	}
    }

  if (values & DEVICE_GRADIENT)
    {
      GimpGradient *gradient;

      gradient = (GimpGradient *)
	gimp_container_get_child_by_name (the_gimp->gradient_factory->container,
					  gradient_name);

      if (gradient)
	{
	  gimp_context_set_gradient (device_info->context, gradient);
	}
      else if (no_data)
	{
	  g_free (device_info->context->gradient_name);
	  device_info->context->gradient_name = g_strdup (gradient_name);
	}
    }
}

void
select_device (GdkDevice *new_device)
{
  DeviceInfo  *device_info;
  GimpContext *context;

  device_info = device_info_get_by_device (current_device);

  gimp_context_unset_parent (device_info->context);

  suppress_update = TRUE;
  
  device_info = device_info_get_by_device (new_device);

  current_device = new_device;

  context = gimp_get_user_context (the_gimp);

  gimp_context_copy_args (device_info->context, context, DEVICE_CONTEXT_MASK);
  gimp_context_set_parent (device_info->context, context);

  suppress_update = FALSE;

  device_status_update_current ();
}

gboolean
devices_check_change (GdkEvent *event)
{
  GdkDevice *device;

  switch (event->type)
    {
    case GDK_MOTION_NOTIFY:
      device = ((GdkEventMotion *) event)->device;
      break;
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      device = ((GdkEventButton *) event)->device;
      break;
    case GDK_PROXIMITY_OUT:
      device = ((GdkEventProximity *) event)->device;
      break;
    default:
      device = current_device;
    }

  if (device != current_device)
    {
      select_device (device);
      return TRUE;
    }

  return FALSE;
}

static void
devices_write_rc_device (DeviceInfo *device_info,
			 FILE       *fp)
{
  GdkDevice  *device = NULL;
  gchar      *mode   = NULL;
  gint        i;

  if (device_info->is_present)
    device = device_info->device;
  
  fprintf (fp, "(device \"%s\"", device_info->name);

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
	fprintf (fp, " \"\"");
    }
  fprintf (fp,")");

  if (gimp_context_get_tool (device_info->context))
    {
      fprintf (fp, "\n    (tool \"%s\")",
	       GIMP_OBJECT (gimp_context_get_tool (device_info->context))->name);
    }

  {
    GimpRGB color;

    gimp_context_get_foreground (device_info->context, &color);

    fprintf (fp, "\n    (foreground (color-rgb %f %f %f))",
	     color.r, color.g, color.b);

    gimp_context_get_background (device_info->context, &color);

    fprintf (fp, "\n    (background (color-rgb %f %f %f))",
	     color.r, color.g, color.b);
  }

  if (gimp_context_get_brush (device_info->context))
    {
      fprintf (fp, "\n    (brush \"%s\")",
	       GIMP_OBJECT (gimp_context_get_brush (device_info->context))->name);
    }

  if (gimp_context_get_pattern (device_info->context))
    {
      fprintf (fp, "\n    (pattern \"%s\")",
	       GIMP_OBJECT (gimp_context_get_pattern (device_info->context))->name);
    }

  if (gimp_context_get_gradient (device_info->context))
    {
      fprintf (fp, "\n    (gradient \"%s\")",
	       GIMP_OBJECT (gimp_context_get_gradient (device_info->context))->name);
    }

  fprintf(fp,")\n");
}

static void
devices_write_rc (void)
{
  DeviceInfo *device_info;
  gchar      *filename;
  FILE       *fp;

  device_info = device_info_get_by_device (current_device);

  filename = gimp_personal_rc_file ("devicerc");
  fp = fopen (filename, "wb");
  g_free (filename);

  if (!fp)
    return;

  g_list_foreach (device_info_list, (GFunc) devices_write_rc_device, fp);

  fclose (fp);
}

GtkWidget *
device_status_create (void)
{
  DeviceInfo *device_info;
  GtkWidget  *label;
  GimpRGB     color;
  GList      *list;
  gint        i;

  if (deviceD)
    return deviceD->shell;

  deviceD = g_new (DeviceInfoDialog, 1);

  deviceD->shell = gimp_dialog_new (_("Device Status"), "device_status",
				    gimp_standard_help_func,
				    "dialogs/device_status.html",
				    GTK_WIN_POS_NONE,
				    FALSE, FALSE, TRUE,

				    GTK_STOCK_SAVE, devices_write_rc,
				    NULL, NULL, NULL, FALSE, FALSE,
				    GTK_STOCK_CLOSE, devices_close_callback,
				    NULL, NULL, NULL, TRUE, TRUE,

				    NULL);

  deviceD->num_devices = 0;

  for (list = device_info_list; list; list = g_list_next (list))
    {
      if (((DeviceInfo *) list->data)->is_present)
	deviceD->num_devices++;
    }

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

  for (list = device_info_list, i = 0; list; list = g_list_next (list), i++)
    {
      if (!((DeviceInfo *) list->data)->is_present)
	continue;

      device_info = (DeviceInfo *) list->data;

      deviceD->devices[i] = device_info->device;

      /*  the device name  */

      deviceD->frames[i] = gtk_frame_new (NULL);

      gtk_frame_set_shadow_type (GTK_FRAME(deviceD->frames[i]), GTK_SHADOW_OUT);
      gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->frames[i],
			0, 1, i, i+1,
			GTK_FILL, GTK_FILL, 2, 0);

      label = gtk_label_new (device_info->name);
      gtk_misc_set_padding (GTK_MISC(label), 2, 0);
      gtk_container_add (GTK_CONTAINER(deviceD->frames[i]), label);
      gtk_widget_show(label);

      /*  the tool  */

      deviceD->tools[i] =
	gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_tool (device_info->context)),
			       CELL_SIZE, CELL_SIZE, 0,
			       FALSE, FALSE, TRUE);
      g_signal_connect_object (G_OBJECT (device_info->context),
                               "tool_changed",
                               G_CALLBACK (gimp_preview_set_viewable),
                               G_OBJECT (deviceD->tools[i]),
                               G_CONNECT_SWAPPED);
      gimp_gtk_drag_dest_set_by_type (deviceD->tools[i],
				      GTK_DEST_DEFAULT_ALL,
				      GIMP_TYPE_TOOL_INFO,
				      GDK_ACTION_COPY);
      gimp_dnd_viewable_dest_set (deviceD->tools[i],
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
      gtk_widget_set_usize (deviceD->foregrounds[i], CELL_SIZE, CELL_SIZE);
      g_signal_connect (G_OBJECT (deviceD->foregrounds[i]), "color_changed",
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
      gtk_widget_set_usize (deviceD->backgrounds[i], CELL_SIZE, CELL_SIZE);
      g_signal_connect (G_OBJECT (deviceD->backgrounds[i]), "color_changed",
                        G_CALLBACK (device_status_background_changed),
                        device_info);
      gtk_table_attach (GTK_TABLE (deviceD->table), 
			deviceD->backgrounds[i],
			3, 4, i, i+1,
			0, 0, 2, 2);

      /*  the brush  */

      deviceD->brushes[i] =
	gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_brush (device_info->context)),
			       CELL_SIZE, CELL_SIZE, 0,
			       FALSE, FALSE, TRUE);
      g_signal_connect_object (G_OBJECT (device_info->context),"brush_changed",
                               G_CALLBACK (gimp_preview_set_viewable),
                               G_OBJECT (deviceD->brushes[i]),
                               G_CONNECT_SWAPPED);
      gimp_gtk_drag_dest_set_by_type (deviceD->brushes[i],
				      GTK_DEST_DEFAULT_ALL,
				      GIMP_TYPE_BRUSH,
				      GDK_ACTION_COPY);
      gimp_dnd_viewable_dest_set (deviceD->brushes[i],
				  GIMP_TYPE_BRUSH,
				  device_status_drop_brush,
				  device_info);
      gtk_table_attach (GTK_TABLE (deviceD->table), deviceD->brushes[i],
			4, 5, i, i+1,
			0, 0, 2, 2);

      /*  the pattern  */

      deviceD->patterns[i] =
	gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_pattern (device_info->context)),
			       CELL_SIZE, CELL_SIZE, 0,
			       FALSE, FALSE, TRUE);
      g_signal_connect_object (G_OBJECT (device_info->context),
                               "pattern_changed",
                               G_CALLBACK (gimp_preview_set_viewable),
                               G_OBJECT (deviceD->patterns[i]),
                               G_CONNECT_SWAPPED);
      gimp_gtk_drag_dest_set_by_type (deviceD->patterns[i],
				      GTK_DEST_DEFAULT_ALL,
				      GIMP_TYPE_PATTERN,
				      GDK_ACTION_COPY);
      gimp_dnd_viewable_dest_set (deviceD->patterns[i],
				  GIMP_TYPE_PATTERN,
				  device_status_drop_pattern,
				  device_info);
      gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->patterns[i],
			5, 6, i, i+1,
			0, 0, 2, 2);

      /*  the gradient  */

      deviceD->gradients[i] =
	gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_gradient (device_info->context)),
			       CELL_SIZE * 2, CELL_SIZE, 0,
			       FALSE, FALSE, TRUE);
      g_signal_connect_object (G_OBJECT (device_info->context),
                               "gradient_changed",
                               G_CALLBACK (gimp_preview_set_viewable),
                               G_OBJECT (deviceD->gradients[i]),
                               G_CONNECT_SWAPPED);
      gimp_gtk_drag_dest_set_by_type (deviceD->gradients[i],
				      GTK_DEST_DEFAULT_ALL,
				      GIMP_TYPE_GRADIENT,
				      GDK_ACTION_COPY);
      gimp_dnd_viewable_dest_set (deviceD->gradients[i],
				  GIMP_TYPE_GRADIENT,
				  device_status_drop_gradient,
				  device_info);
      gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->gradients[i],
			6, 7, i, i+1,
			0, 0, 2, 2);

      device_status_update (device_info->device);
    }

  deviceD->current = NULL;

  device_status_update_current ();

  g_signal_connect (G_OBJECT (deviceD->shell), "destroy",
                    G_CALLBACK (device_status_destroy_callback),
                    NULL);

  return deviceD->shell;
}

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
devices_close_callback (GtkWidget *widget,
			gpointer   data)
{
  gtk_widget_hide (GTK_WIDGET (data));
}

void
device_status_free (void)
{                                     
  /* Save device status on exit */
  if (gimprc.save_device_status)
    devices_write_rc ();

  if (deviceD)
    device_status_destroy_callback (); 
}

static void
device_status_update_current (void)
{
  gint i;

  if (deviceD)
    {
      for (i = 0; i < deviceD->num_devices; i++)
	{
	  if (deviceD->devices[i] == deviceD->current)
	    gtk_frame_set_shadow_type (GTK_FRAME(deviceD->frames[i]), 
				       GTK_SHADOW_OUT);
	  else if (deviceD->devices[i] == current_device)
	    gtk_frame_set_shadow_type (GTK_FRAME(deviceD->frames[i]), 
				       GTK_SHADOW_IN);
	}

      deviceD->current = current_device;
    }
}

void 
device_status_update (GdkDevice *device)
{
  DeviceInfo    *device_info;
  GimpRGB        color;
  guchar         red, green, blue;
  gchar          ttbuf[64];
  gint           i;

  if (!deviceD || suppress_update)
    return;

  if ((device_info = device_info_get_by_device (device)) == NULL)
    return;

  for (i = 0; i < deviceD->num_devices; i++)
    {
      if (deviceD->devices[i] == device)
	break;
    }

  g_return_if_fail (i < deviceD->num_devices);

  if (device->mode == GDK_MODE_DISABLED)
    {
      gtk_widget_hide (deviceD->frames[i]);
      gtk_widget_hide (deviceD->tools[i]);
      gtk_widget_hide (deviceD->foregrounds[i]);
      gtk_widget_hide (deviceD->backgrounds[i]);
      gtk_widget_hide (deviceD->brushes[i]);
      gtk_widget_hide (deviceD->patterns[i]);
      gtk_widget_hide (deviceD->gradients[i]);
    }
  else
    {
      gtk_widget_show (deviceD->frames[i]);

      if (gimp_context_get_tool (device_info->context))
	{
	  gtk_widget_show (deviceD->tools[i]);
	}

      /*  foreground color  */
      gimp_context_get_foreground (device_info->context, &color);
      gimp_color_area_set_color (GIMP_COLOR_AREA (deviceD->foregrounds[i]), 
				 &color);
      gtk_widget_show (deviceD->foregrounds[i]);

      /*  Set the tip to be the RGB value  */
      gimp_rgb_get_uchar (&color, &red, &green, &blue);
      g_snprintf (ttbuf, sizeof (ttbuf), _("Foreground: %d, %d, %d"),
		  red, green, blue);
      gtk_widget_add_events (deviceD->foregrounds[i],
			     GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
      gimp_help_set_help_data (deviceD->foregrounds[i], ttbuf, NULL);

      /*  background color  */
      gimp_context_get_background (device_info->context, &color);
      gimp_color_area_set_color (GIMP_COLOR_AREA (deviceD->backgrounds[i]), 
				 &color);
      gtk_widget_show (deviceD->backgrounds[i]);

      /*  Set the tip to be the RGB value  */
      gimp_rgb_get_uchar (&color, &red, &green, &blue);
      g_snprintf (ttbuf, sizeof (ttbuf), _("Background: %d, %d, %d"),
		  red, green, blue);
      gtk_widget_add_events (deviceD->backgrounds[i],
			     GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
      gimp_help_set_help_data (deviceD->backgrounds[i], ttbuf, NULL);

      if (gimp_context_get_brush (device_info->context))
	{
	  gtk_widget_show (deviceD->brushes[i]);
	}

      if (gimp_context_get_pattern (device_info->context))
	{
	  gtk_widget_show (deviceD->patterns[i]);
	}

      if (gimp_context_get_gradient (device_info->context))
	{
	  gtk_widget_show (deviceD->gradients[i]);
	}
    }
}

/*  dnd stuff  */

static void
device_status_drop_tool (GtkWidget    *widget,
			 GimpViewable *viewable,
			 gpointer      data)
{
  DeviceInfo *device_info;

  device_info = (DeviceInfo *) data;

  if (device_info && device_info->is_present)
    {
      gimp_context_set_tool (device_info->context, GIMP_TOOL_INFO (viewable));
    }
}

static void
device_status_foreground_changed (GtkWidget *widget,
				  gpointer   data)
{
  DeviceInfo *device_info;
  GimpRGB     color;

  device_info = (DeviceInfo *) data;

  if (device_info && device_info->is_present)
    {
      gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &color);
      gimp_context_set_foreground (device_info->context, &color);
    }
}

static void
device_status_background_changed (GtkWidget *widget,
				  gpointer   data)
{
  DeviceInfo *device_info;
  GimpRGB     color;

  device_info = (DeviceInfo *) data;

  if (device_info && device_info->is_present)
    {
      gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &color);
      gimp_context_set_background (device_info->context, &color);
    }
}

static void
device_status_drop_brush (GtkWidget    *widget,
			  GimpViewable *viewable,
			  gpointer      data)
{
  DeviceInfo *device_info;

  device_info = (DeviceInfo *) data;

  if (device_info && device_info->is_present)
    {
      gimp_context_set_brush (device_info->context, GIMP_BRUSH (viewable));
    }
}

static void
device_status_drop_pattern (GtkWidget    *widget,
			    GimpViewable *viewable,
			    gpointer      data)
{
  DeviceInfo *device_info;
  
  device_info = (DeviceInfo *) data;

  if (device_info && device_info->is_present)
    {
      gimp_context_set_pattern (device_info->context, GIMP_PATTERN (viewable));
    }
}

static void
device_status_drop_gradient (GtkWidget    *widget,
			     GimpViewable *viewable,
			     gpointer      data)
{
  DeviceInfo *device_info;
  
  device_info = (DeviceInfo *) data;

  if (device_info && device_info->is_present)
    {
      gimp_context_set_gradient (device_info->context, 
                                 GIMP_GRADIENT (viewable));
    }
}

/*  context callbacks  */

static void
device_status_data_changed (GimpContext *context,
			    gpointer     dummy,
			    gpointer     data)
{
  device_status_update ((GdkDevice *) data);
}

static void
device_status_context_connect  (GimpContext *context,
				GdkDevice   *device)
{
  g_signal_connect (G_OBJECT (context), "foreground_changed",
                    G_CALLBACK (device_status_data_changed),
                    device);
  g_signal_connect (G_OBJECT (context), "background_changed",
                    G_CALLBACK (device_status_data_changed),
                    device);
  g_signal_connect (G_OBJECT (context), "tool_changed",
                    G_CALLBACK (device_status_data_changed),
                    device);
  g_signal_connect (G_OBJECT (context), "brush_changed",
                    G_CALLBACK (device_status_data_changed),
                    device);
  g_signal_connect (G_OBJECT (context), "pattern_changed",
                    G_CALLBACK (device_status_data_changed),
                    device);
  g_signal_connect (G_OBJECT (context), "gradient_changed",
                    G_CALLBACK (device_status_data_changed),
                    device);
}
