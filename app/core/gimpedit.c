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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "core/core-types.h"
#include "tools/tools-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile-manager-crop.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpbuffer.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"
#include "core/gimplist.h"

#include "tools/gimptool.h"
#include "tools/tool_manager.h"

#include "context_manager.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage.h"
#include "global_edit.h"
#include "image_new.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


typedef enum
{
  PASTE,
  PASTE_INTO,
  PASTE_AS_NEW
} PasteAction;


typedef struct _PasteNamedDialog PasteNamedDialog;

struct _PasteNamedDialog
{
  GtkWidget   *shell;
  GtkWidget   *list;
  GimpImage   *gimage;
  PasteAction  action;
};


TileManager *
edit_cut (GimpImage    *gimage,
	  GimpDrawable *drawable)
{
  TileManager *cut;
  TileManager *cropped_cut;
  gboolean     empty;

  g_return_val_if_fail (gimage != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (drawable != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  /*  Start a group undo  */
  undo_push_group_start (gimage, EDIT_CUT_UNDO);

  /*  See if the gimage mask is empty  */
  empty = gimage_mask_is_empty (gimage);

  /*  Next, cut the mask portion from the gimage  */
  cut = gimage_mask_extract (gimage, drawable, TRUE, FALSE, TRUE);

  if (cut)
    image_new_reset_current_cut_buffer ();

  /*  Only crop if the gimage mask wasn't empty  */
  if (cut && ! empty)
    {
      cropped_cut = tile_manager_crop (cut, 0);

      if (cropped_cut != cut)
	{
	  tile_manager_destroy (cut);
	  cut = NULL;
	}
    }
  else if (cut)
    cropped_cut = cut;
  else
    cropped_cut = NULL;

  /*  end the group undo  */
  undo_push_group_end (gimage);

  if (cropped_cut)
    {
      /*  Free the old global edit buffer  */
      if (global_buffer)
	tile_manager_destroy (global_buffer);

      /*  Set the global edit buffer  */
      global_buffer = cropped_cut;
    }

  return cropped_cut;
}

TileManager *
edit_copy (GimpImage    *gimage,
	   GimpDrawable *drawable)
{
  TileManager *copy;
  TileManager *cropped_copy;
  gboolean     empty;

  g_return_val_if_fail (gimage != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (drawable != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  /*  See if the gimage mask is empty  */
  empty = gimage_mask_is_empty (gimage);

  /*  First, copy the masked portion of the gimage  */
  copy = gimage_mask_extract (gimage, drawable, FALSE, FALSE, TRUE);

  if (copy)
    image_new_reset_current_cut_buffer ();

  /*  Only crop if the gimage mask wasn't empty  */
  if (copy && ! empty)
    {
      cropped_copy = tile_manager_crop (copy, 0);

      if (cropped_copy != copy)
	{
	  tile_manager_destroy (copy);
	  copy = NULL;
	}
    }
  else if (copy)
    cropped_copy = copy;
  else
    cropped_copy = NULL;

  if (cropped_copy)
    {
      /*  Free the old global edit buffer  */
      if (global_buffer)
	tile_manager_destroy (global_buffer);

      /*  Set the global edit buffer  */
      global_buffer = cropped_copy;
    }

  return cropped_copy;
}

GimpLayer *
edit_paste (GimpImage    *gimage,
	    GimpDrawable *drawable,
	    TileManager  *paste,
	    gboolean      paste_into)
{
  GimpLayer *layer;
  gint       x1, y1, x2, y2;
  gint       cx, cy;

  g_return_val_if_fail (gimage != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (! drawable || GIMP_IS_DRAWABLE (drawable), NULL);

  /*  Make a new layer: if drawable == NULL,
   *  user is pasting into an empty image.
   */

  if (drawable != NULL)
    layer = gimp_layer_new_from_tiles (gimage,
				       gimp_drawable_type_with_alpha (drawable),
				       paste, 
				       _("Pasted Layer"),
				       OPAQUE_OPACITY, NORMAL_MODE);
  else
    layer = gimp_layer_new_from_tiles (gimage,
				       gimp_image_base_type_with_alpha (gimage),
				       paste, 
				       _("Pasted Layer"),
				       OPAQUE_OPACITY, NORMAL_MODE);

  if (! layer)
    return NULL;

  /*  Start a group undo  */
  undo_push_group_start (gimage, EDIT_PASTE_UNDO);

  /*  Set the offsets to the center of the image  */
  if (drawable != NULL)
    {
      gimp_drawable_offsets (drawable, &cx, &cy);
      gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
      cx += (x1 + x2) >> 1;
      cy += (y1 + y2) >> 1;
    }
  else
    {
      cx = gimage->width  >> 1;
      cy = gimage->height >> 1;
    }

  GIMP_DRAWABLE (layer)->offset_x = cx - (GIMP_DRAWABLE (layer)->width  >> 1);
  GIMP_DRAWABLE (layer)->offset_y = cy - (GIMP_DRAWABLE (layer)->height >> 1);

  /*  If there is a selection mask clear it--
   *  this might not always be desired, but in general,
   *  it seems like the correct behavior.
   */
  if (! gimage_mask_is_empty (gimage) && ! paste_into)
    gimp_channel_clear (gimp_image_get_mask (gimage));

  /*  if there's a drawable, add a new floating selection  */
  if (drawable != NULL)
    {
      floating_sel_attach (layer, drawable);
    }
  else
    {
      gimp_drawable_set_gimage (GIMP_DRAWABLE (layer), gimage);
      gimp_image_add_layer (gimage, layer, 0);
    }

  /*  end the group undo  */
  undo_push_group_end (gimage);

  return layer;
}

GimpImage *
edit_paste_as_new (GimpImage   *invoke,
		   TileManager *paste)
{
  GimpImage *gimage;
  GimpLayer *layer;
  GDisplay  *gdisp;

  if (! global_buffer)
    return FALSE;

  /*  create a new image  (always of type RGB)  */
  gimage = gimage_new (tile_manager_width (paste), 
		       tile_manager_height (paste), 
		       RGB);
  gimp_image_undo_disable (gimage);
  gimp_image_set_resolution (gimage, invoke->xresolution, invoke->yresolution);
  gimp_image_set_unit (gimage, invoke->unit);

  layer = gimp_layer_new_from_tiles (gimage,
				     gimp_image_base_type_with_alpha (gimage),
				     paste, 
				     _("Pasted Layer"),
				     OPAQUE_OPACITY, NORMAL_MODE);

  if (layer)
    {
      /*  add the new layer to the image  */
      gimp_drawable_set_gimage (GIMP_DRAWABLE (layer), gimage);
      gimp_image_add_layer (gimage, layer, 0);

      gimp_image_undo_enable (gimage);

      gdisp = gdisplay_new (gimage, 0x0101);
      gimp_context_set_display (gimp_context_get_user (), gdisp);

      return gimage;
    }

  return NULL;
}

gboolean
edit_clear (GimpImage    *gimage,
	    GimpDrawable *drawable)
{
  TileManager *buf_tiles;
  PixelRegion  bufPR;
  gint         x1, y1, x2, y2;
  guchar       col[MAX_CHANNELS];

  g_return_val_if_fail (gimage != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (drawable != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  gimp_image_get_background (gimage, drawable, col);
  if (gimp_drawable_has_alpha (drawable))
    col [gimp_drawable_bytes (drawable) - 1] = OPAQUE_OPACITY;

  gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  if (!(x2 - x1) || !(y2 - y1))
    return TRUE;  /*  nothing to do, but the clear succeded  */

  buf_tiles = tile_manager_new ((x2 - x1), (y2 - y1),
				gimp_drawable_bytes (drawable));
  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);
  color_region (&bufPR, col);

  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimp_image_apply_image (gimage, drawable, &bufPR, TRUE, OPAQUE_OPACITY,
			  ERASE_MODE, NULL, x1, y1);

  /*  update the image  */
  drawable_update (drawable,
		   x1, y1,
		   (x2 - x1), (y2 - y1));

  /*  free the temporary tiles  */
  tile_manager_destroy (buf_tiles);

  return TRUE;
}

gboolean
edit_fill (GimpImage    *gimage,
	   GimpDrawable *drawable,
	   GimpFillType  fill_type)
{
  TileManager *buf_tiles;
  PixelRegion  bufPR;
  gint         x1, y1, x2, y2;
  guchar       col[MAX_CHANNELS];

  g_return_val_if_fail (gimage != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (drawable != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  if (gimp_drawable_has_alpha (drawable))
    col [gimp_drawable_bytes (drawable) - 1] = OPAQUE_OPACITY;

  switch (fill_type)
    {
    case FOREGROUND_FILL:
      gimp_image_get_foreground (gimage, drawable, col);
      break;

    case BACKGROUND_FILL:
      gimp_image_get_background (gimage, drawable, col);
      break;

    case WHITE_FILL:
      col[RED_PIX]   = 255;
      col[GREEN_PIX] = 255;
      col[BLUE_PIX]  = 255;
      break;

    case TRANSPARENT_FILL:
      col[RED_PIX]   = 0;
      col[GREEN_PIX] = 0;
      col[BLUE_PIX]  = 0;
      if (gimp_drawable_has_alpha (drawable))
	col [gimp_drawable_bytes (drawable) - 1] = TRANSPARENT_OPACITY;
      break;

    case NO_FILL:
      return TRUE;  /*  nothing to do, but the fill succeded  */

    default:
      g_warning ("%s(): unknown fill type", G_GNUC_FUNCTION);
      gimp_image_get_background (gimage, drawable, col);
      break;
    }

  gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  if (!(x2 - x1) || !(y2 - y1))
    return TRUE;  /*  nothing to do, but the fill succeded  */

  buf_tiles = tile_manager_new ((x2 - x1), (y2 - y1),
				gimp_drawable_bytes (drawable));
  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);
  color_region (&bufPR, col);

  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimp_image_apply_image (gimage, drawable, &bufPR, TRUE, OPAQUE_OPACITY,
			  NORMAL_MODE, NULL, x1, y1);

  /*  update the image  */
  drawable_update (drawable,
		   x1, y1,
		   (x2 - x1), (y2 - y1));

  /*  free the temporary tiles  */
  tile_manager_destroy (buf_tiles);

  return TRUE;
}


/*********************************************/
/*        Named buffer operations            */

static void
set_list_of_named_buffers (GtkWidget *list_widget)
{
  GList      *list;
  GimpBuffer *buffer;
  GtkWidget  *list_item;

  gtk_list_clear_items (GTK_LIST (list_widget), 0, -1);

  for (list = GIMP_LIST (named_buffers)->list;
       list;
       list = g_list_next (list))
    {
      buffer = (GimpBuffer *) list->data;

      list_item = gtk_list_item_new_with_label (GIMP_OBJECT (buffer)->name);
      gtk_container_add (GTK_CONTAINER (list_widget), list_item);
      gtk_object_set_user_data (GTK_OBJECT (list_item), buffer);
      gtk_widget_show (list_item);
    }
}

static void
named_buffer_paste_foreach (GtkWidget *widget,
			    gpointer   data)
{
  PasteNamedDialog *pn_dialog;
  GimpBuffer       *buffer;

  if (widget->state == GTK_STATE_SELECTED)
    {
      pn_dialog = (PasteNamedDialog *) data;
      buffer    = (GimpBuffer *) gtk_object_get_user_data (GTK_OBJECT (widget));

      switch (pn_dialog->action)
	{
	case PASTE:
	  edit_paste (pn_dialog->gimage,
		      gimp_image_active_drawable (pn_dialog->gimage),
		      buffer->tiles, FALSE);
	  break;

	case PASTE_INTO:
	  edit_paste (pn_dialog->gimage,
		      gimp_image_active_drawable (pn_dialog->gimage),
		      buffer->tiles, TRUE);
	  break;

	case PASTE_AS_NEW:
	  edit_paste_as_new (pn_dialog->gimage, buffer->tiles);
	  break;

	default:
	  break;
	}
    }

  gdisplays_flush ();
}

static void
named_buffer_paste_callback (GtkWidget *widget,
			     gpointer   data)
{
  PasteNamedDialog *pn_dialog;

  pn_dialog = (PasteNamedDialog *) data;

  pn_dialog->action = PASTE_INTO;
  gtk_container_foreach (GTK_CONTAINER (pn_dialog->list),
			 named_buffer_paste_foreach, data);

  gtk_widget_destroy (pn_dialog->shell);
}

static void
named_buffer_paste_into_callback (GtkWidget *widget,
				  gpointer   data)
{
  PasteNamedDialog *pn_dialog;

  pn_dialog = (PasteNamedDialog *) data;

  pn_dialog->action = PASTE_INTO;
  gtk_container_foreach (GTK_CONTAINER (pn_dialog->list),
			 named_buffer_paste_foreach, data);

  gtk_widget_destroy (pn_dialog->shell);
}

static void
named_buffer_paste_as_new_callback (GtkWidget *widget,
				    gpointer   data)
{
  PasteNamedDialog *pn_dialog;

  pn_dialog = (PasteNamedDialog *) data;

  pn_dialog->action = PASTE_AS_NEW;
  gtk_container_foreach (GTK_CONTAINER (pn_dialog->list),
			 named_buffer_paste_foreach, data);

  gtk_widget_destroy (pn_dialog->shell);
}

static void
named_buffer_delete_foreach (GtkWidget *widget,
			     gpointer   data)
{
  PasteNamedDialog *pn_dialog;
  GimpBuffer       *buffer;

  if (widget->state == GTK_STATE_SELECTED)
    {
      pn_dialog = (PasteNamedDialog *) data;
      buffer    = (GimpBuffer *) gtk_object_get_user_data (GTK_OBJECT (widget));

      gimp_container_remove (named_buffers, GIMP_OBJECT (buffer));
    }
}

static void
named_buffer_delete_callback (GtkWidget *widget,
			      gpointer   data)
{
  PasteNamedDialog *pn_dialog;

  pn_dialog = (PasteNamedDialog *) data;

  gtk_container_foreach (GTK_CONTAINER (pn_dialog->list),
			 named_buffer_delete_foreach, data);
  set_list_of_named_buffers (pn_dialog->list);
}

static void
paste_named_buffer (GimpImage *gimage)
{
  PasteNamedDialog *pn_dialog;
  GtkWidget        *vbox;
  GtkWidget        *label;
  GtkWidget        *listbox;
  GtkWidget        *bbox;
  GtkWidget        *button;
  gint              i;

  static gchar *paste_action_labels[] =
  {
    N_("Paste"),
    N_("Paste Into"),
    N_("Paste as New"),
  };

  static GtkSignalFunc paste_action_functions[] =
  {
    named_buffer_paste_callback,
    named_buffer_paste_into_callback,
    named_buffer_paste_as_new_callback,
  };

  pn_dialog = g_new0 (PasteNamedDialog, 1);

  pn_dialog->gimage = gimage;

  pn_dialog->shell = 
    gimp_dialog_new (_("Paste Named Buffer"), "paste_named_buffer",
		     gimp_standard_help_func,
		     "dialogs/paste_named.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("Delete"), named_buffer_delete_callback,
		     pn_dialog, NULL, NULL, FALSE, FALSE,
		     _("Cancel"), gtk_widget_destroy,
		     NULL, 1, NULL, TRUE, TRUE,

		     NULL);

  gtk_signal_connect_object (GTK_OBJECT (pn_dialog->shell), "destroy",
			     GTK_SIGNAL_FUNC (g_free),
			     (GtkObject *) pn_dialog);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 1);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (pn_dialog->shell)->vbox), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Select a buffer to paste:"));
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  listbox = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (listbox),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), listbox, TRUE, TRUE, 0);
  gtk_widget_set_usize (listbox, 125, 150);
  gtk_widget_show (listbox);

  pn_dialog->list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (pn_dialog->list), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (listbox),
					 pn_dialog->list);
  set_list_of_named_buffers (pn_dialog->list);
  gtk_widget_show (pn_dialog->list);

  bbox = gtk_hbutton_box_new ();
  gtk_container_set_border_width (GTK_CONTAINER (bbox), 6);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 2);
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);
  for (i = 0; i < 3; i++)
    {
      button = gtk_button_new_with_label (gettext (paste_action_labels[i]));
      gtk_container_add (GTK_CONTAINER (bbox), button);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) paste_action_functions[i],
			  pn_dialog);
      gtk_widget_show (button);
    }
  gtk_widget_show (bbox);

  gtk_widget_show (pn_dialog->shell);
}

gboolean
named_edit_paste (GimpImage *gimage)
{
  paste_named_buffer (gimage);

  return TRUE;
}
