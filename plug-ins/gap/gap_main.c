/* gap_main.c
 * 1997.11.05 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Package
 *
 * This Module contains:
 * - MAIN of all GAP_Plugins
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

static char *gap_main_version =  "1.1.11a; 1999/11/16";

/* revision history:
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
#include "gap_mpege.h"
#include "gap_mod_layer.h"
#include "gap_arr_dialog.h"

/* ------------------------
 * global gap DEBUG switch
 * ------------------------
 */

/* int gap_debug = 1; */    /* print debug infos */
/* int gap_debug = 0; */    /* 0: dont print debug infos */

int gap_debug = 0;



static void query(void);
static void run(char *name, int nparam, GParam *param,
                int *nretvals, GParam **retvals);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};

MAIN ()

static void
query ()
{
  static GParamDef args_std[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
  };
  static int nargs_std = sizeof(args_std) / sizeof(args_std[0]);

  static GParamDef args_goto[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "nr", "frame nr where to go"},
  };
  static int nargs_goto = sizeof(args_goto) / sizeof(args_goto[0]);

  static GParamDef args_del[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "nr", "number of frames to delete (delete starts at current frame)"},
  };
  static int nargs_del = sizeof(args_del) / sizeof(args_del[0]);


  static GParamDef args_dup[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "nr", "how often to copy current frame"},
    {PARAM_INT32, "range_from", "frame nr to start"},
    {PARAM_INT32, "range_to", "frame nr to stop (can be lower than range_from)"},
  };
  static int nargs_dup = sizeof(args_dup) / sizeof(args_dup[0]);

  static GParamDef args_exchg[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "nr", "nr of frame to exchange with current frame"},
  };
  static int nargs_exchg = sizeof(args_exchg) / sizeof(args_exchg[0]);

  static GParamDef args_mov[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
  };
  static int nargs_mov = sizeof(args_std) / sizeof(args_mov[0]);


  static GParamDef args_f2multi[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "range_from", "frame nr to start"},
    {PARAM_INT32, "range_to", "frame nr to stop (can be lower than range_from)"},
    {PARAM_INT32, "flatten_mode", "{ expand-as-necessary(0), CLIP-TO_IMG(1), CLIP-TO-BG-LAYER(2), FLATTEN(3) }"},
    {PARAM_INT32, "bg_visible", "{ BG_NOT_VISIBLE (0), BG_VISIBLE(1) }"},
    {PARAM_INT32, "framerate", "frame delaytime in ms"},
    {PARAM_STRING, "frame_basename", "basename for all generated layers"},
    {PARAM_INT32, "select_mode", "Mode how to identify a layer: 0-3 by layername 0=equal, 1=prefix, 2=suffix, 3=contains, 4=layerstack_numberslist, 5=inv_layerstack, 6=all_visible"},
    {PARAM_INT32, "select_case", "0: ignore case 1: select_string is case sensitive"},
    {PARAM_INT32, "select_invert", "0: select normal 1: invert (select all unselected layers)"},
    {PARAM_STRING, "select_string", "string to match with layername (how to match is defined by select_mode)"},
  };
  static int nargs_f2multi = sizeof(args_f2multi) / sizeof(args_f2multi[0]);

  static GParamDef return_f2multi[] =
  {
    { PARAM_IMAGE, "new_image", "Output image" }
  };
  static int nreturn_f2multi = sizeof(return_f2multi) / sizeof(return_f2multi[0]);




  static GParamDef args_rflatt[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "range_from", "frame nr to start"},
    {PARAM_INT32, "range_to", "frame nr to stop (can be lower than range_from)"},
  };
  static int nargs_rflatt = sizeof(args_rflatt) / sizeof(args_rflatt[0]);

  static GParamDef args_rlayerdel[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "range_from", "frame nr to start"},
    {PARAM_INT32, "range_to", "frame nr to stop (can be lower than range_from)"},
    {PARAM_INT32, "nr", "layerstack position (0 == on top)"},
  };
  static int nargs_rlayerdel = sizeof(args_rlayerdel) / sizeof(args_rlayerdel[0]);


  static GParamDef args_rconv[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "range_from", "frame nr to start"},
    {PARAM_INT32, "range_to", "frame nr to stop (can be lower than range_from)"},
    {PARAM_INT32, "flatten", "0 .. dont flatten image before save"},
    {PARAM_INT32, "dest_type", "0=RGB, 1=GRAY, 2=INDEXED"},
    {PARAM_INT32, "dest_colors", "1 upto 256 (used only for dest_type INDEXED)"},
    {PARAM_INT32, "dest_dither", "0=no, 1=floyd-steinberg  2=fs/low-bleed, 3=fixed (used only for dest_type INDEXED)"},
    {PARAM_STRING, "extension", "extension for the destination filetype (jpg, tif ...or any other gimp supported type)"},
    {PARAM_STRING, "basename", "(optional parameter) here you may specify the basename of the destination frames \"/my_dir/myframe\"  _0001.ext is added)"},
  };
  static int nargs_rconv = sizeof(args_rconv) / sizeof(args_rconv[0]);


  static GParamDef args_rconv2[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "range_from", "frame nr to start"},
    {PARAM_INT32, "range_to", "frame nr to stop (can be lower than range_from)"},
    {PARAM_INT32, "flatten", "0 .. dont flatten image before save"},
    {PARAM_INT32, "dest_type", "0=RGB, 1=GRAY, 2=INDEXED"},
    {PARAM_INT32, "dest_colors", "1 upto 256 (used only for dest_type INDEXED)"},
    {PARAM_INT32, "dest_dither", "0=no, 1=floyd-steinberg 2=fs/low-bleed, 3=fixed(used only for dest_type INDEXED)"},
    {PARAM_STRING, "extension", "extension for the destination filetype (jpg, tif ...or any other gimp supported type)"},
    {PARAM_STRING, "basename", "(optional parameter) here you may specify the basename of the destination frames \"/my_dir/myframe\"  _0001.ext is added)"},
    {PARAM_INT32,  "palette_type", "0 == MAKE_PALETTE, 2 == WEB_PALETTE, 3 == MONO_PALETTE (bw) 4 == CUSTOM_PALETTE (used only for dest_type INDEXED)"},
    {PARAM_INT32,  "alpha_dither", "dither transparency to fake partial opacity (used only for dest_type INDEXED)"},
    {PARAM_INT32,  "remove_unused", "remove unused or double colors from final palette (used only for dest_type INDEXED)"},
    {PARAM_STRING, "palette", "name of the cutom palette to use (used only for dest_type INDEXED and palette_type == CUSTOM_PALETTE) "},
  };
  static int nargs_rconv2 = sizeof(args_rconv2) / sizeof(args_rconv2[0]);

  /* resize and crop share the same parameters */
  static GParamDef args_resize[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "new_width", "width of the resulting  anim_frame images in pixels"},
    {PARAM_INT32, "new_height", "height of the resulting  anim_frame images in pixels"},
    {PARAM_INT32, "offset_x", "X offset in pixels"},
    {PARAM_INT32, "offset_y", "Y offset in pixels"},
  };
  static int nargs_resize = sizeof(args_resize) / sizeof(args_resize[0]);

  static GParamDef args_scale[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "new_width", "width of the resulting  anim_frame images in pixels"},
    {PARAM_INT32, "new_height", "height of the resulting  anim_frame images in pixels"},
  };
  static int nargs_scale = sizeof(args_scale) / sizeof(args_scale[0]);


  static GParamDef args_split[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (NO Anim Frame allowed)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "inverse_order", "True/False"},
    {PARAM_INT32, "no_alpha", "True: remove alpha channel(s) in the destination frames"},
    {PARAM_STRING, "extension", "extension for the destination filetype (jpg, tif ...or any other gimp supported type)"},
  };
  static int nargs_split = sizeof(args_split) / sizeof(args_split[0]);

  static GParamDef return_split[] =
  {
    { PARAM_IMAGE, "new_image", "Output image (first or last resulting frame)" }
  };
  static int nreturn_split = sizeof(return_split) / sizeof(return_split[0]);


  static GParamDef args_mpege[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
  };
  static int nargs_mpege = sizeof(args_mpege) / sizeof(args_mpege[0]);

  static GParamDef *return_vals = NULL;
  static int nreturn_vals = 0;


  static GParamDef args_shift[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "nr", "how many framenumbers to shift the framesequence"},
    {PARAM_INT32, "range_from", "frame nr to start"},
    {PARAM_INT32, "range_to", "frame nr to stop"},
  };
  static int nargs_shift = sizeof(args_shift) / sizeof(args_shift[0]);

  static GParamDef args_modify[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image (current one of the Anim Frames)"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_INT32, "range_from", "frame nr to start"},
    {PARAM_INT32, "range_to", "frame nr to stop"},
    {PARAM_INT32, "action_mode", "0:set_visible, 1:set_invisible, 2:set_linked, 3:set_unlinked, 4:raise, 5:lower, 6:merge_expand, 7:merge_img, 8:merge_bg, 9:apply_filter, 10:duplicate, 11:delete, 12:rename"},
    {PARAM_INT32, "select_mode", "Mode how to identify a layer: 0-3 by layername 0=equal, 1=prefix, 2=suffix, 3=contains, 4=layerstack_numberslist, 5=inv_layerstack, 6=all_visible"},
    {PARAM_INT32, "select_case", "0: ignore case 1: select_string is case sensitive"},
    {PARAM_INT32, "select_invert", "0: select normal 1: invert (select all unselected layers)"},
    {PARAM_STRING, "select_string", "string to match with layername (how to match is defined by select_mode)"},
    {PARAM_STRING, "new_layername", "is only used at action rename. [####] is replaced by the framnumber"},
  };
  static int nargs_modify = sizeof(args_modify) / sizeof(args_modify[0]);

  INIT_I18N();

  gimp_install_procedure("plug_in_gap_next",
			 _("This plugin exchanges current image with (next nubered) image from disk."),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Goto Next"),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_std, nreturn_vals,
			 args_std, return_vals);

  gimp_install_procedure("plug_in_gap_prev",
			 _("This plugin exchanges current image with (previous nubered) image from disk."),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Goto Prev"),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_std, nreturn_vals,
			 args_std, return_vals);

  gimp_install_procedure("plug_in_gap_first",
			 _("This plugin exchanges current image with (lowest nubered) image from disk."),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Goto First"),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_std, nreturn_vals,
			 args_std, return_vals);

  gimp_install_procedure("plug_in_gap_last",
			 _("This plugin exchanges current image with (highest nubered) image from disk."),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Goto Last"),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_std, nreturn_vals,
			 args_std, return_vals);

  gimp_install_procedure("plug_in_gap_goto",
			 _("This plugin exchanges current image with requested image (nr) from disk."),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Goto Any"),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_goto, nreturn_vals,
			 args_goto, return_vals);

  gimp_install_procedure("plug_in_gap_del",
			 _("This plugin deletes the given number of frames from disk including the current frame."),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Delete Frames"),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_del, nreturn_vals,
			 args_del, return_vals);

  gimp_install_procedure("plug_in_gap_dup",
			 _("This plugin duplicates the current frames on disk n-times."),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Duplicate Frames..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_dup, nreturn_vals,
			 args_dup, return_vals);

  gimp_install_procedure("plug_in_gap_exchg",
			 _("This plugin exchanges content of the current with destination frame."),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Exchange Frame..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_exchg, nreturn_vals,
			 args_exchg, return_vals);

  gimp_install_procedure("plug_in_gap_move",
			 _("This plugin copies layer(s) from one sourceimage to multiple frames on disk, varying position, size and opacity."),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Move Path..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_mov, nreturn_vals,
			 args_mov, return_vals);

  gimp_install_procedure("plug_in_gap_range_to_multilayer",
			 _("This plugin creates a new image from the given range of frame-images. Each frame is converted to one layer in the new image, according to flatten_mode. (the frames on disk are not changed)."),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames to Image..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_f2multi, nreturn_f2multi,
			 args_f2multi, return_f2multi);

  gimp_install_procedure("plug_in_gap_range_flatten",
			 _("This plugin flattens the given range of frame-images (on disk)"),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames Flatten..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_rflatt, nreturn_vals,
			 args_rflatt, return_vals);

  gimp_install_procedure("plug_in_gap_range_layer_del",
			 _("This plugin deletes one layer in the given range of frame-images (on disk). exception: the last remaining layer of a frame is not deleted"),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames LayerDel..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_rlayerdel, nreturn_vals,
			 args_rlayerdel, return_vals);

  gimp_install_procedure("plug_in_gap_range_convert",
			 _("This plugin converts the given range of frame-images to other fileformats (on disk) depending on extension"),
			 "WARNING this procedure is obsolete, please use plug_in_gap_range_convert2",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 NULL,                      /* do not appear in menus */
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_rconv, nreturn_vals,
			 args_rconv, return_vals);

  gimp_install_procedure("plug_in_gap_range_convert2",
			 _("This plugin converts the given range of frame-images to other fileformats (on disk) depending on extension"),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames Convert..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_rconv2, nreturn_vals,
			 args_rconv2, return_vals);

  gimp_install_procedure("plug_in_gap_anim_resize",
			 _("This plugin resizes all anim_frames (images on disk) to the given new_width/new_height"),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames Resize..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_resize, nreturn_vals,
			 args_resize, return_vals);

  gimp_install_procedure("plug_in_gap_anim_crop",
			 _("This plugin crops all anim_frames (images on disk) to the given new_width/new_height"),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames Crop..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_resize, nreturn_vals,
			 args_resize, return_vals);

  gimp_install_procedure("plug_in_gap_anim_scale",
			 _("This plugin scales all anim_frames (images on disk) to the given new_width/new_height"),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames Scale..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_scale, nreturn_vals,
			 args_scale, return_vals);

  gimp_install_procedure("plug_in_gap_split",
			 _("This plugin splits the current image to anim frames (images on disk). Each layer is saved as one frame"),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Split Img to Frames..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_split, nreturn_split,
			 args_split, return_split);

  gimp_install_procedure("plug_in_gap_mpeg_encode",
			 _("This plugin calls mpeg_encode to convert anim frames to MPEG1, or just generates a param file for mpeg_encode. (mpeg_encode must be installed on your system)"),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Encode/MPEG1..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_mpege, nreturn_vals,
			 args_mpege, return_vals);


  gimp_install_procedure("plug_in_gap_mpeg2encode",
			 _("This plugin calls mpeg2encode to convert anim frames to MPEG1 or MPEG2, or just generates a param file for mpeg2encode. (mpeg2encode must be installed on your system)"),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Encode/MPEG2 mpeg2encode...)"),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_mpege, nreturn_vals,
			 args_mpege, return_vals);


  gimp_install_procedure("plug_in_gap_shift",
			 _("This plugin exchanges frame numbers in the given range. (discfile frame_0001.xcf is renamed to frame_0002.xcf, 2->3, 3->4 ... n->1)"),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Framesequence Shift..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_shift, nreturn_vals,
			 args_shift, return_vals);

  gimp_install_procedure("plug_in_gap_modify",
			 _("This plugin performs a modifying action on each selected layer in each selected framerange"),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_main_version,
			 N_("<Image>/Video/Frames Modify..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_modify, nreturn_vals,
			 args_modify, return_vals);
}	/* end query */



static void
run (char    *name,
     int      n_params,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  typedef struct
  {
    long   lock;        /* 0 ... NOT Locked, 1 ... locked */
    gint32 image_id;
    long   timestamp;   /* locktime not used for now */
  } t_lockdata;

  t_lockdata  l_lock;
  static char l_lockname[50];
  char       *l_env;
  
  char        l_extension[32];
  char        l_sel_str[MAX_LAYERNAME];
  char        l_layername[MAX_LAYERNAME];
  char       *l_basename_ptr;
  char       *l_palette_ptr;
  static GParam values[2];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gint32     image_id;
  gint32     nr;
  long       range_from, range_to;
  GImageType dest_type;
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


  if(gap_debug) fprintf(stderr, "\n\ngap_main: debug name = %s\n", name);
  
  image_id = param[1].data.d_image;
  
  /* check for locks */
  l_lock.lock = 0;
  sprintf(l_lockname, "plug_in_gap_plugins_LOCK_%d", (int)image_id);
  gimp_get_data (l_lockname, &l_lock);

  if((l_lock.lock != 0) && (l_lock.image_id == image_id))
  {
       fprintf(stderr, "gap_plugin is LOCKED for Image ID=%s\n", l_lockname);

       status = STATUS_EXECUTION_ERROR;
       values[0].type = PARAM_STATUS;
       values[0].data.d_status = status;
       return ;
  }


  /* set LOCK on current image (for all gap_plugins) */
  l_lock.lock = 1;
  l_lock.image_id = image_id;
  gimp_set_data (l_lockname, &l_lock, sizeof(l_lock));
   
  if (run_mode == RUN_NONINTERACTIVE) {
    INIT_I18N();
  } else {
    INIT_I18N_UI();
  }

  if (strcmp (name, "plug_in_gap_next") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 3)
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;

        l_rc = gap_next(run_mode, image_id);

      }
  }
  else if (strcmp (name, "plug_in_gap_prev") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 3)
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;

        l_rc = gap_prev(run_mode, image_id);

      }
  }
  else if (strcmp (name, "plug_in_gap_first") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 3)
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;

        l_rc = gap_first(run_mode, image_id);

      }
  }
  else if (strcmp (name, "plug_in_gap_last") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 3)
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;

        l_rc = gap_last(run_mode, image_id);

      }
  }
  else if (strcmp (name, "plug_in_gap_goto") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 4)
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;
        nr       = param[3].data.d_int32;  /* destination frame nr */

        l_rc = gap_goto(run_mode, image_id, nr);

      }
  }
  else if (strcmp (name, "plug_in_gap_del") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 4)
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;
        nr       = param[3].data.d_int32;  /* number of frames to delete */

        l_rc = gap_del(run_mode, image_id, nr);

      }
  }
  else if (strcmp (name, "plug_in_gap_dup") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        /* accept the old interface with 4 parameters */
        if ((n_params != 4) && (n_params != 6))
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;
        nr       = param[3].data.d_int32;  /* how often to copy current frame */
        if (n_params != 6)
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
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 4)
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;
        nr       = param[3].data.d_int32;  /* nr of frame to exchange with current frame */

        l_rc = gap_exchg(run_mode, image_id, nr);

      }
  }
  else if (strcmp (name, "plug_in_gap_move") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        /* if (n_params != ?? ) */
          status = STATUS_CALLING_ERROR;
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;

        l_rc = gap_move(run_mode, image_id);

      }
  }
  else if (strcmp (name, "plug_in_gap_range_to_multilayer") == 0)
  {
      l_rc = -1;
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if ((n_params != 7) && (n_params != 9) && (n_params != 13))
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;
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
      values[1].type = PARAM_IMAGE;
      values[1].data.d_int32 = l_rc;   /* return the new generated image_id */
  }
  else if (strcmp (name, "plug_in_gap_range_flatten") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 5)
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;
        range_from = param[3].data.d_int32;  /* frame nr to start */	
        range_to   = param[4].data.d_int32;  /* frame nr to stop  */	

        l_rc = gap_range_flatten(run_mode, image_id, range_from, range_to);

      }
  }
  else if (strcmp (name, "plug_in_gap_range_layer_del") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 6)
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;
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
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if ((n_params != 10) && (n_params != 11) && (n_params != 15))
        {
          status = STATUS_CALLING_ERROR;
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

      if (status == STATUS_SUCCESS)
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
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 7)
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;
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
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 7)
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;
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
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 5)
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;
        new_width  = param[3].data.d_int32;	
        new_height = param[4].data.d_int32;	

        l_rc = gap_anim_sizechange(run_mode, ASIZ_SCALE, image_id,
                                   new_width, new_height, 0, 0);

      }
  }
  else if (strcmp (name, "plug_in_gap_split") == 0)
  {
      l_rc = -1;
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 6)
        {
          status = STATUS_CALLING_ERROR;
        }
        else
        {
          strncpy(l_extension, param[5].data.d_string, sizeof(l_extension) -1);
          l_extension[sizeof(l_extension) -1] = '\0';
        }
      }

      if (status == STATUS_SUCCESS)
      {
        image_id      = param[1].data.d_image;
        inverse_order = param[3].data.d_int32;
        no_alpha      = param[4].data.d_int32;

        l_rc = gap_split_image(run_mode, image_id, 
                              inverse_order, no_alpha, l_extension);

      }
      *nreturn_vals = 2;
      values[1].type = PARAM_IMAGE;
      values[1].data.d_int32 = l_rc;   /* return the new generated image_id */
  }
  else if (strcmp (name, "plug_in_gap_mpeg_encode") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 3)
        {
          status = STATUS_CALLING_ERROR;
        }
        else
        {
          /* planed: define non interactive PARAMS */
          l_extension[sizeof(l_extension) -1] = '\0';
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id    = param[1].data.d_image;
        /* planed: define non interactive PARAMS */

        l_rc = gap_mpeg_encode(run_mode, image_id, MPEG_ENCODE /* more PARAMS */);

      }
  }
  else if (strcmp (name, "plug_in_gap_mpeg2encode") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 3)
        {
          status = STATUS_CALLING_ERROR;
        }
        else
        {
          /* planed: define non interactive PARAMS */
          l_extension[sizeof(l_extension) -1] = '\0';
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id    = param[1].data.d_image;
        /* planed: define non interactive PARAMS */

        l_rc = gap_mpeg_encode(run_mode, image_id, MPEG2ENCODE /* more PARAMS */);

      }
  }
  else if (strcmp (name, "plug_in_gap_shift") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 6)
        {
          status = STATUS_CALLING_ERROR;
        }
      }

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;
        nr       = param[3].data.d_int32;  /* how many framenumbers to shift the framesequence */
        range_from = param[4].data.d_int32;  /* frame nr to start */	
        range_to   = param[5].data.d_int32;  /* frame nr to stop  */	

        l_rc = gap_shift(run_mode, image_id, nr, range_from, range_to );

      }
  }
  else if (strcmp (name, "plug_in_gap_modify") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 11)
        {
          status = STATUS_CALLING_ERROR;
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

      if (status == STATUS_SUCCESS)
      {

        image_id = param[1].data.d_image;
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
    status = STATUS_EXECUTION_ERROR;
 }
 
  
 if (run_mode != RUN_NONINTERACTIVE)
    gimp_displays_flush();

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  /* remove LOCK on this image for all gap_plugins */
  l_lock.lock = 0;
  l_lock.image_id = -1;
  gimp_set_data (l_lockname, &l_lock, sizeof(l_lock));

}
