/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis and others
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

#include "apptypes.h"

#include "tool.h"
#include "tool_manager.h"

#include "airbrush.h"
#include "blend.h"
#include "bucket_fill.h"
#include "by_color_select.h"
#include "clone.h"
#include "gimpcolorpickertool.h"
#include "convolve.h"
#include "crop.h"
#include "dodgeburn.h"
#include "drawable.h"
#include "ellipse_select.h"
#include "eraser.h"
#include "flip_tool.h"
#include "free_select.h"
#include "fuzzy_select.h"
#include "gimpimage.h"
#include "ink.h"
#include "magnify.h"
#include "measure.h"
#include "move.h"
#include "gimppaintbrushtool.h"
#include "pencil.h"
#include "perspective_tool.h"
#include "rect_select.h"
#include "rotate_tool.h"
#include "scale_tool.h"
#include "shear_tool.h"
#include "smudge.h"
#include "text_tool.h"


void
register_tools (void)
{
  gimp_ink_tool_register ();
  gimp_paintbrush_tool_register ();
  gimp_bucket_fill_tool_register ();
  gimp_measure_tool_register ();
  gimp_magnify_tool_register ();
  gimp_crop_tool_register ();
  gimp_color_picker_tool_register ();
  gimp_text_tool_register ();
  gimp_move_tool_register ();

/*
  snatched from the pdb.  For inspiration only.  ;)

  procedural_db_register (&airbrush_proc);
  procedural_db_register (&airbrush_default_proc);
  procedural_db_register (&blend_proc);
  procedural_db_register (&bucket_fill_proc);
  procedural_db_register (&by_color_select_proc);
  procedural_db_register (&clone_proc);
  procedural_db_register (&clone_default_proc);
  procedural_db_register (&color_picker_proc);
  procedural_db_register (&convolve_proc);
  procedural_db_register (&convolve_default_proc);
  procedural_db_register (&crop_proc);
  procedural_db_register (&dodgeburn_proc);
  procedural_db_register (&dodgeburn_default_proc);
  procedural_db_register (&ellipse_select_proc);
  procedural_db_register (&eraser_proc);
  procedural_db_register (&eraser_default_proc);
  procedural_db_register (&flip_proc);
  procedural_db_register (&free_select_proc);
  procedural_db_register (&fuzzy_select_proc);
  procedural_db_register (&pencil_proc);
  procedural_db_register (&perspective_proc);
  procedural_db_register (&rect_select_proc);
  procedural_db_register (&rotate_proc);
  procedural_db_register (&scale_proc);
  procedural_db_register (&shear_proc);
  procedural_db_register (&smudge_proc);
  procedural_db_register (&smudge_default_proc);
  procedural_db_register (&transform_2d_proc); */
}
