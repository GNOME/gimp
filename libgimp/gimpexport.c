/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                             

#include <gtk/gtk.h>

#include "gimp.h"
#include "gimpenums.h"
#include "gimpexport.h"
#include "gimpintl.h"

typedef void (* ExportFunc) (gint32 imageID, gint32 *drawable_ID);

/* the export action structure */
typedef struct 
{
  ExportFunc  default_action;
  ExportFunc  alt_action;
  gchar      *reason;
  gchar      *possibilities[2];
  gint        choice;
} ExportAction;


/* the functions that do the actual export */
static void
export_merge (gint32  image_ID,
	      gint32 *drawable_ID)
{
  *drawable_ID = gimp_image_merge_visible_layers (image_ID, GIMP_EXPAND_AS_NECESSARY);
}

static void
export_flatten (gint32  image_ID,
		gint32 *drawable_ID)
{  
  *drawable_ID = gimp_image_flatten (image_ID);
}

static void
export_convert_rgb (gint32  image_ID,
		    gint32 *drawable_ID)
{  
  gimp_image_convert_rgb (image_ID);
}

static void
export_convert_grayscale (gint32  image_ID,
			  gint32 *drawable_ID)
{  
  gimp_image_convert_grayscale (image_ID);
}

static void
export_convert_indexed (gint32  image_ID,
			gint32 *drawable_ID)
{  
  gint nlayers;
  
  /* check alpha */
  g_free (gimp_image_get_layers (image_ID, &nlayers));
  if (nlayers > 1 || gimp_drawable_has_alpha (*drawable_ID))
    gimp_image_convert_indexed (image_ID, GIMP_FS_DITHER, GIMP_MAKE_PALETTE, 255, FALSE, FALSE, "");
  else
    gimp_image_convert_indexed (image_ID, GIMP_FS_DITHER, GIMP_MAKE_PALETTE, 256, FALSE, FALSE, ""); 
}

/* a set of predefined actions */

static ExportAction export_action_merge =
{
  export_merge,
  NULL,
  N_("can't handle layers"),
  { N_("Merge visible layers"), NULL },
  0
};

static ExportAction export_action_animate_or_merge =
{
  NULL,
  export_merge,
  N_("can only handle layers as animation frames"),
  { N_("Save as animation"), N_("Merge visible layers") },
  0
};

static ExportAction export_action_flatten =
{
  &export_flatten,
  NULL,
  N_("can't handle transparency"),
  { N_("Flatten Image"), NULL },
  0
};

static ExportAction export_action_convert_rgb =
{
  &export_convert_rgb,
  NULL,
  N_("can only handle RGB images"),
  { N_("Convert to RGB"), NULL },
  0
};

static ExportAction export_action_convert_grayscale =
{
  &export_convert_grayscale,
  NULL,
  N_("can only handle grayscale images"),
  { N_("Convert to grayscale"), NULL },
  0
};

static ExportAction export_action_convert_indexed =
{
  &export_convert_indexed,
  NULL,
  N_("can only handle indexed images"),
  { N_("Convert to indexed using default settings\n(Do it manually to tune the result)"), NULL },
  0
};

static ExportAction export_action_convert_rgb_or_grayscale =
{
  &export_convert_rgb,
  &export_convert_grayscale,
  N_("can only handle RGB or grayscale images"),
  { N_("Convert to RGB"), N_("Convert to grayscale")},
  0
};

static ExportAction export_action_convert_rgb_or_indexed =
{
  &export_convert_rgb,
  &export_convert_indexed,
  N_("can only handle RGB or indexed images"),
  { N_("Convert to RGB"), N_("Convert to indexed using default settings\n(Do it manually to tune the result)")},
  0
};

static ExportAction export_action_convert_indexed_or_grayscale =
{
  &export_convert_indexed,
  &export_convert_grayscale,
  N_("can only handle grayscale or indexed images"),
  { N_("Convert to indexed using default settings\n(Do it manually to tune the result)"), 
    N_("Convert to grayscale") },
  0
};


/* dialog functions */

static GtkWidget            *dialog = NULL;
static GimpExportReturnType  dialog_return = EXPORT_CANCEL;

static void
export_export_callback (GtkWidget *widget,
			gpointer   data)
{
  gtk_widget_destroy (dialog);
  dialog_return = EXPORT_EXPORT;
}

static void
export_skip_callback (GtkWidget *widget,
		      gpointer   data)
{
  gtk_widget_destroy (dialog);
  dialog_return = EXPORT_IGNORE;
}

static void
export_cancel_callback (GtkWidget *widget,
			gpointer   data)
{
  dialog_return = EXPORT_CANCEL;
  dialog = NULL;
  gtk_main_quit ();
}

static void
export_toggle_callback (GtkWidget *widget,
			gpointer   data)
{
  gint *choice = (gint*)data;
  
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    *choice = 0;
  else
    *choice = 1;
}

static gint
export_dialog (GList *actions,
	       gchar  *format)
{
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *hbbox;
  GtkWidget *label;
  GList     *list;
  gchar     *text;
  ExportAction *action;

  dialog_return = EXPORT_CANCEL;
  g_return_val_if_fail (actions != NULL && format != NULL, dialog_return);

  /*
   *  Plug-ins have called gtk_init () before calling gimp_export ().
   *  Otherwise bad things will happen now!!
   */

  /* the dialog */
  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("Export File"));
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      (GtkSignalFunc) export_cancel_callback, NULL);

  /*  Action area  */
  gtk_container_set_border_width
    (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area), FALSE);

  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->action_area), hbbox,
		    FALSE, FALSE, 0);
  gtk_widget_show (hbbox);

  button = gtk_button_new_with_label (_("Export"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) export_export_callback, NULL);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Ignore"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) export_skip_callback, NULL);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy, GTK_OBJECT (dialog));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* the headline */
  gtk_container_set_border_width
    (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 6);
  label = gtk_label_new (_("Your image should be exported before it can be saved for the following reasons:"));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, TRUE, TRUE, 4);
  gtk_widget_show (label);

  for (list = actions; list; list = list->next)
    {
      action = (ExportAction*)(list->data);
      text = g_strdup_printf ("%s %s", format, gettext (action->reason));
      frame = gtk_frame_new (text);
      g_free (text);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame, TRUE, TRUE, 4);
      hbox = gtk_hbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (frame), hbox);
    
      if (action->possibilities[0] && action->possibilities[1])
	{
	  GSList *radio_group = NULL;
 
	  button = gtk_radio_button_new_with_label (radio_group, 
						    gettext (action->possibilities[0]));
	  gtk_label_set_justify (GTK_LABEL (GTK_BIN (button)->child), GTK_JUSTIFY_LEFT);
	  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
	  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 2);
	  gtk_signal_connect (GTK_OBJECT (button), "toggled",
			      (GtkSignalFunc) export_toggle_callback, &action->choice);
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	  gtk_widget_show (button);

	  button = gtk_radio_button_new_with_label (radio_group, 
						    gettext (action->possibilities[1]));
	  gtk_label_set_justify (GTK_LABEL (GTK_BIN (button)->child), GTK_JUSTIFY_LEFT);
	  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
	  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 4);
	  gtk_widget_show (button);
	} 
      else if (action->possibilities[0])
	{
	  label = gtk_label_new (gettext (action->possibilities[0]));
	  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
	  gtk_widget_show (label);
	  action->choice = 0;
	}
      else if (action->possibilities[1])
	{
	  label = gtk_label_new (gettext (action->possibilities[1]));
	  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
	  gtk_widget_show (label);
	  action->choice = 1;
	}      
      gtk_widget_show (hbox);
      gtk_widget_show (frame);
    }

  /* the footline */
  label = gtk_label_new (_("The export conversion won't modify your original image."));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, TRUE, TRUE, 4);
  gtk_widget_show (label);
  
  gtk_widget_show (dialog);
  gtk_main ();

  return (dialog_return);
}


GimpExportReturnType
gimp_export_image (gint32 *image_ID_ptr,
		   gint32 *drawable_ID_ptr,
		   gchar  *format,
		   gint    cap)  /* cap like capabilities */
{
  GList *actions = NULL;
  GList *list;
  GimpImageBaseType type;
  gint nlayers;
  gboolean added_flatten = FALSE;

  ExportAction *action;

  g_return_val_if_fail (*image_ID_ptr > -1 && *drawable_ID_ptr > -1, FALSE);

  /* check alpha */
  g_free (gimp_image_get_layers (*image_ID_ptr, &nlayers));
  if (nlayers > 1 || gimp_drawable_has_alpha (*drawable_ID_ptr))
    {
      if ( !(cap & CAN_HANDLE_ALPHA ) )
	{
	  actions = g_list_append (actions, &export_action_flatten);
	  added_flatten = TRUE;
	} 
    }

  /* check multiple layers */
  if (!added_flatten && nlayers > 1)
    {
      if ( !(cap & CAN_HANDLE_LAYERS) && (cap & CAN_HANDLE_LAYERS_AS_ANIMATION))
	actions = g_list_append (actions, &export_action_animate_or_merge);
      else if (! (cap & CAN_HANDLE_LAYERS))
	actions = g_list_append (actions, &export_action_merge);
    }

  /* check the image type */	  
  type = gimp_image_base_type (*image_ID_ptr);
  switch (type)
    {
    case GIMP_RGB:
       if ( !(cap & CAN_HANDLE_RGB) )
	{
	  if ((cap & CAN_HANDLE_INDEXED) && (cap & CAN_HANDLE_GRAY))
	    actions = g_list_append (actions, &export_action_convert_indexed_or_grayscale);
	  else if (cap & CAN_HANDLE_INDEXED)
	    actions = g_list_append (actions, &export_action_convert_indexed);
	  else if (cap & CAN_HANDLE_GRAY)
	    actions = g_list_append (actions, &export_action_convert_grayscale);
	}
      break;
    case GIMP_GRAY:
      if ( !(cap & CAN_HANDLE_GRAY) )
	{
	  if ((cap & CAN_HANDLE_RGB) && (cap & CAN_HANDLE_INDEXED))
	    actions = g_list_append (actions, &export_action_convert_rgb_or_indexed);
	  else if (cap & CAN_HANDLE_RGB)
	    actions = g_list_append (actions, &export_action_convert_rgb);
	  else if (cap & CAN_HANDLE_INDEXED)
	    actions = g_list_append (actions, &export_action_convert_indexed);
	}
      break;
    case GIMP_INDEXED:
       if ( !(cap & CAN_HANDLE_INDEXED) )
	{
	  if ((cap & CAN_HANDLE_RGB) && (cap & CAN_HANDLE_GRAY))
	    actions = g_list_append (actions, &export_action_convert_rgb_or_grayscale);
	  else if (cap & CAN_HANDLE_RGB)
	    actions = g_list_append (actions, &export_action_convert_rgb);
	  else if (cap & CAN_HANDLE_GRAY)
	    actions = g_list_append (actions, &export_action_convert_grayscale);
	}
      break;
    }
  
  if (actions)
    dialog_return = export_dialog (actions, format);
  else
    dialog_return = EXPORT_IGNORE;

  if (dialog_return == EXPORT_EXPORT)
    {
      *image_ID_ptr = gimp_image_duplicate (*image_ID_ptr);
      *drawable_ID_ptr = gimp_image_get_active_layer (*image_ID_ptr);
      gimp_image_disable_undo (*image_ID_ptr);
      for (list = actions; list; list = list->next)
	{  
	  action = (ExportAction*)(list->data);
	  if (action->choice == 0 && action->default_action)  
	    action->default_action (*image_ID_ptr, drawable_ID_ptr);
	  else if (action->choice == 1 && action->alt_action)
	    action->alt_action (*image_ID_ptr, drawable_ID_ptr);
	}
    }

  return (dialog_return);
}







