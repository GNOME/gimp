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

#include <gtk/gtk.h>

#include "core-types.h"

#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpundostack.h"


void
gimp_image_undo (GimpImage *gimage)
{
  GimpUndo *undo;

  undo = GIMP_UNDO (gimp_undo_stack_pop (gimage->new_undo_stack));
  
  if (undo)
    gimp_undo_stack_push (gimage->new_redo_stack, undo);
}


void
gimp_image_redo (GimpImage *gimage)
{
  GimpUndo *redo;

  redo = GIMP_UNDO (gimp_undo_stack_pop (gimage->new_redo_stack));
  
  if (redo)
    gimp_undo_stack_push (gimage->new_undo_stack, redo);
}
