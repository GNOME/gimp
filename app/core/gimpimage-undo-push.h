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

#ifndef __UNDO_H__
#define __UNDO_H__


/*  Argument to undo_event signal emitted by images  */

typedef enum 
{
  UNDO_PUSHED,	/* a new undo has been added to the undo stack       */
  UNDO_EXPIRED,	/* an undo has been freed from the undo stack        */
  UNDO_POPPED,	/* an undo has been executed and moved to redo stack */
  UNDO_REDO,    /* a redo has been executed and moved to undo stack  */
  UNDO_FREE     /* all undo and redo info has been cleared           */
} undo_event_t;


/*  Stack peeking functions  */

typedef gint (*undo_map_fn) (const gchar *undoitemname, 
                             gpointer     data);


/*  main undo functions  */

gboolean      undo_pop                     (GimpImage     *gimage);
gboolean      undo_redo                    (GimpImage     *gimage);
void          undo_free                    (GimpImage     *gimage);

const gchar * undo_get_undo_name           (GimpImage     *gimage);
const gchar * undo_get_redo_name           (GimpImage     *gimage);


void          undo_map_over_undo_stack     (GimpImage     *gimage, 
                                            undo_map_fn    fn, 
                                            gpointer       data);
void          undo_map_over_redo_stack     (GimpImage     *gimage, 
                                            undo_map_fn    fn, 
                                            gpointer       data);

UndoType      undo_get_undo_top_type       (GimpImage     *gimage);


/*  undo groups  */

gboolean      undo_push_group_start        (GimpImage     *gimage,
                                            UndoType       type);
gboolean      undo_push_group_end          (GimpImage     *gimage);


/*  image undos  */

gboolean      undo_push_image                 (GimpImage     *gimage,
                                               GimpDrawable  *drawable,
                                               gint           x1,
                                               gint           y1,
                                               gint           x2,
                                               gint           y2);
gboolean      undo_push_image_mod             (GimpImage     *gimage,
                                               GimpDrawable  *drawable,
                                               gint           x1,
                                               gint           y1,
                                               gint           x2,
                                               gint           y2,
                                               TileManager   *tiles,
                                               gboolean       sparse);
gboolean      undo_push_image_type            (GimpImage     *gimage);
gboolean      undo_push_image_size            (GimpImage     *gimage);
gboolean      undo_push_image_resolution      (GimpImage     *gimage);
gboolean      undo_push_image_mask            (GimpImage     *gimage,
                                               TileManager   *tiles,
                                               gint           x,
                                               gint           y);
gboolean      undo_push_image_qmask           (GimpImage     *gimage);
gboolean      undo_push_image_guide           (GimpImage     *gimage,
                                               GimpGuide     *guide);


/*  item undos  */

gboolean      undo_push_item_rename           (GimpImage     *gimage, 
                                               GimpItem      *item);


/*  layer undos  */

gboolean      undo_push_layer_add             (GimpImage     *gimage,
                                               GimpLayer     *layer,
                                               gint           prev_position,
                                               GimpLayer     *prev_layer);
gboolean      undo_push_layer_remove          (GimpImage     *gimage,
                                               GimpLayer     *layer,
                                               gint           prev_position,
                                               GimpLayer     *prev_layer);
gboolean      undo_push_layer_mod             (GimpImage     *gimage,
                                               GimpLayer     *layer);
gboolean      undo_push_layer_mask_add        (GimpImage     *gimage, 
                                               GimpLayer     *layer,
                                               GimpLayerMask *mask);
gboolean      undo_push_layer_mask_remove     (GimpImage     *gimage, 
                                               GimpLayer     *layer,
                                               GimpLayerMask *mask);
gboolean      undo_push_layer_reposition      (GimpImage     *gimage, 
                                               GimpLayer     *layer);
gboolean      undo_push_layer_displace        (GimpImage     *gimage,
                                               GimpLayer     *layer);


/*  channel undos  */

gboolean      undo_push_channel_add           (GimpImage     *gimage, 
                                               GimpChannel   *channel,
                                               gint           prev_position,
                                               GimpChannel   *prev_channel);
gboolean      undo_push_channel_remove        (GimpImage     *gimage, 
                                               GimpChannel   *channel,
                                               gint           prev_position,
                                               GimpChannel   *prev_channel);
gboolean      undo_push_channel_mod           (GimpImage     *gimage, 
                                               GimpChannel   *channel);
gboolean      undo_push_channel_reposition    (GimpImage     *gimage, 
                                               GimpChannel   *channel);


/*  vectors undos  */

gboolean      undo_push_vectors_add           (GimpImage     *gimage, 
                                               GimpVectors   *vectors,
                                               gint           prev_position,
                                               GimpVectors   *prev_vectors);
gboolean      undo_push_vectors_remove        (GimpImage     *gimage, 
                                               GimpVectors   *channel,
                                               gint           prev_position,
                                               GimpVectors   *prev_vectors);
gboolean      undo_push_vectors_mod           (GimpImage     *gimage, 
                                               GimpVectors   *vectors);
gboolean      undo_push_vectors_reposition    (GimpImage     *gimage,
                                               GimpVectors   *vectors);


/*  floating selection undos  */

gboolean      undo_push_fs_to_layer           (GimpImage     *gimage,
                                               GimpLayer     *floating_layer,
                                               GimpDrawable  *drawable);
gboolean      undo_push_fs_rigor              (GimpImage     *gimage, 
                                               gint32         layer_ID);
gboolean      undo_push_fs_relax              (GimpImage     *gimage, 
                                               gint32         layer_ID);


/*  transform/paint drawable undos  */

gboolean      undo_push_transform             (GimpImage     *gimage,
                                               gint           tool_ID,
                                               GType          tool_type,
                                               gdouble       *trans_info,
                                               TileManager   *original,
                                               GSList        *path_undo);
gboolean      undo_push_paint                 (GimpImage     *gimage,
                                               gint           core_ID,
                                               GType          core_type,
                                               GimpCoords    *last_coords);


/*  parasite undos  */

gboolean      undo_push_image_parasite        (GimpImage     *gimage,
                                               gpointer       parasite);
gboolean      undo_push_image_parasite_remove (GimpImage     *gimage, 
                                               const gchar   *name);

gboolean      undo_push_item_parasite         (GimpImage     *gimage, 
                                               GimpItem      *item,
                                               gpointer       parasite);
gboolean      undo_push_item_parasite_remove  (GimpImage     *gimage, 
                                               GimpItem      *item,
                                               const gchar   *name);


/*  EEK undo  */

gboolean      undo_push_cantundo              (GimpImage     *gimage, 
                                               const gchar   *action);


#endif  /* __UNDO_H__ */
