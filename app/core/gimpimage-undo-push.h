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


#include "undo_types.h"

#include "gimage.h"


/*  Undo interface functions  */

int      undo_push_group_start       (GImage *, UndoType);
int      undo_push_group_end         (GImage *);
int      undo_push_image             (GImage *, GimpDrawable *, int, int, int, int);
int      undo_push_image_mod         (GImage *, GimpDrawable *, int, int, int, int, void *, int);
int      undo_push_mask              (GImage *, void *);
int      undo_push_layer_displace    (GImage *, GimpLayer*);
int      undo_push_transform         (GImage *, void *);
int      undo_push_paint             (GImage *, void *);
int      undo_push_layer             (GImage *, UndoType, void *);
int      undo_push_layer_mod         (GImage *, void *);
int      undo_push_layer_mask        (GImage *, UndoType, void *);
int      undo_push_layer_change      (GImage *, GimpLayer *);
int      undo_push_layer_reposition  (GImage *, GimpLayer *);
int      undo_push_channel           (GImage *, UndoType, void *);
int      undo_push_channel_mod       (GImage *, void *);
int      undo_push_fs_to_layer       (GImage *, void *);
int      undo_push_fs_rigor          (GImage *, int);
int      undo_push_fs_relax          (GImage *, int);
int      undo_push_gimage_mod        (GImage *);
int      undo_push_guide             (GImage *, void *);
int      undo_push_image_parasite    (GImage *, void *);
int      undo_push_drawable_parasite (GImage *, GimpDrawable *, void *);
int      undo_push_image_parasite_remove    (GImage *, const char *);
int      undo_push_drawable_parasite_remove (GImage *, GimpDrawable *,
					     const char *);
int      undo_push_qmask	     (GImage *);
int      undo_push_resolution	     (GImage *);
int      undo_push_layer_rename      (GImage *, GimpLayer *);
int      undo_push_cantundo          (GImage *, const char *);

int      undo_pop                    (GImage *);
int      undo_redo                   (GImage *);
void     undo_free                   (GImage *);

const char *undo_get_undo_name (GImage *);
const char *undo_get_redo_name (GImage *);

/* Stack peeking functions */
typedef int (*undo_map_fn) (const char *undoitemname, void *data);
void undo_map_over_undo_stack (GImage *, undo_map_fn, void *data);
void undo_map_over_redo_stack (GImage *, undo_map_fn, void *data);


/* Not really appropriate here, since undo_history_new is not defined
 * in undo.c, but it saves on having a full header file for just one
 * function prototype. */
GtkWidget *undo_history_new (GImage *gimage);


/* Argument to undo_event signal emitted by gimages: */
typedef enum {
    UNDO_PUSHED,	/* a new undo has been added to the undo stack */
    UNDO_EXPIRED,	/* an undo has been freed from the undo stack */
    UNDO_POPPED,	/* an undo has been executed and moved to redo stack */
    UNDO_REDO,		/* a redo has been executed and moved to undo stack */
    UNDO_FREE		/* all undo and redo info has been cleared */
} undo_event_t;

#endif  /* __UNDO_H__ */
