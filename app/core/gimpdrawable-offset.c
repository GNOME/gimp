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

#include "appenv.h"
#include "channel_ops.h"
#include "cursorutil.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimpcontext.h"
#include "gimpui.h"
#include "interface.h"
#include "parasitelist.h"

#include "libgimp/gimpsizeentry.h"
#include "libgimp/gimpmath.h"
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
  ChannelOffsetType  fill_type;

  GimpImage         *gimage;
};

/*  Forward declarations  */
static void  offset_ok_callback         (GtkWidget *, gpointer);
static void  offset_cancel_callback     (GtkWidget *, gpointer);

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
  GtkWidget *frame;
  GtkWidget *radio_button;

  GimpDrawable *drawable;

  drawable = gimage_active_drawable (gimage);

  off_d = g_new (OffsetDialog, 1);
  off_d->wrap_around = TRUE;
  off_d->fill_type   = drawable_has_alpha (drawable);
  off_d->gimage      = gimage;

  off_d->dlg = gimp_dialog_new (_("Offset"), "offset",
				gimp_standard_help_func,
				"dialogs/offset.html",
				GTK_WIN_POS_NONE,
				FALSE, TRUE, FALSE,

				_("OK"), offset_ok_callback,
				off_d, NULL, NULL, TRUE, FALSE,
				_("Cancel"), offset_cancel_callback,
				off_d, NULL, NULL, FALSE, TRUE,

				NULL);
				
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

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (off_d->off_se), GIMP_UNIT_PIXEL);

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
  check = gtk_check_button_new_with_label (_("Wrap Around"));
  gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
  gtk_widget_show (check);

  /*  The fill options  */
  frame =
    gimp_radio_group_new2 (TRUE, _("Fill Type"),
			   gimp_radio_button_update,
			   &off_d->fill_type, (gpointer) off_d->fill_type,

			   _("Background"), (gpointer) OFFSET_BACKGROUND, NULL,
			   _("Transparent"), (gpointer) OFFSET_TRANSPARENT,
			   &radio_button,

			   NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  if (!drawable_has_alpha (drawable))
    gtk_widget_set_sensitive (radio_button, FALSE);

  /*  The by half height and half width option */
  push = gtk_button_new_with_label (_("Offset by (x/2),(y/2)"));
  gtk_container_set_border_width (GTK_CONTAINER (push), 2);
  gtk_box_pack_start (GTK_BOX (vbox), push, FALSE, FALSE, 0);
  gtk_widget_show (push);

  /*  Hook up the wrap around  */
  gtk_signal_connect (GTK_OBJECT (check), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &off_d->wrap_around);
  gtk_object_set_data (GTK_OBJECT (check), "inverse_sensitive", frame);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), off_d->wrap_around);

  /*  Hook up the by half  */
  gtk_signal_connect (GTK_OBJECT (push), "clicked",
		      (GtkSignalFunc) offset_halfheight_callback,
		      off_d);

  gtk_widget_show (vbox);
  gtk_widget_show (off_d->dlg);
}

void
offset (GimpImage         *gimage,
	GimpDrawable      *drawable,
	gboolean           wrap_around,
	ChannelOffsetType  fill_type,
	gint               offset_x,
	gint               offset_y)
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
      offset_x = CLAMP (offset_x, -width, width);
      offset_y = CLAMP (offset_y, -height, height);
    }

  if (offset_x == 0 && offset_y == 0)
    return;

  new_tiles = tile_manager_new (width, height, drawable_bytes (drawable));
  if (offset_x >= 0)
    {
      src_x = 0;
      dest_x = offset_x;
      width = CLAMP ((width - offset_x), 0, width);
    }
  else
    {
      src_x = -offset_x;
      dest_x = 0;
      width = CLAMP ((width + offset_x), 0, width);
    }

  if (offset_y >= 0)
    {
      src_y = 0;
      dest_y = offset_y;
      height = CLAMP ((height - offset_y), 0, height);
    }
  else
    {
      src_y = -offset_y;
      dest_y = 0;
      height = CLAMP ((height + offset_y), 0, height);
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
				 drawable_width (drawable) - ABS (offset_x),
				 ABS (offset_y), FALSE);
	      pixel_region_init (&destPR, new_tiles, dest_x + offset_x, dest_y,
				 drawable_width (drawable) - ABS (offset_x),
				 ABS (offset_y), TRUE);
	    }
	  else if (offset_x < 0)
	    {
	      pixel_region_init (&srcPR, drawable_data (drawable),
				 src_x - offset_x, src_y,
				 drawable_width (drawable) - ABS (offset_x),
				 ABS (offset_y), FALSE);
	      pixel_region_init (&destPR, new_tiles, 0, dest_y,
				 drawable_width (drawable) - ABS (offset_x),
				 ABS (offset_y), TRUE);
	    }

	  copy_region (&srcPR, &destPR);
	}
    }
  /*  Otherwise, fill the vacated regions  */
  else
    {
      if (fill_type == OFFSET_BACKGROUND)
	{
	  gimp_context_get_background (NULL, &fill[0], &fill[1], &fill[2]);
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
	  pixel_region_init (&destPR, new_tiles, dest_x, dest_y,
			     ABS (offset_x), ABS (offset_y), TRUE);
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
			      1, gimage->height / 2);
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
  ParasiteList *parasites;
  PathList *paths;
  gint count;

  gimp_add_busy_cursors_until_idle ();

  /*  Create a new image  */
  new_gimage = gimage_new (gimage->width, gimage->height, gimage->base_type);
  gimage_disable_undo (new_gimage);

  /*  Copy resolution and unit information  */
  new_gimage->xresolution = gimage->xresolution;
  new_gimage->yresolution = gimage->yresolution;
  new_gimage->unit = gimage->unit;

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
  pixel_region_init (&srcPR, drawable_data (GIMP_DRAWABLE(gimage->selection_mask)), 
		     0, 0, gimage->width, gimage->height, FALSE);
  pixel_region_init (&destPR, drawable_data (GIMP_DRAWABLE(new_gimage->selection_mask)), 
		     0, 0, gimage->width, gimage->height, TRUE);
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
	case ORIENTATION_HORIZONTAL:
	  new_guide = gimp_image_add_hguide(new_gimage);
	  new_guide->position = guide->position;
	  break;
	case ORIENTATION_VERTICAL:
	  new_guide = gimp_image_add_vguide(new_gimage);
	  new_guide->position = guide->position;
	  break;
	default:
	  g_error("Unknown guide orientation.\n");
	}
    }
  /* Copy the qmask info */
  new_gimage->qmask_state = gimage->qmask_state;
  for (count=0;count<3;count++)
    new_gimage->qmask_color[count] = gimage->qmask_color[count];
  new_gimage->qmask_opacity = gimage->qmask_opacity;

  /* Copy parasites */
  parasites = gimage->parasites;
  if (parasites)
    new_gimage->parasites = parasite_list_copy (parasites);

  /* Copy paths */
  paths = gimp_image_get_paths (gimage);
  if (paths)
    {
      GSList *plist = NULL;
      GSList *new_plist = NULL;
      Path *path;
      PathList *new_paths;
      
      for (plist = paths->bz_paths; plist; plist = plist->next)
	{
	  path = plist->data;
	  new_plist = g_slist_append (new_plist, path_copy (new_gimage, path));
	}
      
      new_paths = path_list_new (new_gimage, paths->last_selected_row, new_plist);
      gimp_image_set_paths (new_gimage, new_paths);
    }

  gimage_enable_undo (new_gimage);

  return new_gimage;
}


#ifdef I_LIKE_BOGUS_CRAP
static void
duplicate_projection (GimpImage *oldgimage, GimpImage *newgimage,
		      GDisplay *newgdisplay)
{
  PixelRegion srcPR, destPR;

  /*  Copy-on-write the projection tilemanager so we don't have
   *  to reproject the new gimage - since if we do the duplicate
   *  operation correctly, the projection for the new gimage is
   *  identical to that of the source.  (Is this true?  View-only
   *  attributes vs. image meta-attributes? -- View-only attributes
   *  *should* strictly be applied post-projection.)
   */

#if 0
  newgimage->construct_flag = oldgimage->construct_flag;

  g_warning("CONSTR:%d %dx%dx%d",newgimage->construct_flag,
	    tile_manager_level_width(gimp_image_projection (newgimage)),
	    tile_manager_level_height(gimp_image_projection (newgimage)),
	    tile_manager_level_bpp(gimp_image_projection (newgimage))
	    );

  gdisplay_expose_area(newgdisplay,
		       newgdisplay->disp_xoffset, newgdisplay->disp_yoffset,
		       newgdisplay->disp_width, newgdisplay->disp_height);
  
  if (newgimage->construct_flag)
#endif
    {
      tile_manager_set_user_data(gimp_image_projection (newgimage),
				 (void *) newgimage);

      /*
      newgimage->proj_type = oldgimage->proj_type;
      newgimage->proj_bytes = oldgimage->proj_bytes;
      newgimage->proj_level = oldgimage->proj_level;
      
      gimage_projection_realloc (new_gimage);*/

      
      pixel_region_init (&srcPR, gimp_image_projection (oldgimage), 0, 0,
			 oldgimage->width, oldgimage->height, FALSE);
      pixel_region_init (&destPR, gimp_image_projection (newgimage), 0, 0,
			 newgimage->width, newgimage->height, TRUE);
      copy_region(&srcPR, &destPR);
      
      /*
      pixel_region_init (&srcPR, gimp_image_projection (oldgimage),
			 newgdisplay->disp_xoffset, newgdisplay->disp_yoffset,
			 newgdisplay->disp_width, newgdisplay->disp_height,
			 FALSE);
      pixel_region_init (&destPR, gimp_image_projection (newgimage),
			 newgdisplay->disp_xoffset, newgdisplay->disp_yoffset,
			 newgdisplay->disp_width, newgdisplay->disp_height,
			 TRUE);
      copy_region(&srcPR, &destPR);
      */

      if (1)
      {
	GDisplay* gdisp;
	int x,y,w,h;
	int x1, y1, x2, y2;
	
	gdisp = newgdisplay;

	fprintf (stderr, " [pointers: %p, %p ] ", oldgimage, gdisp->gimage);

	gdisplay_untransform_coords (gdisp, 0, 0, &x1, &y1, FALSE, FALSE);
	gdisplay_untransform_coords (gdisp, gdisp->disp_width, gdisp->disp_height,
				     &x2, &y2, FALSE, FALSE);

	fprintf(stderr," <%dx%d %dx%d %d,%d->%d,%d> ",
		oldgimage->width, oldgimage->height,
		newgimage->width, newgimage->height,
		x1,y1, x2,y2);

	gimage_invalidate_without_render (gdisp->gimage, 0,0,
					  gdisp->gimage->width,
					  gdisp->gimage->height,
					  //	  64,64,128,128);
					  //					  newgdisplay->disp_width,newgdisplay->disp_height,
					  //					  newgdisplay->disp_width+newgdisplay->disp_xoffset,newgdisplay->disp_height+newgdisplay->disp_yoffset
					  x1, y1, x2, y2
					  );
	
      }
    }
}
#endif


void
channel_ops_duplicate (GimpImage *gimage)
{
  GDisplay *new_gdisp;
  GImage   *new_gimage;

  new_gimage = duplicate (gimage);

  /* We don't want to copy a half-redrawn projection, so force a flush. */
  /* gdisplays_finish_draw();
  gdisplays_flush_now(); */

  new_gdisp = gdisplay_new (new_gimage, 0x0101);

  /* duplicate_projection(gimage, new_gimage, new_gdisp);*/
}
