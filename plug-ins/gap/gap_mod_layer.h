/* gap_mod_layer.h
 * 1998.10.14 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 * modify Layer (perform actions (like raise, set visible, apply filter)
 *               - foreach selected layer
 *               - in each frame of the selected framerange)
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
 * version 0.98.00   1998.11.27  hof: - use new module gap_pdb_calls.h
 * version 0.97.00              hof: - created module (as extract gap_fileter_foreach)
 */

#ifndef _GAP_MOD_LAYER_H
#define _GAP_MOD_LAYER_H

#define MAX_LAYERNAME 128

/* action_mode values */
#define	 ACM_SET_VISIBLE    0
#define	 ACM_SET_INVISIBLE  1
#define	 ACM_SET_LINKED	    2
#define	 ACM_SET_UNLINKED   3
#define	 ACM_RAISE          4
#define	 ACM_LOWER          5
#define	 ACM_MERGE_EXPAND   6
#define	 ACM_MERGE_IMG      7
#define	 ACM_MERGE_BG       8
#define	 ACM_APPLY_FILTER   9
#define	 ACM_DUPLICATE     10
#define	 ACM_DELETE        11
#define	 ACM_RENAME        12

typedef struct
{
  gint32 layer_id;
  gint   visible;
  gint   selected;
}  t_LayliElem;

t_LayliElem *p_alloc_layli(gint32 image_id, gint32 *l_sel_cnt, gint *nlayers,
        		   gint32 sel_mode,
        		   gint32 sel_case,
			   gint32 sel_invert,
        		   char *sel_pattern );
int  p_get_1st_selected (t_LayliElem * layli_ptr, gint nlayers);
void p_prevent_empty_image(gint32 image_id);

gint gap_mod_layer(GimpRunModeType run_mode, gint32 image_id,
                   gint32 range_from,  gint32 range_to,
                   gint32 action_mode, gint32 sel_mode,
                   gint32 sel_case, gint32 sel_invert,
                   char *sel_pattern, char *new_layername);

#endif
