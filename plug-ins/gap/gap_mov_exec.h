/* gap_lib.h
 * 1997.11.01 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * basic anim functions
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
 * gimp    1.1.29b; 2000/11/25  hof: NONINTEACTIV PDB interface for Movepath
 * gimp    1.1.20a; 2000/04/25  hof: support for keyframes, anim_preview
 * 0.96.00; 1998/06/27   hof: added gap animation sizechange plugins
 *                            (moved range_ops to seperate .h file)
 * 0.94.01; 1998/04/27   hof: added flatten_mode to plugin: gap_range_to_multilayer
 * 0.90.00;              hof: 1.st (pre) release
 */

#ifndef _GAP_MOV_EXEC_H
#define _GAP_MOV_EXEC_H

#include "libgimp/gimp.h"
#include "gap_mov_dialog.h"

int     gap_move_path(GimpRunModeType run_mode, gint32 image_id, t_mov_values *pvals, gchar *pointfile, gint rotation_follow, gint32 startangle);
gint32  p_mov_anim_preview(t_mov_values *pvals_orig, t_anim_info *ainfo_ptr, gint preview_frame_nr);

gchar  *p_gap_chk_keyframes(t_mov_values *pvals);
gint    p_conv_keyframe_to_rel(gint abs_keyframe, t_mov_values *pvals);
gint    p_conv_keyframe_to_abs(gint rel_keyframe, t_mov_values *pvals);
gint    p_gap_save_pointfile(char *filename, t_mov_values *pvals);
gint    p_gap_load_pointfile(char *filename, t_mov_values *pvals);
void    p_calculate_rotate_follow(t_mov_values *pvals, gint32 startangle);


#endif


