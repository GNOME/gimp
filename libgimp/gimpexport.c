/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                             
#include "config.h"

#include <gtk/gtk.h>

#include "gimp.h"
#include "gimpdialog.h"
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
  gint nlayers;
  gint i;
  gint32 *layers;

  *drawable_ID = gimp_image_merge_visible_layers (image_ID, GIMP_CLIP_TO_IMAGE);
  
  /* remove any remaining (invisible) layers */ 
  layers = gimp_image_get_layers (image_ID, &nlayers);
  for (i = 0; i < nlayers; i++)
    {
      if (layers[i] != *drawable_ID)
	gimp_image_remove_layer (image_ID, layers[i]);
    }
  g_free (layers);  
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

static void
export_add_alpha (gint32  image_ID,
		  gint32 *drawable_ID)
{  
  gint nlayers;
  gint i;
  gint32 *layers;

  layers = gimp_image_get_layers (image_ID, &nlayers);
  for (i = 0; i < nlayers; i++)
    {
      if (!gimp_drawable_has_alpha (layers[i]))
	gimp_layer_add_alpha (layers[i]);
    }
  g_free (layers);  
}


/* a set of predefined actions */

static ExportAction export_action_merge =
{
  export_merge,
  NULL,
  N_("can't Handle Layers"),
  { N_("Merge Visible Layers"), NULL },
  0
};

static ExportAction export_action_animate_or_merge =
{
  NULL,
  export_merge,
  N_("can only Handle Layers as Animation Frames"),
  { N_("Save as Animation"), N_("Merge Visible Layers") },
  0
};

static ExportAction export_action_flatten =
{
  &export_flatten,
  NULL,
  N_("can't Handle Transparency"),
  { N_("Flatten Image"), NULL },
  0
};

static ExportAction export_action_convert_rgb =
{
  &export_convert_rgb,
  NULL,
  N_("can only Handle RGB Images"),
  { N_("Convert to RGB"), NULL },
  0
};

static ExportAction export_action_convert_grayscale =
{
  &export_convert_grayscale,
  NULL,
  N_("can only Handle Grayscale Images"),
  { N_("Convert to Grayscale"), NULL },
  0
};

static ExportAction export_action_convert_indexed =
{
  &export_convert_indexed,
  NULL,
  N_("can only Handle Indexed Images"),
  { N_("Convert to indexed using default settings\n"
       "(Do it manually to tune the result)"), NULL },
  0
};

static ExportAction export_action_convert_rgb_or_grayscale =
{
  &export_convert_rgb,
  &export_convert_grayscale,
  N_("can only Handle RGB or Grayscale Images"),
  { N_("Convert to RGB"), N_("Convert to Grayscale")},
  0
};

static ExportAction export_action_convert_rgb_or_indexed =
{
  &export_convert_rgb,
  &export_convert_indexed,
  N_("can only Handle RGB or Indexed Images"),
  { N_("Convert to RGB"), N_("Convert to indexed using default settings\n"
			     "(Do it manually to tune the result)")},
  0
};

static ExportAction export_action_convert_indexed_or_grayscale =
{
  &export_convert_indexed,
  &export_convert_grayscale,
  N_("can only Handle Grayscale or Indexed Images"),
  { N_("Convert to indexed using default settings\n"
       "(Do it manually to tune the result)"), 
    N_("Convert to Grayscale") },
  0
};

static ExportAction export_action_add_alpha =
{
  &export_add_alpha,
  NULL,
  N_("needs an Alpha Channel"),
  { N_("Add Alpha Channel"), NULL},
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
export_dialog (GSList *actions,
	       gchar  *format)
{
  GtkWidget    *frame;
  GtkWidget    *vbox;
  GtkWidget    *hbox;
  GtkWidget    *button;
  GtkWidget    *label;
  GSList       *list;
  gchar        *text;
  ExportAction *action;

  dialog_return = EXPORT_CANCEL;
  g_return_val_if_fail (actions != NULL && format != NULL, dialog_return);

  /*
   *  Plug-ins have called gtk_init () before calling gimp_export ().
   *  Otherwise bad things will happen now!!
   */

  /* the dialog */
  dialog = gimp_dialog_new (_("Export File"), "export_file",
			    gimp_standard_help_func, "dialogs/export_file.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, FALSE, FALSE,

			    _("Export"), export_export_callback,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Ignore"), export_skip_callback,
			    NULL, NULL, NULL, FALSE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (export_cancel_callback),
		      NULL);

  /* the headline */
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Your image should be exported before it "
			   "can be saved for the following reasons:"));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  for (list = actions; list; list = list->next)
    {
      action = (ExportAction *) (list->data);

      text = g_strdup_printf ("%s %s", format, gettext (action->reason));
      frame = gtk_frame_new (text);
      g_free (text);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

      hbox = gtk_hbox_new (FALSE, 4);
      gtk_container_add (GTK_CONTAINER (frame), hbox);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);

      if (action->possibilities[0] && action->possibilities[1])
	{
	  GSList *radio_group = NULL;
 
	  button = gtk_radio_button_new_with_label (radio_group, 
						    gettext (action->possibilities[0]));
	  gtk_label_set_justify (GTK_LABEL (GTK_BIN (button)->child), GTK_JUSTIFY_LEFT);
	  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
	  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	  gtk_signal_connect (GTK_OBJECT (button), "toggled",
			      GTK_SIGNAL_FUNC (export_toggle_callback),
			      &action->choice);
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	  gtk_widget_show (button);

	  button = gtk_radio_button_new_with_label (radio_group, 
						    gettext (action->possibilities[1]));
	  gtk_label_set_justify (GTK_LABEL (GTK_BIN (button)->child), GTK_JUSTIFY_LEFT);
	  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
	  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	  gtk_widget_show (button);
	} 
      else if (action->possibilities[0])
	{
	  label = gtk_label_new (gettext (action->possibilities[0]));
	  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
	  gtk_widget_show (label);
	  action->choice = 0;
	}
      else if (action->possibilities[1])
	{
	  label = gtk_label_new (gettext (action->possibilities[1]));
	  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
	  gtk_widget_show (label);
	  action->choice = 1;
	}      
      gtk_widget_show (hbox);
      gtk_widget_show (frame);
    }

  /* the footline */
  label = gtk_label_new (_("The export conversion won't modify "
			   "your original image."));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);
  gtk_main ();

  return dialog_return;
}

GimpExportReturnType
gimp_export_image (gint32 *image_ID_ptr,
		   gint32 *drawable_ID_ptr,
		   gchar  *format,
		   gint    cap)  /* cap like capabilities */
{
  GSList *actions = NULL;
  GSList *list;
  GimpImageBaseType type;
  gint i;
  gint nlayers;
  gint32* layers;
  gboolean added_flatten = FALSE;

  ExportAction *action;

  g_return_val_if_fail (*image_ID_ptr > -1 && *drawable_ID_ptr > -1, FALSE);

  /* do some sanity checks */
  if (cap & NEEDS_ALPHA)
    cap |= CAN_HANDLE_ALPHA;
  if (cap & CAN_HANDLE_LAYERS_AS_ANIMATION)
    cap |= CAN_HANDLE_LAYERS;

  /* check alpha */
  layers = gimp_image_get_layers (*image_ID_ptr, &nlayers);
  for (i = 0; i < nlayers; i++)
    {
      if (gimp_drawable_has_alpha (layers[i]))
	{
	  if ( !(cap & CAN_HANDLE_ALPHA) )
	    {
	      actions = g_slist_prepend (actions, &export_action_flatten);
	      added_flatten = TRUE;
	      break;
	    }
	}
      else 
	{
	  if (cap & NEEDS_ALPHA)
	    {
	      actions = g_slist_prepend (actions, &export_action_add_alpha);
	      break;
	    }
	}
    }
  g_free (layers);

  /* check multiple layers */
  if (!added_flatten && nlayers > 1)
    {
      if (cap & CAN_HANDLE_LAYERS_AS_ANIMATION)
	actions = g_slist_prepend (actions, &export_action_animate_or_merge);
      else if ( !(cap & CAN_HANDLE_LAYERS))
	actions = g_slist_prepend (actions, &export_action_merge);
    }

  /* check the image type */	  
  type = gimp_image_base_type (*image_ID_ptr);
  switch (type)
    {
    case GIMP_RGB:
       if ( !(cap & CAN_HANDLE_RGB) )
	{
	  if ((cap & CAN_HANDLE_INDEXED) && (cap & CAN_HANDLE_GRAY))
	    actions = g_slist_prepend (actions, &export_action_convert_indexed_or_grayscale);
	  else if (cap & CAN_HANDLE_INDEXED)
	    actions = g_slist_prepend (actions, &export_action_convert_indexed);
	  else if (cap & CAN_HANDLE_GRAY)
	    actions = g_slist_prepend (actions, &export_action_convert_grayscale);
	}
      break;
    case GIMP_GRAY:
      if ( !(cap & CAN_HANDLE_GRAY) )
	{
	  if ((cap & CAN_HANDLE_RGB) && (cap & CAN_HANDLE_INDEXED))
	    actions = g_slist_prepend (actions, &export_action_convert_rgb_or_indexed);
	  else if (cap & CAN_HANDLE_RGB)
	    actions = g_slist_prepend (actions, &export_action_convert_rgb);
	  else if (cap & CAN_HANDLE_INDEXED)
	    actions = g_slist_prepend (actions, &export_action_convert_indexed);
	}
      break;
    case GIMP_INDEXED:
       if ( !(cap & CAN_HANDLE_INDEXED) )
	{
	  if ((cap & CAN_HANDLE_RGB) && (cap & CAN_HANDLE_GRAY))
	    actions = g_slist_prepend (actions, &export_action_convert_rgb_or_grayscale);
	  else if (cap & CAN_HANDLE_RGB)
	    actions = g_slist_prepend (actions, &export_action_convert_rgb);
	  else if (cap & CAN_HANDLE_GRAY)
	    actions = g_slist_prepend (actions, &export_action_convert_grayscale);
	}
      break;
    }
  
  if (actions)
    {
      actions = g_slist_reverse (actions);
      dialog_return = export_dialog (actions, format);
    }
  else
    dialog_return = EXPORT_IGNORE;

  if (dialog_return == EXPORT_EXPORT)
    {
      *image_ID_ptr = gimp_image_duplicate (*image_ID_ptr);
      *drawable_ID_ptr = gimp_image_get_active_layer (*image_ID_ptr);
      gimp_image_undo_disable (*image_ID_ptr);
      for (list = actions; list; list = list->next)
	{
	  action = (ExportAction*)(list->data);
	  if (action->choice == 0 && action->default_action)  
	    action->default_action (*image_ID_ptr, drawable_ID_ptr);
	  else if (action->choice == 1 && action->alt_action)
	    action->alt_action (*image_ID_ptr, drawable_ID_ptr);
	}
    }
  g_slist_free (actions);

  return dialog_return;
}
