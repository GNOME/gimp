/* gap_mpege.c
 * 1998.07.04 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 * - gap_mpeg_encode
 *        GIMP/GAP-frontend interfaces for 2 MPEG encoder Programs
 *
 *        1) mpeg_encode 1.5 
 *             the free Berkeley MPEG-1 encoder
 *        2) mpeg2encode 
 *             MPEG-2 and MPEG-1 Encoder / Decoder, Version 1.2
 *             (MPEG Software Simulation Group)
 *                 Web:      http://www.mpeg.org/MSSG/
 *                 FTP:      ftp://ftp.mpeg.org/pub/mpeg/mssg/
 *                 E-mail:   mssg@mpeg.org  (author contact)
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

/* revision history
 * 1.1.11b; 1999/11/20   hof: Changed menunames AnimFrames to Video in menu hints
 * 1.1.8a;  1999/08/31   hof: accept anim framenames without underscore '_'
 * 0.99.00; 1999/03/15   hof: prepared for win/dos filename conventions
 * 0.96.00; 1998/07/08   hof: first release
 */
 
/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "config.h"
#include "libgimp/stdplugins-intl.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_lib.h"
#include "gap_arr_dialog.h"
#include "gap_mpege.h"

extern      int gap_debug; /* ==0  ... dont print debug infos */

#define MBUF_SIZE 128

typedef struct t_mpg_par
{
  gint      from;
  gint      to;

  gint      bitrate;

  char      *outfile;
  char      *parfile;
  char      *startscript;
  char      *ext;
  
  /* members used in mpeg_encode only: */
  gint      const_bitrate;
  gint      iqscale;
  gint      pqscale;
  gint      bqscale;
  char      *pattern;
  char      *framerate;
  char      *psearch;
  char      *bsearch;

  /* members used in mpeg2encode only: */
  gint      frate;
  gint      videoformat;
  gint      mpegtype;
} t_mpg_par;


/* ============================================================================
 * p_mpege_info
 * ============================================================================
 */
static
int p_mpege_info(t_anim_info *ainfo_ptr, char *errlist, t_gap_mpeg_encoder encoder)
{
  t_arr_arg  argv[13];
  t_but_arg  b_argv[2];

  int        l_idx;
  int        l_rc;
  

  l_idx = 0;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("Conditions to run mpeg_encode 1.5:");
  if(encoder == MPEG2ENCODE)
  {
    argv[l_idx].label_txt = _("Conditions to run mpeg2encode 1.2:");
  }
  

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "";

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("1.) mpeg_encode 1.5 must be installed");
  if(encoder == MPEG2ENCODE)
  {
    argv[l_idx].label_txt = _("1.) mpeg2encode 1.2 must be installed");
  }

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("    you can get mpeg_encode at");
  if(encoder == MPEG2ENCODE)
  {
    argv[l_idx].label_txt = _("    you can get mpeg2encode at http://www.mpeg.org/MSSG");
  }

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("    ftp://mm-ftp.cs.berkeley.edu/pub/multimedia/mpeg/bmt1r1.tar.gz");
  if(encoder == MPEG2ENCODE)
  {
    argv[l_idx].label_txt = _("   or at ftp://ftp.mpeg.org/pub/mpeg/mssg ");
  }

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("2.) You need a series of single Images on disk (AnimFrames)");

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("    all with fileformat JPEG (or YUV or PNM or PPM)");
  if(encoder == MPEG2ENCODE)
  {
    argv[l_idx].label_txt = _("   all with fileformat PPM (or YUV)");
  }

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("    (use 'Frames Convert' from the Video Menu");

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("    or 'Split Img to Frames' from the Video Menu)");

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("3.) All Images must have the same size,");

  if(encoder == MPEG_ENCODE)
  {

     l_idx++;
     p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
     argv[l_idx].label_txt = _("    width and height must be a multiple of 16");

     l_idx++;
     p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
     argv[l_idx].label_txt = _("    (use Scale or Crop from the Video Menu)");
  }

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = errlist;

  l_idx++;

  /* the  Action Button */
    b_argv[0].but_txt  = _("Cancel");
    b_argv[0].but_val  = -1;
    b_argv[1].but_txt  = _("OK");
    b_argv[1].but_val  = 0;
  
  l_rc = p_array_std_dialog(_("MPEG_ENCODE Information"),
                             "",
                              l_idx,   argv,      /* widget array */
                              2,       b_argv,    /* button array */
                              -1);

  return (l_rc); 
}	/* end p_mpege_info */


/* ============================================================================
 * p_mpege_dialog
 *   retcode    -1   ... on cancel
 *               0   ... Generate Paramfile
 *               1   ... Generate Paramfile and start mpeg_encode
 * ============================================================================
 */
static
int p_mpege_dialog(t_anim_info *ainfo_ptr, t_mpg_par *mp_ptr, t_gap_mpeg_encoder encoder)
{
  static t_arr_arg  argv[15];
  static t_but_arg  b_argv[3];
  gint   l_rc;
  gint   l_idx;
  char  *l_str;
  static int gettextize_loop = 0;

  static char   l_startscript[MBUF_SIZE];
  static char   l_parfile[MBUF_SIZE];
  static char   l_outfile[MBUF_SIZE];
  static char   l_pattern[MBUF_SIZE];

  static char *frate_args[8]  = {"23.976", "24", "25", "29.97", "30", "50", "59.94", "60" };
  static char *psearch_args[3]  = {"EXHAUSTIVE", "SUBSAMPLE", "LOGARITHMIC" };
  static char *bsearch_args[3]  = {"SIMPLE", "CROSS2", "EXHAUSTIVE" };
  static char *video_args[5]    = {"Comp", "PAL", "NTSC", "SECAM", "MAC" };
  static char *mpeg_args[2]    = {"MPEG1", "MPEG2" };
  static char *mpeg_help[2]    = { N_("generate MPEG1 (ISO/IEC 11172-2) stream"),
                                  N_("generate MPEG2 (ISO/IEC DIS 13818-2) stream") };

  for (;gettextize_loop < 2; gettextize_loop++)
    mpeg_help[gettextize_loop] = gettext(mpeg_help[gettextize_loop]);

  l_rc = -1;
  
  /* the 3 Action Buttons */
    b_argv[0].but_txt  = _("Cancel");
    b_argv[0].but_val  = -1;
    b_argv[1].but_txt  = _("GenParams");
    b_argv[1].but_val  = 0;
    b_argv[2].but_txt  = _("Gen + Encode");
    b_argv[2].but_val  = 1;

  l_str = p_strdup_del_underscore(ainfo_ptr->basename);
  g_snprintf (l_outfile, MBUF_SIZE, "%s.mpg", l_str);
  g_snprintf (l_parfile, MBUF_SIZE, "%s.par_mpg", l_str);
  g_snprintf (l_startscript, MBUF_SIZE, "%s.sh", l_str);
  g_free(l_str);

  p_init_arr_arg(&argv[0], WGT_LABEL);
  argv[0].label_txt = "";

  p_init_arr_arg(&argv[1], WGT_INT_PAIR);
  argv[1].constraint = TRUE;
  argv[1].label_txt = _("From Frame:");
  argv[1].help_txt  = _("first handled frame");
  argv[1].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[1].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[1].int_ret   = (gint)ainfo_ptr->curr_frame_nr;

  p_init_arr_arg(&argv[2], WGT_INT_PAIR);
  argv[2].constraint = TRUE;
  argv[2].label_txt = _("To   Frame:");
  argv[2].help_txt  = _("last handled frame");
  argv[2].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[2].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[2].int_ret   = (gint)ainfo_ptr->last_frame_nr;

  p_init_arr_arg(&argv[3], WGT_OPTIONMENU);
  argv[3].label_txt = _("Framerate :");
  argv[3].help_txt  = _("framerate in frames/second");
  argv[3].radio_argc = 8;
  argv[3].radio_argv = frate_args;
  argv[3].radio_ret  = 4;

  p_init_arr_arg(&argv[4], WGT_INT_PAIR);
  argv[4].constraint = FALSE;
  argv[4].label_txt = _("Bitrate:");
  argv[4].help_txt  = _("used for constant bitrates (bit/sec)                  \n(low rate gives good compression + bad quality)");
  argv[4].int_min   = 500000;
  argv[4].int_step  = 100000;
  argv[4].int_max   = 9000000;
  argv[4].int_ret   = 3000000;
  argv[4].umin      = 100;
  argv[4].entry_width = 80;
  argv[4].pagestep  = 1000000;

  if(encoder == MPEG_ENCODE) l_idx = 12;
  else                       l_idx = 7;
  

  p_init_arr_arg(&argv[l_idx], WGT_FILESEL);
  argv[l_idx].label_txt = _("Outputfile:");
  argv[l_idx].entry_width = 250;       /* pixel */
  argv[l_idx].help_txt  = _("Name of the resulting MPEG outputfile");
  argv[l_idx].text_buf_len = sizeof(l_outfile);
  argv[l_idx].text_buf_ret = &l_outfile[0];

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_FILESEL);
  argv[l_idx].label_txt = _("Paramfile:");
  argv[l_idx].entry_width = 250;       /* pixel */
  argv[l_idx].help_txt  = _("Name of the Encoder-Parameterfile\n(is generated)");
  argv[l_idx].text_buf_len = sizeof(l_parfile);
  argv[l_idx].text_buf_ret = &l_parfile[0];

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_FILESEL);
  argv[l_idx].label_txt = _("Startscript:");
  argv[l_idx].entry_width = 250;       /* pixel */
  argv[l_idx].help_txt  = _("Name of the Startscript           \n(is generated/executed)");
  argv[l_idx].text_buf_len = sizeof(l_startscript);
  argv[l_idx].text_buf_ret = &l_startscript[0];

  if(encoder == MPEG_ENCODE)
  {
     argv[0].label_txt = _("Generate parameterfile for mpeg_encode 1.5\n(the freely distributed Berkeley MPEG-1 Video  Encoder.)\n");


     p_init_arr_arg(&argv[5], WGT_TOGGLE);
     argv[5].label_txt = _("Constant Bitrate :");
     argv[5].help_txt  = _("Iqnore I/P/QSCALE values and use constant bit-rate)");
     argv[5].int_ret   = 1;

     g_snprintf (l_pattern, MBUF_SIZE, "IBBPBBPBBPBBPBBP");
     p_init_arr_arg(&argv[6], WGT_TEXT);
     argv[6].label_txt = _("Pattern:");
     argv[6].entry_width = 140;       /* pixel */
     argv[6].help_txt  = _("How to encode MPEG framesequence (I/P/B frames)");
     argv[6].text_buf_len = sizeof(l_pattern);
     argv[6].text_buf_ret = &l_pattern[0];

     p_init_arr_arg(&argv[7], WGT_INT_PAIR);
     argv[7].constraint = TRUE;
     argv[7].label_txt = _("IQSCALE:");
     argv[7].help_txt  = _("Quality scale for I-Frames                       \n(1 = best quality, 31 = best comression)");
     argv[7].int_min   = 1;
     argv[7].int_max   = 31;
     argv[7].int_ret   = 2;

     p_init_arr_arg(&argv[8], WGT_INT_PAIR);
     argv[8].constraint = TRUE;
     argv[8].label_txt = _("PQSCALE:");
     argv[8].help_txt  = _("Quality scale for P-Frames                  \n(1 = best quality, 31 = best comression)");
     argv[8].int_min   = 1;
     argv[8].int_max   = 31;
     argv[8].int_ret   = 5;

     p_init_arr_arg(&argv[9], WGT_INT_PAIR);
     argv[9].constraint = TRUE;
     argv[9].label_txt = _("BQSCALE:");
     argv[9].help_txt  = _("Quality scale for B-Frames                  \n(1 = best quality, 31 = best comression)");
     argv[9].int_min   = 1;
     argv[9].int_max   = 31;
     argv[9].int_ret   = 9;


     p_init_arr_arg(&argv[10], WGT_OPTIONMENU);
     argv[10].label_txt = _("P-Search :");
     argv[10].help_txt  = _("Search Algorithmus used for P-frames");
     argv[10].radio_argc = 3;
     argv[10].radio_argv = psearch_args;
     argv[10].radio_ret  = 1;

     p_init_arr_arg(&argv[11], WGT_OPTIONMENU);
     argv[11].label_txt = _("B-Search :");
     argv[11].help_txt  = _("Search Algorithmus used for B-frames");
     argv[11].radio_argc = 3;
     argv[11].radio_argv = bsearch_args;
     argv[11].radio_ret  = 1;

     l_rc =  p_array_std_dialog( _("Gen MPEG_ENCODE Parameters"),
                                _("Encode Values"),
                                 15,   argv,      /* widget array */
                                 3,    b_argv,    /* button array */
                                 0);

     mp_ptr->const_bitrate   = argv[5].int_ret;
     mp_ptr->iqscale         = argv[7].int_ret;
     mp_ptr->pqscale         = argv[8].int_ret;
     mp_ptr->bqscale         = argv[9].int_ret;
     mp_ptr->psearch       = psearch_args[argv[10].radio_ret];
     mp_ptr->bsearch       = bsearch_args[argv[11].radio_ret];
  }
  
  
  if(encoder == MPEG2ENCODE)
  {
     argv[0].label_txt = _("Generate parameterfile for mpeg2encode 1.2\n(MPEG-2 Video  Encoder.)\n");

     p_init_arr_arg(&argv[5], WGT_RADIO);
     argv[5].label_txt = _("MPEG-type :");
     argv[5].radio_argc = 2;
     argv[5].radio_argv = mpeg_args;
     argv[5].radio_help_argv = mpeg_help;
     argv[5].radio_ret  = 1;



     p_init_arr_arg(&argv[6], WGT_OPTIONMENU);
     argv[6].label_txt = _("Videoformat :");
     argv[6].help_txt  = _("Videoformat");
     argv[6].radio_argc = 5;
     argv[6].radio_argv = video_args;
     argv[6].radio_ret  = 1;


     argv[3].radio_argc = 5; /* framerates above 30 ar not allowed in mpeg2encode */

     l_rc =  p_array_std_dialog( _("Gen MPEG2ENCODE Parameters"),
                                 _("Encode Values"),
                                 10,   argv,      /* widget array */
                                 3,    b_argv,    /* button array */
                                 0);
     mp_ptr->mpegtype    = argv[5].radio_ret;
     mp_ptr->videoformat = argv[6].radio_ret;

  }

  if(argv[1].int_ret <= argv[2].int_ret)
  {
    mp_ptr->from          = argv[1].int_ret;
    mp_ptr->to            = argv[2].int_ret;
  }
  else
  {
    mp_ptr->from          = argv[2].int_ret;
    mp_ptr->to            = argv[1].int_ret;
  }
  
  mp_ptr->frate         = argv[3].radio_ret +1;
  mp_ptr->framerate     = frate_args[argv[3].radio_ret];
  mp_ptr->bitrate       = argv[4].int_ret;


  mp_ptr->pattern       = &l_pattern[0];
  mp_ptr->outfile       = &l_outfile[0];
  mp_ptr->parfile       = &l_parfile[0];
  mp_ptr->startscript   = &l_startscript[0];
  
  return (l_rc);   
}	/* end p_mpege_dialog */

/* ============================================================================
 * p_mpege_extension_check
 * ============================================================================
 */

static
char *p_mpege_extension_check(t_anim_info *ainfo_ptr)
{
  /* list of direct supported fileformats of mpeg_encode */
  static char *base_file_formats [7]  = { "\0",  "JPEG", "PNM", "PPM", "JMOVIE", "Y" "YUV"};

  int    l_ffidx;
  l_ffidx = 0;  /* undefined */
  if(ainfo_ptr->extension != NULL)
  {
     if ( strcmp(ainfo_ptr->extension, ".jpg" ) == 0)  l_ffidx = 1;
     if ( strcmp(ainfo_ptr->extension, ".jpeg") == 0)  l_ffidx = 1;
     if ( strcmp(ainfo_ptr->extension, ".JPG" ) == 0)  l_ffidx = 1;
     if ( strcmp(ainfo_ptr->extension, ".JPEG") == 0)  l_ffidx = 1;

     if ( strcmp(ainfo_ptr->extension, ".pnm" ) == 0)  l_ffidx = 2;
     if ( strcmp(ainfo_ptr->extension, ".PNM" ) == 0)  l_ffidx = 2;

     if ( strcmp(ainfo_ptr->extension, ".ppm" ) == 0)  l_ffidx = 3;
     if ( strcmp(ainfo_ptr->extension, ".PPM" ) == 0)  l_ffidx = 3;

     if ( strcmp(ainfo_ptr->extension, ".yuv" ) == 0)  l_ffidx = 6;
     if ( strcmp(ainfo_ptr->extension, ".YUV" ) == 0)  l_ffidx = 6;
  }

  return(base_file_formats[l_ffidx]);
}	/* end p_mpege_extension_check */

static
int p_mpeg2_extension_check(t_anim_info *ainfo_ptr)
{
  int    l_ffidx;
  l_ffidx = -1;  /* format not supported */

  if(ainfo_ptr->extension != NULL)
  {
     if ( strcmp(ainfo_ptr->extension, ".ppm" ) == 0)  l_ffidx = 2;
     if ( strcmp(ainfo_ptr->extension, ".PPM" ) == 0)  l_ffidx = 2;

     if ( strcmp(ainfo_ptr->extension, ".yuv" ) == 0)  l_ffidx = 1;
     if ( strcmp(ainfo_ptr->extension, ".YUV" ) == 0)  l_ffidx = 1;
  }
  
  return(l_ffidx);
}	/* end p_mpege_extension_check */


/* ============================================================================
 * p_mpeg2encode_gen_parfile
 * ============================================================================
 */
static
int p_mpeg2encode_gen_parfile(t_anim_info *ainfo_ptr, t_mpg_par *mp_ptr)
{
  FILE *l_fp;
  int    l_idx;
  int    l_base_ffidx;
  gint   l_width;
  gint   l_height;
  char  *l_dirname_ptr;
  char  *l_basename_ptr;
  char   l_basename_buff[1024];
  
  l_fp = fopen(mp_ptr->parfile, "w");
  if(l_fp == NULL)
  {
     fprintf(stderr, "cant open MPEG Paramfile %s for write\n", mp_ptr->parfile);
     return -1;
  }

  l_base_ffidx = p_mpeg2_extension_check(ainfo_ptr);

  /* check if ainfo_ptr->basename contains directory part */
  strcpy(l_basename_buff, ainfo_ptr->basename);
  l_dirname_ptr = ".";
  l_basename_ptr = &l_basename_buff[0];
  
  for(l_idx = strlen(l_basename_buff) -1; l_idx >= 0; l_idx--)
  {
    if(l_basename_buff[l_idx] == G_DIR_SEPARATOR)
    {
       l_basename_buff[l_idx] = '\0';
       l_basename_ptr = &l_basename_buff[l_idx +1];
       l_dirname_ptr = &l_basename_buff[0];
       break;
    }
  }

  /* get info about the image (size is common to all frames) */
  l_width  = gimp_image_width(ainfo_ptr->image_id);
  l_height = gimp_image_height(ainfo_ptr->image_id);

  if(mp_ptr->mpegtype == 0)
  {
    fprintf(l_fp, "MPEG-1 stream %s frames/sec\n", mp_ptr->framerate);
  }
  else
  {
    fprintf(l_fp, "MPEG-2 stream %s frames/sec\n", mp_ptr->framerate);
  }
  
  fprintf(l_fp, "%s%%04d   /* name of source files */\n", ainfo_ptr->basename);

  fprintf(l_fp, "-         /* name of reconstructed images (\"-\": don't store) */\n");
  fprintf(l_fp, "-         /* name of intra quant matrix file     (\"-\": default matrix) */\n");
  fprintf(l_fp, "-         /* name of non intra quant matrix file (\"-\": default matrix) */\n");
  fprintf(l_fp, "-         /* name of statistics file (\"-\": stdout ) */\n");

  fprintf(l_fp, "%d         /* input picture file format: 0=*.Y,*.U,*.V, 1=*.yuv, 2=*.ppm */\n", l_base_ffidx);
  fprintf(l_fp, "%d       /* number of frames */\n", (int)(mp_ptr->to - mp_ptr->from)+1);
  fprintf(l_fp, "%d         /* number of first frame */\n",(int)mp_ptr->from);

  fprintf(l_fp, "00:00:00:00 /* timecode of first frame */\n");
  fprintf(l_fp, "12        /* N (# of frames in GOP) */\n");
  fprintf(l_fp, "3         /* M (I/P frame distance) */\n");

  if(mp_ptr->mpegtype == 0)
  {
    fprintf(l_fp, "1         /* ISO/IEC 11172-2 stream */\n");    /* MPEG1 */
  }
  else
  {
    fprintf(l_fp, "0         /* ISO/IEC 11172-2 stream */\n");    /* MPEG2 */
  }
  fprintf(l_fp, "0         /* 0:frame pictures, 1:field pictures */\n");

  fprintf(l_fp, "%d       /* horizontal_size */\n", (int)l_width);
  fprintf(l_fp, "%d       /* vertical_size */\n", (int)l_height);

  fprintf(l_fp, "2         /* aspect_ratio_information 1=square pel, 2=4:3, 3=16:9, 4=2.11:1 */\n");

  fprintf(l_fp, "%d         /* frame_rate_code 1=23.976, 2=24, 3=25, 4=29.97, 5=30 frames/sec. */\n", (int)mp_ptr->frate);
  fprintf(l_fp, "%d.0 /* bit_rate (bits/s) */\n", (int)mp_ptr->bitrate);
  fprintf(l_fp, "112       /* vbv_buffer_size (in multiples of 16 kbit) */\n");
  fprintf(l_fp, "0         /* low_delay  */\n");
  fprintf(l_fp, "0         /* constrained_parameters_flag */\n");
  fprintf(l_fp, "4         /* Profile ID: Simple = 5, Main = 4, SNR = 3, Spatial = 2, High = 1 */\n");
  fprintf(l_fp, "8         /* Level ID:   Low = 10, Main = 8, High 1440 = 6, High = 4          */\n");
  fprintf(l_fp, "0         /* progressive_sequence */\n");
  fprintf(l_fp, "1         /* chroma_format: 1=4:2:0, 2=4:2:2, 3=4:4:4 */\n");

  fprintf(l_fp, "%d         /* video_format: 0=comp., 1=PAL, 2=NTSC, 3=SECAM, 4=MAC, 5=unspec. */\n", (int)mp_ptr->videoformat);

  fprintf(l_fp, "5         /* color_primaries */\n");
  fprintf(l_fp, "5         /* transfer_characteristics */\n");
  fprintf(l_fp, "5         /* matrix_coefficients */\n");

  fprintf(l_fp, "%d       /* display_horizontal_size */\n", (int)l_width);
  fprintf(l_fp, "%d       /* display_vertical_size */\n", (int)l_height);

  fprintf(l_fp, "0         /* intra_dc_precision (0: 8 bit, 1: 9 bit, 2: 10 bit, 3: 11 bit */\n");
  fprintf(l_fp, "1         /* top_field_first */\n");
  fprintf(l_fp, "0 0 0     /* frame_pred_frame_dct (I P B) */\n");
  fprintf(l_fp, "0 0 0     /* concealment_motion_vectors (I P B) */\n");
  fprintf(l_fp, "1 1 1     /* q_scale_type  (I P B) */\n");
  fprintf(l_fp, "1 0 0     /* intra_vlc_format (I P B)*/\n");
  fprintf(l_fp, "0 0 0     /* alternate_scan (I P B) */\n");
  fprintf(l_fp, "0         /* repeat_first_field */\n");
  fprintf(l_fp, "0         /* progressive_frame */\n");
  fprintf(l_fp, "0         /* P distance between complete intra slice refresh */\n");
  fprintf(l_fp, "0         /* rate control: r (reaction parameter) */\n");
  fprintf(l_fp, "0         /* rate control: avg_act (initial average activity) */\n");
  fprintf(l_fp, "0         /* rate control: Xi (initial I frame global complexity measure) */\n");
  fprintf(l_fp, "0         /* rate control: Xp (initial P frame global complexity measure) */\n");
  fprintf(l_fp, "0         /* rate control: Xb (initial B frame global complexity measure) */\n");
  fprintf(l_fp, "0         /* rate control: d0i (initial I frame virtual buffer fullness) */\n");
  fprintf(l_fp, "0         /* rate control: d0p (initial P frame virtual buffer fullness) */\n");
  fprintf(l_fp, "0         /* rate control: d0b (initial B frame virtual buffer fullness) */\n");
  fprintf(l_fp, "2 2 11 11 /* P:  forw_hor_f_code forw_vert_f_code search_width/height */\n");
  fprintf(l_fp, "1 1 3  3  /* B1: forw_hor_f_code forw_vert_f_code search_width/height */\n");
  fprintf(l_fp, "1 1 7  7  /* B1: back_hor_f_code back_vert_f_code search_width/height */\n");
  fprintf(l_fp, "1 1 7  7  /* B2: forw_hor_f_code forw_vert_f_code search_width/height */\n");
  fprintf(l_fp, "1 1 3  3  /* B2: back_hor_f_code back_vert_f_code search_width/height */\n");


  fclose(l_fp);

  /* generate a startscript */
  
  l_fp = fopen(mp_ptr->startscript, "w");
  if(l_fp == NULL)
  {
     fprintf(stderr, "cant open Startscript %s for write\n", mp_ptr->startscript);
     return -1;
  }
  
  fprintf(l_fp, "#!/bin/sh\n");
  fprintf(l_fp, "mpeg2encode %s %s\n", mp_ptr->parfile, mp_ptr->outfile);
  fprintf(l_fp, "echo 'mpeg2encode done.'\n");
  fprintf(l_fp, "read DUMMY\n");
  fclose(l_fp);
  
  return 0;  
}

/* ============================================================================
 * p_mpeg_encode_gen_parfile
 * ============================================================================
 */

static
int p_mpeg_encode_gen_parfile(t_anim_info *ainfo_ptr, t_mpg_par *mp_ptr)
{
  FILE *l_fp;
  int    l_idx;
  gint   l_width;
  gint   l_height;
  char  *l_dirname_ptr;
  char  *l_basename_ptr;
  char   l_basename_buff[1024];
  char  *l_base_file_format;
  
  l_fp = fopen(mp_ptr->parfile, "w");
  if(l_fp == NULL)
  {
     fprintf(stderr, "cant open MPEG Paramfile %s for write\n", mp_ptr->parfile);
     return -1;
  }

  l_base_file_format = p_mpege_extension_check(ainfo_ptr);

  /* check if ainfo_ptr->basename contains directory part */
  strcpy(l_basename_buff, ainfo_ptr->basename);
  l_dirname_ptr = ".";
  l_basename_ptr = &l_basename_buff[0];
  
  for(l_idx = strlen(l_basename_buff) -1; l_idx >= 0; l_idx--)
  {
    if(l_basename_buff[l_idx] == G_DIR_SEPARATOR)
    {
       l_basename_buff[l_idx] = '\0';
       l_basename_ptr = &l_basename_buff[l_idx +1];
       l_dirname_ptr = &l_basename_buff[0];
       break;
    }
  }

  /* get info about the image (size is common to all frames) */
  l_width  = gimp_image_width(ainfo_ptr->image_id);
  l_height = gimp_image_height(ainfo_ptr->image_id);

  fprintf(l_fp, "# MPEG_ENCODE Parameterfile (generated by GIMP Plugin gap_mpege)\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# any line beginning with # is a comment\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# no line should be longer than 255 characters\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# general format of each line is:\n");
  fprintf(l_fp, "#	<option> <spaces and/or tabs> <value>\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# lines can generally be in any order\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# an exception is the option 'INPUT' which must be followed by input\n");
  fprintf(l_fp, "# files in the order in which they must appear, followed by END_INPUT\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# Also, if you use the `command` method of generating input file names,\n");
  fprintf(l_fp, "# the command will only be executed in the INPUT_DIR if INPUT_DIR preceeds\n");
  fprintf(l_fp, "# the INPUT parameter.\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# <option> MUST be in UPPER CASE\n");
  fprintf(l_fp, "#\n");
  
  fprintf(l_fp, "PATTERN    %s\n", mp_ptr->pattern);
  fprintf(l_fp, "OUTPUT     %s\n\n", mp_ptr->outfile);

  fprintf(l_fp, "# mpeg_encode really only accepts 3 different file formats, but using a\n");
  fprintf(l_fp, "# conversion statement it can effectively handle ANY file format\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# You must specify the type of the input files.  The choices are:\n");
  fprintf(l_fp, "#    YUV, PPM, JMOVIE, Y, JPEG, PNM\n");
  fprintf(l_fp, "#	(must be upper case)\n");
  fprintf(l_fp, "#\n");

  if(*l_base_file_format == '\0')
  {
    fprintf(l_fp, "BASE_FILE_FORMAT   PPM\n\n");
  }
  else
  {
    fprintf(l_fp, "BASE_FILE_FORMAT   %s\n\n", l_base_file_format);
  }

  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# if YUV format (or using parallel version), must provide width and height\n");
  fprintf(l_fp, "# YUV_SIZE	widthxheight\n");
  fprintf(l_fp, "# this option is ignored if BASE_FILE_FORMAT is not YUV and you're running\n");
  fprintf(l_fp, "# on just one machine\n");
  fprintf(l_fp, "#\n");

  fprintf(l_fp, "YUV_SIZE   %dx%d\n\n", (int)l_width, (int)l_height);

  fprintf(l_fp, "# If you are using YUV, there are different supported file formats.\n");
  fprintf(l_fp, "# EYUV or UCB are the same as previous versions of this encoder.\n");
  fprintf(l_fp, "# (All the Y's, then U's then V's, in 4:2:0 subsampling.)\n");
  fprintf(l_fp, "# Other formats, such as Abekas, Phillips, or a general format are\n");
  fprintf(l_fp, "# permissible, the general format is a string of Y's, U's, and V's\n");
  fprintf(l_fp, "# to specify the file order.\n\n");
  fprintf(l_fp, "INPUT_FORMAT UCB\n\n");

  fprintf(l_fp, "# the conversion statement\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# Each occurrence of '*' will be replaced by the input file\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# e.g., if you have a bunch of GIF files, then this might be:\n");
  fprintf(l_fp, "#	INPUT_CONVERT	giftoppm *\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# e.g., if you have a bunch of files like a.Y a.U a.V, etc., then:\n");
  fprintf(l_fp, "#	INPUT_CONVERT	cat *.Y *.U *.V\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# e.g., if you are grabbing from laser disc you might have something like\n");
  fprintf(l_fp, "#	INPUT_CONVERT	goto frame *; grabppm\n");
  fprintf(l_fp, "# 'INPUT_CONVERT *' means the files are already in the base file format\n");
  fprintf(l_fp, "#\n");

  if(*l_base_file_format == '\0')
  {
    fprintf(l_fp, "INPUT_CONVERT %stoppm *\n\n", mp_ptr->ext);
  }
  else
  {
    fprintf(l_fp, "INPUT_CONVERT *\n\n");
  }

  fprintf(l_fp, "# number of frames in a GOP.\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# since each GOP must have at least one I-frame, the encoder will find the\n");
  fprintf(l_fp, "# the first I-frame after GOP_SIZE frames to start the next GOP\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# later, will add more flexible GOP signalling\n");
  fprintf(l_fp, "#\n");

  fprintf(l_fp, "GOP_SIZE	%d\n", strlen(mp_ptr->pattern));

  fprintf(l_fp, "# number of slices in a frame\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# 1 is a good number.  another possibility is the number of macroblock rows\n");
  fprintf(l_fp, "# (which is the height divided by 16)\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "SLICES_PER_FRAME	1\n\n");

  fprintf(l_fp, "# directory to get all input files from (makes this file easier to read)\n");
  fprintf(l_fp, "INPUT_DIR	%s\n", l_dirname_ptr);

  fprintf(l_fp, "# There are a bunch of ways to specify the input files.\n");
  fprintf(l_fp, "# from a simple one-per-line listing, to the following \n");
  fprintf(l_fp, "# way of numbering them.  See the manual for more information.\n");
  fprintf(l_fp, "INPUT\n");
  fprintf(l_fp, "# '*' is replaced by the numbers 01, 02, 03, 04\n");
  fprintf(l_fp, "# if I instead do [01-11], it would be 01, 02, ..., 09, 10, 11\n");
  fprintf(l_fp, "# if I instead do [1-11], it would be 1, 2, 3, ..., 9, 10, 11\n");
  fprintf(l_fp, "# if I instead do [1-11+3], it would be 1, 4, 7, 10\n");
  fprintf(l_fp, "# the program assumes none of your input files has a name ending in ']'\n");
  fprintf(l_fp, "# if you do, too bad!!!\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "#\n");

  fprintf(l_fp, "%s*.%s  [%04d-%04d]\n", l_basename_ptr
                                       , mp_ptr->ext
                                       , mp_ptr->from
                                       , mp_ptr->to);

  fprintf(l_fp, "# can have more files here if you want...there is no limit on the number\n");
  fprintf(l_fp, "# of files\n");
  fprintf(l_fp, "END_INPUT\n\n\n");



  fprintf(l_fp, "# Many of the remaining options have to do with the motion search and qscale\n\n");

  fprintf(l_fp, "# FULL or HALF -- must be upper case\n");
  fprintf(l_fp, "PIXEL		HALF\n\n");

  fprintf(l_fp, "# means +/- this many pixels for both P and B frame searches\n");
  fprintf(l_fp, "# specify two numbers if you wish to serc different ranges in the two.\n");
  fprintf(l_fp, "RANGE		10\n");

  fprintf(l_fp, "# this must be one of {EXHAUSTIVE, SUBSAMPLE, LOGARITHMIC}\n");

  fprintf(l_fp, "PSEARCH_ALG	%s\n\n", mp_ptr->psearch);

  fprintf(l_fp, "# this must be one of {SIMPLE, CROSS2, EXHAUSTIVE}\n");
  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# note that EXHAUSTIVE is really, really, really slow\n");
  fprintf(l_fp, "#\n");

  fprintf(l_fp, "BSEARCH_ALG	%s\n\n", mp_ptr->bsearch);

  fprintf(l_fp, "#\n");
  fprintf(l_fp, "# these specify the q-scale for I, P, and B frames\n");
  fprintf(l_fp, "# (values must be between 1 and 31)\n");
  fprintf(l_fp, "# These are the Qscale values for the entire frame in variable bit-rate\n");
  fprintf(l_fp, "# mode, and starting points (but not important) for constant bit rate\n");
  fprintf(l_fp, "#\n");

  fprintf(l_fp, "IQSCALE		%d\n",   (int)mp_ptr->iqscale);
  fprintf(l_fp, "PQSCALE		%d\n",   (int)mp_ptr->pqscale);
  fprintf(l_fp, "BQSCALE		%d\n\n", (int)mp_ptr->bqscale);

  fprintf(l_fp, "# this must be ORIGINAL or DECODED\n");
  fprintf(l_fp, "REFERENCE_FRAME	ORIGINAL\n\n");

  fprintf(l_fp, "# for parallel parameters see parallel.param in the exmaples subdirectory\n\n");

  fprintf(l_fp, "# if you want constant bit-rate mode, specify it as follows (number is bits/sec):\n");

  if(mp_ptr->const_bitrate == 1)
  {
    fprintf(l_fp, "BIT_RATE  %d\n\n", (int)mp_ptr->bitrate);
  }
  else
  {
    fprintf(l_fp, "#BIT_RATE  %d\n\n", (int)mp_ptr->bitrate);
  }

  fprintf(l_fp, "# To specify the buffer size (327680 is default, measused in bits, for 16bit words)\n");
  fprintf(l_fp, "BUFFER_SIZE 3276800\n\n");

  fprintf(l_fp, "# The frame rate is the number of frames/second (legal values:\n");
  fprintf(l_fp, "# 23.976, 24, 25, 29.97, 30, 50 ,59.94, 60\n");

  fprintf(l_fp, "FRAME_RATE %s\n\n", mp_ptr->framerate);

  fprintf(l_fp, "# There are many more options, see the users manual for examples....\n");
  fprintf(l_fp, "# ASPECT_RATIO, USER_DATA, GAMMA, IQTABLE, etc.\n");



  fclose(l_fp);

  /* generate a startscript */
  
  l_fp = fopen(mp_ptr->startscript, "w");
  if(l_fp == NULL)
  {
     fprintf(stderr, "cant open Startscript %s for write\n", mp_ptr->startscript);
     return -1;
  }
  
  fprintf(l_fp, "#!/bin/sh\n");
  fprintf(l_fp, "mpeg_encode %s\n", mp_ptr->parfile);
  fprintf(l_fp, "echo 'mpeg_encode done.'\n");
  fprintf(l_fp, "read DUMMY\n");
  fclose(l_fp);
  
  return 0;  
}      /* end p_mpege_gen_parfile */


/* ============================================================================
 * p_mpege_gen_parfile
 * ============================================================================
 */
static
int p_mpege_gen_parfile(t_anim_info *ainfo_ptr, t_mpg_par *mp_ptr, t_gap_mpeg_encoder encoder)
{
  int    l_rc;
  gchar *l_cmd;
  
  l_rc = -1;
  switch(encoder)
  {
    case MPEG_ENCODE:
        l_rc = p_mpeg_encode_gen_parfile(ainfo_ptr, mp_ptr);
        break;
    case MPEG2ENCODE:
        l_rc = p_mpeg2encode_gen_parfile(ainfo_ptr, mp_ptr);
        break;
  }
  
  if(l_rc >= 0)
  {
      /* make startscript executable */
      l_cmd = g_strdup_printf("chmod a+x %s", mp_ptr->startscript);
      system(l_cmd);
      g_free(l_cmd);
  }

  return(l_rc);
}

/* ============================================================================
 * gap_mpeg_encode
 * ============================================================================
 */

int gap_mpeg_encode(GRunModeType run_mode,
                             gint32 image_id,
                             t_gap_mpeg_encoder encoder
                             )
{
  int    l_rc;
  int    l_genmode;
  t_anim_info *ainfo_ptr;
  t_mpg_par    mp_par;
  gint   l_width;
  gint   l_height;
  char  *l_base_file_format;
  char   l_errlist[512];
  char  *l_cmd;
  gint   l_base_ffidx;
  
  l_rc = 0;
  l_genmode = 0;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      l_base_file_format = p_mpege_extension_check(ainfo_ptr);
      l_base_ffidx = p_mpeg2_extension_check(ainfo_ptr);
      l_width  = gimp_image_width(ainfo_ptr->image_id);
      l_height = gimp_image_height(ainfo_ptr->image_id);
      
      l_errlist[0] = '\0';
      mp_par.ext = "\0";
      if(ainfo_ptr->extension != NULL)
      {
        if(*ainfo_ptr->extension == '.' ) mp_par.ext = &ainfo_ptr->extension[1];
        else                              mp_par.ext = ainfo_ptr->extension;
      }
      
      if(encoder == MPEG_ENCODE)
      {
        if(*l_base_file_format == '\0')
        {
          strcat(l_errlist, _("\nWARNING: mpeg_encode does not support Fileformat "));
          strcat(l_errlist, mp_par.ext);
        }
        if((l_width % 16) != 0)         { strcat(l_errlist, _("\nERROR: width not a multiple of 16")); }
        if((l_height % 16) != 0)        { strcat(l_errlist, _("\nERROR: height not a multiple of 16")); }
      }
      if(encoder == MPEG2ENCODE)
      {
         if(l_base_ffidx < 0)
         {
           strcat(l_errlist, _("\nWARNING: mpeg2encode does not support Fileformat "));
           strcat(l_errlist, mp_par.ext);
         }
      }

      if(ainfo_ptr->frame_cnt == 0)   { strcat(l_errlist, _("\nERROR: invoked from a single image, animframe required")); }
      
      if((l_rc == 0) && (l_errlist[0] != '\0'))
      {
         /* show warnings and errors (the user can decide to cancel or to continue) */
         l_rc = p_mpege_info(ainfo_ptr, l_errlist, encoder);
      }
      
      if(l_rc == 0)
      {
         if(run_mode == RUN_INTERACTIVE)
         {
            l_genmode = p_mpege_dialog(ainfo_ptr, &mp_par, encoder);
            if(l_genmode < 0) l_rc = -1;
            else              l_rc = 0;
         }
         else
         {
             printf ("sorry folks, NON_INTERACTIVE call .. not implemented yet\n");
             l_rc = -1;
         }

         if(l_rc == 0)
         {
             l_rc = p_mpege_gen_parfile(ainfo_ptr, &mp_par, encoder);
             if (l_rc == 0)
             {
                 if (l_genmode == 1)
                 {
                    /* execute mpeg encoder startscript in an xterm window */
                    l_cmd = g_strdup_printf("xterm -e %s &", mp_par.startscript);
                    l_rc = system(l_cmd);
		    g_free(l_cmd);
                    if(l_rc != 0)
                    {
                       fprintf(stderr, "ERROR: could not execute mpeg_encode (not installed or not in PATH)");
                    }
                 }
             }
         }
      }
    }
    p_free_ainfo(&ainfo_ptr);
  }
  

  return(l_rc);    
}	/* end gap_mpeg_encode */
