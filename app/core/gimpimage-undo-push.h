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


gboolean      undo_push_group_start       (GimpImage    *gimage, 
					   UndoType      type);
gboolean      undo_push_group_end         (GimpImage    *gimage);
gboolean      undo_push_image             (GimpImage    *gimage, 
					   GimpDrawable *drawable, 
					   gint          x1, 
					   gint          y1, 
					   gint          x2, 
					   gint          y2);
gboolean      undo_push_image_mod         (GimpImage    *gimage, 
					   GimpDrawable *drawable, 
					   gint          x1, 
					   gint          y1, 
					   gint          x2, 
					   gint          y2, 
					   gpointer      tiles_ptr, 
					   gboolean      sparse);
gboolean      undo_push_mask              (GimpImage    *gimage, 
					   gpointer      mask_ptr);
gboolean      undo_push_layer_displace    (GimpImage    *gimage,
					   GimpLayer    *layer);
gboolean      undo_push_transform         (GimpImage    *gimage, 
					   gpointer      tu_ptr);
gboolean      undo_push_paint             (GimpImage    *gimage, 
					   gpointer      pu_ptr);
gboolean      undo_push_layer             (GimpImage    *gimage, 
					   UndoType      type, 
					   gpointer      lu_ptr);
gboolean      undo_push_layer_mod         (GimpImage    *gimage, 
					   gpointer      layer_ptr);
gboolean      undo_push_layer_mask        (GimpImage    *gimage, 
					   UndoType      type, 
					   gpointer      lmu_prt);
gboolean      undo_push_layer_change      (GimpImage    *gimage, 
					   GimpLayer    *layer);
gboolean      undo_push_layer_reposition  (GimpImage    *gimage, 
					   GimpLayer    *layer);
gboolean      undo_push_channel           (GimpImage    *gimage, 
					   UndoType      type, 
					   gpointer      cu_ptr);
gboolean      undo_push_channel_mod       (GimpImage    *gimage, 
					   gpointer      cmu_ptr);
gboolean      undo_push_fs_to_layer       (GimpImage    *gimage,
					   gpointer      fsu_ptr);
gboolean      undo_push_fs_rigor          (GimpImage    *gimage, 
					   gint32        layer_ID);
gboolean      undo_push_fs_relax          (GimpImage    *gimage, 
					   gint32        layer_ID);
gboolean      undo_push_gimage_mod        (GimpImage    *gimage);
gboolean      undo_push_guide             (GimpImage    *gimage, 
					   gpointer      guide);
gboolean      undo_push_image_parasite    (GimpImage    *gimage,
					   gpointer      parasite);
gboolean      undo_push_drawable_parasite (GimpImage    *gimage, 
					   GimpDrawable *drawable, 
					   gpointer      parasite);
gboolean      undo_push_image_parasite_remove    
                                          (GimpImage    *gimage, 
					   const gchar  *name);
gboolean      undo_push_drawable_parasite_remove    
                                          (GimpImage    *gimage, 
					   GimpDrawable *drabable,
					   const gchar  *name);
gboolean      undo_push_qmask             (GimpImage    *gimage);
gboolean      undo_push_resolution        (GimpImage    *gimage);
gboolean      undo_push_layer_rename      (GimpImage    *gimage, 
					   GimpLayer    *layer);
gboolean      undo_push_cantundo          (GimpImage    *gimage, 
					   const gchar  *action);

gboolean      undo_pop                    (GimpImage    *gimage);
gboolean      undo_redo                   (GimpImage    *gimage);
void          undo_free                   (GimpImage    *gimage);

const gchar  *undo_get_undo_name          (GimpImage    *gimage);
const gchar  *undo_get_redo_name          (GimpImage    *gimage);


/* Stack peeking functions */
typedef gint (*undo_map_fn)     (const gchar *undoitemname, 
				 gpointer     data);

void undo_map_over_undo_stack   (GimpImage   *gimage, 
				 undo_map_fn  fn, 
				 gpointer     data);
void undo_map_over_redo_stack   (GimpImage   *gimage, 
				 undo_map_fn  fn, 
				 gpointer     data);
UndoType undo_get_undo_top_type (GimpImage   *gimage);


/* Argument to undo_event signal emitted by gimages: */
typedef enum 
{
  UNDO_PUSHED,	/* a new undo has been added to the undo stack       */
  UNDO_EXPIRED,	/* an undo has been freed from the undo stack        */
  UNDO_POPPED,	/* an undo has been executed and moved to redo stack */
  UNDO_REDO,    /* a redo has been executed and moved to undo stack  */
  UNDO_FREE     /* all undo and redo info has been cleared           */
} undo_event_t;


#endif  /* __UNDO_H__ */
