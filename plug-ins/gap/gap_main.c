/* gap_main.c
 * 1997.11.05 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Package
 *
 * This Module contains:
 * - MAIN of the most GAP_Plugins
 * - query   registration of GAP Procedures (Video Menu) in the PDB
 * - run     invoke the selected GAP procedure by its PDB name
 * 
 *
 * GAP provides Animation Functions for the gimp,
 * working on a series of Images stored on disk in gimps .xcf Format.
 *
 * Frames are Images with naming convention like this:
 * Imagename_<number>.<ext>
 * Example:   snoopy_0001.xcf, snoopy_0002.xcf, snoopy_0003.xcf
 *
 * if gzip is installed on your system you may optional
 * use gziped xcf frames with extensions ".xcfgz"
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

static char *gap_main_version =  "1.1.29b; 2000/11/30";

/* revision history:
 * gimp    1.1.29b; 2000/11/30  hof: - GAP locks do check (only on UNIX) if locking Process is still alive
 *                                   - NONINTERACTIVE PDB Interface(s) for MovePath
 *                                      plug_in_gap_get_animinfo, plug_in_gap_set_framerate
 *                                   - updated main version, e-mail adress
 * gimp    1.1.20a; 2000/04/25  hof: NON_INTERACTIVE PDB-Interfaces video_edit_copy/paste/clear
 * gimp    1.1.14a; 2000/01/01  hof: bugfix params for gap_dup in noninteractive mode
 * gimp    1.1.13a; 1999/11/26  hof: splitted frontends for external programs (mpeg encoders)
 *                                   to gap_frontends_main.c
 * gimp    1.1.11a; 1999/11/15  hof: changed Menunames (AnimFrames to Video, Submenu Encode)
 * gimp    1.1.10a; 1999/10/22  hof: extended dither options for gap_range_convert
 * gimp    1.1.8a;  1999/08/31  hof: updated main version
 * version 0.99.00; 1999/03/17  hof: updated main version
 * version 0.98.02; 1999/01/27  hof: updated main version
 * version 0.98.01; 1998/12/21  hof: updated main version, e-mail adress
 * version 0.98.00; 1998/11/27  hof: updated main version, started port to GIMP 1.1 interfaces
 *                                   Use no '_' (underscore) in menunames. (has special function in 1.1)
 * version 0.96.03; 1998/08/31  hof: updated main version,
 *                                         gap_range_to_multilayer now returns image_id
 *                                         gap_split_image now returns image_id
 * version 0.96.02; 1998/08/05  hof: updated main version, 
 *                                   added gap_shift
 * version 0.96.00; 1998/06/27  hof: added gap animation sizechange plugins
 *                                         gap_split_image
 *                                         gap_mpeg_encode
 * version 0.94.01; 1998/04/28  hof: updated main version,
 *                                   added flatten_mode to plugin: gap_range_to_multilayer
 * version 0.94.00; 1998/04/25  hof: updated main version
 * version 0.93.01; 1998/02/03  hof:
 * version 0.92.00;             hof: set gap_debug from environment 
 * version 0.91.00; 1997/12/22  hof: 
 * version 0.90.00;             hof: 1.st (pre) release
 */
 
/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "config.h"
#include "libgimp/stdplugins-intl.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_lib.h"
#include "gap_match.h"
#include "gap_range_ops.h"
#include "gap_split.h"
#include "gap_mov_exec.h"
#include "gap_mod_layer.h"
#include "gap_arr_dialog.h"
#include "gap_pdb_calls.h"

/* ------------------------
 * global gap DEBUG switch
 * ------------------------
 */

/* int gap_debug = 1; */    /* print debug infos */
/* int gap_debug = 0; */    /* 0: dont print debug infos */

int gap_debug = 0;



static void query(void);
static void run(char *name, int nparam, GimpParam *param,
                int *nretvals, GimpParam **retvals);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};


  static GimpParamDef args_std[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
  };
  static int nargs_std = sizeof(args_std) / sizeof(args_std[0]);

  static GimpParamDef args_goto[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "nr", "frame nr where to go"},
  };
  static int nargs_goto = sizeof(args_goto) / sizeof(args_goto[0]);

  static GimpParamDef args_del[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "nr", "number of frames to delete (delete starts at current frame)"},
  };
  static int nargs_del = sizeof(args_del) / sizeof(args_del[0]);


  static GimpParamDef args_dup[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "nr", "how often to copy current frame"},
    {GIMP_PDB_INT32, "range_from", "frame nr to start"},
    {GIMP_PDB_INT32, "range_to", "frame nr to stop (can be lower than range_from)"},
  };
  static int nargs_dup = sizeof(args_dup) / sizeof(args_dup[0]);

  static GimpParamDef args_exchg[] =
  {
    {GIMP_PDB_INT32, "run_mode", "non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "nr", "nr of frame to exchange with current frame"},
  };
  static int nargs_exchg = sizeof(args_exchg) / sizeof(args_exchg[0]);

  static GimpParamDef args_mov[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
  };
  static int nargs_mov = sizeof(args_mov) / sizeof(args_mov[0]);

  static GimpParamDef args_mov_path[] =
  {
    {GIMP_PDB_INT32,        "run_mode",   "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE,        "dst_image",  "Destination image (one of the Anim Frames), where to insert the animated source layers"},
    {GIMP_PDB_DRAWABLE,     "drawable",   "drawable (unused)"},
    {GIMP_PDB_INT32,        "range_from", "destination frame nr to start"},
    {GIMP_PDB_INT32,        "range_to",   "destination frame nr to stop (can be lower than range_from)"},
    {GIMP_PDB_INT32,        "nr",         "layerstack position where to insert source layer (0 == on top)"},
    /* source specs */
    { GIMP_PDB_LAYER,      "src_layer_id",      "starting LayerID of SourceObject. (use any Multilayeranimated Image, or an AnimFrame of anoter Animation)"},
    { GIMP_PDB_INT32,      "src_stepmode",      "0-5     derive inserted object as copy of one layer from a multilayer src_image \n"
                                                "100-105 derive inserted object as copy of merged visible layers of a source animframe \n"
                                                "0:  Layer Loop  1: Layer Loop reverse  2: Layer Once  3: Layer Once reverse  4: Layer PingPong \n"
						"5: None (use onle the selected src_layer)\n"
                                                "100: Frame Loop  101: Frame Loop reverse  102: Frame Once  103: Frame Once reverse  104: Frame PingPong \n"
						"105: Frame None (use onle the flat copy of the selected frame)\n"
						},
    { GIMP_PDB_INT32,      "src_handle",        "0: handle left top   1: handle left bottom \n"
                                                "2: handle right top  3: handle right bottom \n"
						"4: handle center"},
    { GIMP_PDB_INT32,      "src_paintmode",     "0: GIMP_NORMAL_MODE (see GimpLayerModeEffects -- libgimp/gimpenums.h -- for more information)"},
    { GIMP_PDB_INT32,      "src_force_visible", "1: Set inserted layres visible, 0: insert layers as is"},
    { GIMP_PDB_INT32,      "clip_to_img",       "1: Clip inserted layers to Image size of the destination AnimFrame, 0: dont clip"},
    /* extras */
    { GIMP_PDB_INT32,      "rotation_follow",   "0: NO automatic calculation (use the rotation array parameters as it is) \n"
                                                "1: Automatic calculation of rotation, following the path vectors, (Ignore rotation array parameters)\n"},
    { GIMP_PDB_INT32,      "startangle",        "start angle for the first contolpoint (only used if rotation-follow is on)"},
    /* CONTROLPOINT Arrays */
    { GIMP_PDB_INT32ARRAY, "p_x",               "Controlpoint x-koordinate"},
    { GIMP_PDB_INT32,      "argc_p_x",          "number of controlpoints"},
    { GIMP_PDB_INT32ARRAY, "p_y",               "Controlpoint y-koordinate"},
    { GIMP_PDB_INT32,      "argc_p_y",          "number of controlpoints"},
    { GIMP_PDB_INT32ARRAY, "opacity",           "Controlpoint opacity value 0 <= value <= 100"},
    { GIMP_PDB_INT32,      "argc_opacity",      "number of controlpoints"},
    { GIMP_PDB_INT32ARRAY, "w_resize",          "width scaling in percent"},
    { GIMP_PDB_INT32,      "argc_w_resize",     "number of controlpoints"},
    { GIMP_PDB_INT32ARRAY, "h_resize",          "height scaling in percent"},
    { GIMP_PDB_INT32,      "argc_h_resize",     "number of controlpoints"},
    { GIMP_PDB_INT32ARRAY, "rotation",          "rotation in degrees"},
    { GIMP_PDB_INT32,      "argc_rotation",     "number of controlpoints"},
    { GIMP_PDB_INT32ARRAY, "keyframe_abs",      "n: fix controlpoint to this frame number, 0: for controlpoints that are not fixed to a frame."},
    { GIMP_PDB_INT32,      "argc_keyframe_abs", "number of controlpoints"},
  };
  static int nargs_mov_path = sizeof(args_mov_path) / sizeof(args_mov_path[0]);

  static GimpParamDef args_mov_path2[] =
  {
    {GIMP_PDB_INT32,        "run_mode",   "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE,        "dst_image",  "Destination image (one of the Anim Frames), where to insert the animated source layers"},
    {GIMP_PDB_DRAWABLE,     "drawable",   "drawable (unused)"},
    {GIMP_PDB_INT32,        "range_from", "destination frame nr to start"},
    {GIMP_PDB_INT32,        "range_to",   "destination frame nr to stop (can be lower than range_from)"},
    {GIMP_PDB_INT32,        "nr",         "layerstack position where to insert source layer (0 == on top)"},
    /* source specs */
    { GIMP_PDB_LAYER,      "src_layer_id",      "starting LayerID of SourceObject. (use any Multilayeranimated Image, or an AnimFrame of anoter Animation)"},
    { GIMP_PDB_INT32,      "src_stepmode",      "0-5     derive inserted object as copy of one layer from a multilayer src_image \n"
                                                "100-105 derive inserted object as copy of merged visible layers of a source animframe \n"
                                                "0:  Layer Loop  1: Layer Loop reverse  2: Layer Once  3: Layer Once reverse  4: Layer PingPong \n"
						"5: None (use onle the selected src_layer)\n"
                                                "100: Frame Loop  101: Frame Loop reverse  102: Frame Once  103: Frame Once reverse  104: Frame PingPong \n"
						"105: Frame None (use onle the flat copy of the selected frame)\n"
						},
    { GIMP_PDB_INT32,      "src_handle",        "0: handle left top   1: handle left bottom \n"
                                                "2: handle right top  3: handle right bottom \n"
						"4: handle center"},
    { GIMP_PDB_INT32,      "src_paintmode",     "0: GIMP_NORMAL_MODE (see GimpLayerModeEffects -- libgimp/gimpenums.h -- for more information)"},
    { GIMP_PDB_INT32,      "src_force_visible", "1: Set inserted layres visible, 0: insert layers as is"},
    { GIMP_PDB_INT32,      "clip_to_img",       "1: Clip inserted layers to Image size of the destination AnimFrame, 0: dont clip"},
    /* extras */
    { GIMP_PDB_INT32,      "rotation_follow",   "0: NO automatic calculation (use the rotation array parameters as it is) \n"
                                                "1: Automatic calculation of rotation, following the path vectors, (Ignore rotation array parameters)\n"},
    { GIMP_PDB_INT32,      "startangle",        "start angle for the first contolpoint (only used if rotation-follow is on)"},
    /* CONTROLPOINTs from file */
    { GIMP_PDB_STRING,     "pointfile",         "a file with contolpoints (readable text file with one line per controlpoint)"},
  };
  static int nargs_mov_path2 = sizeof(args_mov_path2) / sizeof(args_mov_path2[0]);
    


  static GimpParamDef args_f2multi[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "range_from", "frame nr to start"},
    {GIMP_PDB_INT32, "range_to", "frame nr to stop (can be lower than range_from)"},
    {GIMP_PDB_INT32, "flatten_mode", "{ expand-as-necessary(0), CLIP-TO_IMG(1), CLIP-TO-BG-LAYER(2), FLATTEN(3) }"},
    {GIMP_PDB_INT32, "bg_visible", "{ BG_NOT_VISIBLE (0), BG_VISIBLE(1) }"},
    {GIMP_PDB_INT32, "framerate", "frame delaytime in ms"},
    {GIMP_PDB_STRING, "frame_basename", "basename for all generated layers"},
    {GIMP_PDB_INT32, "select_mode", "Mode how to identify a layer: 0-3 by layername 0=equal, 1=prefix, 2=suffix, 3=contains, 4=layerstack_numberslist, 5=inv_layerstack, 6=all_visible"},
    {GIMP_PDB_INT32, "select_case", "0: ignore case 1: select_string is case sensitive"},
    {GIMP_PDB_INT32, "select_invert", "0: select normal 1: invert (select all unselected layers)"},
    {GIMP_PDB_STRING, "select_string", "string to match with layername (how to match is defined by select_mode)"},
  };
  static int nargs_f2multi = sizeof(args_f2multi) / sizeof(args_f2multi[0]);

  static GimpParamDef return_f2multi[] =
  {
    { GIMP_PDB_IMAGE, "new_image", "Output image" }
  };
  static int nreturn_f2multi = sizeof(return_f2multi) / sizeof(return_f2multi[0]);




  static GimpParamDef args_rflatt[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "range_from", "frame nr to start"},
    {GIMP_PDB_INT32, "range_to", "frame nr to stop (can be lower than range_from)"},
  };
  static int nargs_rflatt = sizeof(args_rflatt) / sizeof(args_rflatt[0]);

  static GimpParamDef args_rlayerdel[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "range_from", "frame nr to start"},
    {GIMP_PDB_INT32, "range_to", "frame nr to stop (can be lower than range_from)"},
    {GIMP_PDB_INT32, "nr", "layerstack position (0 == on top)"},
  };
  static int nargs_rlayerdel = sizeof(args_rlayerdel) / sizeof(args_rlayerdel[0]);


  static GimpParamDef args_rconv[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "range_from", "frame nr to start"},
    {GIMP_PDB_INT32, "range_to", "frame nr to stop (can be lower than range_from)"},
    {GIMP_PDB_INT32, "flatten", "0 .. dont flatten image before save"},
    {GIMP_PDB_INT32, "dest_type", "0=RGB, 1=GRAY, 2=INDEXED"},
    {GIMP_PDB_INT32, "dest_colors", "1 upto 256 (used only for dest_type INDEXED)"},
    {GIMP_PDB_INT32, "dest_dither", "0=no, 1=floyd-steinberg  2=fs/low-bleed, 3=fixed (used only for dest_type INDEXED)"},
    {GIMP_PDB_STRING, "extension", "extension for the destination filetype (jpg, tif ...or any other gimp supported type)"},
    {GIMP_PDB_STRING, "basename", "(optional parameter) here you may specify the basename of the destination frames \"/my_dir/myframe\"  _0001.ext is added)"},
  };
  static int nargs_rconv = sizeof(args_rconv) / sizeof(args_rconv[0]);


  static GimpParamDef args_rconv2[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "range_from", "frame nr to start"},
    {GIMP_PDB_INT32, "range_to", "frame nr to stop (can be lower than range_from)"},
    {GIMP_PDB_INT32, "flatten", "0 .. dont flatten image before save"},
    {GIMP_PDB_INT32, "dest_type", "0=RGB, 1=GRAY, 2=INDEXED"},
    {GIMP_PDB_INT32, "dest_colors", "1 upto 256 (used only for dest_type INDEXED)"},
    {GIMP_PDB_INT32, "dest_dither", "0=no, 1=floyd-steinberg 2=fs/low-bleed, 3=fixed(used only for dest_type INDEXED)"},
    {GIMP_PDB_STRING, "extension", "extension for the destination filetype (jpg, tif ...or any other gimp supported type)"},
    {GIMP_PDB_STRING, "basename", "(optional parameter) here you may specify the basename of the destination frames \"/my_dir/myframe\"  _0001.ext is added)"},
    {GIMP_PDB_INT32,  "palette_type", "0 == MAKE_PALETTE, 2 == WEB_PALETTE, 3 == MONO_PALETTE (bw) 4 == CUSTOM_PALETTE (used only for dest_type INDEXED)"},
    {GIMP_PDB_INT32,  "alpha_dither", "dither transparency to fake partial opacity (used only for dest_type INDEXED)"},
    {GIMP_PDB_INT32,  "remove_unused", "remove unused or double colors from final palette (used only for dest_type INDEXED)"},
    {GIMP_PDB_STRING, "palette", "name of the cutom palette to use (used only for dest_type INDEXED and palette_type == CUSTOM_PALETTE) "},
  };
  static int nargs_rconv2 = sizeof(args_rconv2) / sizeof(args_rconv2[0]);

  /* resize and crop share the same parameters */
  static GimpParamDef args_resize[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "new_width", "width of the resulting  anim_frame images in pixels"},
    {GIMP_PDB_INT32, "new_height", "height of the resulting  anim_frame images in pixels"},
    {GIMP_PDB_INT32, "offset_x", "X offset in pixels"},
    {GIMP_PDB_INT32, "offset_y", "Y offset in pixels"},
  };
  static int nargs_resize = sizeof(args_resize) / sizeof(args_resize[0]);

  static GimpParamDef args_scale[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "new_width", "width of the resulting  anim_frame images in pixels"},
    {GIMP_PDB_INT32, "new_height", "height of the resulting  anim_frame images in pixels"},
  };
  static int nargs_scale = sizeof(args_scale) / sizeof(args_scale[0]);


  static GimpParamDef args_split[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (NO Anim Frame allowed)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "inverse_order", "True/False"},
    {GIMP_PDB_INT32, "no_alpha", "True: remove alpha channel(s) in the destination frames"},
    {GIMP_PDB_STRING, "extension", "extension for the destination filetype (jpg, tif ...or any other gimp supported type)"},
  };
  static int nargs_split = sizeof(args_split) / sizeof(args_split[0]);

  static GimpParamDef return_split[] =
  {
    { GIMP_PDB_IMAGE, "new_image", "Output image (first or last resulting frame)" }
  };
  static int nreturn_split = sizeof(return_split) / sizeof(return_split[0]);


  static GimpParamDef *return_vals = NULL;
  static int nreturn_vals = 0;

  static GimpParamDef args_shift[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "nr", "how many framenumbers to shift the framesequence"},
    {GIMP_PDB_INT32, "range_from", "frame nr to start"},
    {GIMP_PDB_INT32, "range_to", "frame nr to stop"},
  };
  static int nargs_shift = sizeof(args_shift) / sizeof(args_shift[0]);

  static GimpParamDef args_modify[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "range_from", "frame nr to start"},
    {GIMP_PDB_INT32, "range_to", "frame nr to stop"},
    {GIMP_PDB_INT32, "action_mode", "0:set_visible, 1:set_invisible, 2:set_linked, 3:set_unlinked, 4:raise, 5:lower, 6:merge_expand, 7:merge_img, 8:merge_bg, 9:apply_filter, 10:duplicate, 11:delete, 12:rename"},
    {GIMP_PDB_INT32, "select_mode", "Mode how to identify a layer: 0-3 by layername 0=equal, 1=prefix, 2=suffix, 3=contains, 4=layerstack_numberslist, 5=inv_layerstack, 6=all_visible"},
    {GIMP_PDB_INT32, "select_case", "0: ignore case 1: select_string is case sensitive"},
    {GIMP_PDB_INT32, "select_invert", "0: select normal 1: invert (select all unselected layers)"},
    {GIMP_PDB_STRING, "select_string", "string to match with layername (how to match is defined by select_mode)"},
    {GIMP_PDB_STRING, "new_layername", "is only used at action rename. [####] is replaced by the framnumber"},
  };
  static int nargs_modify = sizeof(args_modify) / sizeof(args_modify[0]);

  static GimpParamDef args_video_copy[] =
  {
    {GIMP_PDB_INT32, "run_mode", "always non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "range_from", "frame nr to start"},
    {GIMP_PDB_INT32, "range_to", "frame nr to stop"},
  };
  static int nargs_video_copy = sizeof(args_video_copy) / sizeof(args_video_copy[0]);

  static GimpParamDef args_video_paste[] =
  {
    {GIMP_PDB_INT32, "run_mode", "always non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "paste_mode", "0 .. paste at current frame (replacing current and following frames)"
                                "1 .. paste insert before current frame "
				"2 .. paste insert after current frame"},
  };
  static int nargs_video_paste = sizeof(args_video_paste) / sizeof(args_video_paste[0]);

  static GimpParamDef args_video_clear[] =
  {
    {GIMP_PDB_INT32, "run_mode", "always non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (is ignored)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
  };
  static int nargs_video_clear = sizeof(args_video_clear) / sizeof(args_video_clear[0]);

  static GimpParamDef return_ainfo[] =
  {
    { GIMP_PDB_INT32,  "first_frame_nr", "lowest frame number of all AnimFrame discfiles" },
    { GIMP_PDB_INT32,  "last_frame_nr",  "highest frame number of all AnimFrame discfiles" },
    { GIMP_PDB_INT32,  "curr_frame_nr",  "current frame number (extracted from the imagename)" },
    { GIMP_PDB_INT32,  "frame_cnt",      "total number of all AnimFrame discfiles that belong to the passed image" },
    { GIMP_PDB_STRING, "basename",       "basename of the AnimFrame (without frame number and extension)\n"
                                         " may also include path (depending how the current frame image was opened)" },
    { GIMP_PDB_STRING, "extension",      "extension of the AnimFrames (.xcf)" },
    { GIMP_PDB_FLOAT,  "framerate",      "framerate in frames per second" }
  };
  static int nreturn_ainfo = sizeof(return_ainfo) / sizeof(return_ainfo[0]);

  static GimpParamDef args_setrate[] =
  {
    {GIMP_PDB_INT32, "run_mode", "non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_FLOAT,  "framerate",      "framerate in frames per second" }
  };
  static int nargs_setrate = sizeof(args_setrate) / sizeof(args_setrate[0]);



MAIN ()

static void
query ()
{
  gchar *l_help_str;
  INIT_I18N();

  gimp_install_procedure("plug_in_gap_next",
			 "This plugin exchanges current image with (next nubered) image from disk.",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Goto/Next Frame"),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_std, nreturn_vals,
			 args_std, return_vals);

  gimp_install_procedure("plug_in_gap_prev",
			 "This plugin exchanges current image with (previous nubered) image from disk.",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Goto/Previous Frame"),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_std, nreturn_vals,
			 args_std, return_vals);

  gimp_install_procedure("plug_in_gap_first",
			 "This plugin exchanges current image with (lowest nubered) image from disk.",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Goto/First Frame"),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_std, nreturn_vals,
			 args_std, return_vals);

  gimp_install_procedure("plug_in_gap_last",
			 "This plugin exchanges current image with (highest nubered) image from disk.",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Goto/Last Frame"),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_std, nreturn_vals,
			 args_std, return_vals);

  gimp_install_procedure("plug_in_gap_goto",
			 "This plugin exchanges current image with requested image (nr) from disk.",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Goto/Any Frame..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_goto, nreturn_vals,
			 args_goto, return_vals);

  gimp_install_procedure("plug_in_gap_del",
			 "This plugin deletes the given number of frames from disk including the current frame.",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Delete Frames..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_del, nreturn_vals,
			 args_del, return_vals);

  gimp_install_procedure("plug_in_gap_dup",
			 "This plugin duplicates the current frames on disk n-times.",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Duplicate Frames..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_dup, nreturn_vals,
			 args_dup, return_vals);

  gimp_install_procedure("plug_in_gap_exchg",
			 "This plugin exchanges content of the current with destination frame.",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Exchange Frame..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_exchg, nreturn_vals,
			 args_exchg, return_vals);

  gimp_install_procedure("plug_in_gap_move",
			 "This plugin copies layer(s) from one sourceimage to multiple frames on disk, varying position, size and opacity.",
			 "For NONINTERACTIVE PDB interfaces see also (plug_in_gap_move_path, plug_in_gap_move_path2)",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Move Path..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_mov, nreturn_vals,
			 args_mov, return_vals);

  l_help_str = g_strdup_printf(
			 "This plugin inserts one layer in each frame of the selected frame range of an Animation\n"
			 " (specified by the dst_image parameter).\n"
			 " An Animation is a series of numbered AnimFrame Images on disk where only the current\n"
			 " Frame is opened in the gimp\n"
			 " The inserted layer is derived from another (multilayer)image\n"
			 " or from another Animation (as merged copy of the visible layers in a source frame)\n"
			 " the affected destination frame range is selected by the range_from and range_to parameters\n"
			 " the src_stepmode parameter controls how to derive the layer that is to be inserted.\n"
			 " With the Controlpoint Parameters you can control position (koordinates),\n"
			 " size, rotation and opacity values of the inserted layer\n"
			 " If you want to move an Object from position AX/AY to BX/BY in a straight line within the range of 24 frames\n"
			 " you need 2 Contolpoints, if you want the object to move folowing a path\n"
			 " you need some more Controlpoints to do that.\n"
			 " With the rotation_follow Parameter you can force automatic calculation of the rotation for the inserted\n"
			 " layer according to the path vectors it is moving along.\n"
			 " A controlpoint can be fixed to a special framenumber using the keyframe_abs controlpoint-parameter.\n"
			 " Restictions:\n"
			 " - keyframe_abs numbers must be 0 (== not fixed) or a frame_number within the affected frame range\n"
			 " - keyframes_abs must be in sequence (ascending or descending)\n"
			 " - the first and last controlpoint are always implicite keyframes, and should be passed with keyframe_abs = 0\n"
			 " - the number of controlpoints is limitied to a maximum of %d.\n"
                         "   the number of controlpoints must be passed in all argc_* parameters\n"
			 "See also (plug_in_gap_move_path, plug_in_gap_move)",
			 (int)GAP_MOV_MAX_POINT);
			 
  gimp_install_procedure("plug_in_gap_move_path",
			 "This plugin copies layer(s) from one sourceimage or source animation to multiple frames on disk,"
			 " varying position, size and opacity.",
			 l_help_str,
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 NULL,                      /* do not appear in menus */
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_mov_path, nreturn_vals,
			 args_mov_path, return_vals);
  g_free(l_help_str);
 
  gimp_install_procedure("plug_in_gap_move_path2",
			 "This plugin copies layer(s) from one sourceimage or source animation to multiple frames on disk,"
			 " varying position, size and opacity."
			 ,
			 "This plugin is just another Interface for the MovePath (plug_in_gap_move_path)\n"
			 " using a File to specify Controlpoints (rather than Array parameters).\n"
			 " Notes:\n"
			 " - you can create a controlpoint file with in the MovePath Dialog (interactive call of plug_in_gap_move)\n"
			 " - for more infos about controlpoints see help of (plug_in_gap_move_path)\n"
			 ,
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 NULL,                      /* do not appear in menus */
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_mov_path2, nreturn_vals,
			 args_mov_path2, return_vals);

  gimp_install_procedure("plug_in_gap_range_to_multilayer",
			 "This plugin creates a new image from the given range of frame-images. Each frame is converted to one layer in the new image, according to flatten_mode. (the frames on disk are not changed).",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames to Image..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_f2multi, nreturn_f2multi,
			 args_f2multi, return_f2multi);

  gimp_install_procedure("plug_in_gap_range_flatten",
			 "This plugin flattens the given range of frame-images (on disk)",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames Flatten..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_rflatt, nreturn_vals,
			 args_rflatt, return_vals);

  gimp_install_procedure("plug_in_gap_range_layer_del",
			 "This plugin deletes one layer in the given range of frame-images (on disk). exception: the last remaining layer of a frame is not deleted",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames LayerDel..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_rlayerdel, nreturn_vals,
			 args_rlayerdel, return_vals);

  gimp_install_procedure("plug_in_gap_range_convert",
			 "This plugin converts the given range of frame-images to other fileformats (on disk) depending on extension",
			 "WARNING this procedure is obsolete, please use plug_in_gap_range_convert2",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 NULL,                      /* do not appear in menus */
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_rconv, nreturn_vals,
			 args_rconv, return_vals);

  gimp_install_procedure("plug_in_gap_range_convert2",
			 "This plugin converts the given range of frame-images to other fileformats (on disk) depending on extension",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames Convert..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_rconv2, nreturn_vals,
			 args_rconv2, return_vals);

  gimp_install_procedure("plug_in_gap_anim_resize",
			 "This plugin resizes all anim_frames (images on disk) to the given new_width/new_height",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames Resize..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_resize, nreturn_vals,
			 args_resize, return_vals);

  gimp_install_procedure("plug_in_gap_anim_crop",
			 "This plugin crops all anim_frames (images on disk) to the given new_width/new_height",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames Crop..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_resize, nreturn_vals,
			 args_resize, return_vals);

  gimp_install_procedure("plug_in_gap_anim_scale",
			 "This plugin scales all anim_frames (images on disk) to the given new_width/new_height",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames Scale..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_scale, nreturn_vals,
			 args_scale, return_vals);

  gimp_install_procedure("plug_in_gap_split",
			 "This plugin splits the current image to anim frames (images on disk). Each layer is saved as one frame",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Split Image to Frames..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_split, nreturn_split,
			 args_split, return_split);


  gimp_install_procedure("plug_in_gap_shift",
			 "This plugin exchanges frame numbers in the given range. (discfile frame_0001.xcf is renamed to frame_0002.xcf, 2->3, 3->4 ... n->1)",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Framesequence Shift..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_shift, nreturn_vals,
			 args_shift, return_vals);

  gimp_install_procedure("plug_in_gap_modify",
			 "This plugin performs a modifying action on each selected layer in each selected framerange",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames Modify..."),
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_modify, nreturn_vals,
			 args_modify, return_vals);

  gimp_install_procedure("plug_in_gap_video_edit_copy",
			 "This plugin appends the selected framerange to the video paste buffer"
			 "the video paste buffer is a directory configured by gimprc (video-paste-dir )"
			 "and a framefile basename configured by gimprc (video-paste-basename)",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 NULL,                     /* do not appear in menus */
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_video_copy, nreturn_vals,
			 args_video_copy, return_vals);

  gimp_install_procedure("plug_in_gap_video_edit_paste",
			 "This plugin copies all frames from the video paste buffer"
			 "to the current video. Depending on the paste_mode parameter"
			 "the copied frames are replacing frames beginning at current frame"
			 "or are inserted before or after the current frame"
			 "the pasted frames are scaled to fit the current video size"
			 "and converted in Imagetype (RGB,GRAY,INDEXED) if necessary"
			 "the video paste buffer is a directory configured by gimprc (video-paste-dir )"
			 "and a framefile basename configured by gimprc (video-paste-basename)",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 NULL,                     /* do not appear in menus */
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_video_paste, nreturn_vals,
			 args_video_paste, return_vals);

  gimp_install_procedure("plug_in_gap_video_edit_clear",
			 "clear the video paste buffer by deleting all framefiles"
			 "the video paste buffer is a directory configured by gimprc (video-paste-dir )"
			 "and a framefile basename configured by gimprc (video-paste-basename)",
			 "",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 NULL,                     /* do not appear in menus */
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_video_clear, nreturn_vals,
			 args_video_clear, return_vals);

  gimp_install_procedure("plug_in_gap_get_animinfo",
			 "This plugin gets animation infos about AnimFrames."
                         ,
			 "Informations about the AnimFrames belonging to the\n"
                         " passed image_id are returned. (therefore the directory\n"
                         " is scanned and checked for AnimFrame discfiles.\n"
                         " If you call this plugin on images without a Name\n"
                         " You will get just default values."
                         ,
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 NULL,                /* no menu */
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_std, nreturn_ainfo,
			 args_std, return_ainfo);

  gimp_install_procedure("plug_in_gap_set_framerate",
			 "This plugin sets the framerate for AnimFrames",
			 "The framerate is stored in a video info file"
                         " named like the basename of the AnimFrames"
                         " without a framenumber. The extension"
                         " of this video info file is .vin",
			 "Wolfgang Hofer (hof@gimp.org)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 NULL,                      /* no menu */
			 "RGB*, INDEXED*, GRAY*",
			 GIMP_PLUGIN,
			 nargs_setrate, nreturn_vals,
			 args_setrate, return_vals);
			 
}	/* end query */



static void
run (char    *name,
     int      n_params,
     GimpParam  *param,
     int     *nreturn_vals,
     GimpParam **return_vals)
{
  char       *l_env;
  
  char        l_extension[32];
  char        l_sel_str[MAX_LAYERNAME];
  char        l_layername[MAX_LAYERNAME];
  char       *l_basename_ptr;
  char       *l_palette_ptr;
  static GimpParam values[10];
  GimpRunModeType run_mode;
  GimpRunModeType lock_run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32     image_id;
  gint32     lock_image_id;
  gint32     nr;
  long       range_from, range_to;
  GimpImageBaseType dest_type;
  gint32     dest_colors;
  gint32     dest_dither;
  gint32     palette_type;
  gint32     alpha_dither;
  gint32     remove_unused;
  gint32     mode;
  long       new_width;
  long       new_height;
  long       offs_x;
  long       offs_y;
  gint32     inverse_order;
  gint32     no_alpha;
  long       framerate;
#define      FRAME_BASENAME_LEN   256  
  char       frame_basename[FRAME_BASENAME_LEN];
  gint32     sel_mode, sel_case, sel_invert;
   
  gint32     l_rc;

  *nreturn_vals = 1;
  *return_vals = values;
  nr = 0;
  l_rc = 0;


  l_env = g_getenv("GAP_DEBUG");
  if(l_env != NULL)
  {
    if((*l_env != 'n') && (*l_env != 'N')) gap_debug = 1;
  }

  run_mode = param[0].data.d_int32;
  lock_run_mode = run_mode;
  if((strcmp (name, "plug_in_gap_prev") == 0)
  || (strcmp (name, "plug_in_gap_next") == 0))
  {
    /* stepping from frame to frame is often done
     * in quick sequence via Shortcut Key, and should
     * not open dialog windows if the previous GAP lock is still active.
     * therefore i set NONINTERACTIVE run_mode to prevent
     * p_gap_lock_is_locked from open a gimp_message window.
     */
    lock_run_mode = GIMP_RUN_NONINTERACTIVE;
  }

  if(gap_debug) fprintf(stderr, "\n\ngap_main: debug name = %s\n", name);
  
  values[0].type = GIMP_PDB_STATUS;
  image_id = param[1].data.d_image;
  lock_image_id = image_id;


  /* ---------------------------
   * NON-LOCKING gap_plugins
   * ---------------------------
   */
  if (strcmp (name, "plug_in_gap_get_animinfo") == 0)
  {
      t_anim_info *ainfo_ptr;
      t_video_info *vin_ptr;

      ainfo_ptr = p_alloc_ainfo(image_id, GIMP_RUN_NONINTERACTIVE);
      if(ainfo_ptr == NULL)
      {
          status = GIMP_PDB_EXECUTION_ERROR;
      }
      else if (0 != p_dir_ainfo(ainfo_ptr))
      {
          status = GIMP_PDB_EXECUTION_ERROR;;
      }
 
  
      /* return the new generated image_id */
      *nreturn_vals = nreturn_ainfo +1;

      values[0].data.d_status = status;
      
      values[1].type = GIMP_PDB_INT32;
      values[2].type = GIMP_PDB_INT32;
      values[3].type = GIMP_PDB_INT32;
      values[4].type = GIMP_PDB_INT32;
      values[5].type = GIMP_PDB_STRING;
      values[6].type = GIMP_PDB_STRING;
      values[7].type = GIMP_PDB_FLOAT;
      
      if(ainfo_ptr != NULL)
      {
        values[1].data.d_int32  = ainfo_ptr->first_frame_nr;
        values[2].data.d_int32  = ainfo_ptr->last_frame_nr;
        values[3].data.d_int32  = ainfo_ptr->curr_frame_nr;
        values[4].data.d_int32  = ainfo_ptr->frame_cnt;
        values[5].data.d_string = g_strdup(ainfo_ptr->basename);
        values[6].data.d_string = g_strdup(ainfo_ptr->extension);
        values[7].data.d_float  = -1.0;
        
        vin_ptr = p_get_video_info(ainfo_ptr->basename);
        if(vin_ptr != NULL)
        {
           values[7].data.d_float = vin_ptr->framerate;
        }
      }
      return;
  }
  else if (strcmp (name, "plug_in_gap_set_framerate") == 0)
  {
      t_anim_info *ainfo_ptr;
      t_video_info *vin_ptr;
 
      if (n_params != nargs_setrate)
      {
         status = GIMP_PDB_CALLING_ERROR;
      }
      else
      {
        ainfo_ptr = p_alloc_ainfo(image_id, GIMP_RUN_NONINTERACTIVE);
        if(ainfo_ptr == NULL)
        {
            status = GIMP_PDB_EXECUTION_ERROR;
        }
        else if (0 != p_dir_ainfo(ainfo_ptr))
        {
            status = GIMP_PDB_EXECUTION_ERROR;;
        }
        else
        {
          vin_ptr = p_get_video_info(ainfo_ptr->basename);
          if(vin_ptr == NULL)
          {
            status = GIMP_PDB_EXECUTION_ERROR;;
          }
          else
          {
            vin_ptr->framerate = param[3].data.d_float;
            p_set_video_info(vin_ptr, ainfo_ptr->basename);
          }
        }
      }
  
     values[0].data.d_status = status;
     return;
  }

  

  /* ---------------------------
   * check for LOCKS
   * ---------------------------
   */
  if(p_gap_lock_is_locked(lock_image_id, lock_run_mode))
  {
       status = GIMP_PDB_EXECUTION_ERROR;
       values[0].data.d_status = status;
       return ;
  }


  /* set LOCK on current image (for all gap_plugins) */
  p_gap_lock_set(lock_image_id);
   
  if (run_mode == GIMP_RUN_NONINTERACTIVE) {
    INIT_I18N();
  } else {
    INIT_I18N_UI();
  }

  /* ---------------------------
   * LOCKING gap_plugins
   * ---------------------------
   */

  if (strcmp (name, "plug_in_gap_next") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_std)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        l_rc = gap_next(run_mode, image_id);
      }
  }
  else if (strcmp (name, "plug_in_gap_prev") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_std)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        l_rc = gap_prev(run_mode, image_id);
      }
  }
  else if (strcmp (name, "plug_in_gap_first") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_std)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        l_rc = gap_first(run_mode, image_id);
      }
  }
  else if (strcmp (name, "plug_in_gap_last") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_std)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        l_rc = gap_last(run_mode, image_id);
      }
  }
  else if (strcmp (name, "plug_in_gap_goto") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_goto)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        nr       = param[3].data.d_int32;  /* destination frame nr */
        l_rc = gap_goto(run_mode, image_id, nr);
      }
  }
  else if (strcmp (name, "plug_in_gap_del") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_del)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        nr       = param[3].data.d_int32;  /* number of frames to delete */
        l_rc = gap_del(run_mode, image_id, nr);
      }
  }
  else if (strcmp (name, "plug_in_gap_dup") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        /* accept the old interface with 4 parameters */
        if ((n_params != 4) && (n_params != nargs_dup))
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        nr       = param[3].data.d_int32;  /* how often to copy current frame */
        if (n_params > 5)
        {
           range_from = param[4].data.d_int32;  /* frame nr to start */	
           range_to   = param[5].data.d_int32;  /* frame nr to stop  */	
        }
        else
        {
           range_from = -1;	
           range_to   = -1;	
        }

        l_rc = gap_dup(run_mode, image_id, nr, range_from, range_to );

      }
  }
  else if (strcmp (name, "plug_in_gap_exchg") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_exchg)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        nr       = param[3].data.d_int32;  /* nr of frame to exchange with current frame */
        l_rc = gap_exchg(run_mode, image_id, nr);
      }
  }
  else if (strcmp (name, "plug_in_gap_move") == 0)
  {
      t_mov_values *pvals;
           
      pvals = g_malloc(sizeof(t_mov_values));
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
          status = GIMP_PDB_CALLING_ERROR;
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        l_rc = gap_move_path(run_mode, image_id, pvals, NULL, 0, 0);
      }
      g_free(pvals);
  }
  else if ((strcmp (name, "plug_in_gap_move_path") == 0)
       ||  (strcmp (name, "plug_in_gap_move_path2") == 0))
  {
      t_mov_values *pvals;
      gchar        *pointfile;
      gint          l_idx;
      gint          l_numpoints;
      gint          l_rotation_follow;
      gint32        l_startangle;
      
      pointfile = NULL;
      pvals = g_malloc(sizeof(t_mov_values));
      l_rotation_follow = 0;
      l_startangle = 0;

      pvals->dst_image_id = image_id;
      pvals->tmp_image_id = -1;
      pvals->apv_mode = 0;
      pvals->apv_src_frame = -1;
      pvals->apv_mlayer_image =  -1;
      pvals->apv_gap_paste_buff = NULL;
      pvals->apv_framerate = 24;
      pvals->apv_scalex = 100.0;
      pvals->apv_scaley = 100.0;
      pvals->cache_src_image_id = -1;
      pvals->cache_tmp_image_id = -1;
      pvals->cache_tmp_layer_id = -1;
      pvals->cache_frame_number = -1;
      pvals->cache_ainfo_ptr = NULL;
      pvals->point_idx = 0;
      pvals->point_idx_max = 0;
      
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if ( ((n_params != nargs_mov_path)  && (strcmp (name, "plug_in_gap_move_path")  == 0))
	||   ((n_params != nargs_mov_path2) && (strcmp (name, "plug_in_gap_move_path2") == 0)))
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
	else
	{
	   pvals->dst_range_start   = param[3].data.d_int32;
	   pvals->dst_range_end     = param[4].data.d_int32;
	   pvals->dst_layerstack    = param[5].data.d_int32;

	   pvals->src_layer_id      = param[6].data.d_layer;
	   pvals->src_stepmode      = param[7].data.d_int32;
	   pvals->src_handle        = param[8].data.d_int32;
	   pvals->src_paintmode     = param[9].data.d_int32;
	   pvals->src_force_visible = param[10].data.d_int32;
	   pvals->clip_to_img       = param[11].data.d_int32;

           l_rotation_follow        = param[12].data.d_int32;
           l_startangle             = param[13].data.d_int32;
	   
	   if (strcmp (name, "plug_in_gap_move_path")  == 0)
	   {
	      /* "plug_in_gap_move_path" passes controlpoints as array parameters */
	      l_numpoints = param[15].data.d_int32;
	      if ((l_numpoints != param[17].data.d_int32)
	      ||  (l_numpoints != param[19].data.d_int32)
	      ||  (l_numpoints != param[21].data.d_int32)
	      ||  (l_numpoints != param[23].data.d_int32)
	      ||  (l_numpoints != param[25].data.d_int32)
	      ||  (l_numpoints != param[27].data.d_int32))
	      {
	        printf("plug_in_gap_move_path: CallingError: different numbers in the controlpoint array argc parameters\n"); 
                status = GIMP_PDB_CALLING_ERROR;
	      }
	      else
	      {
		pvals->point_idx_max = l_numpoints -1;
		for(l_idx = 0; l_idx < l_numpoints; l_idx++)
		{
        	   pvals->point[l_idx].p_x = param[14].data.d_int32array[l_idx];
        	   pvals->point[l_idx].p_y = param[16].data.d_int32array[l_idx];
        	   pvals->point[l_idx].opacity = param[18].data.d_int32array[l_idx];
        	   pvals->point[l_idx].w_resize = param[20].data.d_int32array[l_idx];
        	   pvals->point[l_idx].h_resize = param[22].data.d_int32array[l_idx];
        	   pvals->point[l_idx].rotation = param[24].data.d_int32array[l_idx];
        	   pvals->point[l_idx].keyframe_abs = param[26].data.d_int32array[l_idx];
        	   /* pvals->point[l_idx].keyframe = ; */ /* relative keyframes are calculated later */
		}
	      }
	   }
	   else
	   {
	      /* "plug_in_gap_move_path2" operates with controlpoint file */
	      if(param[14].data.d_string != NULL)
	      {
	         pointfile = g_strdup(param[14].data.d_string);
	      }
	   }
	}

      }

      if (status == GIMP_PDB_SUCCESS)
      {
        l_rc = gap_move_path(run_mode, image_id, pvals, pointfile, l_rotation_follow, l_startangle);
      }
      g_free(pvals);
      if(pointfile != NULL) g_free(pointfile);
  }
  else if (strcmp (name, "plug_in_gap_range_to_multilayer") == 0)
  {
      l_rc = -1;
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if ((n_params != 7) && (n_params != 9) && (n_params != nargs_f2multi))
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        range_from = param[3].data.d_int32;  /* frame nr to start */	
        range_to   = param[4].data.d_int32;  /* frame nr to stop  */	
        mode       = param[5].data.d_int32;  /* { expand-as-necessary(0), CLIP-TO_IMG(1), CLIP-TO-BG-LAYER(2), FLATTEN(3)} */
        nr         = param[6].data.d_int32;  /* { BG_NOT_VISIBLE (0), BG_VISIBLE(1)} */
        /* old interface: use default values for framerate and frame_basename */
        framerate = 50;
        strcpy(frame_basename, "frame");
	
	sel_mode = MTCH_ALL_VISIBLE;
	sel_invert = FALSE;
	sel_case   = TRUE;

        if ( n_params >= 9 )
        {
           framerate  = param[7].data.d_int32;
           if(param[8].data.d_string != NULL)
           {
              strncpy(frame_basename, param[8].data.d_string, FRAME_BASENAME_LEN -1);
              frame_basename[FRAME_BASENAME_LEN -1] = '\0';
           }
        }
        if ( n_params >= 13 )
        {
          sel_mode   = param[9].data.d_int32;
          sel_case   = param[10].data.d_int32;
          sel_invert = param[11].data.d_int32;
	  if(param[12].data.d_string != NULL)
	  {
            strncpy(l_sel_str, param[12].data.d_string, sizeof(l_sel_str) -1);
	    l_sel_str[sizeof(l_sel_str) -1] = '\0';
	  }
	}

        l_rc = gap_range_to_multilayer(run_mode, image_id, range_from, range_to, mode, nr,
                                       framerate, frame_basename, FRAME_BASENAME_LEN,
				       sel_mode, sel_case, sel_invert, l_sel_str);

      }

      *nreturn_vals = 2;
      values[1].type = GIMP_PDB_IMAGE;
      values[1].data.d_int32 = l_rc;   /* return the new generated image_id */
  }
  else if (strcmp (name, "plug_in_gap_range_flatten") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_rflatt)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        range_from = param[3].data.d_int32;  /* frame nr to start */	
        range_to   = param[4].data.d_int32;  /* frame nr to stop  */	

        l_rc = gap_range_flatten(run_mode, image_id, range_from, range_to);

      }
  }
  else if (strcmp (name, "plug_in_gap_range_layer_del") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_rlayerdel)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        range_from = param[3].data.d_int32;  /* frame nr to start */	
        range_to   = param[4].data.d_int32;  /* frame nr to stop  */	
        nr         = param[5].data.d_int32;  /* layerstack position (0 == on top) */

        l_rc = gap_range_layer_del(run_mode, image_id, range_from, range_to, nr);

      }
  }
  else if ((strcmp (name, "plug_in_gap_range_convert") == 0) 
       ||  (strcmp (name, "plug_in_gap_range_convert2") == 0))
  {
      l_basename_ptr = NULL;
      l_palette_ptr = NULL;
      palette_type = GIMP_MAKE_PALETTE;
      alpha_dither = 0;
      remove_unused = 1;
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if ((n_params != 10) && (n_params != nargs_rconv) && (n_params != nargs_rconv2))
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
        else
        {
          strncpy(l_extension, param[9].data.d_string, sizeof(l_extension) -1);
          l_extension[sizeof(l_extension) -1] = '\0';
          if (n_params >= 11)
          {
            l_basename_ptr = param[10].data.d_string;
          }
          if (n_params >= 15)
          {
             l_palette_ptr = param[14].data.d_string;
	     palette_type = param[11].data.d_int32;
	     alpha_dither = param[12].data.d_int32;
	     remove_unused = param[13].data.d_int32;
          }
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        image_id    = param[1].data.d_image;
        range_from  = param[3].data.d_int32;  /* frame nr to start */	
        range_to    = param[4].data.d_int32;  /* frame nr to stop  */	
        nr          = param[5].data.d_int32;  /* flatten (0 == no , 1 == flatten) */
        dest_type   = param[6].data.d_int32;
        dest_colors = param[7].data.d_int32;
        dest_dither = param[8].data.d_int32;

        l_rc = gap_range_conv(run_mode, image_id, range_from, range_to, nr,
                              dest_type, dest_colors, dest_dither,
                              l_basename_ptr, l_extension,
			      palette_type,
			      alpha_dither,
			      remove_unused,
			      l_palette_ptr);

      }
  }
  else if (strcmp (name, "plug_in_gap_anim_resize") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_resize)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        new_width  = param[3].data.d_int32;	
        new_height = param[4].data.d_int32;	
        offs_x     = param[5].data.d_int32;
        offs_y     = param[6].data.d_int32;

        l_rc = gap_anim_sizechange(run_mode, ASIZ_RESIZE, image_id,
                                   new_width, new_height, offs_x, offs_y);

      }
  }
  else if (strcmp (name, "plug_in_gap_anim_crop") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != 7)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        new_width  = param[3].data.d_int32;	
        new_height = param[4].data.d_int32;	
        offs_x     = param[5].data.d_int32;
        offs_y     = param[6].data.d_int32;

        l_rc = gap_anim_sizechange(run_mode, ASIZ_CROP, image_id,
                                   new_width, new_height, offs_x, offs_y);

      }
  }
  else if (strcmp (name, "plug_in_gap_anim_scale") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_scale)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        new_width  = param[3].data.d_int32;	
        new_height = param[4].data.d_int32;	

        l_rc = gap_anim_sizechange(run_mode, ASIZ_SCALE, image_id,
                                   new_width, new_height, 0, 0);

      }
  }
  else if (strcmp (name, "plug_in_gap_split") == 0)
  {
      l_rc = -1;
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_split)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
        else
        {
          strncpy(l_extension, param[5].data.d_string, sizeof(l_extension) -1);
          l_extension[sizeof(l_extension) -1] = '\0';
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        inverse_order = param[3].data.d_int32;
        no_alpha      = param[4].data.d_int32;

        l_rc = gap_split_image(run_mode, image_id, 
                              inverse_order, no_alpha, l_extension);

      }
      *nreturn_vals = 2;
      values[1].type = GIMP_PDB_IMAGE;
      values[1].data.d_int32 = l_rc;   /* return the new generated image_id */
  }
  else if (strcmp (name, "plug_in_gap_shift") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_shift)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        nr       = param[3].data.d_int32;  /* how many framenumbers to shift the framesequence */
        range_from = param[4].data.d_int32;  /* frame nr to start */	
        range_to   = param[5].data.d_int32;  /* frame nr to stop  */	

        l_rc = gap_shift(run_mode, image_id, nr, range_from, range_to );

      }
  }
  else if (strcmp (name, "plug_in_gap_video_edit_copy") == 0)
  {
     if (n_params != nargs_video_copy)
     {
          status = GIMP_PDB_CALLING_ERROR;
     }

      if (status == GIMP_PDB_SUCCESS)
      {
        range_from = param[3].data.d_int32;  /* frame nr to start */	
        range_to   = param[4].data.d_int32;  /* frame nr to stop  */	

        l_rc = gap_vid_edit_copy(run_mode, image_id, range_from, range_to );
      }
  }
  else if (strcmp (name, "plug_in_gap_video_edit_paste") == 0)
  {
      if (n_params != nargs_video_paste)
      {
        status = GIMP_PDB_CALLING_ERROR;
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        nr       = param[3].data.d_int32;  /* paste_mode (0,1 or 2) */

        l_rc = gap_vid_edit_paste(run_mode, image_id, nr );
      }
  }
  else if (strcmp (name, "plug_in_gap_video_edit_clear") == 0)
  {
      if (status == GIMP_PDB_SUCCESS)
      {
        if(p_vid_edit_clear() < 0)
	{
          l_rc = -1;
	}
	else
	{
          l_rc = 0;
	}
      }
  }
  else if (strcmp (name, "plug_in_gap_modify") == 0)
  {
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_modify)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
	else
	{
	  if(param[9].data.d_string != NULL)
	  {
            strncpy(l_sel_str, param[9].data.d_string, sizeof(l_sel_str) -1);
	    l_sel_str[sizeof(l_sel_str) -1] = '\0';
	  }
	  if(param[10].data.d_string != NULL)
	  {
            strncpy(l_layername, param[10].data.d_string, sizeof(l_layername) -1);
	    l_layername[sizeof(l_layername) -1] = '\0';
	  }
	}
      }

      if (status == GIMP_PDB_SUCCESS)
      {
        range_from = param[3].data.d_int32;  /* frame nr to start */	
        range_to   = param[4].data.d_int32;  /* frame nr to stop  */	
        nr         = param[5].data.d_int32;    /* action_nr */
        sel_mode   = param[6].data.d_int32;
        sel_case   = param[7].data.d_int32;
        sel_invert = param[8].data.d_int32;

        l_rc = gap_mod_layer(run_mode, image_id, range_from, range_to,
                             nr, sel_mode, sel_case, sel_invert,
                             l_sel_str, l_layername);

      }
  }

  /* ---------- return handling --------- */

 if(l_rc < 0)
 {
    status = GIMP_PDB_EXECUTION_ERROR;
 }
 
  
 if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush();

  values[0].data.d_status = status;

  /* remove LOCK on this image for all gap_plugins */
  p_gap_lock_remove(lock_image_id);
}
