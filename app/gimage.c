#include "gimpimageP.h"
#include "gimage.h"
#include "gimpimage.h"
#include "channels_dialog.h"
#include "layers_dialog.h"

#include "indexed_palette.h"

#include "drawable.h"
#include "gdisplay.h"


#include "palette.h"
#include "undo.h"

#include "layer.h"
#include "layer_pvt.h"
#include "channel.h"
#include "tools.h"
#include "general.h"

/* gimage.c: Junk (ugly dependencies) from gimpimage.c on its way
   to proper places. That is, the handlers should be moved to
   layers_dialog, gdisplay, tools, etc.. */

/* Todo: obliterate the idiotic id system. We get rid of lotsa list
   lookups, and it clears the code.. */

static int global_gimage_ID = 1;

/* Any global vars like this are damn well app-specific, and don't
   belong in class implementations */

/* To do: a more generic "set" class */

GSList* image_list;

static void gimage_dirty_handler (GimpImage* gimage);
static void gimage_destroy_handler (GimpImage* gimage);
static void gimage_rename_handler (GimpImage* gimage);
static void gimage_resize_handler (GimpImage* gimage);
static void gimage_restructure_handler (GimpImage* gimage);
static void gimage_repaint_handler (GimpImage* gimage, gint, gint, gint, gint);



GImage*
gimage_new(int width, int height, GimpImageBaseType base_type)
{
  GimpImage* gimage = gimp_image_new (width, height, base_type);

  gtk_signal_connect (GTK_OBJECT (gimage), "dirty",
		      GTK_SIGNAL_FUNC(gimage_dirty_handler), NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "destroy",
		      GTK_SIGNAL_FUNC(gimage_destroy_handler), NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "rename",
		      GTK_SIGNAL_FUNC(gimage_rename_handler), NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "resize",
		      GTK_SIGNAL_FUNC(gimage_resize_handler), NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "restructure",
		      GTK_SIGNAL_FUNC(gimage_restructure_handler), NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "repaint",
		      GTK_SIGNAL_FUNC(gimage_repaint_handler), NULL);
  
  gimage->ref_count = 0;
  gimage->ID = global_gimage_ID ++;
  image_list = g_slist_append (image_list, gimage);
  
  lc_dialog_update_image_list ();
  indexed_palette_update_image_list ();
  return gimage;
}

GImage*
gimage_get_ID (gint ID)
{
  GSList *tmp = image_list;
  GimpImage *gimage;

  while (tmp)
    {
      gimage = (GimpImage *) tmp->data;
      if (gimage->ID == ID)
	return gimage;

      tmp = g_slist_next (tmp);
    }

  return NULL;
  
}


/* Ack! GImages have their own ref counts! This is going to cause
   trouble.. It should be pretty easy to convert to proper GtkObject
   ref counting, though. */

void
gimage_delete (GImage *gimage)
{
  gimage->ref_count--;
   if (gimage->ref_count <= 0)
     gtk_object_unref (GTK_OBJECT(gimage));
};

void
gimage_invalidate_previews (void)
{
  GSList *tmp = image_list;
  GimpImage *gimage;

  while (tmp)
    {
      gimage = (GimpImage *) tmp->data;
      gimp_image_invalidate_preview (gimage);
      tmp = g_slist_next (tmp);
    }
}
static void
gimage_dirty_handler (GimpImage* gimage){
  if (active_tool && !active_tool->preserve) {
    GDisplay* gdisp = active_tool->gdisp_ptr;
    if (gdisp) {
      if (gdisp->gimage->ID == gimage->ID)
        tools_initialize (active_tool->type, gdisp);
      else
        tools_initialize (active_tool->type, NULL);
    }
  }
}

static void
gimage_destroy_handler (GimpImage* gimage)
{
  /*  free the undo list  */
  undo_free (gimage);

  image_list = g_slist_remove (image_list, (void *) gimage);
  lc_dialog_update_image_list ();

  indexed_palette_update_image_list ();
}

static void
gimage_rename_handler (GimpImage* gimage)
{
  gdisplays_update_title (gimage);
  lc_dialog_update_image_list ();
  indexed_palette_update_image_list ();
}

static void
gimage_resize_handler (GimpImage* gimage)
{
  undo_push_group_end (gimage);

  /*  shrink wrap and update all views  */
  channel_invalidate_previews (gimage);
  layer_invalidate_previews (gimage);
  gimp_image_invalidate_preview (gimage);
  gdisplays_update_full (gimage);
  gdisplays_shrink_wrap (gimage);
}

static void
gimage_restructure_handler (GimpImage* gimage)
{
  gdisplays_update_title (gimage);
}

static void
gimage_repaint_handler (GimpImage* gimage, gint x, gint y, gint w, gint h)
{
  gdisplays_update_area (gimage, x, y, w, h);
}

  

/* These really belong in the layer class */

void
gimage_set_layer_mask_apply (GImage *gimage, GimpLayer* layer)
{
  int off_x, off_y;

  g_return_if_fail(gimage);
  g_return_if_fail(layer);
  
  if (! layer->mask)
    return;

  layer->apply_mask = ! layer->apply_mask;
  drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
  gdisplays_update_area (gimage, off_x, off_y,
			 drawable_width (GIMP_DRAWABLE(layer)), 
			 drawable_height (GIMP_DRAWABLE(layer)));
}



void
gimage_set_layer_mask_edit (GImage *gimage, Layer * layer, int edit)
{
  /*  find the layer  */
  if (!layer)
    return;

  if (layer->mask)
    layer->edit_mask = edit;
}


void
gimage_set_layer_mask_show (GImage *gimage, GimpLayer* layer)
{
  int off_x, off_y;

  g_return_if_fail(gimage);
  g_return_if_fail(layer);
  
  if (! layer->mask)
    return;

  layer->show_mask = ! layer->show_mask;
  drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
  gdisplays_update_area (gimage, off_x, off_y,
			 drawable_width (GIMP_DRAWABLE(layer)), drawable_height (GIMP_DRAWABLE(layer)));
}

void
gimage_foreach (GFunc func, gpointer user_data){
	GSList* l;
	for(l=image_list;l;l=l->next)
		func(l->data, user_data);
}

GImage *
gimage_get_named (gchar *name)
{
  GSList *tmp = image_list;
  GimpImage *gimage;
  char *str;
  while (tmp)
    {
      gimage = tmp->data;
      str = prune_filename (gimp_image_filename (gimage));
      if (strcmp (str, name) == 0)
	return gimage;

      tmp = g_slist_next (tmp);
    }
  return NULL;
}

