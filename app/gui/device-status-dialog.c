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
#include <stdio.h>

#include "appenv.h"
#include "actionarea.h"
#include "gimpbrushlist.h"
#include "devices.h"
#include "interface.h"
#include "gimprc.h"
#include "patterns.h"
#include "palette.h"
#include "session.h"
#include "tools.h"
#include "dialog_handler.h"
#include "indicator_area.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimpenv.h"

#define CELL_SIZE 20 /* The size of the preview cells */
#define PREVIEW_EVENT_MASK  GDK_EXPOSURE_MASK | \
                          GDK_BUTTON_PRESS_MASK | \
			  GDK_BUTTON_RELEASE_MASK | \
                          GDK_BUTTON1_MOTION_MASK | \
			  GDK_ENTER_NOTIFY_MASK | \
                          GDK_LEAVE_NOTIFY_MASK

#define BRUSH_PREVIEW 1
#define PATTERN_PREVIEW 2

typedef struct _DeviceInfo DeviceInfo;

struct _DeviceInfo {
  guint32 device;		/* device ID */
  gchar *name;
  
  short is_init;		/* has the user used it? */
  short is_present;		/* is the device currently present */
  
  /* gdk_input options - for not present devices */

  GdkInputMode mode;
  gint num_axes;
  GdkAxisUse *axes;
  gint num_keys;
  GdkDeviceKey *keys;

  GimpBrushP brush;
  GPatternP pattern;
  ToolType tool;
  unsigned char foreground[3];
};

typedef struct _DeviceInfoDialog DeviceInfoDialog;

struct _DeviceInfoDialog {
  int num_devices;
  guint32 current;
  guint32 *ids;

  GtkWidget *shell;
  GtkWidget *table;

  GtkWidget **frames;
  GtkWidget **tools;
  GtkWidget **colors;
  GtkWidget **brushes;
  GtkWidget **patterns;
  GtkWidget **eventboxes;

};

/* Local Data */

GList *devices_info = NULL;
DeviceInfoDialog *deviceD = NULL;
GtkWidget *device_bpopup = NULL; /* Can't share since differnt depths */
GtkWidget *device_bpreview = NULL;
GtkWidget *device_patpopup = NULL; 
GtkWidget *device_patpreview = NULL;
/* If true, don't update device information dialog */
int suppress_update = FALSE;

/* Local functions */
static void input_dialog_destroy_callback (GtkWidget *, gpointer);
void input_dialog_able_callback (GtkWidget *w, guint32 deviceid, 
				 gpointer data);
static void devices_save_current_info (void);
static void devices_write_rc_device (DeviceInfo *device_info, FILE *fp);
static void devices_write_rc (void);
static void device_status_destroy_callback (void);
static void devices_close_callback (GtkWidget *, gpointer);
static void device_status_update_current ();
static gint brush_preview_events (GtkWidget *, GdkEvent *, int);
static gint pattern_preview_events (GtkWidget *, GdkEvent *, int);

/* Global data */
int current_device = GDK_CORE_POINTER;

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { N_("Save"), (ActionCallback)devices_write_rc, NULL, NULL },
  { N_("Close"), devices_close_callback, NULL, NULL }
};

void 
create_input_dialog (void)
{
  static GtkWidget *inputd = NULL;
  GtkWidget        *hbbox;

  if (!inputd)
    {
      inputd = gtk_input_dialog_new();

      /* register this one only */
      dialog_register(inputd);

      gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG(inputd)->action_area), 2);
      gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (inputd)->action_area),
			       FALSE);

      hbbox = gtk_hbutton_box_new();
      gtk_button_box_set_spacing(GTK_BUTTON_BOX (hbbox), 4);

      gtk_widget_reparent (GTK_INPUT_DIALOG (inputd)->save_button, hbbox);
      GTK_WIDGET_SET_FLAGS (GTK_INPUT_DIALOG (inputd)->save_button,
			    GTK_CAN_DEFAULT);
      gtk_widget_reparent (GTK_INPUT_DIALOG (inputd)->close_button, hbbox);
      GTK_WIDGET_SET_FLAGS (GTK_INPUT_DIALOG (inputd)->close_button,
			    GTK_CAN_DEFAULT);

      gtk_box_pack_end(GTK_BOX (GTK_DIALOG (inputd)->action_area), hbbox,
		       FALSE, FALSE, 0);
      gtk_widget_grab_default (GTK_INPUT_DIALOG (inputd)->close_button);
      gtk_widget_show(hbbox);

      gtk_signal_connect (GTK_OBJECT(GTK_INPUT_DIALOG(inputd)->save_button),
			  "clicked",
			  GTK_SIGNAL_FUNC(devices_write_rc), NULL);
      gtk_signal_connect (GTK_OBJECT(GTK_INPUT_DIALOG(inputd)->close_button),
			  "clicked",
			  GTK_SIGNAL_FUNC(devices_close_callback), inputd);

      gtk_signal_connect (GTK_OBJECT(inputd), "destroy",
			  (GtkSignalFunc)input_dialog_destroy_callback, 
			  &inputd);
      gtk_signal_connect (GTK_OBJECT(inputd), "enable_device",
			  GTK_SIGNAL_FUNC(input_dialog_able_callback), NULL);
      gtk_signal_connect (GTK_OBJECT(inputd), "disable_device",
			  GTK_SIGNAL_FUNC(input_dialog_able_callback), NULL);
      gtk_widget_show (inputd);
    }
  else
    {
      if (!GTK_WIDGET_MAPPED(inputd))
	gtk_widget_show(inputd);
      else
	gdk_window_raise(inputd->window);
    }
}

void
input_dialog_able_callback (GtkWidget *w, guint32 deviceid, gpointer data)
{
  device_status_update (deviceid);
}

static void
input_dialog_destroy_callback (GtkWidget *w, 
			       gpointer call_data)
{
  *((GtkWidget **)call_data) = NULL;
}

void 
devices_init (void)
{
  GList *tmp_list;

  /* Create device info structures for present devices */

  tmp_list = gdk_input_list_devices ();
  
  while (tmp_list)
    {
      GdkDeviceInfo *gdk_info = (GdkDeviceInfo *)tmp_list->data;
      DeviceInfo *device_info = g_new (DeviceInfo, 1);

      device_info->device = gdk_info->deviceid;
      device_info->name = g_strdup (gdk_info->name);

      device_info->is_init = FALSE;
      device_info->is_present = TRUE;

      device_info->mode = gdk_info->mode;
      device_info->num_axes = gdk_info->num_axes;
      device_info->axes = NULL;
      
      device_info->brush = NULL;
      device_info->pattern = NULL;
      device_info->tool = RECT_SELECT;
      device_info->foreground[0] = 0;
      device_info->foreground[1] = 0;
      device_info->foreground[2] = 0;

      devices_info = g_list_append (devices_info, device_info);
	       
      tmp_list = tmp_list->next;
    }
}

void
devices_restore()
{  
  char *filename;
  GList *tmp_list;
  DeviceInfo *device_info;

  /* Augment with information from rc file */
  filename = gimp_personal_rc_file ("devicerc");
  parse_gimprc_file (filename);
  g_free (filename);

  tmp_list = devices_info;
  device_info = NULL;
  while (tmp_list)
    {
      if (((DeviceInfo *)tmp_list->data)->device == current_device)
	{
	  device_info = (DeviceInfo *)tmp_list->data;
	  break;
	}
      tmp_list = tmp_list->next;
    }

  g_return_if_fail (device_info != NULL);

  suppress_update = TRUE;

  if (device_info->is_init)
    {
      if (device_info->brush)
	select_brush (device_info->brush);
      gtk_widget_activate (tool_info[(int) device_info->tool].tool_widget);
      
      palette_set_foreground (device_info->foreground[0], 
			      device_info->foreground[1], 
			      device_info->foreground[2]);
      
      if (device_info->pattern)
	select_pattern(device_info->pattern);
    }
  
  suppress_update = FALSE;
}

void
devices_rc_update (gchar *name, DeviceValues values,
		   GdkInputMode mode, gint num_axes,
		   GdkAxisUse *axes, gint num_keys, GdkDeviceKey *keys,
		   gchar *brush_name, ToolType tool,
		   guchar foreground[],gchar *pattern_name)
{
  GList *tmp_list;
  DeviceInfo *device_info;

  /* Find device if we have it */

  tmp_list = devices_info;
  device_info = NULL;
  while (tmp_list)
    {
      if (!strcmp (((DeviceInfo *)tmp_list->data)->name, name))
	{
	  device_info = (DeviceInfo *)tmp_list->data;
	  break;
	}
      tmp_list = tmp_list->next;
    }

  if (!device_info)
    {
      device_info = g_new (DeviceInfo, 1);
      device_info->name = g_strdup (name);
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
	  device_info->axes = NULL;
	}

      if (values & DEVICE_KEYS)
	{
	  device_info->num_keys = num_keys;
	  device_info->keys = g_new (GdkDeviceKey, num_keys);
	  memcpy (device_info->keys, axes, num_keys * sizeof (GdkDeviceKey));
	}

      if (values & DEVICE_MODE)
	device_info->mode = mode;
      else
	device_info->mode = GDK_MODE_DISABLED;

      device_info->brush = NULL;
      device_info->pattern = NULL;
      device_info->tool = RECT_SELECT;
      device_info->foreground[0] = 0;
      device_info->foreground[1] = 0;
      device_info->foreground[2] = 0;

      devices_info = g_list_append (devices_info, device_info);
    }
  else
    {
      GdkDeviceInfo *gdk_info = NULL;	/* Quiet gcc */

      tmp_list = gdk_input_list_devices();
      while (tmp_list)
	{
	  GdkDeviceInfo *info = (GdkDeviceInfo *)tmp_list->data;
	  
	  if (info->deviceid == device_info->device)
	    {
	      gdk_info = info;
	      break;
	    }
	  tmp_list = tmp_list->next;
	}

      if (gdk_info != NULL)
	{
	  if (values & DEVICE_MODE)
	    gdk_input_set_mode (gdk_info->deviceid, mode);
	  
	  if ((values & DEVICE_AXES) && num_axes >= gdk_info->num_axes)
	    gdk_input_set_axes (gdk_info->deviceid, axes);
	  
	  if ((values & DEVICE_KEYS) && num_keys >= gdk_info->num_keys)
	    {
	      int i;
	      
	      for (i=0; i<MAX (num_keys, gdk_info->num_keys); i++)
		gdk_input_set_key (gdk_info->deviceid, i,
				   keys[i].keyval, keys[i].modifiers);
	    }
	}
      else
	{
	  g_warning (_("devices_rc_update called multiple times for not present device\n"));
	  return;
	}
    }

  if (values & (DEVICE_PATTERN | DEVICE_BRUSH | DEVICE_TOOL | DEVICE_FOREGROUND))
    device_info->is_init = TRUE;


  if (values & DEVICE_BRUSH)
    {
      device_info->brush = gimp_brush_list_get_brush(brush_list, brush_name);
    }

  if (values & DEVICE_PATTERN)
    {
      device_info->pattern = pattern_list_get_pattern(pattern_list,pattern_name);
    }

  if (values & DEVICE_TOOL)
    device_info->tool = tool;

  if (values & DEVICE_FOREGROUND)
    {
      device_info->foreground[0] = foreground[0];
      device_info->foreground[1] = foreground[1];
      device_info->foreground[2] = foreground[2];
    }
}

void 
select_device (guint32 new_device)
{
  GList *tmp_list;
  DeviceInfo *device_info;

  suppress_update = TRUE;
  
  /* store old information */

  devices_save_current_info();
  
  /* Now see if there is already information about the new device */

  tmp_list = devices_info;
  device_info = NULL;
  while (tmp_list)
    {
      if (((DeviceInfo *)tmp_list->data)->device == new_device)
	{
	  device_info = (DeviceInfo *)tmp_list->data;
	  break;
	}
      tmp_list = tmp_list->next;
    }

  current_device = new_device;
  
  if (device_info->is_init)
    {
      if (device_info->brush)
	select_brush (device_info->brush);
      gtk_widget_activate (tool_info[(int) device_info->tool].tool_widget);
      palette_set_foreground (device_info->foreground[0], 
			      device_info->foreground[1], 
			      device_info->foreground[2]);
      if (device_info->pattern)
	select_pattern(device_info->pattern);
    }

  suppress_update = FALSE;

  device_status_update_current ();
}

gint
devices_check_change (GdkEvent *event)
{
  guint32 device;

  switch (event->type)
    {
    case GDK_MOTION_NOTIFY:
      device = ((GdkEventMotion *)event)->deviceid;
      break;
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      device = ((GdkEventButton *)event)->deviceid;
      break;
    case GDK_PROXIMITY_OUT:
      device = ((GdkEventProximity *)event)->deviceid;
      break;
    default:
      device = current_device;
    }
  
  if (device != current_device)
    {
      select_device (device);
      return TRUE;
    }
  else
    return FALSE;
}

static void
devices_save_current_info (void)
{
  GList *tmp_list;
  DeviceInfo *device_info;
  
  tmp_list = devices_info;
  device_info = NULL;
  while (tmp_list)
    {
      if (((DeviceInfo *)tmp_list->data)->device == current_device)
	{
	  device_info = (DeviceInfo *)tmp_list->data;
	  break;
	}
      tmp_list = tmp_list->next;
    }

  device_info->is_init = TRUE;
  device_info->device = current_device;
  device_info->brush = get_active_brush ();
  device_info->pattern = get_active_pattern();
  if (active_tool_type >= 0)
    device_info->tool = active_tool_type;
  else
    device_info->tool = RECT_SELECT;
  palette_get_foreground (&device_info->foreground[0], 
			  &device_info->foreground[1], 
			  &device_info->foreground[2]);
}

static void
devices_write_rc_device (DeviceInfo *device_info, FILE *fp)
{
  GdkDeviceInfo *gdk_info;
  GList *tmp_list;
  gchar *mode = NULL;		/* Quiet gcc */
  int i;

  gdk_info = NULL;
  if (device_info->is_present)
    {
      /* gdk_input_list_devices returns an internal list, so we shouldn't
	 free it afterwards */
      tmp_list = gdk_input_list_devices();
      while (tmp_list)
	{
	  GdkDeviceInfo *info = (GdkDeviceInfo *)tmp_list->data;
	  
	  if (info->deviceid == device_info->device)
	    {
	      gdk_info = info;
	      break;
	    }
	  tmp_list = tmp_list->next;
	}
    }
  
  fprintf(fp, "(device \"%s\"",device_info->name);

  switch (gdk_info ? gdk_info->mode : device_info->mode)
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
  
  fprintf(fp, "\n        (mode %s)",mode);

  fprintf(fp, "\n        (axes %d",gdk_info ? gdk_info->num_axes : device_info->num_axes);

  for (i=0; i<(gdk_info ? gdk_info->num_axes : device_info->num_axes); i++)
    {
      gchar *axis_type = NULL;	/* Quiet gcc */
      
      switch (gdk_info ? gdk_info->axes[i] : device_info->axes[i])
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
      fprintf(fp, " %s",axis_type);
    }
  fprintf(fp,")");

  fprintf(fp, "\n        (keys %d", gdk_info ? gdk_info->num_keys : device_info->num_keys);

  for (i=0; i<(gdk_info ? gdk_info->num_keys : device_info->num_keys); i++)
    {
      GdkModifierType modifiers = gdk_info ? gdk_info->keys[i].modifiers :
	device_info->keys[i].modifiers;
      guint keyval = gdk_info ? gdk_info->keys[i].keyval :
	device_info->keys[i].keyval;
	
      if (keyval)
	{
	  /* FIXME: integrate this back with menus_install_accelerator */
	  char accel[64];
	  char t2[2];

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
  fprintf(fp,")");

  if (device_info->is_init)
    {
      if (device_info->brush)
        fprintf(fp, "\n        (brush \"%s\")",device_info->brush->name);
      if (device_info->pattern)
        fprintf(fp, "\n        (pattern \"%s\")",device_info->pattern->name);
      /* Fixme: hard coded last tool....  see gimprc */
      if (device_info->tool && device_info->tool <= LAST_TOOLBOX_TOOL)
        fprintf(fp, "\n        (tool \"%s\")",
	      tool_info[device_info->tool].tool_name);
      fprintf(fp, "\n        (foreground %d %d %d)",
	      device_info->foreground[0],
	      device_info->foreground[1],
	      device_info->foreground[2]);
    }
  fprintf(fp,")\n");
  
}

static void
devices_write_rc (void)
{
  char *filename;
  FILE *fp;

  devices_save_current_info ();

  filename = gimp_personal_rc_file ("devicerc");
  fp = fopen (filename, "wb");
  g_free (filename);

  if (!fp)
    return;

  g_list_foreach (devices_info, (GFunc)devices_write_rc_device, fp);

  fclose (fp);
}

void 
create_device_status (void)
{
  GtkWidget *label;
  GList *tmp_list;
  DeviceInfo *device_info;
  int i;
  
  if (deviceD == NULL)
    {
      deviceD = g_new (DeviceInfoDialog, 1);
      deviceD->shell = gtk_dialog_new ();

      /* register this one only */
      dialog_register(deviceD->shell);

      gtk_window_set_title (GTK_WINDOW(deviceD->shell), _("Device Status"));
      gtk_window_set_policy (GTK_WINDOW (deviceD->shell), FALSE, FALSE, TRUE);
      /*  don't set the dialog's size, as the number of devices may have
       *  changed since the last session
       */
      session_set_window_geometry (deviceD->shell,
				   &device_status_session_info, FALSE);

      deviceD->num_devices = 0;
      tmp_list = devices_info;
      while (tmp_list)
	{
	  if (((DeviceInfo *)tmp_list->data)->is_present)
	    deviceD->num_devices++;
	  tmp_list = tmp_list->next;
	}
/* devices table */
      deviceD->table = gtk_table_new (deviceD->num_devices, 5, FALSE);
      gtk_container_border_width (GTK_CONTAINER(deviceD->table), 3);
      gtk_container_add (GTK_CONTAINER(GTK_DIALOG(deviceD->shell)->vbox),
			 deviceD->table);
      gtk_widget_realize (deviceD->table);
      gtk_widget_show (deviceD->table);

      deviceD->ids = g_new (guint32, deviceD->num_devices);
      deviceD->frames = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->tools = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->colors = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->brushes = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->patterns = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->eventboxes = g_new (GtkWidget *, deviceD->num_devices);

      tmp_list = devices_info;
      i=0;
      while (tmp_list)
	{
	  if (!((DeviceInfo *)tmp_list->data)->is_present)
	    {
	      tmp_list = tmp_list->next;
	      i++;
	      continue;
	    }
	  
	  device_info = (DeviceInfo *)tmp_list->data;

	  deviceD->ids[i] = device_info->device;

	  deviceD->frames[i] = gtk_frame_new (NULL);
	  
	  gtk_frame_set_shadow_type (GTK_FRAME(deviceD->frames[i]), GTK_SHADOW_OUT);
	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->frames[i],
			    0, 1, i, i+1,
			    GTK_FILL, GTK_FILL, 2, 0);

	  label = gtk_label_new (device_info->name);
	  gtk_misc_set_padding (GTK_MISC(label), 2, 0);
	  gtk_container_add (GTK_CONTAINER(deviceD->frames[i]), label);
	  gtk_widget_show(label);

	  deviceD->eventboxes[i] = gtk_event_box_new();

	  deviceD->tools[i] = gtk_pixmap_new (create_tool_pixmap(deviceD->table,
								 RECT_SELECT),
					      NULL);

	  gtk_container_add (GTK_CONTAINER (deviceD->eventboxes[i]),deviceD->tools[i]);

	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->eventboxes[i],
			    1, 2, i, i+1,
			    0, 0, 2, 2);

	  deviceD->colors[i] = gtk_preview_new (GTK_PREVIEW_COLOR);
	  gtk_widget_set_events (deviceD->colors[i], PREVIEW_EVENT_MASK);
	  gtk_preview_size (GTK_PREVIEW (deviceD->colors[i]), CELL_SIZE, CELL_SIZE);
	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->colors[i],
			    2, 3, i, i+1,
			    0, 0, 2, 2);

	  deviceD->brushes[i] = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
          gtk_preview_size (GTK_PREVIEW (deviceD->brushes[i]), CELL_SIZE, CELL_SIZE); 
	  gtk_widget_set_events (deviceD->brushes[i], PREVIEW_EVENT_MASK);
	  gtk_signal_connect (GTK_OBJECT (deviceD->brushes[i]), "event",
			      (GtkSignalFunc) brush_preview_events,
			      (gpointer)i);

	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->brushes[i],
			    3, 4, i, i+1,
			    0, 0, 2, 2);


	  deviceD->patterns[i] = gtk_preview_new (GTK_PREVIEW_COLOR);
          gtk_preview_size (GTK_PREVIEW (deviceD->patterns[i]), CELL_SIZE, CELL_SIZE); 
	  gtk_widget_set_events (deviceD->patterns[i], PREVIEW_EVENT_MASK);
	  gtk_signal_connect (GTK_OBJECT (deviceD->patterns[i]), "event",
			      (GtkSignalFunc) pattern_preview_events,
			      (gpointer)i);

	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->patterns[i],
			    4, 5, i, i+1,
			    0, 0, 2, 2);


	  if (device_info->is_present)
	    device_status_update (device_info->device);

	  
	  tmp_list = tmp_list->next;
	  i++;
	}

      action_items[1].user_data = deviceD->shell;
      build_action_area (GTK_DIALOG (deviceD->shell), action_items, 2, 1);

      deviceD->current = 0xffffffff; /* random, but doesn't matter */
      device_status_update_current ();

      gtk_widget_show (deviceD->shell);

      gtk_signal_connect (GTK_OBJECT (deviceD->shell), "destroy",
			  GTK_SIGNAL_FUNC(device_status_destroy_callback),
			  NULL);
    }
  else
    {
      if (!GTK_WIDGET_MAPPED(deviceD->shell))
	gtk_widget_show(deviceD->shell);
      else
	gdk_window_raise(deviceD->shell->window);
    }
}

static void
device_status_destroy_callback (void)
{
  g_free(deviceD->ids);
  g_free(deviceD->frames);
  g_free(deviceD->tools);
  g_free(deviceD->eventboxes);
  g_free(deviceD->colors);
  g_free(deviceD->brushes);
  g_free(deviceD->patterns);

  g_free(deviceD);
  deviceD = NULL;
}

static void
devices_close_callback (GtkWidget *w,
			gpointer data)
{
  gtk_widget_hide (GTK_WIDGET(data));
}

void
device_status_free (void)
{                                     
  /* Save device status on exit */
  if(save_device_status)
    devices_write_rc();

  if (deviceD)
    {
      session_get_window_info (deviceD->shell, &device_status_session_info);
      device_status_destroy_callback (); 
    }
}

static void
device_status_update_current ()
{
  int i;

  if (deviceD)
    {
      for (i=0; i<deviceD->num_devices; i++)
	{
	  if (deviceD->ids[i] == deviceD->current)
	    gtk_frame_set_shadow_type (GTK_FRAME(deviceD->frames[i]), 
				       GTK_SHADOW_OUT);
	  else if (deviceD->ids[i] == current_device)
	    gtk_frame_set_shadow_type (GTK_FRAME(deviceD->frames[i]), 
				       GTK_SHADOW_IN);
	}
      deviceD->current = current_device;
    }
}

static void
brush_popup_open (int preview_id,
		  int          x,
		  int          y,
		  GimpBrushP   brush)
{
  gint x_org, y_org;
  gint scr_w, scr_h;
  gchar *src, *buf;

  /* make sure the popup exists and is not visible */
  if (device_bpopup == NULL)
    {
      GtkWidget *frame;

      device_bpopup = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_policy (GTK_WINDOW (device_bpopup), FALSE, FALSE, TRUE);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (device_bpopup), frame);
      gtk_widget_show (frame);
      device_bpreview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
      gtk_container_add (GTK_CONTAINER (frame), device_bpreview);
      gtk_widget_show (device_bpreview);
    }
  else
    {
      gtk_widget_hide (device_bpopup);
    }

  /* decide where to put the popup */
  gdk_window_get_origin (deviceD->brushes[preview_id]->window, &x_org, &y_org);
  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();
  x = x_org + x - (brush->mask->width >> 1);
  y = y_org + y - (brush->mask->height >> 1);
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + brush->mask->width > scr_w) ? scr_w - brush->mask->width : x;
  y = (y + brush->mask->height > scr_h) ? scr_h - brush->mask->height : y;
  gtk_preview_size (GTK_PREVIEW (device_bpreview), brush->mask->width, brush->mask->height);

  gtk_widget_popup (device_bpopup, x, y);
  
  /*  Draw the brush  */
  buf = g_new (gchar, brush->mask->width);
  src = (gchar *)temp_buf_data (brush->mask);
  for (y = 0; y < brush->mask->height; y++)
    {
      /*  Invert the mask for display.  We're doing this because
       *  a value of 255 in the  mask means it is full intensity.
       *  However, it makes more sense for full intensity to show
       *  up as black in this brush preview window...
       */
      for (x = 0; x < brush->mask->width; x++)
	buf[x] = 255 - src[x];
      gtk_preview_draw_row (GTK_PREVIEW (device_bpreview), (guchar *)buf, 0, y, brush->mask->width);
      src += brush->mask->width;
    }
  g_free(buf);
  
  /*  Draw the brush preview  */
  gtk_widget_draw (device_bpreview, NULL);
}

static void
pattern_popup_open (int preview_id,
		  int          x,
		  int          y,
		  GPatternP   pattern)
{
  gint x_org, y_org;
  gint scr_w, scr_h;
  gchar *src, *buf;

  /* make sure the popup exists and is not visible */
  if (device_patpopup == NULL)
    {
      GtkWidget *frame;

      device_patpopup = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_policy (GTK_WINDOW (device_patpopup), FALSE, FALSE, TRUE);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (device_patpopup), frame);
      gtk_widget_show (frame);
      device_patpreview = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_container_add (GTK_CONTAINER (frame), device_patpreview);
      gtk_widget_show (device_patpreview);
    }
  else
    {
      gtk_widget_hide (device_patpopup);
    }

  /* decide where to put the popup */
  gdk_window_get_origin (deviceD->patterns[preview_id]->window, &x_org, &y_org);
  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();
  x = x_org + x - (pattern->mask->width >> 1);
  y = y_org + y - (pattern->mask->height >> 1);
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + pattern->mask->width > scr_w) ? scr_w - pattern->mask->width : x;
  y = (y + pattern->mask->height > scr_h) ? scr_h - pattern->mask->height : y;
  gtk_preview_size (GTK_PREVIEW (device_patpreview), pattern->mask->width, pattern->mask->height);

  gtk_widget_popup (device_patpopup, x, y);
  
  /*  Draw the pattern  */
  buf = g_new (gchar, pattern->mask->width * 3);
  src = (gchar *)temp_buf_data (pattern->mask);
  for (y = 0; y < pattern->mask->height; y++)
    {
      if (pattern->mask->bytes == 1)
	for (x = 0; x < pattern->mask->width; x++)
	  {
	    buf[x*3+0] = src[x];
	    buf[x*3+1] = src[x];
	    buf[x*3+2] = src[x];
	  }
      else
	for (x = 0; x < pattern->mask->width; x++)
	  {
	    buf[x*3+0] = src[x*3+0];
	    buf[x*3+1] = src[x*3+1];
	    buf[x*3+2] = src[x*3+2];
	  }
      gtk_preview_draw_row (GTK_PREVIEW (device_patpreview), (guchar *)buf, 0, y, pattern->mask->width);
      src += pattern->mask->width * pattern->mask->bytes;
    }
  g_free(buf);
  
  /*  Draw the brush preview  */
  gtk_widget_draw (device_patpreview, NULL);
}

static void
device_popup_close (int type)
{
  if(type == BRUSH_PREVIEW)
    {
      if (device_bpopup != NULL)
	gtk_widget_hide (device_bpopup);
    }
  else if (type == PATTERN_PREVIEW)
    {
      if (device_patpopup != NULL)
	gtk_widget_hide (device_patpopup);
    }
}

static gint 
device_preview_events (GtkWidget    *widget,
		       GdkEvent     *event,
		       int deviceid,
		       int type)
{
  GList *tmp_list;
  DeviceInfo *device_info;
  GdkEventButton *bevent;
  GimpBrushP brush;
  GPatternP pattern;

  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	  tmp_list = devices_info;
	  while (tmp_list)
	    {
	      device_info = (DeviceInfo *)tmp_list->data;
	      
	      if (device_info->device == deviceD->ids[deviceid])
		break;
	      
	      tmp_list = tmp_list->next;
	    }

	  if(tmp_list == NULL)
	    {
	      g_message(_("Failed to find device_info\n"));
	      break; /* Error no device info */
	    }
	    
	  /*  Grab the pointer  */
	  gdk_pointer_grab (widget->window, FALSE,
			    (GDK_POINTER_MOTION_HINT_MASK |
			     GDK_BUTTON1_MOTION_MASK |
			     GDK_BUTTON_RELEASE_MASK),
			    NULL, NULL, bevent->time);

	  if(type == BRUSH_PREVIEW)
	    {
	      brush = device_info->brush;
	      
	      /*  Show the brush popup window if the brush is too large  */
	      if (brush->mask->width > CELL_SIZE ||
		  brush->mask->height > CELL_SIZE)
		brush_popup_open (deviceid, bevent->x, bevent->y, brush);
	    }
	  else if(type == PATTERN_PREVIEW)
	    {
	      pattern = device_info->pattern;
	      
	      /*  Show the pattern popup window if the pattern is too large  */
	      if (pattern->mask->width > CELL_SIZE ||
		  pattern->mask->height > CELL_SIZE)
		pattern_popup_open (deviceid, bevent->x, bevent->y, pattern);
	    }
	}
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	  /*  Ungrab the pointer  */
	  gdk_pointer_ungrab (bevent->time);

	  /*  Close the device preview popup window  */
	  device_popup_close (type);
	}
      break;
    case GDK_DELETE:
      break;

    default:
      break;
    }

  return FALSE;
}

static gint
brush_preview_events (GtkWidget    *widget,
		     GdkEvent     *event,
		     int deviceid)
{
  return(device_preview_events(widget,event,deviceid,BRUSH_PREVIEW));
}

static gint
pattern_preview_events (GtkWidget    *widget,
		     GdkEvent     *event,
		     int deviceid)
{
  return(device_preview_events(widget,event,deviceid,PATTERN_PREVIEW));
}

static void
device_update_brush(GimpBrushP brush,int preview_id)
{
  guchar buffer[CELL_SIZE]; 
  TempBuf * brush_buf;
  unsigned char * src, *s = NULL;
  unsigned char *b;
  int width,height;
  int offset_x, offset_y;
  int yend;
  int ystart;
  int i, j;

  /* Set the tip to be the name of the brush */
  gtk_tooltips_set_tip(tool_tips,deviceD->brushes[preview_id],brush->name,NULL);

  brush_buf = brush->mask;

  /*  Get the pointer into the brush mask data  */
  src = mask_buf_data (brush_buf);

  /* Limit to cell size */
  width = (brush_buf->width > CELL_SIZE) ? CELL_SIZE: brush_buf->width;
  height = (brush_buf->height > CELL_SIZE) ? CELL_SIZE: brush_buf->height;

  /* Set buffer to white */  
  memset(buffer, 255, sizeof(buffer));
  for (i = 0; i < CELL_SIZE; i++)
    gtk_preview_draw_row (GTK_PREVIEW(deviceD->brushes[preview_id]), buffer, 0, i, CELL_SIZE);

  offset_x = ((CELL_SIZE - width) >> 1);
  offset_y = ((CELL_SIZE - height) >> 1);

  ystart = BOUNDS (offset_y, 0, CELL_SIZE);
  yend = BOUNDS (offset_y + height, 0, CELL_SIZE);

  for (i = ystart; i < yend; i++)
    {
      /*  Invert the mask for display.  We're doing this because
       *  a value of 255 in the  mask means it is full intensity.
       *  However, it makes more sense for full intensity to show
       *  up as black in this brush preview window...
       */
      s = src;
      b = buffer;
      for (j = 0; j < width ; j++)
	*b++ = 255 - *s++;

      gtk_preview_draw_row (GTK_PREVIEW(deviceD->brushes[preview_id]), 
			    buffer, 
			    offset_x, i, width);

      src += brush_buf->width;
    }
}

static void
device_update_pattern(GPatternP pattern,int preview_id)
{
  guchar *buffer = NULL; 
  TempBuf * pattern_buf;
  unsigned char * src, *s = NULL;
  unsigned char *b;
  int rowstride;
  int width,height;
  int offset_x, offset_y;
  int yend;
  int ystart;
  int i, j;

  /* Set the tip to be the name of the brush */
  gtk_tooltips_set_tip(tool_tips,deviceD->patterns[preview_id],pattern->name,NULL);

  buffer = g_new (guchar, pattern->mask->width * 3);
  pattern_buf = pattern->mask;

  /* Limit to cell size */
  width = (pattern_buf->width > CELL_SIZE) ? CELL_SIZE: pattern_buf->width;
  height = (pattern_buf->height > CELL_SIZE) ? CELL_SIZE: pattern_buf->height;

  /* Set buffer to white */  
  memset(buffer, 255, sizeof(buffer));
  for (i = 0; i < CELL_SIZE; i++)
    gtk_preview_draw_row (GTK_PREVIEW(deviceD->patterns[preview_id]), buffer, 0, i, CELL_SIZE);

  offset_x = ((CELL_SIZE - width) >> 1);
  offset_y = ((CELL_SIZE - height) >> 1);

  ystart = BOUNDS (offset_y, 0, CELL_SIZE);
  yend = BOUNDS (offset_y + height, 0, CELL_SIZE);

  /*  Get the pointer into the brush mask data  */
  rowstride = pattern_buf->width * pattern_buf->bytes;
  src = mask_buf_data (pattern_buf)  + (ystart - offset_y) * rowstride;

  for (i = ystart; i < yend; i++)
    {
      /*  Invert the mask for display.  We're doing this because
       *  a value of 255 in the  mask means it is full intensity.
       *  However, it makes more sense for full intensity to show
       *  up as black in this brush preview window...
       */
      s = src;
      b = buffer;

      if (pattern_buf->bytes == 1)
	for (j = 0; j < width; j++)
	  {
	    *b++ = *s;
	    *b++ = *s;
	    *b++ = *s++;
	  }
      else
	for (j = 0; j < width; j++)
	  {
	    *b++ = *s++;
	    *b++ = *s++;
	    *b++ = *s++;
	  }

      gtk_preview_draw_row (GTK_PREVIEW(deviceD->patterns[preview_id]), 
			    buffer, 
			    offset_x, i, width);

      src += rowstride;
    }
  g_free(buffer);
}

void 
device_status_update (guint32 deviceid)
{
  int i, j;
  guchar buffer[CELL_SIZE*3];
  gchar ttbuf[20]; /* [xxx,xxx,xxx] + null */
  GList *tmp_list;
  DeviceInfo *device_info;
  GdkDeviceInfo *gdk_info;

  if (deviceD && !suppress_update)
    {
      if (deviceid == current_device)
	{
	  devices_save_current_info();
	}
      
      tmp_list = devices_info;
      while (tmp_list)
	{
	  device_info = (DeviceInfo *)tmp_list->data;
	  
	  if (device_info->device == deviceid)
	    break;

	  tmp_list = tmp_list->next;
	}
      
      g_return_if_fail (tmp_list != NULL);

      tmp_list = gdk_input_list_devices();
      while (tmp_list)
	{
	  gdk_info = (GdkDeviceInfo *)tmp_list->data;
	  
	  if (gdk_info->deviceid == deviceid)
	    break;

	  tmp_list = tmp_list->next;
	}
      
      g_return_if_fail (tmp_list != NULL);
      
      for (i=0; i<deviceD->num_devices; i++)
	{
	  if (deviceD->ids[i] == deviceid)
	    break;
	}
      
      g_return_if_fail (i<deviceD->num_devices);
      
      if (gdk_info->mode == GDK_MODE_DISABLED)
	{
	  gtk_widget_hide (deviceD->frames[i]);
	  gtk_widget_hide (deviceD->tools[i]);
	  gtk_widget_hide (deviceD->eventboxes[i]);
	  gtk_widget_hide (deviceD->colors[i]);
	  gtk_widget_hide (deviceD->brushes[i]);
	  gtk_widget_hide (deviceD->patterns[i]);
	}
      else
	{
	  gtk_widget_show (deviceD->frames[i]);

	  gtk_pixmap_set (GTK_PIXMAP (deviceD->tools[i]),
			  create_tool_pixmap (deviceD->table, 
					      device_info->tool),
			  NULL);
	  
	  gtk_widget_draw (deviceD->tools[i], NULL);
	  gtk_widget_show (deviceD->tools[i]);
	  gtk_widget_show (deviceD->eventboxes[i]);
	  
	  gtk_tooltips_set_tip(tool_tips,deviceD->eventboxes[i],
			       tool_info[(int) device_info->tool].tool_desc,
			       NULL);

	  if(!device_info->is_init)
	    return; /* None of the entries have been set */

	  for (j=0;j<CELL_SIZE*3;j+=3)
	    {
	      buffer[j] = device_info->foreground[0];
	      buffer[j+1] = device_info->foreground[1];
	      buffer[j+2] = device_info->foreground[2];
	    }
	  
	  for (j=0;j<CELL_SIZE;j++)
	    gtk_preview_draw_row (GTK_PREVIEW(deviceD->colors[i]), buffer, 
				  0, j, CELL_SIZE);
	  gtk_widget_draw (deviceD->colors[i], NULL);
	  gtk_widget_show (deviceD->colors[i]);

	  /* Set the tip to be the RGB value */
	  sprintf(ttbuf,"[%3d,%3d,%3d]",
		  device_info->foreground[0],
		  device_info->foreground[1],
		  device_info->foreground[2]);
		  
	  gtk_tooltips_set_tip(tool_tips,deviceD->colors[i],ttbuf,NULL);


	  device_update_brush(device_info->brush,i);
	  gtk_widget_draw (deviceD->brushes[i],NULL);
	  gtk_widget_show (deviceD->brushes[i]);

	  device_update_pattern(device_info->pattern,i);
	  gtk_widget_draw (deviceD->patterns[i],NULL);
	  gtk_widget_show (deviceD->patterns[i]);
	}
    }
  brush_area_update();
}




