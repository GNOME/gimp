/*  gap_pdb_calls.h
 *
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
 * version 1.1.16a; 2000/02/05  hof: path lockedstaus
 * version 1.1.15b; 2000/01/30  hof: image parasites
 * version 1.1.15a; 2000/01/26  hof: pathes, removed gimp 1.0.x support
 * version 1.1.14a; 2000/01/06  hof: thumbnail save/load,
 *                              Procedures for video_info file
 * version 0.98.00; 1998/11/30  hof: all PDB-calls of GIMP PDB-Procedures
 */

#ifndef _GAP_PDB_CALLS_H
#define _GAP_PDB_CALLS_H

#include "libgimp/gimp.h"

typedef struct t_video_info {
   gdouble     framerate;    /* playback rate in frames per second */
   gint32      timezoom;
} t_video_info;


gint p_pdb_procedure_available(char *proc_name);
gint p_get_gimp_selection_bounds (gint32 image_id, gint32 *x1, gint32 *y1, gint32 *x2, gint32 *y2);
gint p_gimp_selection_load (gint32 channel_id);
int  p_layer_set_linked (gint32 layer_id, gint32 new_state);
gint p_layer_get_linked(gint32 layer_id);

gint32 p_gimp_image_floating_sel_attached_to(gint32 image_id);
gint   p_gimp_floating_sel_attach(gint32 layer_id, gint32 drawable_id);
gint   p_gimp_floating_sel_rigor(gint32 layer_id, gint32 undo);
gint   p_gimp_floating_sel_relax(gint32 layer_id, gint32 undo);

gint32 p_gimp_image_add_guide(gint32 image_id, gint32 position, gint32 orientation);
gint32 p_gimp_image_findnext_guide(gint32 image_id, gint32 guide_id);
gint32 p_gimp_image_get_guide_position(gint32 image_id, gint32 guide_id);
gint32 p_gimp_image_get_guide_orientation(gint32 image_id, gint32 guide_id);
gint32 p_gimp_image_delete_guide(gint32 image_id, gint32 guide_id);
gint   p_gimp_selection_none(gint32 image_id);

gint   p_gimp_rotate(gint32 drawable_id, gint32 interpolation, gdouble angle_deg);
gint32 p_gimp_channel_ops_duplicate  (gint32     image_ID);


gint   p_gimp_drawable_set_image(gint32 drawable_id, gint32 image_id);

char*  p_gimp_gimprc_query(char *key);

gint   p_gimp_file_save_thumbnail(gint32 image_id, char* filename);
gint   p_gimp_file_load_thumbnail(char* filename, gint32 *th_width, gint32 *th_height, gint32 *th_data_count, unsigned char **th_data);

gint   p_gimp_image_thumbnail(gint32 image_id, gint32 width, gint32 height,
                              gint32 *th_width, gint32 *th_height, gint32 *th_bpp,
			      gint32 *th_data_count, unsigned char **th_data);


gint     p_gimp_path_set_points(gint32 image_id, char *name,
                              gint32 path_type, gint32 num_points, 
                              gdouble *path_points);                              
gdouble *p_gimp_path_get_points(gint32 image_id, char *name,
                              gint32 *path_type, gint32 *path_closed,
                              gint32 *num_points);
gint     p_gimp_path_delete(gint32 image_id, char *name);
                              
char   **p_gimp_path_list(gint32 image_id, gint32 *num_paths);
gint     p_gimp_path_set_current(gint32 image_id, char *name);
char    *p_gimp_path_get_current(gint32 image_id);
gint32   p_gimp_path_get_locked(gint32 image_id, gchar *name);
gint     p_gimp_path_set_locked(gint32 image_id, gchar *name, gint32 lockstatus);
gchar** p_gimp_image_parasite_list (gint32 image_id, gint32 *num_parasites);


char *p_alloc_video_info_name(char *basename);
int   p_set_video_info(t_video_info *vin_ptr, char *basename);
t_video_info *p_get_video_info(char *basename);
#endif
