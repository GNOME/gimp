/* gap_range_ops.h
 * 1998.07.03 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * GAP operations on frame Ranges (from - to)
 *
 */
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

/* revision history:
 * 1.1.10a; 1999/10/22   hof: extended dither options in gap_range_conv
 * 0.97.00; 1998/10/19   hof: extended gap_range_to_multilayer layer seletion
 * 0.96.03; 1998/08/31   hof: gap_range_to_multilayer: all params available
 *                            in non-interactive runmode
 * 0.96.00; 1998/07/02   hof: (extracted from gap_lib.h)
 * 0.94.01; 1998/04/27   hof: added flatten_mode to plugin: gap_range_to_multilayer
 */

#ifndef _GAP_RANGE_OPS_H
#define _GAP_RANGE_OPS_H

#include "libgimp/gimp.h"

/* flatten mode bits used in gap_range_to_multilayer */
#define FLAM_MERG_EXPAND    0
#define FLAM_MERG_CLIP_IMG  1
#define FLAM_MERG_CLIP_BG   2
#define FLAM_MERG_FLAT      3

/* Animation sizechange modes */
typedef enum
{
   ASIZ_SCALE  
 , ASIZ_RESIZE 
 , ASIZ_CROP   
} t_gap_asiz;




gint32 gap_range_to_multilayer(GimpRunModeType run_mode,
                             gint32 image_id,
                             long range_from, long range_to,
                             long flatten_mode, long bg_visible,
                             long   framerate, char  *frame_basename, int frame_basename_len,
			     gint32 sel_mode, gint32 sel_case,
			     gint32 sel_invert, char *sel_pattern
			     );

int gap_range_flatten(GimpRunModeType run_mode,
                             gint32 image_id,
                             long range_from, long range_to);
int gap_range_layer_del(GimpRunModeType run_mode,
                             gint32 image_id,
                             long range_from, long range_to, long position);

int gap_range_conv(GimpRunModeType run_mode,
                             gint32 image_id,
                             long   range_from, long range_to, 
                             long   flatten,
                             GimpImageBaseType dest_type, 
                             gint32     dest_colors,
                             gint32     dest_dither,
                             char   *basename,
                             char   *extension,
			     gint32  palette_type,
			     gint32  alpha_dither,
			     gint32  remove_unused,
                             char   *palette
			     );

int gap_anim_sizechange(GimpRunModeType run_mode,
                             t_gap_asiz asiz_mode,
                             gint32 image_id,
                             long size_x,
                             long size_y,
                             long offs_x,
                             long offs_y);

#endif


