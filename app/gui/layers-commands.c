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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h> 

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimp/gimplimits.h"

#include "apptypes.h"

#include "paint-funcs/paint-funcs.h"

#include "gdisplay.h"
#include "gimpui.h"
#include "layers-commands.h"
#include "resize.h"

#include "drawable.h"
#include "floating_sel.h"
#include "gimage_mask.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "gimplist.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


static void   layers_add_mask_query     (GimpLayer *layer);
static void   layers_scale_layer_query  (GimpImage *gimage,
					 GimpLayer *layer);
static void   layers_resize_layer_query (GimpImage *gimage,
					 GimpLayer *layer);


void
layers_previous_cmd_callback (GtkWidget *widget,
			      gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *new_layer;
  gint       current_layer;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  current_layer =
    gimp_image_get_layer_index (gimage, gimp_image_get_active_layer (gimage));

  if (current_layer > 0)
    {
      new_layer = (GimpLayer *)
	gimp_container_get_child_by_index (gimage->layers, current_layer - 1);

      if (new_layer)
	{
	  gimp_image_set_active_layer (gimage, new_layer);
	  gdisplays_flush ();
	}
    }
}

void
layers_next_cmd_callback (GtkWidget *widget,
			  gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *new_layer;
  gint       current_layer;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  current_layer =
    gimp_image_get_layer_index (gimage, gimp_image_get_active_layer (gimage));

  new_layer = 
    GIMP_LAYER (gimp_container_get_child_by_index (gimage->layers, 
						   current_layer + 1));

  if (new_layer)
    {
      gimp_image_set_active_layer (gimage, new_layer);
      gdisplays_flush ();
    }
}

void
layers_raise_cmd_callback (GtkWidget *widget,
			   gpointer   data)
{
  GimpImage *gimage;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  gimp_image_raise_layer (gimage, gimp_image_get_active_layer (gimage));
  gdisplays_flush ();
}

void
layers_lower_cmd_callback (GtkWidget *widget,
			   gpointer   data)
{
  GimpImage *gimage;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  gimp_image_lower_layer (gimage, gimp_image_get_active_layer (gimage));
  gdisplays_flush ();
}

void
layers_raise_to_top_cmd_callback (GtkWidget *widget,
				  gpointer   data)
{
  GimpImage *gimage;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  gimp_image_raise_layer_to_top (gimage, gimp_image_get_active_layer (gimage));
  gdisplays_flush ();
}

void
layers_lower_to_bottom_cmd_callback (GtkWidget *widget,
				     gpointer   data)
{
  GimpImage *gimage;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  gimp_image_lower_layer_to_bottom (gimage,
				    gimp_image_get_active_layer (gimage));
  gdisplays_flush ();
}

void
layers_new_cmd_callback (GtkWidget *widget,
			 gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  /*  If there is a floating selection, the new command transforms
   *  the current fs into a new layer
   */
  if ((layer = gimp_image_floating_sel (gimage)))
    {
      floating_sel_to_layer (layer);

      gdisplays_flush ();
    }
  else
    {
      layers_new_layer_query (gimage);
    }
}

void
layers_duplicate_cmd_callback (GtkWidget *widget,
			       gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *active_layer;
  GimpLayer *new_layer;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  active_layer = gimp_image_get_active_layer (gimage);
  new_layer = gimp_layer_copy (active_layer, TRUE);
  gimp_image_add_layer (gimage, new_layer, -1);

  gdisplays_flush ();
}

void
layers_delete_cmd_callback (GtkWidget *widget,
			    gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  layer = gimp_image_get_active_layer (gimage);

  if (! layer)
    return;

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_remove (layer);
  else
    gimp_image_remove_layer (gimage, layer);

  gdisplays_flush_now ();
}

void
layers_scale_cmd_callback (GtkWidget *widget,
			   gpointer   data)
{
  GimpImage *gimage;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  layers_scale_layer_query (gimage, gimp_image_get_active_layer (gimage));
}

void
layers_resize_cmd_callback (GtkWidget *widget,
			    gpointer   data)
{
  GimpImage *gimage;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  layers_resize_layer_query (gimage, gimp_image_get_active_layer (gimage));
}

void
layers_resize_to_image_cmd_callback (GtkWidget *widget,
				     gpointer   data)
{
  GimpImage *gimage;
  
  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;
  
  gimp_layer_resize_to_image (gimp_image_get_active_layer (gimage));

  gdisplays_flush ();
}

void
layers_add_layer_mask_cmd_callback (GtkWidget *widget,
				    gpointer   data)
{
  GimpImage *gimage;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  layers_add_mask_query (gimp_image_get_active_layer (gimage));
}

void
layers_apply_layer_mask_cmd_callback (GtkWidget *widget,
				      gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  layer = gimp_image_get_active_layer (gimage);

  /*  Make sure there is a layer mask to apply  */
  if (layer && gimp_layer_get_mask (layer))
    {
      gboolean flush = ! layer->mask->apply_mask || layer->mask->show_mask;

      gimp_layer_apply_mask (layer, APPLY, TRUE);

      if (flush)
	gdisplays_flush ();
    }
}

void
layers_delete_layer_mask_cmd_callback (GtkWidget *widget,
				       gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  layer = gimp_image_get_active_layer (gimage);

  /*  Make sure there is a layer mask to apply  */
  if (layer && gimp_layer_get_mask (layer))
    {
      gboolean flush = layer->mask->apply_mask || layer->mask->show_mask;

      gimp_layer_apply_mask (layer, DISCARD, TRUE);

      if (flush)
	gdisplays_flush ();
    }
}

void
layers_anchor_cmd_callback (GtkWidget *widget,
			    gpointer   data)
{
  GimpImage *gimage;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  floating_sel_anchor (gimp_image_get_active_layer (gimage));
  gdisplays_flush ();
}

void
layers_merge_layers_cmd_callback (GtkWidget *widget,
				  gpointer   data)
{
  GimpImage *gimage;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  layers_layer_merge_query (gimage, TRUE);
}

void
layers_merge_down_cmd_callback (GtkWidget *widget,
				gpointer   data)
{
  GimpImage *gimage;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;
  
  gimp_image_merge_down (gimage, gimp_image_get_active_layer (gimage),
			 EXPAND_AS_NECESSARY);
  gdisplays_flush ();
}

void
layers_flatten_image_cmd_callback (GtkWidget *widget,
				   gpointer   data)
{
  GimpImage *gimage;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  gimp_image_flatten (gimage);
  gdisplays_flush ();
}

void
layers_alpha_select_cmd_callback (GtkWidget *widget,
				  gpointer   data)
{
  GimpImage *gimage;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  gimage_mask_layer_alpha (gimage, gimp_image_get_active_layer (gimage));
  gdisplays_flush ();
}

void
layers_mask_select_cmd_callback (GtkWidget *widget,
				 gpointer   data)
{
  GimpImage *gimage;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  gimage_mask_layer_mask (gimage, gimp_image_get_active_layer (gimage));
  gdisplays_flush ();
}

void
layers_add_alpha_channel_cmd_callback (GtkWidget *widget,
				       gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  layer = gimp_image_get_active_layer (gimage);

  if (! layer)
    return;

  gimp_layer_add_alpha (layer);
  gdisplays_flush ();
}

void
layers_edit_attributes_cmd_callback (GtkWidget *widget,
				     gpointer   data)
{
  GimpImage *gimage;
  GimpLayer *layer;

  gimage = (GimpImage *) gimp_widget_get_callback_context (widget);

  if (! gimage)
    return;

  layer = gimp_image_get_active_layer (gimage);

  if (! layer)
    return;

  layers_edit_layer_query (layer);
}


/********************************/
/*  The new layer query dialog  */
/********************************/

typedef struct _NewLayerOptions NewLayerOptions;

struct _NewLayerOptions
{
  GtkWidget    *query_box;
  GtkWidget    *name_entry;
  GtkWidget    *size_se;

  GimpFillType  fill_type;
  gint          xsize;
  gint          ysize;

  GimpImage    *gimage;
};

static GimpFillType  fill_type  = TRANSPARENT_FILL;
static gchar        *layer_name = NULL;

static void
new_layer_query_ok_callback (GtkWidget *widget,
			     gpointer   data)
{
  NewLayerOptions *options;
  GimpLayer       *layer;
  GimpImage       *gimage;

  options = (NewLayerOptions *) data;

  if (layer_name)
    g_free (layer_name);
  layer_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (options->name_entry)));

  options->xsize = 
    RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (options->size_se), 0));
  options->ysize =
    RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (options->size_se), 1));

  fill_type = options->fill_type;

  if ((gimage = options->gimage))
    {
      /*  Start a group undo  */
      undo_push_group_start (gimage, LAYER_ADD_UNDO);

      layer = gimp_layer_new (gimage, options->xsize, options->ysize,
			      gimp_image_base_type_with_alpha (gimage),
			      layer_name, OPAQUE_OPACITY, NORMAL_MODE);
      if (layer) 
	{
	  drawable_fill (GIMP_DRAWABLE (layer), fill_type);
	  gimp_image_add_layer (gimage, layer, -1);
	  
	  /*  End the group undo  */
	  undo_push_group_end (gimage);
	  
	  gdisplays_flush ();
	} 
      else 
	{
	  g_message ("new_layer_query_ok_callback():\n"
		     "could not allocate new layer");
	}
    }

  gtk_widget_destroy (options->query_box);
}

void
layers_new_layer_query (GimpImage *gimage)
{
  NewLayerOptions *options;
  GtkWidget       *vbox;
  GtkWidget       *table;
  GtkWidget       *label;
  GtkObject       *adjustment;
  GtkWidget       *spinbutton;
  GtkWidget       *frame;

  /*  The new options structure  */
  options = g_new (NewLayerOptions, 1);
  options->fill_type = fill_type;
  options->gimage    = gimage;

  /*  The dialog  */
  options->query_box =
    gimp_dialog_new (_("New Layer Options"), "new_layer_options",
		     gimp_standard_help_func,
		     "dialogs/layers/new_layer.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("OK"), new_layer_query_ok_callback,
		     options, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), gtk_widget_destroy,
		     NULL, 1, NULL, FALSE, TRUE,

		     NULL);

  gtk_signal_connect_object (GTK_OBJECT (options->query_box), "destroy",
			     GTK_SIGNAL_FUNC (g_free),
			     (GtkObject *) options);

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     vbox);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  The name label and entry  */
  label = gtk_label_new (_("Layer Name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 1);
  gtk_widget_show (label);

  options->name_entry = gtk_entry_new ();
  gtk_widget_set_usize (options->name_entry, 75, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), 
			     options->name_entry, 1, 2, 0, 1);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),
		      (layer_name ? layer_name : _("New Layer")));
  gtk_widget_show (options->name_entry);

  /*  The size labels  */
  label = gtk_label_new (_("Layer Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /*  The size sizeentry  */
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 2);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  
  options->size_se = gimp_size_entry_new (1, gimage->unit, "%a",
					  TRUE, TRUE, FALSE, 75,
					  GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (options->size_se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (options->size_se), spinbutton,
                             1, 2, 0, 1);
  gtk_widget_show (spinbutton);
  gtk_table_attach (GTK_TABLE (table), options->size_se, 1, 2, 1, 3,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (options->size_se);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (options->size_se), 
			    GIMP_UNIT_PIXEL);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (options->size_se), 0,
                                  gimage->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (options->size_se), 1,
                                  gimage->yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (options->size_se), 0,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (options->size_se), 1,
                                         GIMP_MIN_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (options->size_se), 0,
			    0, gimage->width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (options->size_se), 1,
			    0, gimage->height);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (options->size_se), 0,
			      gimage->width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (options->size_se), 1,
			      gimage->height);

  gtk_widget_show (table);

  /*  The radio frame  */
  frame =
    gimp_radio_group_new2 (TRUE, _("Layer Fill Type"),
			   gimp_radio_button_update,
			   &options->fill_type, (gpointer) options->fill_type,

			   _("Foreground"),  (gpointer) FOREGROUND_FILL, NULL,
			   _("Background"),  (gpointer) BACKGROUND_FILL, NULL,
			   _("White"),       (gpointer) WHITE_FILL, NULL,
			   _("Transparent"), (gpointer) TRANSPARENT_FILL, NULL,

			   NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}

/**************************************/
/*  The edit layer attributes dialog  */
/**************************************/

typedef struct _EditLayerOptions EditLayerOptions;

struct _EditLayerOptions
{
  GtkWidget *query_box;
  GtkWidget *name_entry;
  GimpLayer *layer;
  GimpImage *gimage;
};

static void
edit_layer_query_ok_callback (GtkWidget *widget,
			      gpointer   data)
{
  EditLayerOptions *options;
  GimpLayer        *layer;

  options = (EditLayerOptions *) data;

  if ((layer = options->layer))
    {
      /*  Set the new layer name  */
      if (GIMP_OBJECT (layer)->name && gimp_layer_is_floating_sel (layer))
	{
	  /*  If the layer is a floating selection, make it a layer  */
	  floating_sel_to_layer (layer);
	}
      else
        {
	  /* We're doing a plain rename */
          undo_push_layer_rename (options->gimage, layer);
        }

      gimp_object_set_name (GIMP_OBJECT (layer),
			    gtk_entry_get_text (GTK_ENTRY (options->name_entry)));
    }

  gdisplays_flush ();

  gtk_widget_destroy (options->query_box); 
}

void
layers_edit_layer_query (GimpLayer *layer)
{
  EditLayerOptions *options;
  GtkWidget        *vbox;
  GtkWidget        *hbox;
  GtkWidget        *label;

  /*  The new options structure  */
  options = g_new (EditLayerOptions, 1);
  options->layer  = layer;
  options->gimage = gimp_drawable_gimage (GIMP_DRAWABLE (layer));

  /*  The dialog  */
  options->query_box =
    gimp_dialog_new (_("Edit Layer Attributes"), "edit_layer_attributes",
		     gimp_standard_help_func,
		     "dialogs/layers/edit_layer_attributes.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("OK"), edit_layer_query_ok_callback,
		     options, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), gtk_widget_destroy,
		     NULL, 1, NULL, FALSE, TRUE,

		     NULL);

  gtk_signal_connect_object (GTK_OBJECT (options->query_box), "destroy",
			     GTK_SIGNAL_FUNC (g_free),
			     (GtkObject *) options);

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     vbox);

  /*  The name hbox, label and entry  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Layer name:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), options->name_entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry),
		      ((gimp_layer_is_floating_sel (layer) ?
			_("Floating Selection") :
			gimp_object_get_name (GIMP_OBJECT (layer)))));
  gtk_signal_connect (GTK_OBJECT (options->name_entry), "activate",
		      edit_layer_query_ok_callback,
		      options);
  gtk_widget_show (options->name_entry);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}

/*******************************/
/*  The add mask query dialog  */
/*******************************/

typedef struct _AddMaskOptions AddMaskOptions;

struct _AddMaskOptions
{
  GtkWidget   *query_box;
  GimpLayer   *layer;
  AddMaskType  add_mask_type;
};

static void
add_mask_query_ok_callback (GtkWidget *widget,
			    gpointer   data)
{
  AddMaskOptions *options;
  GimpImage      *gimage;
  GimpLayerMask  *mask;
  GimpLayer      *layer;

  options = (AddMaskOptions *) data;

  if ((layer = (options->layer)) &&
      (gimage = GIMP_DRAWABLE (layer)->gimage))
    {
      mask = gimp_layer_create_mask (layer, options->add_mask_type);
      gimp_layer_add_mask (layer, mask, TRUE);
      gdisplays_flush ();
    }

  gtk_widget_destroy (options->query_box);
}

static void
layers_add_mask_query (GimpLayer *layer)
{
  AddMaskOptions *options;
  GtkWidget      *frame;
  GimpImage      *gimage;
  
  /*  The new options structure  */
  options = g_new (AddMaskOptions, 1);
  options->layer         = layer;
  options->add_mask_type = ADD_WHITE_MASK;

  gimage = GIMP_DRAWABLE (layer)->gimage;
  
  /*  The dialog  */
  options->query_box =
    gimp_dialog_new (_("Add Mask Options"), "add_mask_options",
		     gimp_standard_help_func,
		     "dialogs/layers/add_layer_mask.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("OK"), add_mask_query_ok_callback,
		     options, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), gtk_widget_destroy,
		     NULL, 1, NULL, FALSE, TRUE,

		     NULL);

  gtk_signal_connect_object (GTK_OBJECT (options->query_box), "destroy",
			     GTK_SIGNAL_FUNC (g_free),
			     (GtkObject *) options);

  /*  The radio frame and box  */
  if (gimage->selection_mask)
    {
      options->add_mask_type = ADD_SELECTION_MASK;

      frame = gimp_radio_group_new2 (TRUE, _("Initialize Layer Mask to:"),
				     gimp_radio_button_update,
				     &options->add_mask_type,
				     (gpointer) options->add_mask_type,

				     _("Selection"),
				     (gpointer) ADD_SELECTION_MASK, NULL,
				     _("Inverse Selection"),
				     (gpointer) ADD_INV_SELECTION_MASK, NULL,
				     _("White (Full Opacity)"),
				     (gpointer) ADD_WHITE_MASK, NULL,
				     _("Black (Full Transparency)"),
				     (gpointer) ADD_BLACK_MASK, NULL,
				     _("Layer's Alpha Channel"),
				     (gpointer) ADD_ALPHA_MASK, NULL,
				     NULL);
    }
  else
    {
      frame = gimp_radio_group_new2 (TRUE, _("Initialize Layer Mask to:"),
				     gimp_radio_button_update,
				     &options->add_mask_type,
				     (gpointer) options->add_mask_type,

				     _("White (Full Opacity)"),
				     (gpointer) ADD_WHITE_MASK, NULL,
				     _("Black (Full Transparency)"),
				     (gpointer) ADD_BLACK_MASK, NULL,
				     _("Layer's Alpha Channel"),
				     (gpointer) ADD_ALPHA_MASK, NULL,
				     NULL);
    }
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     frame);
  gtk_widget_show (frame);

  gtk_widget_show (options->query_box);
}


/****************************/
/*  The scale layer dialog  */
/****************************/

typedef struct _ScaleLayerOptions ScaleLayerOptions;

struct _ScaleLayerOptions
{
  GimpLayer *layer;
  Resize    *resize;
};

static void
scale_layer_query_ok_callback (GtkWidget *widget,
			       gpointer   data)
{
  ScaleLayerOptions *options;
  GimpImage         *gimage;
  GimpLayer         *layer;

  options = (ScaleLayerOptions *) data;

  if (options->resize->width > 0 && options->resize->height > 0 &&
      (layer =  (options->layer)))
    {
      gtk_widget_set_sensitive (options->resize->resize_shell, FALSE);

      if ((gimage = GIMP_DRAWABLE (layer)->gimage) != NULL)
	{
	  undo_push_group_start (gimage, LAYER_SCALE_UNDO);

	  if (gimp_layer_is_floating_sel (layer))
	    floating_sel_relax (layer, TRUE);

	  gimp_layer_scale (layer, 
			    options->resize->width, options->resize->height,
			    TRUE);

	  if (gimp_layer_is_floating_sel (layer))
	    floating_sel_rigor (layer, TRUE);

	  undo_push_group_end (gimage);

	  gdisplays_flush ();
	}

      gtk_widget_destroy (options->resize->resize_shell);
    }
  else
    {
      g_message (_("Invalid width or height.\n"
		   "Both must be positive."));
    }
}

static void
layers_scale_layer_query (GimpImage *gimage,
				 GimpLayer *layer)
{
  ScaleLayerOptions *options;

  /*  the new options structure  */
  options = g_new (ScaleLayerOptions, 1);
  options->layer  = layer;
  options->resize =
    resize_widget_new (ScaleWidget,
		       ResizeLayer,
		       GTK_OBJECT (layer),
		       "removed",
		       gimp_drawable_width (GIMP_DRAWABLE (layer)),
		       gimp_drawable_height (GIMP_DRAWABLE (layer)),
		       gimage->xresolution,
		       gimage->yresolution,
		       gimage->unit,
		       TRUE,
		       scale_layer_query_ok_callback,
		       NULL,
		       options);

  gtk_signal_connect_object (GTK_OBJECT (options->resize->resize_shell),
			     "destroy",
			     GTK_SIGNAL_FUNC (g_free),
			     (GtkObject *) options);

  gtk_widget_show (options->resize->resize_shell);
}

/*****************************/
/*  The resize layer dialog  */
/*****************************/

typedef struct _ResizeLayerOptions ResizeLayerOptions;

struct _ResizeLayerOptions
{
  GimpLayer *layer;
  Resize    *resize;
};

static void
resize_layer_query_ok_callback (GtkWidget *widget,
				gpointer   data)
{
  ResizeLayerOptions *options;
  GimpImage          *gimage;
  GimpLayer          *layer;

  options = (ResizeLayerOptions *) data;

  if (options->resize->width > 0 && options->resize->height > 0 &&
      (layer = (options->layer)))
    {
      gtk_widget_set_sensitive (options->resize->resize_shell, FALSE);

      if ((gimage = GIMP_DRAWABLE (layer)->gimage) != NULL)
	{
	  undo_push_group_start (gimage, LAYER_RESIZE_UNDO);

	  if (gimp_layer_is_floating_sel (layer))
	    floating_sel_relax (layer, TRUE);

	  gimp_layer_resize (layer,
			     options->resize->width,
			     options->resize->height,
			     options->resize->offset_x,
			     options->resize->offset_y);

	  if (gimp_layer_is_floating_sel (layer))
	    floating_sel_rigor (layer, TRUE);

	  undo_push_group_end (gimage);

	  gdisplays_flush ();
	}

      gtk_widget_destroy (options->resize->resize_shell);
    }
  else
    {
      g_message (_("Invalid width or height.\n"
		   "Both must be positive."));
    }
}

static void
layers_resize_layer_query (GimpImage *gimage,
				  GimpLayer *layer)
{
  ResizeLayerOptions *options;

  /*  the new options structure  */
  options = g_new (ResizeLayerOptions, 1);
  options->layer  = layer;
  options->resize =
    resize_widget_new (ResizeWidget,
		       ResizeLayer,
		       GTK_OBJECT (layer),
		       "removed",
		       gimp_drawable_width (GIMP_DRAWABLE (layer)),
		       gimp_drawable_height (GIMP_DRAWABLE (layer)),
		       gimage->xresolution,
		       gimage->yresolution,
		       gimage->unit,
		       TRUE,
		       resize_layer_query_ok_callback,
		       NULL,
		       options);

  gtk_signal_connect_object (GTK_OBJECT (options->resize->resize_shell),
			     "destroy",
			     GTK_SIGNAL_FUNC (g_free),
			     (GtkObject *) options);

  gtk_widget_show (options->resize->resize_shell);
}

/****************************/
/*  The layer merge dialog  */
/****************************/

typedef struct _LayerMergeOptions LayerMergeOptions;

struct _LayerMergeOptions
{
  GtkWidget *query_box;
  GimpImage *gimage;
  gboolean   merge_visible;
  MergeType  merge_type;
};

static void
layer_merge_query_ok_callback (GtkWidget *widget,
			       gpointer   data)
{
  LayerMergeOptions *options;
  GimpImage         *gimage;

  options = (LayerMergeOptions *) data;
  if (! (gimage = options->gimage))
    return;

  if (options->merge_visible)
    gimp_image_merge_visible_layers (gimage, options->merge_type);

  gdisplays_flush ();

  gtk_widget_destroy (options->query_box);
}

void
layers_layer_merge_query (GimpImage   *gimage,
				 /*  if FALSE, anchor active layer  */
				 gboolean     merge_visible)
{
  LayerMergeOptions *options;
  GtkWidget         *vbox;
  GtkWidget         *frame;

  /*  The new options structure  */
  options = g_new (LayerMergeOptions, 1);
  options->gimage        = gimage;
  options->merge_visible = merge_visible;
  options->merge_type    = EXPAND_AS_NECESSARY;

  /* The dialog  */
  options->query_box =
    gimp_dialog_new (_("Layer Merge Options"), "layer_merge_options",
		     gimp_standard_help_func,
		     "dialogs/layers/merge_visible_layers.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("OK"), layer_merge_query_ok_callback,
		     options, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), gtk_widget_destroy,
		     NULL, 1, NULL, FALSE, TRUE,

		     NULL);

  gtk_signal_connect_object (GTK_OBJECT (options->query_box), "destroy",
			     GTK_SIGNAL_FUNC (g_free),
			     (GtkObject *) options);

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->query_box)->vbox),
		     vbox);

  frame = gimp_radio_group_new2 (TRUE,
				 merge_visible ?
				 _("Final, Merged Layer should be:") :
				 _("Final, Anchored Layer should be:"),
				 gimp_radio_button_update,
				 &options->merge_type,
				 (gpointer) options->merge_type,

				 _("Expanded as necessary"),
				 (gpointer) EXPAND_AS_NECESSARY, NULL,
				 _("Clipped to image"),
				 (gpointer) CLIP_TO_IMAGE, NULL,
				 _("Clipped to bottom layer"),
				 (gpointer) CLIP_TO_BOTTOM_LAYER, NULL,

				 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (vbox);
  gtk_widget_show (options->query_box);
}
