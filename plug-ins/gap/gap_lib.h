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
 * 1.1.20a; 2000/04/25   hof: new: p_get_video_paste_name p_clear_video_paste
 * 1.1.14a; 2000/01/02   hof: new: p_get_frame_nr
 * 1.1.8a;  1999/08/31   hof: new: p_strdup_del_underscore and p_strdup_add_underscore
 * 0.99.00; 1999/03/15   hof: prepared for win/dos filename conventions
 * 0.96.02; 1998/08/05   hof: extended gap_dup (duplicate range instead of singele frame)
 *                            added gap_shift (framesequence shift)
 * 0.96.00; 1998/06/27   hof: added gap animation sizechange plugins
 *                            (moved range_ops to seperate .h file)
 * 0.94.01; 1998/04/27   hof: added flatten_mode to plugin: gap_range_to_multilayer
 * 0.90.00;              hof: 1.st (pre) release
 */

#ifndef _GAP_LIB_H
#define _GAP_LIB_H

#include "libgimp/gimp.h"

/* G_DIR_SEPARATOR (is defined in glib.h if you have glib-1.2.0 or later) */
#ifdef G_OS_WIN32

/* Filenames in WIN/DOS Style */
#ifndef G_DIR_SEPARATOR
#define G_DIR_SEPARATOR '\\'
#endif
#define DIR_ROOT ':'

#else  /* !G_OS_WIN32 */

/* Filenames in UNIX Style */
#ifndef G_DIR_SEPARATOR
#define G_DIR_SEPARATOR '/'
#endif
#define DIR_ROOT '/'

#endif /* !G_OS_WIN32 */


typedef struct t_anim_info {
   gint32      image_id;
   char        *basename;    /* may include path */
   long         frame_nr; 
   char        *extension;
   char        *new_filename;
   char        *old_filename;
   GimpRunModeType run_mode;
   long         width;       
   long         height;      
   long         type;    
   long         frame_cnt;   
   long         curr_frame_nr; 
   long         first_frame_nr; 
   long         last_frame_nr;
} t_anim_info;

/* procedures used in other gap*.c files */
int          p_file_exists(char *fname);
int          p_file_copy(char *fname, char *fname_copy);
void         p_free_ainfo(t_anim_info **ainfo);
char*        p_alloc_basename(char *imagename, long *number);
char*        p_alloc_extension(char *imagename);
t_anim_info* p_alloc_ainfo(gint32 image_id, GimpRunModeType run_mode);
int          p_dir_ainfo(t_anim_info *ainfo_ptr);
int          p_chk_framerange(t_anim_info *ainfo_ptr);
int          p_chk_framechange(t_anim_info *ainfo_ptr);

int    p_save_named_frame (gint32 image_id, char *sav_name);
int    p_load_named_frame (gint32 image_id, char *lod_name);
gint32 p_load_image (char *lod_name);
gint32 p_save_named_image(gint32 image_id, char *sav_name, GimpRunModeType run_mode);
char*  p_alloc_fname(char *basename, long nr, char *extension);
char*  p_gzip (char *orig_name, char *new_name, char *zip);
char*  p_strdup_add_underscore(char *name);
char*  p_strdup_del_underscore(char *name);

long  p_get_frame_nr(gint32 image_id);
long  p_get_frame_nr_from_name(char *fname);
char *p_alloc_fname_thumbnail(char *name);
int   p_image_file_copy(char *fname, char *fname_copy);

/* animation menu fuctions provided by gap_lib.c */

int gap_next(GimpRunModeType run_mode, gint32 image_id);
int gap_prev(GimpRunModeType run_mode, gint32 image_id);
int gap_first(GimpRunModeType run_mode, gint32 image_id);
int gap_last(GimpRunModeType run_mode, gint32 image_id);
int gap_goto(GimpRunModeType run_mode, gint32 image_id, int nr);

int gap_dup(GimpRunModeType run_mode, gint32 image_id, int nr, long range_from, long range_to);
int gap_del(GimpRunModeType run_mode, gint32 image_id, int nr);
int gap_exchg(GimpRunModeType run_mode, gint32 image_id, int nr);
int gap_shift(GimpRunModeType run_mode, gint32 image_id, int nr, long range_from, long range_to);

void p_msg_win(GimpRunModeType run_mode, char *msg);
gchar *p_get_video_paste_name(void);
gint32 p_vid_edit_clear(void);
gint32 p_vid_edit_framecount(void);
gint   gap_vid_edit_copy(GimpRunModeType run_mode, gint32 image_id, long range_from, long range_to);
gint   gap_vid_edit_paste(GimpRunModeType run_mode, gint32 image_id, long paste_mode);

#define  VID_PASTE_REPLACE         0
#define  VID_PASTE_INSERT_BEFORE   1
#define  VID_PASTE_INSERT_AFTER    2

#endif


