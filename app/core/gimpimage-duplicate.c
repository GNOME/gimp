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
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "actionarea.h"
#include "channel_ops.h"
#include "cursorutil.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "interface.h"
#include "palette.h"

#include "libgimp/gimpsizeentry.h"
#include "libgimp/gimpintl.h"

#include "channel_pvt.h"
#include "layer_pvt.h"

#define ENTRY_WIDTH  60

typedef struct _OffsetDialog OffsetDialog;

struct _OffsetDialog
{
  GtkWidget         *dlg;
  GtkWidget         *off_se;

  gboolean           wrap_around;

  GtkWidget         *fill_options;
  ChannelOffsetType  fill_type;

  GimpImage         *gimage;
};

/*  Forward declarations  */
static void  offset_ok_callback         (GtkWidget *, gpointer);
static void  offset_cancel_callback     (GtkWidget *, gpointer);
static gint  offset_delete_callback     (GtkWidget *, GdkEvent *, gpointer);

static void  offset_wraparound_update   (GtkWidget *, gpointer);
static void  offset_fill_type_update    (GtkWidget *, gpointer);
static void  offset_halfheight_callback (GtkWidget *, gpointer);


void
channel_ops_offset (GimpImage* gimage)
{
  OffsetDialog *off_d;
  GtkWidget *label;
  GtkWidget *check;
  GtkWidget *push;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkObject *adjustment;
  GtkWidget *spinbutton;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GSList *group = NULL;

  GimpDrawable *drawable;

  static ActionAreaItem action_items[] =
  {
    { N_("OK"), offset_ok_callback, NULL, NULL },
    { N_("Cancel"), offset_cancel_callback, NULL, NULL }
  };

  drawable = gimage_active_drawable (gimage);

  off_d = g_new (OffsetDialog, 1);
  off_d->wrap_around = TRUE;
  off_d->fill_type   = drawable_has_alpha (drawable);
  off_d->gimage      = gimage;

  off_d->dlg = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (off_d->dlg), "offset", "Gimp");
  gtk_window_set_title (GTK_WINDOW (off_d->dlg), _("Offset"));

  /*  Handle the wm close signal  */
  gtk_signal_connect (GTK_OBJECT (off_d->dlg), "delete_event",
		      GTK_SIGNAL_FUNC (offset_delete_callback),
		      off_d);

  /*  The vbox for first column of options  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (off_d->dlg)->vbox), vbox);

  /*  The table for the offsets  */
  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  The offset labels  */
  label = gtk_label_new (_("Offset X:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Y:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /*  The offset sizeentry  */
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 2);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
                                   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  
  off_d->off_se = gimp_size_entry_new (1, gimage->unit, "%a",
				       TRUE, TRUE, FALSE, 75,
				       GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (off_d->off_se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (off_d->off_se), spinbutton,
                             1, 2, 0, 1);
  gtk_widget_show (spinbutton);
  gtk_table_attach (GTK_TABLE (table), off_d->off_se, 1, 2, 0, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (off_d->off_se);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (off_d->off_se), UNIT_PIXEL);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (off_d->off_se), 0,
                                  gimage->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (off_d->off_se), 1,
                                  gimage->yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (off_d->off_se), 0,
                                         -gimage->width, gimage->width);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (off_d->off_se), 1,
					 -gimage->height, gimage->height);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (off_d->off_se), 0,
                            0, gimage->width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (off_d->off_se), 1,
                            0, gimage->height);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (off_d->off_se), 0, 0);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (off_d->off_se), 1, 0);

  gtk_widget_show (table);

  /*  The wrap around option  */
  check = gtk_check_button_new_with_label (_("Wrap-Around"));
  gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
  gtk_widget_show (check);

  /*  The fill options  */
  off_d->fill_options = gtk_frame_new (_("Fill Options"));
  gtk_box_pack_start (GTK_BOX (vbox), off_d->fill_options, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (off_d->fill_options), radio_box);

  radio_button = gtk_radio_button_new_with_label (group, _("Background"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
  gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (radio_button), "merge_type",
		       (gpointer) OFFSET_BACKGROUND);
  gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
		      (GtkSignalFunc) offset_fill_type_update,
		      &off_d->fill_type);
  gtk_widget_show (radio_button);

  if (drawable_has_alpha (drawable))
    {
      radio_button = gtk_radio_button_new_with_label (group, _("Transparent"));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_object_set_data (GTK_OBJECT (radio_button), "merge_type",
			   (gpointer) OFFSET_TRANSPARENT);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) offset_fill_type_update,
			  &off_d->fill_type);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_button), TRUE);
      gtk_widget_show (radio_button);
    }

  /*  The by half height and half width option */
  push = gtk_button_new_with_label (_("Offset by (x/2),(y/2)"));
  gtk_container_set_border_width (GTK_CONTAINER (push), 2);
  gtk_box_pack_start (GTK_BOX (vbox), push, FALSE, FALSE, 0);
  gtk_widget_show (push);

  gtk_widget_show (radio_box);
  gtk_widget_show (off_d->fill_options);

  /*  Hook up the wrap around  */
  gtk_signal_connect (GTK_OBJECT (check), "toggled",
		      (GtkSignalFunc) offset_wraparound_update,
		      off_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), off_d->wrap_around);

  /*  Hook up the by half  */
  gtk_signal_connect (GTK_OBJECT (push), "clicked",
		      (GtkSignalFunc) offset_halfheight_callback,
		      off_d);

  action_items[0].user_data = off_d;
  action_items[1].user_data = off_d;
  build_action_area (GTK_DIALOG (off_d->dlg), action_items, 2, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (off_d->dlg);
}


void
offset (GimpImage    *gimage,
	GimpDrawable *drawable,
	gboolean      wrap_around,
	gint          fill_type,
	gint          offset_x,
	gint          offset_y)
{
  PixelRegion srcPR, destPR;
  TileManager *new_tiles;
  int width, height;
  int src_x, src_y;
  int dest_x, dest_y;
  unsigned char fill[MAX_CHANNELS] = { 0 };

  if (!drawable) 
    return;

  width = drawable_width (drawable);
  height = drawable_height (drawable);

  if (wrap_around)
    {
      offset_x %= width;
      offset_y %= height;
    }
  else
    {
      offset_x = BOUNDS (offset_x, -width, width);
      offset_y = BOUNDS (offset_y, -height, height);
    }

  if (offset_x == 0 && offset_y == 0)
    return;

  new_tiles = tile_manager_new (width, height, drawable_bytes (drawable));
  if (offset_x >= 0)
    {
      src_x = 0;
      dest_x = offset_x;
      width = BOUNDS ((width - offset_x), 0, width);
    }
  else
    {
      src_x = -offset_x;
      dest_x = 0;
      width = BOUNDS ((width + offset_x), 0, width);
    }

  if (offset_y >= 0)
    {
      src_y = 0;
      dest_y = offset_y;
      height = BOUNDS ((height - offset_y), 0, height);
    }
  else
    {
      src_y = -offset_y;
      dest_y = 0;
      height = BOUNDS ((height + offset_y), 0, height);
    }

  /*  Copy the center region  */
  if (width && height)
    {
      pixel_region_init (&srcPR, drawable_data (drawable), src_x, src_y, width, height, FALSE);
      pixel_region_init (&destPR, new_tiles, dest_x, dest_y, width, height, TRUE);

      copy_region (&srcPR, &destPR);
    }

  /*  Copy appropriately for wrap around  */
  if (wrap_around == TRUE)
    {
      if (offset_x >= 0 && offset_y >= 0)
	{
	  src_x = drawable_width (drawable) - offset_x;
	  src_y = drawable_height (drawable) - offset_y;
	}
      else if (offset_x >= 0 && offset_y < 0)
	{
	  src_x = drawable_width (drawable) - offset_x;
	  src_y = 0;
	}
      else if (offset_x < 0 && offset_y >= 0)
	{
	  src_x = 0;
	  src_y = drawable_height (drawable) - offset_y;
	}
      else if (offset_x < 0 && offset_y < 0)
	{
	  src_x = 0;
	  src_y = 0;
	}

      dest_x = (src_x + offset_x) % drawable_width (drawable);
      if (dest_x < 0)
	dest_x = drawable_width (drawable) + dest_x;
      dest_y = (src_y + offset_y) % drawable_height (drawable);
      if (dest_y < 0)
	dest_y = drawable_height (drawable) + dest_y;

      /*  intersecting region  */
      if (offset_x != 0 && offset_y != 0)
	{
	  pixel_region_init (&srcPR, drawable_data (drawable), src_x, src_y,
			     ABS (offset_x), ABS (offset_y), FALSE);
	  pixel_region_init (&destPR, new_tiles, dest_x, dest_y, ABS (offset_x), ABS (offset_y), TRUE);
	  copy_region (&srcPR, &destPR);
	}

      /*  X offset  */
      if (offset_x != 0)
	{
	  if (offset_y >= 0)
	    {
	      pixel_region_init (&srcPR, drawable_data (drawable), src_x, 0,
				 ABS (offset_x), drawable_height (drawable) - ABS (offset_y), FALSE);
	      pixel_region_init (&destPR, new_tiles, dest_x, dest_y + offset_y,
				 ABS (offset_x), drawable_height (drawable) - ABS (offset_y), TRUE);
	    }
	  else if (offset_y < 0)
	    {
	      pixel_region_init (&srcPR, drawable_data (drawable), src_x, src_y - offset_y,
				 ABS (offset_x), drawable_height (drawable) - ABS (offset_y), FALSE);
	      pixel_region_init (&destPR, new_tiles, dest_x, 0,
				 ABS (offset_x), drawable_height (drawable) - ABS (offset_y), TRUE);
	    }

	  copy_region (&srcPR, &destPR);
	}

      /*  X offset  */
      if (offset_y != 0)
	{
	  if (offset_x >= 0)
	    {
	      pixel_region_init (&srcPR, drawable_data (drawable), 0, src_y,
				 drawable_width (drawable) - ABS (offset_x), ABS (offset_y), FALSE);
	      pixel_region_init (&destPR, new_tiles, dest_x + offset_x, dest_y,
				 drawable_width (drawable) - ABS (offset_x), ABS (offset_y), TRUE);
	    }
	  else if (offset_x < 0)
	    {
	      pixel_region_init (&srcPR, drawable_data (drawable), src_x - offset_x, src_y,
				 drawable_width (drawable) - ABS (offset_x), ABS (offset_y), FALSE);
	      pixel_region_init (&destPR, new_tiles, 0, dest_y,
				 drawable_width (drawable) - ABS (offset_x), ABS (offset_y), TRUE);
	    }

	  copy_region (&srcPR, &destPR);
	}
    }
  /*  Otherwise, fill the vacated regions  */
  else
    {
      if (fill_type == OFFSET_BACKGROUND)
	{
	  palette_get_background (&fill[0], &fill[1], &fill[2]);
	  if (drawable_has_alpha (drawable))
	    fill[drawable_bytes (drawable) - 1] = OPAQUE_OPACITY;
	}

      if (offset_x >= 0 && offset_y >= 0)
	{
	  dest_x = 0;
	  dest_y = 0;
	}
      else if (offset_x >= 0 && offset_y < 0)
	{
	  dest_x = 0;
	  dest_y = drawable_height (drawable) + offset_y;
	}
      else if (offset_x < 0 && offset_y >= 0)
	{
	  dest_x = drawable_width (drawable) + offset_x;
	  dest_y = 0;
	}
      else if (offset_x < 0 && offset_y < 0)
	{
	  dest_x = drawable_width (drawable) + offset_x;
	  dest_y = drawable_height (drawable) + offset_y;
	}

      /*  intersecting region  */
      if (offset_x != 0 && offset_y != 0)
	{
	  pixel_region_init (&destPR, new_tiles, dest_x, dest_y, ABS (offset_x), ABS (offset_y), TRUE);
	  color_region (&destPR, fill);
	}

      /*  X offset  */
      if (offset_x != 0)
	{
	  if (offset_y >= 0)
	    pixel_region_init (&destPR, new_tiles, dest_x, dest_y + offset_y,
			       ABS (offset_x), drawable_height (drawable) - ABS (offset_y), TRUE);
	  else if (offset_y < 0)
	    pixel_region_init (&destPR, new_tiles, dest_x, 0,
			       ABS (offset_x), drawable_height (drawable) - ABS (offset_y), TRUE);

	  color_region (&destPR, fill);
	}

      /*  X offset  */
      if (offset_y != 0)
	{
	  if (offset_x >= 0)
	    pixel_region_init (&destPR, new_tiles, dest_x + offset_x, dest_y,
			       drawable_width (drawable) - ABS (offset_x), ABS (offset_y), TRUE);
	  else if (offset_x < 0)
	    pixel_region_init (&destPR, new_tiles, 0, dest_y,
			       drawable_width (drawable) - ABS (offset_x), ABS (offset_y), TRUE);

	  color_region (&destPR, fill);
	}
    }

  /*  push an undo  */
  drawable_apply_image (drawable, 0, 0,
			drawable_width (drawable), drawable_height (drawable),
			drawable_data (drawable), FALSE);

  /*  swap the tiles  */
  drawable->tiles = new_tiles;


  /*  update the drawable  */
  drawable_update (drawable, 0, 0,
		   drawable_width (drawable), drawable_height (drawable));
}

/*
 *  Interface callbacks
 */

static void
offset_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  OffsetDialog *off_d;
  GImage       *gimage;
  GimpDrawable *drawable;
  gint offset_x;
  gint offset_y;

  off_d = (OffsetDialog *) data;
  if ((gimage = off_d->gimage) != NULL)
    {
      drawable = gimage_active_drawable (gimage);

      offset_x = (gint)
	(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (off_d->off_se), 0) + 0.5);
      offset_y = (gint)
	(gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (off_d->off_se), 1) + 0.5);

      offset (gimage, drawable, off_d->wrap_around, off_d->fill_type,
	      offset_x, offset_y);
      gdisplays_flush ();
    }

  gtk_widget_destroy (off_d->dlg);
  g_free (off_d);
}

static gint
offset_delete_callback (GtkWidget *widget,
			GdkEvent  *event,
			gpointer   data)
{
  offset_cancel_callback (widget, data);

  return TRUE;
}

static void
offset_cancel_callback (GtkWidget *widget,
			gpointer   data)
{
  OffsetDialog *off_d;

  off_d = (OffsetDialog *) data;
  gtk_widget_destroy (off_d->dlg);
  g_free (off_d);
}

static void
offset_wraparound_update (GtkWidget *widget,
			  gpointer   data)
{
  OffsetDialog *off_d;

  off_d = (OffsetDialog *) data;

  off_d->wrap_around =  GTK_TOGGLE_BUTTON (widget)->active ? TRUE : FALSE;

  gtk_widget_set_sensitive (off_d->fill_options, !off_d->wrap_around);
}

static void
offset_fill_type_update (GtkWidget *widget,
			 gpointer   data)
{
  OffsetDialog *off_d;

  off_d = (OffsetDialog *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    off_d->fill_type =
      (ChannelOffsetType) gtk_object_get_data (GTK_OBJECT (widget), "fill_type");

}

static void
offset_halfheight_callback (GtkWidget *widget,
			    gpointer   data)
{
  OffsetDialog *off_d;
  GImage *gimage;

  off_d = (OffsetDialog *) data;
  gimage = off_d->gimage;

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (off_d->off_se),
			      0, gimage->width / 2);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (off_d->off_se),
			      1, gimage->width / 2);
}

GimpImage *
duplicate (GimpImage *gimage)
{
  PixelRegion srcPR, destPR;
  GimpImage *new_gimage;
  Layer *layer, *new_layer;
  Layer *floating_layer;
  Channel *channel, *new_channel;
  GSList *list;
  GList *glist;
  Guide *guide = NULL;
  Layer *active_layer = NULL;
  Channel *active_channel = NULL;
  GimpDrawable *new_floating_sel_drawable = NULL;
  GimpDrawable *floating_sel_drawable = NULL;
  gint count;

  gimp_add_busy_cursors_until_idle ();

  /*  Create a new image  */
  new_gimage = gimage_new (gimage->width, gimage->height, gimage->base_type);
  gimage_disable_undo (new_gimage);

  /*  Copy-on-write the projection tilemanager so we don't have
   *  to reproject the new gimage - since if we do the duplicate
   *  operation correctly, the projection for the new gimage is
   *  identical to that of the source.
   */
  new_gimage->construct_flag = gimage->construct_flag;
  new_gimage->proj_type = gimage->proj_type;
  new_gimage->proj_bytes = gimage->proj_bytes;
  new_gimage->proj_level = gimage->proj_level;
  pixel_region_init (&srcPR, gimp_image_projection (gimage), 0, 0,
		     gimage->width, gimage->height, FALSE);
  pixel_region_init (&destPR, gimp_image_projection (new_gimage), 0, 0,
		     new_gimage->width, new_gimage->height, TRUE);
  /*  We don't want to copy a half-redrawn projection, so force a flush.  */
  gdisplays_finish_draw();
  copy_region(&srcPR, &destPR);

  /*  Copy floating layer  */
  floating_layer = gimage_floating_sel (gimage);
  if (floating_layer)
    {
      floating_sel_relax (floating_layer, FALSE);

      floating_sel_drawable = floating_layer->fs.drawable;
      floating_layer = NULL;
    }

  /*  Copy the layers  */
  list = gimage->layers;
  count = 0;
  layer = NULL;
  while (list)
    {
      layer = (Layer *) list->data;
      list = g_slist_next (list);

      new_layer = layer_copy (layer, FALSE);

      gimp_drawable_set_gimage(GIMP_DRAWABLE(new_layer), new_gimage);

      /*  Make sure the copied layer doesn't say: "<old layer> copy"  */
      layer_set_name(GIMP_LAYER(new_layer),
		     layer_get_name(GIMP_LAYER(layer)));

      /*  Make sure if the layer has a layer mask, it's name isn't screwed up  */
      if (new_layer->mask)
	{
	  gimp_drawable_set_name(GIMP_DRAWABLE(new_layer->mask),
			  gimp_drawable_get_name(GIMP_DRAWABLE(layer->mask)));
	}

      if (gimage->active_layer == layer)
	active_layer = new_layer;

      if (gimage->floating_sel == layer)
	floating_layer = new_layer;

      if (floating_sel_drawable == GIMP_DRAWABLE(layer))
	new_floating_sel_drawable = GIMP_DRAWABLE(new_layer);

      /*  Add the layer  */
      if (floating_layer != new_layer)
	gimage_add_layer (new_gimage, new_layer, count++);
    }

  /*  Copy the channels  */
  list = gimage->channels;
  count = 0;
  while (list)
    {
      channel = (Channel *) list->data;
      list = g_slist_next (list);

      new_channel = channel_copy (channel);

      gimp_drawable_set_gimage(GIMP_DRAWABLE(new_channel), new_gimage);

      /*  Make sure the copied channel doesn't say: "<old channel> copy"  */
      gimp_drawable_set_name(GIMP_DRAWABLE(new_channel),
			     gimp_drawable_get_name(GIMP_DRAWABLE(channel)));

      if (gimage->active_channel == channel)
	active_channel = (new_channel);

      if (floating_sel_drawable == GIMP_DRAWABLE(channel))
	new_floating_sel_drawable = GIMP_DRAWABLE(new_channel);

      /*  Add the channel  */
      gimage_add_channel (new_gimage, new_channel, count++);
    }

  /*  Copy the selection mask  */
  pixel_region_init (&srcPR, drawable_data (GIMP_DRAWABLE(gimage->selection_mask)), 0, 0, gimage->width, gimage->height, FALSE);
  pixel_region_init (&destPR, drawable_data (GIMP_DRAWABLE(new_gimage->selection_mask)), 0, 0, gimage->width, gimage->height, TRUE);
  copy_region (&srcPR, &destPR);
  new_gimage->selection_mask->bounds_known = FALSE;
  new_gimage->selection_mask->boundary_known = FALSE;

  /*  Set active layer, active channel  */
  new_gimage->active_layer = active_layer;
  new_gimage->active_channel = active_channel;
  if (floating_layer)
    floating_sel_attach (floating_layer, new_floating_sel_drawable);

  /*  Copy the colormap if necessary  */
  if (new_gimage->base_type == INDEXED)
    memcpy (new_gimage->cmap, gimage->cmap, gimage->num_cols * 3);
  new_gimage->num_cols = gimage->num_cols;

  /*  copy state of all color channels  */
  for (count = 0; count < MAX_CHANNELS; count++)
    {
      new_gimage->visible[count] = gimage->visible[count];
      new_gimage->active[count] = gimage->active[count];
    }

  /*  Copy any Guides  */
  glist = gimage->guides;
  while (glist)
    {
      Guide* new_guide;

      guide = (Guide*) glist->data;
      glist = g_list_next (glist);

      switch (guide->orientation)
	{
	case HORIZONTAL_GUIDE:
	  new_guide = gimp_image_add_hguide(new_gimage);
	  new_guide->position = guide->position;
	  break;
	case VERTICAL_GUIDE:
	  new_guide = gimp_image_add_vguide(new_gimage);
	  new_guide->position = guide->position;
	  break;
	default:
	  g_error("Unknown guide orientation.\n");
	}
    }

  gimage_enable_undo (new_gimage);

  return new_gimage;
}

void
channel_ops_duplicate (GimpImage *gimage)
{
  gdisplay_new (duplicate (gimage), 0x0101);
}
