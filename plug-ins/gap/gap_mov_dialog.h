/* gap_mov_dialog.h
 * 1997.11.06 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * Dialog Window for gap_mov
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
#ifndef _GAP_MOV_DIALOG_H
#define _GAP_MOV_DIALOG_H

/* revision history:
 * gimp    1.1.20a; 2000/04/25  hof: support for keyframes, anim_preview
 * version 0.96.02; 1998.07.25  hof: added clip_to_img
 */
 
typedef enum
{
  GAP_STEP_LOOP      = 0,
  GAP_STEP_LOOP_REV  = 1,
  GAP_STEP_ONCE      = 2,
  GAP_STEP_ONCE_REV  = 3,
  GAP_STEP_PING_PONG = 4,
  GAP_STEP_NONE      = 5
} t_mov_stepmodes;

typedef enum
{
  GAP_HANDLE_LEFT_TOP  = 0,
  GAP_HANDLE_LEFT_BOT  = 1,
  GAP_HANDLE_RIGHT_TOP = 2,
  GAP_HANDLE_RIGHT_BOT = 3,
  GAP_HANDLE_CENTER    = 4
} t_mov_handle;

typedef enum
{
  GAP_APV_EXACT      = 0,
  GAP_APV_ONE_FRAME  = 1,
  GAP_APV_QUICK      = 2
} t_apv_mode;

typedef struct {
	long    dst_frame_nr;     /* current destination frame_nr */
	long    src_layer_idx;    /* index of current layer */
	gint32 *src_layers;       /* array of source images layer id's */
	long    src_last_layer;   /* index of last layer 0 upto n-1 */
	gdouble currX,  currY;
	gdouble deltaX, deltaY;
        gint    l_handleX;
        gint    l_handleY;
        
	gdouble currOpacity;
	gdouble deltaOpacity;
	gdouble currWidth;
	gdouble deltaWidth;
	gdouble currHeight;
	gdouble deltaHeight;
	gdouble currRotation;
	gdouble deltaRotation;
} t_mov_current;

typedef struct {
	gint    p_x, p_y;   /* +- koordinates */
	gint    opacity;    /* 0  upto 100% */
	gint    w_resize;   /* width resize 10 upto 300% */
	gint    h_resize;   /* height resize 10 upto 300% */
	gint    rotation;   /* rotatation +- degrees */
	
	gint    keyframe_abs;
	gint    keyframe;
} t_mov_point;

#define GAP_MOV_MAX_POINT 256

/*
 * Notes:
 * - exchange of frames (load replace)
 *   keeps an images id, but all its
 *   layers were allocated new.
 *   this results in new layer_id's
 *   For this reason the source image MUST NOT be one of the Frames
 *   of the destination Animation !!
 * - Rotation is now supported (since gap 0.95) (data structs are prepared for)
 */

typedef struct {
	gint32  src_image_id;   /* source image */
	gint32  src_layer_id;   /* id of layer (to begin with) */
	int     src_handle;
        int     src_stepmode;
        int     src_paintmode;
        gint    src_force_visible;      /* TRUE FALSE */
        gint    clip_to_img;            /* TRUE FALSE */
        
	gint    point_idx;           /* 0 upto MAX_POINT -1 */
	gint    point_idx_max;       /* 0 upto MAX_POINT -1 */
	t_mov_point point[GAP_MOV_MAX_POINT];

	gint    dst_range_start;  /* use current frame as default */
	gint    dst_range_end;
	gint    dst_layerstack;
	gint    dst_combination_mode;   /* GimpLayerModeEffects */

        /* for dialog only */	
	gint32  dst_image_id;      /* frame image */
	gint32  tmp_image_id;      /* temp. flattened preview image */

        /* for generating animated preview only */
	t_apv_mode  apv_mode;
        gint32   apv_src_frame;     /* image_id of the (already scaled) baseframe
                                     * or -1 in exact mode.
			             * (exact mode uses unscaled original frames)
			             */
        gint32   apv_mlayer_image;  /* destination image_id for the animated preview
                                     * -1 if we are not in anim_preview mode
				     */
        gchar   *apv_gap_paste_buff;  /* Optional PasteBuffer (to store preview frames)
	                               * "/tmp/gimp_video_paste_buffer/gap_pasteframe_" 
				       * NULL if we do not copy frames to a paste_buffer
				       */

        gdouble  apv_framerate;
        gdouble  apv_scalex;
        gdouble  apv_scaley;

} t_mov_values;

typedef struct {
	t_anim_info  *dst_ainfo_ptr;      /* destination frames */
	t_mov_values *val_ptr;
} t_mov_data;

long  p_move_dialog (t_mov_data *mov_ptr);
void  p_set_handle_offsets(t_mov_values *val_ptr, t_mov_current *cur_ptr);
int   p_mov_render(gint32 image_id, t_mov_values *val_ptr, t_mov_current *cur_ptr);

#endif
