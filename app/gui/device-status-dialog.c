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

#include "appenv.h"
#include "devices.h"
#include "dialog_handler.h"
#include "gimpcontextpreview.h"
#include "gimpdnd.h"
#include "gimpbrushlist.h"
#include "gimpcontext.h"
#include "gimprc.h"
#include "gimpui.h"
#include "gradient.h"
#include "gradient_header.h"
#include "interface.h"
#include "session.h"
#include "tools.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimpenv.h"

#define CELL_SIZE 20 /* The size of the preview cells */

#define PREVIEW_EVENT_MASK  GDK_BUTTON_PRESS_MASK | \
			    GDK_BUTTON_RELEASE_MASK | \
			    GDK_ENTER_NOTIFY_MASK | \
                            GDK_LEAVE_NOTIFY_MASK

#define DEVICE_CONTEXT_MASK GIMP_CONTEXT_TOOL_MASK | \
                            GIMP_CONTEXT_FOREGROUND_MASK | \
			    GIMP_CONTEXT_BRUSH_MASK | \
			    GIMP_CONTEXT_PATTERN_MASK | \
                            GIMP_CONTEXT_GRADIENT_MASK

typedef struct _DeviceInfo DeviceInfo;

struct _DeviceInfo
{
  guint32       device;      /*  device ID  */
  gchar        *name;

  gshort        is_present;  /*  is the device currently present  */

  /*  gdk_input options - for not present devices  */

  GdkInputMode  mode;
  gint          num_axes;
  GdkAxisUse   *axes;
  gint          num_keys;
  GdkDeviceKey *keys;

  GimpContext  *context;
};

typedef struct _DeviceInfoDialog DeviceInfoDialog;

struct _DeviceInfoDialog
{
  gint       num_devices;

  guint32    current;
  guint32   *ids;

  GtkWidget *shell;
  GtkWidget *table;

  GtkWidget **frames;
  GtkWidget **tools;
  GtkWidget **colors;
  GtkWidget **brushes;
  GtkWidget **patterns;
  GtkWidget **gradients;
  GtkWidget **eventboxes;
};

/*  local functions */
static void     input_dialog_able_callback     (GtkWidget *w, guint32 deviceid, 
						gpointer data);

static void     devices_write_rc_device        (DeviceInfo *device_info,
						FILE *fp);
static void     devices_write_rc               (void);

static void     device_status_destroy_callback (void);
static void     devices_close_callback         (GtkWidget *, gpointer);

static void     device_status_update           (guint32 deviceid);
static void     device_status_update_current   (void);

static ToolType device_status_drag_tool        (GtkWidget *,
						gpointer);
static void     device_status_drop_tool        (GtkWidget *,
						ToolType,
						gpointer);
static void     device_status_drag_color       (GtkWidget *,
						guchar *, guchar *, guchar *,
						gpointer);
static void     device_status_drop_color       (GtkWidget *,
						guchar, guchar, guchar,
						gpointer);
static void     device_status_drop_brush       (GtkWidget *,
						GimpBrush *,
						gpointer);
static void     device_status_drop_pattern     (GtkWidget *,
						GPattern *,
						gpointer);
static void     device_status_drop_gradient    (GtkWidget *,
						gradient_t *,
						gpointer);

static void     device_status_color_changed    (GimpContext *context,
						gint         r,
						gint         g,
						gint         b,
						gpointer     data);
static void     device_status_data_changed     (GimpContext *context,
						gpointer     dummy,
						gpointer     data);

static void     device_status_context_connect  (GimpContext *context,
						guint32      deviceid);

/*  global data  */
gint current_device = GDK_CORE_POINTER;

/*  local data  */
static GList            *device_info_list = NULL;
static DeviceInfoDialog *deviceD          = NULL;

/*  if true, don't update device information dialog */
static gboolean          suppress_update  = FALSE;

/*  dnd stuff  */
static GtkTargetEntry tool_target_table[] =
{
  GIMP_TARGET_TOOL
};
static guint n_tool_targets = (sizeof (tool_target_table) /
			       sizeof (tool_target_table[0]));

static GtkTargetEntry color_area_target_table[] =
{
  GIMP_TARGET_COLOR
};
static guint n_color_area_targets = (sizeof (color_area_target_table) /
				     sizeof (color_area_target_table[0]));

/*  utility functions for the device lists  */

static GdkDeviceInfo *
gdk_device_info_get_by_id (guint32 deviceid)
{
  GdkDeviceInfo *info;
  GList *list;

  for (list = gdk_input_list_devices (); list; list = g_list_next (list))
    {
      info = (GdkDeviceInfo *) list->data;

      if (info->deviceid == deviceid)
	return info;
    }

  return NULL;
}

static DeviceInfo *
device_info_get_by_id (guint32 deviceid)
{
  DeviceInfo *info;
  GList *list;

  for (list = device_info_list; list; list = g_list_next (list))
    {
      info = (DeviceInfo *) list->data;

      if (info->device == deviceid)
	return info;
    }

  return NULL;
}

static DeviceInfo *
device_info_get_by_name (gchar *name)
{
  DeviceInfo *info;
  GList *list;

  for (list = device_info_list; list; list = g_list_next (list))
    {
      info = (DeviceInfo *) list->data;

      if (!strcmp (info->name, name))
	return info;
    }

  return NULL;
}

/*  the gtk input dialog  */

void 
input_dialog_create (void)
{
  static GtkWidget *inputd = NULL;
  GtkWidget        *hbbox;

  if (!inputd)
    {
      inputd = gtk_input_dialog_new ();

      /* register this one only */
      dialog_register (inputd);

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

      gtk_signal_connect (GTK_OBJECT (GTK_INPUT_DIALOG (inputd)->save_button),
			  "clicked",
			  GTK_SIGNAL_FUNC (devices_write_rc),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (GTK_INPUT_DIALOG (inputd)->close_button),
			  "clicked",
			  GTK_SIGNAL_FUNC (devices_close_callback),
			  inputd);

      gtk_signal_connect (GTK_OBJECT (inputd), "destroy",
			  GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			  &inputd);

      gtk_signal_connect (GTK_OBJECT (inputd), "enable_device",
			  GTK_SIGNAL_FUNC (input_dialog_able_callback),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (inputd), "disable_device",
			  GTK_SIGNAL_FUNC (input_dialog_able_callback),
			  NULL);

      /*  Connect the "F1" help key  */
      gimp_help_connect_help_accel (inputd,
				    gimp_standard_help_func,
				    "dialogs/input_devices.html");

      gtk_widget_show (inputd);
    }
  else
    {
      if (!GTK_WIDGET_MAPPED (inputd))
	gtk_widget_show (inputd);
      else
	gdk_window_raise (inputd->window);
    }
}

static void
input_dialog_able_callback (GtkWidget *widget,
			    guint32    deviceid,
			    gpointer   data)
{
  device_status_update (deviceid);
}

void 
devices_init (void)
{
  GdkDeviceInfo *gdk_info;
  DeviceInfo    *device_info;
  GList         *list;

  /*  create device info structures for present devices */
  for (list = gdk_input_list_devices (); list; list = g_list_next (list))
    {
      gdk_info = (GdkDeviceInfo *) list->data;

      device_info = g_new (DeviceInfo, 1);

      device_info->device     = gdk_info->deviceid;
      device_info->name       = g_strdup (gdk_info->name);
      device_info->is_present = TRUE;

      device_info->mode       = gdk_info->mode;
      device_info->num_axes   = gdk_info->num_axes;
      device_info->axes       = NULL;

      device_info->context    = gimp_context_new (device_info->name, NULL);
      gimp_context_define_args (device_info->context,
				DEVICE_CONTEXT_MASK,
				FALSE);
      gimp_context_copy_args (gimp_context_get_user (), device_info->context,
			      DEVICE_CONTEXT_MASK);
      device_status_context_connect (device_info->context, device_info->device);

      device_info_list = g_list_append (device_info_list, device_info);
    }
}

void
devices_restore (void)
{
  DeviceInfo *device_info;
  GimpContext *context;
  gchar *filename;

  /* Augment with information from rc file */
  filename = gimp_personal_rc_file ("devicerc");
  parse_gimprc_file (filename);
  g_free (filename);

  if ((device_info = device_info_get_by_id (current_device)) == NULL)
    return;

  suppress_update = TRUE;

  context = gimp_context_get_user ();

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
		   ToolType      tool,
		   guchar        foreground[], 
		   guchar        background[],
		   gchar        *brush_name, 
		   gchar        *pattern_name,
		   gchar        *gradient_name)
{
  DeviceInfo *device_info;

  /*  Find device if we have it  */
  device_info = device_info_get_by_name (name);

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

      device_info->context = gimp_context_new (device_info->name, NULL);
      gimp_context_define_args (device_info->context,
				DEVICE_CONTEXT_MASK,
				FALSE);
      gimp_context_copy_args (gimp_context_get_user (), device_info->context,
			      DEVICE_CONTEXT_MASK);
      device_status_context_connect (device_info->context, device_info->device);

      device_info_list = g_list_append (device_info_list, device_info);
    }
  else
    {
      GdkDeviceInfo *gdk_info;

      gdk_info = gdk_device_info_get_by_id (device_info->device);

      if (gdk_info != NULL)
	{
	  if (values & DEVICE_MODE)
	    gdk_input_set_mode (gdk_info->deviceid, mode);
	  
	  if ((values & DEVICE_AXES) && num_axes >= gdk_info->num_axes)
	    gdk_input_set_axes (gdk_info->deviceid, axes);

	  if ((values & DEVICE_KEYS) && num_keys >= gdk_info->num_keys)
	    {
	      gint i;
	      
	      for (i=0; i<MAX (num_keys, gdk_info->num_keys); i++)
		gdk_input_set_key (gdk_info->deviceid, i,
				   keys[i].keyval, keys[i].modifiers);
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
      gimp_context_set_tool (device_info->context, tool);
    }

  if (values & DEVICE_FOREGROUND)
    {
      gimp_context_set_foreground (device_info->context,
				   foreground[0],
				   foreground[1],
				   foreground[2]);
    }

  if (values & DEVICE_BACKGROUND)
    {
      gimp_context_set_background (device_info->context,
				   background[0],
				   background[1],
				   background[2]);
    }

  if (values & DEVICE_BRUSH)
    {
      GimpBrush *brush;

      brush = gimp_brush_list_get_brush (brush_list, brush_name);

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
      GPattern *pattern;

      pattern = pattern_list_get_pattern (pattern_list, pattern_name);

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
      gradient_t *gradient;

      gradient = gradient_list_get_gradient (gradients_list, gradient_name);

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
select_device (guint32 new_device)
{
  DeviceInfo *device_info;
  GimpContext *context;

  device_info = device_info_get_by_id (current_device);

  gimp_context_unset_parent (device_info->context);

  suppress_update = TRUE;
  
  device_info = device_info_get_by_id (new_device);

  current_device = new_device;

  context = gimp_context_get_user ();

  gimp_context_copy_args (device_info->context, context, DEVICE_CONTEXT_MASK);
  gimp_context_set_parent (device_info->context, context);

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
    {
      return FALSE;
    }
}

static void
devices_write_rc_device (DeviceInfo *device_info,
			 FILE       *fp)
{
  GdkDeviceInfo *gdk_info = NULL;
  gchar *mode = NULL;
  gint i;

  if (device_info->is_present)
    gdk_info = gdk_device_info_get_by_id (device_info->device);
  
  fprintf (fp, "(device \"%s\"", device_info->name);

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
  
  fprintf (fp, "\n        (mode %s)", mode);

  fprintf (fp, "\n        (axes %d",
	   gdk_info ? gdk_info->num_axes : device_info->num_axes);

  for (i=0; i< (gdk_info ? gdk_info->num_axes : device_info->num_axes); i++)
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
#ifdef GTK_HAVE_SIX_VALUATORS
	case GDK_AXIS_WHEEL:
	  axis_type = "wheel";
	  break;
#endif /* GTK_HAVE_SIX_VALUATORS */
	}
      fprintf (fp, " %s",axis_type);
    }
  fprintf (fp,")");

  fprintf (fp, "\n        (keys %d",
	   gdk_info ? gdk_info->num_keys : device_info->num_keys);

  for (i=0; i < (gdk_info ? gdk_info->num_keys : device_info->num_keys); i++)
    {
      GdkModifierType modifiers = gdk_info ? gdk_info->keys[i].modifiers :
	device_info->keys[i].modifiers;
      guint keyval = gdk_info ? gdk_info->keys[i].keyval :
	device_info->keys[i].keyval;

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

  /* Fixme: hard coded last tool....  see gimprc */
  if (gimp_context_get_tool (device_info->context) >= FIRST_TOOLBOX_TOOL &&
      gimp_context_get_tool (device_info->context) <= LAST_TOOLBOX_TOOL)
    {
      fprintf (fp, "\n        (tool \"%s\")",
	       tool_info[gimp_context_get_tool (device_info->context)].tool_name);
    }

  {
    guchar r, g, b;
    gimp_context_get_foreground (device_info->context, &r, &g, &b);
    fprintf (fp, "\n        (foreground %d %d %d)", r, g, b);
  }

  if (gimp_context_get_brush (device_info->context))
    {
      fprintf (fp, "\n        (brush \"%s\")",
	       gimp_context_get_brush (device_info->context)->name);
    }

  if (gimp_context_get_pattern (device_info->context))
    {
      fprintf (fp, "\n        (pattern \"%s\")",
	       gimp_context_get_pattern (device_info->context)->name);
    }

  if (gimp_context_get_gradient (device_info->context))
    {
      fprintf (fp, "\n        (gradient \"%s\")",
	       gimp_context_get_gradient (device_info->context)->name);
    }

  fprintf(fp,")\n");
}

static void
devices_write_rc (void)
{
  DeviceInfo *device_info;
  gchar *filename;
  FILE *fp;

  device_info = device_info_get_by_id (current_device);

  filename = gimp_personal_rc_file ("devicerc");
  fp = fopen (filename, "wb");
  g_free (filename);

  if (!fp)
    return;

  g_list_foreach (device_info_list, (GFunc) devices_write_rc_device, fp);

  fclose (fp);
}

void 
device_status_create (void)
{
  DeviceInfo *device_info;
  GtkWidget *label;
  GList *list;
  gint i;
  
  if (deviceD == NULL)
    {
      deviceD = g_new (DeviceInfoDialog, 1);

      deviceD->shell =
	gimp_dialog_new (_("Device Status"), "device_status",
			 gimp_standard_help_func,
			 "dialogs/device_status.html",
			 GTK_WIN_POS_NONE,
			 FALSE, FALSE, TRUE,

			 _("Save"), (GtkSignalFunc) devices_write_rc,
			 NULL, NULL, NULL, FALSE, FALSE,
			 _("Close"), devices_close_callback,
			 NULL, NULL, NULL, TRUE, TRUE,

			 NULL);

      dialog_register (deviceD->shell);
      session_set_window_geometry (deviceD->shell, &device_status_session_info,
				   FALSE);

      deviceD->num_devices = 0;

      for (list = device_info_list; list; list = g_list_next (list))
	{
	  if (((DeviceInfo *) list->data)->is_present)
	    deviceD->num_devices++;
	}

      /*  devices table  */
      deviceD->table = gtk_table_new (deviceD->num_devices, 6, FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (deviceD->table), 3);
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG (deviceD->shell)->vbox),
			 deviceD->table);
      gtk_widget_realize (deviceD->table);
      gtk_widget_show (deviceD->table);

      deviceD->ids        = g_new (guint32, deviceD->num_devices);
      deviceD->frames     = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->tools      = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->colors     = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->brushes    = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->patterns   = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->gradients  = g_new (GtkWidget *, deviceD->num_devices);
      deviceD->eventboxes = g_new (GtkWidget *, deviceD->num_devices);

      for (list = device_info_list, i = 0; list; list = g_list_next (list), i++)
	{
	  if (!((DeviceInfo *) list->data)->is_present)
	    continue;

	  device_info = (DeviceInfo *) list->data;

	  deviceD->ids[i] = device_info->device;

	  /*  the device name  */

	  deviceD->frames[i] = gtk_frame_new (NULL);

	  gtk_frame_set_shadow_type (GTK_FRAME(deviceD->frames[i]),
				     GTK_SHADOW_OUT);
	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->frames[i],
			    0, 1, i, i+1,
			    GTK_FILL, GTK_FILL, 2, 0);

	  label = gtk_label_new (device_info->name);
	  gtk_misc_set_padding (GTK_MISC(label), 2, 0);
	  gtk_container_add (GTK_CONTAINER(deviceD->frames[i]), label);
	  gtk_widget_show(label);

	  /*  the tool  */

	  deviceD->eventboxes[i] = gtk_event_box_new();

	  deviceD->tools[i] = gtk_pixmap_new (tool_get_pixmap (RECT_SELECT), NULL);
	  
	  gtk_drag_source_set (deviceD->eventboxes[i],
			       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
			       tool_target_table, n_tool_targets,
			       GDK_ACTION_COPY);
	  gimp_dnd_tool_source_set (deviceD->eventboxes[i],
				    device_status_drag_tool, 
				    GUINT_TO_POINTER (device_info->device));
 	  gtk_drag_dest_set (deviceD->eventboxes[i],
 			     GTK_DEST_DEFAULT_HIGHLIGHT |
			     GTK_DEST_DEFAULT_MOTION |
 			     GTK_DEST_DEFAULT_DROP,
 			     tool_target_table, n_tool_targets,
 			     GDK_ACTION_COPY); 
 	  gimp_dnd_tool_dest_set (deviceD->eventboxes[i],
				  device_status_drop_tool, 
				  GUINT_TO_POINTER (device_info->device));
	  gtk_container_add (GTK_CONTAINER (deviceD->eventboxes[i]),
			     deviceD->tools[i]);
	  gtk_table_attach (GTK_TABLE (deviceD->table), deviceD->eventboxes[i],
			    1, 2, i, i+1,
			    0, 0, 2, 2);

	  /*  the foreground color  */

	  deviceD->colors[i] = gtk_preview_new (GTK_PREVIEW_COLOR);
	  gtk_widget_set_events (deviceD->colors[i], PREVIEW_EVENT_MASK);
	  gtk_preview_size (GTK_PREVIEW (deviceD->colors[i]),
			    CELL_SIZE, CELL_SIZE);
	  gtk_drag_source_set (deviceD->colors[i],
			       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
			       color_area_target_table, n_color_area_targets,
			       GDK_ACTION_COPY);
	  gimp_dnd_color_source_set (deviceD->colors[i],
				     device_status_drag_color, 
				     GUINT_TO_POINTER (device_info->device));
 	  gtk_drag_dest_set (deviceD->colors[i],
 			     GTK_DEST_DEFAULT_HIGHLIGHT |
			     GTK_DEST_DEFAULT_MOTION |
 			     GTK_DEST_DEFAULT_DROP,
 			     color_area_target_table, n_color_area_targets,
 			     GDK_ACTION_COPY); 
 	  gimp_dnd_color_dest_set (deviceD->colors[i], device_status_drop_color, 
				   GUINT_TO_POINTER (device_info->device));
	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->colors[i],
			    2, 3, i, i+1,
			    0, 0, 2, 2);

	  /*  the brush  */

	  deviceD->brushes[i] =
	    gimp_context_preview_new (GCP_BRUSH, 
				      CELL_SIZE, CELL_SIZE,
				      FALSE, TRUE,
				      GTK_SIGNAL_FUNC (device_status_drop_brush),
				      GUINT_TO_POINTER (device_info->device));
	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->brushes[i],
			    3, 4, i, i+1,
			    0, 0, 2, 2);

	  /*  the pattern  */

	  deviceD->patterns[i] =
	    gimp_context_preview_new (GCP_PATTERN,
				      CELL_SIZE, CELL_SIZE, 
				      FALSE, TRUE,
				      GTK_SIGNAL_FUNC (device_status_drop_pattern),
				      GUINT_TO_POINTER (device_info->device));
	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->patterns[i],
			    4, 5, i, i+1,
			    0, 0, 2, 2);

	  /*  the gradient  */

	  deviceD->gradients[i] =
	    gimp_context_preview_new (GCP_GRADIENT,
				      CELL_SIZE * 2, CELL_SIZE, 
				      FALSE, TRUE,
				      GTK_SIGNAL_FUNC (device_status_drop_gradient),
				      GUINT_TO_POINTER (device_info->device));
	  gtk_table_attach (GTK_TABLE(deviceD->table), deviceD->gradients[i],
			    5, 6, i, i+1,
			    0, 0, 2, 2);

	  device_status_update (device_info->device);
	}

      deviceD->current = 0xffffffff; /* random, but doesn't matter */
      device_status_update_current ();

      gtk_widget_show (deviceD->shell);

      gtk_signal_connect (GTK_OBJECT (deviceD->shell), "destroy",
			  GTK_SIGNAL_FUNC (device_status_destroy_callback),
			  NULL);
    }
  else
    {
      if (!GTK_WIDGET_MAPPED (deviceD->shell))
	gtk_widget_show (deviceD->shell);
      else
	gdk_window_raise (deviceD->shell->window);
    }
}

static void
device_status_destroy_callback (void)
{
  g_free (deviceD->ids);
  g_free (deviceD->frames);
  g_free (deviceD->tools);
  g_free (deviceD->eventboxes);
  g_free (deviceD->colors);
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
  if (save_device_status)
    devices_write_rc ();

  if (deviceD)
    {
      session_get_window_info (deviceD->shell, &device_status_session_info);
      device_status_destroy_callback (); 
    }
}

static void
device_status_update_current (void)
{
  gint i;

  if (deviceD)
    {
      for (i = 0; i < deviceD->num_devices; i++)
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

void 
device_status_update (guint32 deviceid)
{
  GdkDeviceInfo *gdk_info;
  DeviceInfo    *device_info;
  guchar buffer[CELL_SIZE*3];
  gchar  ttbuf[20]; /* [xxx,xxx,xxx] + null */
  gint   i, j;

  if (!deviceD || suppress_update)
    return;

  if ((device_info = device_info_get_by_id (deviceid)) == NULL)
    return;

  if ((gdk_info = gdk_device_info_get_by_id (deviceid)) == NULL)
    return;

  for (i = 0; i < deviceD->num_devices; i++)
    {
      if (deviceD->ids[i] == deviceid)
	break;
    }

  g_return_if_fail (i < deviceD->num_devices);

  if (gdk_info->mode == GDK_MODE_DISABLED)
    {
      gtk_widget_hide (deviceD->frames[i]);
      gtk_widget_hide (deviceD->tools[i]);
      gtk_widget_hide (deviceD->eventboxes[i]);
      gtk_widget_hide (deviceD->colors[i]);
      gtk_widget_hide (deviceD->brushes[i]);
      gtk_widget_hide (deviceD->patterns[i]);
      gtk_widget_hide (deviceD->gradients[i]);
    }
  else
    {
      gtk_widget_show (deviceD->frames[i]);

      gtk_pixmap_set (GTK_PIXMAP (deviceD->tools[i]), 
		      tool_get_pixmap (gimp_context_get_tool (device_info->context)),
		      NULL);

      gtk_widget_draw (deviceD->tools[i], NULL);
      gtk_widget_show (deviceD->tools[i]);
      gtk_widget_show (deviceD->eventboxes[i]);

      gimp_help_set_help_data (deviceD->eventboxes[i],
			       tool_info[(gint) gimp_context_get_tool (device_info->context)].tool_desc,
			       tool_info[(gint) gimp_context_get_tool (device_info->context)].private_tip);

      for (j = 0; j < CELL_SIZE * 3; j += 3)
	{
	  gimp_context_get_foreground (device_info->context,
				       &buffer[j],
				       &buffer[j+1],
				       &buffer[j+2]);
	}

      for (j = 0; j < CELL_SIZE; j++)
	gtk_preview_draw_row (GTK_PREVIEW(deviceD->colors[i]), buffer, 
			      0, j, CELL_SIZE);
      gtk_widget_draw (deviceD->colors[i], NULL);
      gtk_widget_show (deviceD->colors[i]);

      /*  Set the tip to be the RGB value  */
      g_snprintf (ttbuf, sizeof (ttbuf), "[%3d,%3d,%3d]",
		  buffer[j],
		  buffer[j+1],
		  buffer[j+2]);

      gimp_help_set_help_data (deviceD->colors[i], ttbuf, NULL);

      if (gimp_context_get_brush (device_info->context))
	{
	  gimp_context_preview_update
	    (GIMP_CONTEXT_PREVIEW (deviceD->brushes[i]), 
	     gimp_context_get_brush (device_info->context));
	  gtk_widget_show (deviceD->brushes[i]);
	}

      if (gimp_context_get_pattern (device_info->context))
	{
	  gimp_context_preview_update
	    (GIMP_CONTEXT_PREVIEW (deviceD->patterns[i]), 
	     gimp_context_get_pattern (device_info->context));
	  gtk_widget_show (deviceD->patterns[i]);
	}

      if (gimp_context_get_gradient (device_info->context))
	{
	  gimp_context_preview_update
	    (GIMP_CONTEXT_PREVIEW (deviceD->gradients[i]), 
	     gimp_context_get_gradient (device_info->context));
	  gtk_widget_show (deviceD->gradients[i]);
	}
    }
}

/*  dnd stuff  */

static ToolType
device_status_drag_tool (GtkWidget *widget,
			 gpointer   data)
{
  DeviceInfo *device_info;

  device_info = device_info_get_by_id (GPOINTER_TO_UINT (data));

  if (device_info)
    {
      return gimp_context_get_tool (device_info->context);
    }
  else
    {
      return RECT_SELECT;
    }
}

static void
device_status_drop_tool (GtkWidget *widget,
			 ToolType   tool,
			 gpointer   data)
{
  DeviceInfo *device_info;

  device_info = device_info_get_by_id (GPOINTER_TO_UINT (data));

  if (device_info && device_info->is_present)
    {
      gimp_context_set_tool (device_info->context, tool);
    }
}

static void
device_status_drag_color (GtkWidget *widget,
			  guchar    *r,
			  guchar    *g,
			  guchar    *b,
			  gpointer   data)
{
  DeviceInfo *device_info;

  device_info = device_info_get_by_id (GPOINTER_TO_UINT (data));

  if (device_info)
    {
      gimp_context_get_foreground (device_info->context, r, g, b);
    }
  else
    {
      *r = *g = *b = 0;
    }
}

static void
device_status_drop_color (GtkWidget *widget,
			  guchar     r,
			  guchar     g,
			  guchar     b,
			  gpointer   data)
{
  DeviceInfo *device_info;

  device_info = device_info_get_by_id (GPOINTER_TO_UINT (data));

  if (device_info && device_info->is_present)
    {
      gimp_context_set_foreground (device_info->context, r, g, b);
    }
}

static void
device_status_drop_brush (GtkWidget *widget,
			  GimpBrush *brush,
			  gpointer   data)
{
  DeviceInfo *device_info;

  device_info = device_info_get_by_id (GPOINTER_TO_UINT (data));

  if (device_info && device_info->is_present)
    {
      gimp_context_set_brush (device_info->context, brush);
    }
}

static void
device_status_drop_pattern (GtkWidget *widget,
			    GPattern  *pattern,
			    gpointer   data)
{
  DeviceInfo *device_info;
  
  device_info = device_info_get_by_id (GPOINTER_TO_UINT (data));

  if (device_info && device_info->is_present)
    {
      gimp_context_set_pattern (device_info->context, pattern);
    }
}

static void
device_status_drop_gradient (GtkWidget  *widget,
			     gradient_t *gradient,
			     gpointer    data)
{
  DeviceInfo *device_info;
  
  device_info = device_info_get_by_id (GPOINTER_TO_UINT (data));

  if (device_info && device_info->is_present)
    {
      gimp_context_set_gradient (device_info->context, gradient);
    }
}

/*  context callbacks  */

static void
device_status_color_changed (GimpContext *context,
			     gint         r,
			     gint         g,
			     gint         b,
			     gpointer     data)
{
  DeviceInfo *device_info;

  device_info = device_info_get_by_id (GPOINTER_TO_UINT (data));

  device_status_update (device_info->device);
}

static void
device_status_data_changed (GimpContext *context,
			    gpointer     dummy,
			    gpointer     data)
{
  DeviceInfo *device_info;

  device_info = device_info_get_by_id (GPOINTER_TO_UINT (data));

  device_status_update (device_info->device);
}

static void
device_status_context_connect  (GimpContext *context,
				guint32      deviceid)
{
  gtk_signal_connect (GTK_OBJECT (context), "foreground_changed",
		      GTK_SIGNAL_FUNC (device_status_color_changed),
		      (gpointer) deviceid);
  gtk_signal_connect (GTK_OBJECT (context), "tool_changed",
		      GTK_SIGNAL_FUNC (device_status_data_changed),
		      (gpointer) deviceid);
  gtk_signal_connect (GTK_OBJECT (context), "brush_changed",
		      GTK_SIGNAL_FUNC (device_status_data_changed),
		      (gpointer) deviceid);
  gtk_signal_connect (GTK_OBJECT (context), "pattern_changed",
		      GTK_SIGNAL_FUNC (device_status_data_changed),
		      (gpointer) deviceid);
  gtk_signal_connect (GTK_OBJECT (context), "gradient_changed",
		      GTK_SIGNAL_FUNC (device_status_data_changed),
		      (gpointer) deviceid);
}
