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

#include "gimage.h"
#include "undo_types.h"


/*  Undo interface functions  */

gboolean      undo_push_group_start       (GImage       *gimage, 
					   UndoType      type);
gboolean      undo_push_group_end         (GImage       *gimage);
gboolean      undo_push_image             (GImage       *gimage, 
					   GimpDrawable *drawable, 
					   gint          x1, 
					   gint          y1, 
					   gint          x2, 
					   gint          y2);
gboolean      undo_push_image_mod         (GImage       *gimage, 
					   GimpDrawable *drawable, 
					   gint          x1, 
					   gint          y1, 
					   gint          x2, 
					   gint          y2, 
					   gpointer      tiles_ptr, 
					   gboolean      sparse);
gboolean      undo_push_mask              (GImage       *gimage, 
					   gpointer      mask_ptr);
gboolean      undo_push_layer_displace    (GImage       *gimage,
					   GimpLayer    *layer);
gboolean      undo_push_transform         (GImage       *gimage, 
					   gpointer      tu_ptr);
gboolean      undo_push_paint             (GImage       *gimage, 
					   gpointer      pu_ptr);
gboolean      undo_push_layer             (GImage       *gimage, 
					   UndoType      type, 
					   gpointer      lu_ptr);
gboolean      undo_push_layer_mod         (GImage       *gimage, 
					   gpointer      layer_ptr);
gboolean      undo_push_layer_mask        (GImage       *gimage, 
					   UndoType      type, 
					   gpointer      lmu_prt);
gboolean      undo_push_layer_change      (GImage       *gimage, 
					   GimpLayer    *layer);
gboolean      undo_push_layer_reposition  (GImage       *gimage, 
					   GimpLayer    *layer);
gboolean      undo_push_channel           (GImage       *gimage, 
					   UndoType      type, 
					   gpointer      cu_ptr);
gboolean      undo_push_channel_mod       (GImage       *gimage, 
					   gpointer      cmu_ptr);
gboolean      undo_push_fs_to_layer       (GImage       *gimage,
					   gpointer      fsu_ptr);
gboolean      undo_push_fs_rigor          (GImage       *gimage, 
					   gint32        layer_ID);
gboolean      undo_push_fs_relax          (GImage       *gimage, 
					   gint32        layer_ID);
gboolean      undo_push_gimage_mod        (GImage       *gimage);
gboolean      undo_push_guide             (GImage       *gimage, 
					   gpointer      guide);
gboolean      undo_push_image_parasite    (GImage       *gimage,
					   gpointer      parasite);
gboolean      undo_push_drawable_parasite (GImage       *gimage, 
					   GimpDrawable *drawable, 
					   gpointer      parasite);
gboolean      undo_push_image_parasite_remove    
                                          (GImage       *gimage, 
					   const gchar  *name);
gboolean      undo_push_drawable_parasite_remove    
                                          (GImage       *gimage, 
					   GimpDrawable *drabable,
					   const gchar  *name);
gboolean      undo_push_qmask             (GImage       *gimage);
gboolean      undo_push_resolution        (GImage       *gimage);
gboolean      undo_push_layer_rename      (GImage       *gimage, 
					   GimpLayer    *layer);
gboolean      undo_push_cantundo          (GImage       *gimage, 
					   const gchar  *action);

gboolean      undo_pop                    (GImage       *gimage);
gboolean      undo_redo                   (GImage       *gimage);
void          undo_free                   (GImage       *gimage);

const gchar  *undo_get_undo_name          (GImage       *gimage);
const gchar  *undo_get_redo_name          (GImage       *gimage);


/* Stack peeking functions */
typedef gint (*undo_map_fn)     (const gchar *undoitemname, 
				 gpointer     data);

void undo_map_over_undo_stack   (GImage      *gimage, 
				 undo_map_fn  fn, 
				 gpointer     data);
void undo_map_over_redo_stack   (GImage      *gimage, 
				 undo_map_fn  fn, 
				 gpointer     data);
UndoType undo_get_undo_top_type (GImage      *gimage);


/* Not really appropriate here, since undo_history_new is not defined
 * in undo.c, but it saves on having a full header file for just one
 * function prototype. */
GtkWidget *undo_history_new     (GImage      *gimage);


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
