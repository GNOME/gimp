/* gap_decode_xanim.c
 * 1999.11.22 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 *
 *        GIMP/GAP-frontend interface for XANIM exporting edition from loki entertainmaint
 *         Call xanim exporting edition (the loki version)
 *         To split any xanim supported video into
 *         anim frames (single images on disk)
 *         Audio can also be extracted.
 *
 *        xanim  exporting edition is available at:
 *            Web:        http://heroine.linuxbox.com/toys.html
 *                        http://www.lokigames.com/development/smjpeg.php3
 *            download:   http://heroine.linuxbox.com/xanim_exporting_edition.tar.gz
 *                        http://www.lokigames.com/development/download/smjpeg/xanim2801-loki090899.tar.gz
 *            Send comments or questions to smjpeg@lokigames.com.
 *
 * Warning: This Module needs UNIX environment to run.
 *   It uses programs and commands that are NOT available
 *   on other Operating Systems (Win95, NT ...)
 *
 *     - xanim              2.80 exporting edition with extensions from loki entertainment.
 *                          set environment GAP_XANIM_PROG to configure where to find xanim
 *                          (default: search xanim in your PATH)
 *     - grep               (UNIX command)
 *     - rm                 (UNIX command is used to delete by wildcard (expanded by /bin/sh)
 *                                and to delete a directory with all files
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
 * 1.1.17b;  2000/02/26  hof: bugfixes
 * 1.1.14a;  1999/11/22  hof: fixed gcc warning (too many arguments for format)
 * 1.1.13a;  1999/11/22  hof: first release
 */
 
/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "config.h"
#include "libgimp/stdplugins-intl.h"
#include "libgimp/gimp.h"

#ifdef G_OS_WIN32
#include <io.h>
#  ifndef S_ISDIR
#    define S_ISDIR(m) ((m) & _S_IFDIR)
#  endif
#  ifndef S_ISREG
#    define S_ISREG(m) ((m) & _S_IFREG)
#  endif
#endif

/* GAP includes */
#include "gap_lib.h"
#include "gap_arr_dialog.h"
#include "gap_decode_xanim.h"

extern      int gap_debug; /* ==0  ... dont print debug infos */

static char *global_xanim_input_dir    = "input";

gchar global_xanim_prog[500];
gchar *global_errlist = NULL;

gint32  global_delete_number;

#define MKDIR_MODE (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH )

/* ============================================================================
 * p_xanim_info
 * ============================================================================
 */
static int
p_xanim_info(char *errlist)
{
  t_arr_arg  argv[20];
  t_but_arg  b_argv[2];

  int        l_idx;
  int        l_rc;
  

  l_idx = 0;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("Conditions to run the xanim based video split");

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "";

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("1.) xanim 2.80.0 exporting edition (the loki version)");

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("    must be installed somewhere in your PATH");

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("    you can get xanim exporting edition at");

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "    http://heroine.linuxbox.com/toys.html";

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "    http://www.lokigames.com/development/download/smjpeg/xanim2801-loki090899.tar.gz";

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "";

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("2.) if your xanim exporting edition is not in your PATH or is not named xanim");

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("    you have to set Environment variable GAP_XANIM_PROG ");

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("    to your xanim exporting program and restart gimp");

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "";

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("An ERROR occured while trying to call xanim:");  

  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "--------------------------------------------";  


  l_idx++;
  p_init_arr_arg(&argv[l_idx], WGT_LABEL_LEFT);
  argv[l_idx].label_txt = errlist;

  l_idx++;

  /* the  Action Button */
    b_argv[0].but_txt  = _("CANCEL");
    b_argv[0].but_val  = -1;
    b_argv[1].but_txt  = _("OK");
    b_argv[1].but_val  = 0;
  
  l_rc = p_array_std_dialog(_("XANIM Information"),
                             "",
                              l_idx,   argv,      /* widget array */
                              1,       b_argv,    /* button array */
                              -1);

  return (l_rc); 
}	/* end p_xanim_info */


/* ============================================================================
 * p_xanim_dialog
 * ============================================================================
 */
static int
p_xanim_dialog   (gint32 *first_frame,
                  gint32 *last_frame,
		  char   *filename,
		  gint32 len_filename,
		  char   *basename,
		  gint32 len_basename,
		  t_gap_xa_formats *Format,
		  gint32  *extract_video,
		  gint32  *extract_audio,
		  gint32  *jpeg_quality,
		  gint32  *autoload,
		  gint32  *run_xanim_asynchron)
{
#define XADIALOG_NUM_ARGS 12
  static t_arr_arg  argv[XADIALOG_NUM_ARGS];
  static char *radio_args[3]  = { N_("XCF"), N_("PPM"), N_("JPEG") };

  p_init_arr_arg(&argv[0], WGT_FILESEL);
  argv[0].label_txt = _("Video:");
  argv[0].help_txt  = _("Name of a videofile to READ by xanim\n"
                        "frames are extracted from the videofile\n"
			"and written to seprate diskfiles\n"
			"xanim exporting edition is required");
  argv[0].text_buf_len = len_filename;
  argv[0].text_buf_ret = filename;
  argv[0].entry_width = 250;

  p_init_arr_arg(&argv[1], WGT_INT_PAIR);
  argv[1].label_txt = _("From :");
  argv[1].help_txt  = _("Framenumber of 1.st frame to extract");
  argv[1].constraint = FALSE;
  argv[1].int_min    = 0;
  argv[1].int_max    = 9999;
  argv[1].int_ret    = 0;
  argv[1].umin       = 0;
  argv[1].entry_width = 80;
  
  p_init_arr_arg(&argv[2], WGT_INT_PAIR);
  argv[2].label_txt = _("To :");
  argv[2].help_txt  = _("Framenumber of last frame to extract");
  argv[2].constraint = FALSE;
  argv[2].int_min    = 0;
  argv[2].int_max    = 9999;
  argv[2].int_ret    = 9999;
  argv[2].umin       = 0;
  argv[2].entry_width = 80;
  
  p_init_arr_arg(&argv[3], WGT_FILESEL);
  argv[3].label_txt = _("Framenames:");
  argv[3].help_txt  = _("Basename for the AnimFrames to write on disk\n"
                        "(framenumber and extension is added)");
  argv[3].text_buf_len = len_basename;
  argv[3].text_buf_ret = basename;
  argv[3].entry_width = 250;
  
  p_init_arr_arg(&argv[4], WGT_OPTIONMENU);
  argv[4].label_txt = _("Format");
  argv[4].help_txt  = _("Fileformat for the extracted AnimFrames\n"
        	       "(xcf is extracted as ppm and converted to xcf)");
  argv[4].radio_argc  = 3;
  argv[4].radio_argv = radio_args;
  argv[4].radio_ret  = 0;

  p_init_arr_arg(&argv[5], WGT_TOGGLE);
  argv[5].label_txt = _("Extract Frames");
  argv[5].help_txt  = _("Enable extraction of Frames");
  argv[5].int_ret   = 1;

  p_init_arr_arg(&argv[6], WGT_TOGGLE);
  argv[6].label_txt = _("Extract Audio");
  argv[6].help_txt  = _("Enable extraction of audio to raw audiofile\n"
                        "(frame range limits are ignored for audio)");
  argv[6].int_ret   = 0;
  
  p_init_arr_arg(&argv[7], WGT_INT_PAIR);
  argv[7].label_txt = _("Jpeg Quality:");
  argv[7].help_txt  = _("Quality for resulting jpeg frames\n"
                        "(is ignored when other formats are used)");
  argv[7].constraint = TRUE;
  argv[7].int_min    = 0;
  argv[7].int_max    = 100;
  argv[7].int_ret    = 90;

  p_init_arr_arg(&argv[8], WGT_LABEL);
  argv[8].label_txt = "";

  p_init_arr_arg(&argv[9], WGT_TOGGLE);
  argv[9].label_txt = _("Open");
  argv[9].help_txt  = _("Open the 1.st one of the extracted frames");
  argv[9].int_ret   = 1;

  p_init_arr_arg(&argv[10], WGT_TOGGLE);
  argv[10].label_txt = _("Run asynchronously");
  argv[10].help_txt  = _("Run xanim asynchronously and delete unwanted frames\n"
                        "(out of the specified range) while xanim is still running");
  argv[10].int_ret   = 1;

  p_init_arr_arg(&argv[11], WGT_LABEL_LEFT);
  argv[11].label_txt = _("\nWarning: xanim 2.80 has just limited MPEG support\n"
                        "most of the frames (type P and B) will be skipped");
   
  if(TRUE == p_array_dialog(_("Split any Xanim readable Video to Frames"), 
			    _("Select Frame range"), XADIALOG_NUM_ARGS, argv))
  {
     if(argv[1].int_ret < argv[2].int_ret )
     {
       *first_frame = (long)(argv[1].int_ret);
       *last_frame  = (long)(argv[2].int_ret);
     }
     else
     {
       *first_frame = (long)(argv[2].int_ret);
       *last_frame  = (long)(argv[1].int_ret);
     }
     *Format = (t_gap_xa_formats)(argv[4].int_ret);
     *extract_video  = (long)(argv[5].int_ret);
     *extract_audio  = (long)(argv[6].int_ret);
     *jpeg_quality   = (long)(argv[7].int_ret);
     *autoload       = (long)(argv[9].int_ret);
     *run_xanim_asynchron   = (long)(argv[10].int_ret);
     return (0);    /* OK */
  }
  else
  {
     return -1;     /* Cancel */
  }
   

}	/* end p_xanim_dialog */


static gint
p_overwrite_dialog(char *filename, gint overwrite_mode)
{
  static  t_but_arg  l_argv[3];
  static  t_arr_arg  argv[1];

  if(p_file_exists(filename))
  {
    if (overwrite_mode < 1)
    {
       l_argv[0].but_txt  = _("OVERWRITE frame");
       l_argv[0].but_val  = 0;
       l_argv[1].but_txt  = _("OVERWRITE all");
       l_argv[1].but_val  = 1;
       l_argv[2].but_txt  = _("CANCEL");
       l_argv[2].but_val  = -1;

       p_init_arr_arg(&argv[0], WGT_LABEL);
       argv[0].label_txt = filename;
    
       return(p_array_std_dialog ( _("GAP Question"),
                                   _("File already exists:"),
				   1, argv,
				   3, l_argv, -1));
    }
  }
  return (overwrite_mode);
}




static void
p_build_xanim_framename(char *framename, gint32 frame_nr, char *ext)
{
   sprintf(framename, "%s/frame%d.%s",
                global_xanim_input_dir,
                (int)frame_nr,
                ext);
}

static void
p_build_gap_framename(char *framename, gint32 frame_nr, char *basename, char *ext)
{
   sprintf(framename, "%s%04d.%s", basename, (int)frame_nr, ext);
}

int
p_is_directory(char *fname)
{
  struct stat  l_stat_buf;

  /* get File status */
  if (0 != stat(fname, &l_stat_buf))
  {
    /* stat error (file does not exist) */
    return(0);
  }
  
  if(S_ISDIR(l_stat_buf.st_mode))
  {
    return(1);
  }
    
  return(0);
}	/* end p_is_directory  */

void
p_dirname(char *fname)
{
  int l_idx;
  
  l_idx = strlen(fname) -1;
  while(l_idx > 0)
  {
     if(fname[l_idx] == G_DIR_SEPARATOR)
     {
        fname[l_idx] = '\0';
	return;
     }
     l_idx--;
  }
  *fname = '\0';
}

static void
p_init_xanim_global_name()
{
  char *l_env;
  
  l_env = g_getenv("GAP_XANIM_PROG");
  
  if(l_env != NULL)
  {
     strcpy(global_xanim_prog, l_env);     
     return;
  }
  strcpy(global_xanim_prog, "xanim");  /* default name */
}

static int
p_convert_frames(gint32 frame_from, gint32 frame_to, char *basename, char *ext, char *ext2)
{
   GParam          *return_vals;
   int              nreturn_vals;
   gint32           l_tmp_image_id;
   char             l_first_xa_frame[200];

  /* load 1.st one of those frames generated by xanim  */
   p_build_xanim_framename(l_first_xa_frame, frame_from, ext);
   l_tmp_image_id = p_load_image(l_first_xa_frame);

   /* convert the xanim frames (from ppm) to xcf fileformat
    * (the gap module for range convert is not linked to the frontends
    *  main program, therefore i call the convert procedure by PDB-interface)
    */   
   return_vals = gimp_run_procedure ("plug_in_gap_range_convert2",
                                    &nreturn_vals,
                                    PARAM_INT32, RUN_NONINTERACTIVE,     /* runmode  */
                                    PARAM_IMAGE, l_tmp_image_id,
                                    PARAM_DRAWABLE, 0,         /* (unused)  */
                                    PARAM_INT32, frame_from,
                                    PARAM_INT32, frame_to,
                                    PARAM_INT32, 0,            /* dont flatten */
                                    PARAM_INT32, 4444,         /* dest type (keep type) */
                                    PARAM_INT32, 256,          /* colors (unused)  */
                                    PARAM_INT32, 0,            /* no dither (unused)  */
                                    PARAM_STRING, ext2,        /* extension for dest. filetype  */
                                    PARAM_STRING, basename,    /* basename for dest. filetype  */
                                    PARAM_INT32, 0,            /* (unused)  */
                                    PARAM_INT32, 0,            /* (unused)  */
                                    PARAM_INT32, 0,            /* (unused)  */
                                    PARAM_STRING, "none",      /* (unused) palettename */
                                    PARAM_END);

   /* destroy the tmp image */
   gimp_image_delete(l_tmp_image_id);

   if (return_vals[0].data.d_status != STATUS_SUCCESS)
   {
      return(-1);  
   }

   return(0);  /* OK */
}


static gint32
p_find_max_xanim_frame(gint32 from_nr, char *ext)
{
  gint32 l_high;
  gint32 l_max_found;
  gint32 l_nr;
  gint32 l_delta;
  char   l_frame[500];
  
  l_nr = from_nr;
  l_max_found = 0;
  l_high = 100000;
  
  while(1 == 1)
  {
     p_build_xanim_framename(l_frame, l_nr, ext);

     if(gap_debug) printf("DEBUG find_MAX :%s\n", l_frame);
     
     if(p_file_exists(l_frame))
     {
        l_max_found = l_nr;
        l_delta = (l_high - l_nr) / 2;
        if(l_delta == 0)
	{ 
	   l_delta = 1;
	}
        l_nr = l_max_found + l_delta;
     }
     else
     {	
        if(l_nr == from_nr) { return (-1); }  /* no frames found */

        if(l_nr < l_high)
	{
           l_high = l_nr;
	   l_nr = l_max_found + 1;
	}
	else
	{
	   return(l_max_found);
	}
     }
  } 
}	/* end p_find_max_xanim_frame */

static int
p_rename_frames(gint32 frame_from, gint32 frame_to, char *basename, char *ext)
{
  gint32 l_use_mv;
  gint32 l_frame_nr;
  gint32 l_max_found;
  char   l_src_frame[500];
  char   l_dst_frame[500];
  gint    l_overwrite_mode;


  if(gap_debug) printf("p_rename_frames:\n");
  l_use_mv = TRUE;
  l_overwrite_mode = 0;

  
  l_max_found = p_find_max_xanim_frame (frame_from, ext);
  if(l_max_found < 0)
  {
       global_errlist = g_strdup_printf(
	       _("cant find any extracted frames,\n%s\nmaybe xanim has failed or was canclled"),
	       l_src_frame);
       return(-1);
  }
 
  
  l_frame_nr = frame_from;
   
  while (l_frame_nr <= frame_to)
  {
     p_build_xanim_framename(l_src_frame, l_frame_nr, ext);
     p_build_gap_framename(l_dst_frame, l_frame_nr, basename, ext);
     
     if(!p_file_exists(l_src_frame))
     {
        break;  /* srcfile not found, stop */
     }
     
     if (strcmp(l_src_frame, l_dst_frame) != 0)
     {
        /* check overwrite if Destination frame already exsts */
	l_overwrite_mode = p_overwrite_dialog(l_dst_frame, l_overwrite_mode);
	if (l_overwrite_mode < 0)
	{
	     global_errlist = g_strdup_printf(
	             _("frames are not extracted, because overwrite of %s was cancelled"),
	             l_dst_frame);
	     return(-1);
	}
	else
	{
	   remove(l_dst_frame);
	   if (p_file_exists(l_dst_frame))
           {
	     global_errlist = g_strdup_printf(
	             _("failed to overwrite %s (check permissions ?)"),
	             l_dst_frame);
	     return(-1);
	   }
	}

        if (l_use_mv)
	{	
           rename(l_src_frame, l_dst_frame);
	}
	
	if (!p_file_exists(l_dst_frame)) 
	{
	   p_file_copy(l_src_frame, l_dst_frame);
	   if (p_file_exists(l_dst_frame))
	   {
              l_use_mv = FALSE; /* if destination is on another device use copy-remove strategy */
	      remove(l_src_frame);
	   }
	   else
	   {
	      global_errlist = g_strdup_printf(
	             _("failed to write %s (check permissions ?)"),
	             l_dst_frame);
	      return(-1);
	   }
	}
     }
     l_frame_nr++;
     if(l_max_found > 0) gimp_progress_update ((gdouble)l_frame_nr / (gdouble)l_max_found);
  }
  return(0);
}	/* end p_rename_frames */

static void
p_delete_frames(gint32 max_tries, gint32 frame_from, gint32 frame_to, char *ext)
{
  /* this procedure is performed repeatedly while polling the xanim process
   * and after xanim process has (or was) terminated to clean up unwanted frames.
   */
  gint32 l_tries;
  gint32 l_next_number;
  char   l_framename[500];
  
  if(gap_debug) printf("p_delete_frames: cleaning up unwanted frames (max=%d)\n", (int)max_tries);
  
  l_tries = 0;

  while ((global_delete_number < frame_from) && (l_tries < max_tries))
  {
     l_next_number = global_delete_number + 1;
     p_build_xanim_framename(l_framename, l_next_number, ext);
     
     if (p_file_exists(l_framename))
     {
        /* if xanim has already written the next frame
	 * we can delete the previous (unwanted) frame now
	 */
        p_build_xanim_framename(l_framename, global_delete_number, ext);
	if(gap_debug) printf("delete frame: %s\n", l_framename);
	remove(l_framename);

	global_delete_number = l_next_number;
     }
     l_tries++;
  }
}	/* end p_delete_frames */


static void
p_poll(pid_t xanim_pid, char *one_past_last_frame, gint32 frame_from, gint32 frame_to, char *ext)
{
  /* loop as long as the Xanim Process is alive */

  if(gap_debug) printf("poll started on xanim pid: %d\n", (int)xanim_pid);


  /* kill  with signal 0 checks only if the process is alive (no signal is sent)
   *       returns 0 if alive, 1 if no process with given pid found.
   */
  while (0 == kill(xanim_pid, 0))
  {
    usleep(100000);  /* sleep 1 second, and let xanim write some frames */
    if (p_file_exists(one_past_last_frame))
    {
       /* if the last desired frame is written
        * we can stop (kill with signal 9) the xanim process.
        */
       kill(xanim_pid, 9);
       break;
    }

    /* check for max unwanted frames and delete (upto 20 of them) */
    p_delete_frames(20, frame_from, frame_to, ext);
  }   
              
  if(gap_debug) printf("poll ended on xanim pid: %d\n", (int)xanim_pid);
}	/* end p_poll */


static int
p_grep(char *pattern, char *file)
{
  gint    l_rc;
  gchar  *l_cmd;

  l_cmd = g_strdup_printf("grep -c '%s' \"%s\" >/dev/null", pattern, file);
  l_rc =  system(l_cmd);
  g_free(l_cmd);
  if (l_rc == 0)
  {
     return(0); /* pattern found */
  }  
  return(1);    /* pattern NOT found */
}

static gint
p_check_xanim()
{
  gint l_rc;
  gint l_grep_counter1;
  gint l_grep_counter2;
  gint l_grep_counter3;
  gchar *l_cmd;
  static char *l_xanim_help_output = "tmp_xanim_help.output";
  FILE *l_fp;

  l_fp = fopen(l_xanim_help_output, "w+");
  if (l_fp == NULL)
  {
    global_errlist = g_strdup_printf("no write permission for current directory");
    return(10);
  }
  fprintf(l_fp, "dummy");
  fclose(l_fp);

  /* execute xanim with -h option and 
   * store its output in a file.
   */
  l_cmd = g_strdup_printf("%s -h 2>&1 >>%s", global_xanim_prog, l_xanim_help_output);
  l_rc =  system(l_cmd);

  if(gap_debug) printf("DEBUG: executed :%s\n  Retcode: %d\n", l_cmd, (int)l_rc);
  g_free(l_cmd);
  
  if ((l_rc == 127) || (l_rc == (127 << 8)))
  {
        global_errlist = g_strdup_printf(
                _("could not execute %s (check if xanim is installed)"),
                global_xanim_prog  );
        return(10);
  }

  if(!p_file_exists(l_xanim_help_output)) 
  {
    global_errlist = g_strdup_printf(
            _("%s does not look like xanim"),
            global_xanim_prog  );
    return(10);
  }
  
  /* check the help output of xanim (using grep) */
  l_grep_counter1 = 0;
  /* l_grep_counter1 += p_grep("anim", l_xanim_help_output); */

  /* check for the exporting options */     
  l_grep_counter2 = 0;
  l_grep_counter2 += p_grep("Ea", l_xanim_help_output);
  l_grep_counter2 += p_grep("Ee", l_xanim_help_output);
  l_grep_counter2 += p_grep("Eq", l_xanim_help_output);

  /* check for the loki version that is able to write single frames */     
  l_grep_counter3 = 0;
  l_grep_counter3 += p_grep("Write video to input/frameN.EXT", l_xanim_help_output);

  remove(l_xanim_help_output);

  if(l_grep_counter2 != 0)
  {
     global_errlist = g_strdup_printf(
             _("The xanim program on your system \"%s\"\ndoes not support the exporting options Ea, Ee, Eq"),
             global_xanim_prog  );
     return(10);
  }
  if(l_grep_counter3 != 0)
  {
     global_errlist = g_strdup_printf(
             _("The xanim program on your system \"%s\"\ndoes not support exporting of single frames"),
             global_xanim_prog  );
     return(10);
  }
  return (0);  /* OK, xanim output looks like expected */
}	/* end p_check_xanim */

static pid_t
p_start_xanim_process(gint32 first_frame, gint32 last_frame,
                  char   *filename,
		  t_gap_xa_formats Format,
		  gint32  extract_video,
		  gint32  extract_audio,
		  gint32  jpeg_quality , 
		  char *one_past_last_frame,
		  gint32  run_xanim_asynchron)
{
   gchar  l_cmd[500];
   gchar  l_buf[40];
   pid_t l_xanim_pid;
   int   l_rc;
   FILE  *l_fp;
   static char *l_xanim_startscript = "tmp_xanim_startscript.sh";
   static char *l_xanim_pidfile = "tmp_xanim_pidfile";
   
   l_xanim_pid = -1;
   
   /* allocate and prepare args for the xanim call */
   sprintf(l_cmd, "%s +f ", global_xanim_prog);  /* programname */
   
   if (extract_audio)
   {
     strcat(l_cmd, "+Ea ");
   }

   if (extract_video)
   {
     strcat(l_cmd, "+v ");    /* +v is verbose mode */

     switch(Format)
     { 
       case XAENC_PPMRAW:
          strcat(l_cmd, "+Ee ");
	  break;
       case XAENC_JPEG:
          sprintf(l_buf, "+Eq%d ", (int)jpeg_quality);
	  strcat(l_cmd, l_buf);
	  break;
       default:
          strcat(l_cmd, "+Ee ");
	  break;
      }

     /* additional option "Pause after N Frames" is used,
      * to stop xanim exporting frames beyond the requested limit
      */
     if (run_xanim_asynchron)
     {
       sprintf(l_buf, "+Zp%d ", (int)(last_frame +1));
       strcat(l_cmd, l_buf);
     }
      
   }
   
   /* add the videofilename as last parameter */
   strcat(l_cmd, filename);

   if (run_xanim_asynchron)
   {
     /* asynchron start */
     remove(l_xanim_pidfile);   
     /* generate a shelscript */
     l_fp = fopen(l_xanim_startscript, "w+");
     if (l_fp != NULL)
     {
	 fprintf(l_fp, "#!/bin/sh\n");
	 /* fprintf(l_fp, "(%s ; touch %s) &\n" */
	 fprintf(l_fp, "%s & # ; touch %s) &\n"
                       , l_cmd                 /* start xanim as background process */
		       , one_past_last_frame   /* and create a dummy frame when xanim is done */
		 );
	 fprintf(l_fp, "XANIM_PID=$!\n");
	 fprintf(l_fp, "echo \"$XANIM_PID # XANIM_PID\"\n");
	 fprintf(l_fp, "echo \"$XANIM_PID # XANIM_PID\" > \"%s\"\n", l_xanim_pidfile);

	 /* we pass the xanim pid in a file, 
          * exitcodes are truncated to 8 bit
          * by the system call
          */
	 /* fprintf(l_fp, "exit $XANIM_PID\n"); */
	 fclose(l_fp);

	 chmod(l_xanim_startscript, MKDIR_MODE);
     }

     l_rc = system(l_xanim_startscript);

     l_fp = fopen(l_xanim_pidfile, "r");
     if (l_fp != NULL)
     {
	fscanf(l_fp, "%d", &l_rc);
	fclose(l_fp);
	l_xanim_pid = (pid_t)l_rc;
     }

     remove(l_xanim_startscript);
     remove(l_xanim_pidfile);


     if(gap_debug) printf("ASYNCHRON CALL: %s\nl_xanim_pid:%d\n", l_cmd, (int)l_xanim_pid);
   }
   else
   {
     /* synchron start (blocks until xanim process has finished */
     l_rc = system(l_cmd);
     if ((l_rc & 0xff) == 0) l_xanim_pid = 0;
     else                    l_xanim_pid = -1;

     if(gap_debug) printf("ASYNCHRON CALL: %s\nretcode:%d (%d)\n", l_cmd, (int)l_rc, (int)l_xanim_pid);
   }
  
   return(l_xanim_pid);   
}	/* end p_start_xanim_process */


#ifdef THIS_IS_A_COMMENT_EXEC_DID_NOT_WORK_AND_LEAVES_A_ZOMBIE_PROCESS
static pid_t
p_start_xanim_process_exec(gint32 first_frame, gint32 last_frame,
                  char   *filename,
		  t_gap_xa_formats Format,
		  gint32  extract_video,
		  gint32  extract_audio,
		  gint32  jpeg_quality
		  )
{
   char *args[20];
   char  l_buf[40];
   int   l_idx;
   pid_t l_xanim_pid;

   /* allocate and prepare args for the xanim call */
   l_idx = 0;
   args[l_idx] = g_strdup(global_xanim_prog);  /* programname */
   
   l_idx++;
   args[l_idx] = g_strdup("+f");

   if (extract_audio)
   {
     l_idx++;
     args[l_idx] = g_strdup("+Ea");
   }

   if (extract_video)
   {
     l_idx++;
     args[l_idx] = g_strdup("+v");    /* +v is verbose mode */

     l_idx++;
     switch(Format)
     { 
       case XAENC_PPMRAW:
          args[l_idx] = g_strdup("+Ee");
	  break;
       case XAENC_JPEG:
          sprintf(l_buf, "+Eq%d", (int)jpeg_quality);
	  args[l_idx] = g_strdup(l_buf);
	  break;
       default:
          args[l_idx] = g_strdup("+Ee");
	  break;
      }

     /* additional option "Pause after N Frames" is used,
      * to stop xanim exporting frames beyond the requested limit
      */      
     l_idx++;
     sprintf(l_buf, "+Zp%d", (int)(last_frame +1));
     args[l_idx] = g_strdup(l_buf);
      
   }
   
   /* add the videofilename as last parameter */
   l_idx++;
   args[l_idx] = g_strdup(filename);
   
   l_idx++;
   args[l_idx] = NULL;  /* terminate args list with a NULL pointer */
   
   l_xanim_pid = fork();

   if(l_xanim_pid == 0)
   {
      /* here we are in the forked child process
       * execute xanim
       */
      execvp(args[0], args);
      
      /* this point should never be reached */
      _exit (1);
   }
   
   return(l_xanim_pid);   
}	/* end p_start_xanim_process */
#endif


/* ============================================================================
 * gap_xanim_decode
 * ============================================================================
 */

gint32
gap_xanim_decode(GRunModeType run_mode)
{
  gint32 l_rc;
  gint32 first_frame;
  gint32 last_frame;
  char   filename[200];
  char   basename[200];
  char   extension[20];
  char   extension2[20];
  t_gap_xa_formats Format;
  gint32 extract_audio;
  gint32 extract_video; 
  gint32 jpeg_quality;
  gint32 autoload;
  gint32 run_xanim_asynchron;
  char   l_cmd[300];
  char   l_one_past_last_frame[200];
  char   l_first_to_laod[200];
  char  *l_dst_dir;
  pid_t  l_xanim_pid;
  int    l_input_dir_created_by_myself;
  
  l_rc = 0;
  l_input_dir_created_by_myself = FALSE;
  global_errlist = NULL;
  p_init_xanim_global_name();
  
  filename[0] = '\0';
  strcpy(&basename[0], "frame_");

  l_rc = p_xanim_dialog (&first_frame,
                 &last_frame,
		 filename, sizeof(filename),
		 basename, sizeof(basename),
		 &Format,
		 &extract_video,
		 &extract_audio,
		 &jpeg_quality,
		 &autoload,
		 &run_xanim_asynchron);


  if(l_rc != 0)
  {
    return(l_rc);
  }
  
  if(!p_file_exists(filename))
  {
     global_errlist = g_strdup_printf(
            _("videofile %s not existent or empty\n"),
            filename);
            l_rc = 10;
  }
  else
  {
     l_rc = p_check_xanim();
  }
  
  
  if (l_rc == 0)
  {
    switch(Format)
    { 
      case XAENC_PPMRAW:
         strcpy(extension,  "ppm");
         strcpy(extension2, ".ppm");
 	 break;
      case XAENC_JPEG:
         strcpy(extension,  "jpg");
         strcpy(extension2, ".jpg");
	 break;
      default:
         strcpy(extension,  "ppm");
         strcpy(extension2, ".xcf");
	 break;

     }
     p_build_xanim_framename(l_one_past_last_frame, last_frame +1 , extension);

    if (extract_video)
    {
         /* for the frames we need a directory named "input" */
         if (p_is_directory(global_xanim_input_dir))
         {
           /* the input directory already exists,
            * remove frames
            */
           sprintf(l_cmd, "rm -f %s/*.%s", global_xanim_input_dir, extension);
           system(l_cmd);
         }
         else
         {
            /* create input directory (needed by xanim to store the frames) */
            mkdir(global_xanim_input_dir, MKDIR_MODE);

            if (p_is_directory(global_xanim_input_dir))
            {
              l_input_dir_created_by_myself = TRUE;
            }
            else
            {
               global_errlist = g_strdup_printf(
                      _("could not create %s directory\n"
                       "(that is required for xanim frame export)"),
                       global_xanim_input_dir);
               l_rc = 10;
            }
         }
     }
  }
   
  if(l_rc == 0)
  {
     gimp_progress_init (_("extracting frames..."));
     gimp_progress_update (0.1);  /* fake some progress */
     /* note:
      *  we cant show realistic progress for the extracting process
      *  because we know nothing about videofileformat and how much frames
      *  are realy stored in the videofile.
      *
      *  one guess could assume, that xanim will write 0 upto last_frame
      *  to disk, and check for the frames that the xanim process creates.
      *  The periodically checking can be done in the poll procedure for asynchron
      *  startmode only.
      */

     l_xanim_pid = p_start_xanim_process(first_frame, last_frame,
	                                filename,
                                        Format,
					extract_video,
					extract_audio,
					jpeg_quality,
                                        l_one_past_last_frame,
					run_xanim_asynchron);

     if (l_xanim_pid == -1 )
     {
        global_errlist = g_strdup_printf(
           _("could not start xanim process\n(program=%s)"),
           global_xanim_prog  );
	l_rc = -1;
     }
  }

  if(l_rc == 0)
  {
     if(run_xanim_asynchron)
     {
       p_poll(l_xanim_pid, l_one_past_last_frame, first_frame, last_frame, extension);
     }
     
     p_delete_frames(99999, first_frame, last_frame, extension);
     remove(l_one_past_last_frame);

     gimp_progress_update (1.0);
     
     if (p_find_max_xanim_frame (first_frame, extension) < first_frame)
     {
        global_errlist = g_strdup_printf(
	       _("cant find any extracted frames,\n"
	         "xanim has failed or was canclled"));
        l_rc = -1;
     }
     else
     {
       /* if destination directorypart does not exist, try to create it */
       l_dst_dir = g_strdup(basename);
       p_dirname(l_dst_dir);
       if (*l_dst_dir != '\0')
       {
	 if ( !p_is_directory(l_dst_dir) )
	 {
            mkdir (l_dst_dir, MKDIR_MODE);
	 }
       }

       if(strcmp(extension, &extension2[1]) == 0)
       {
          gimp_progress_init (_("renaming frames..."));
          l_rc = p_rename_frames(first_frame, last_frame, basename, extension);
       }
       else
       {
          gimp_progress_init (_("converting frames..."));
          l_rc = p_convert_frames(first_frame, last_frame, basename, extension, extension2);
       }

       if (l_input_dir_created_by_myself)
       {
	 if (strcmp(l_dst_dir, global_xanim_input_dir) != 0)
	 {
            /* remove input dir with all files */
            sprintf(l_cmd, "rm -rf \"%s\"", global_xanim_input_dir);
            system(l_cmd);         
	 }
       }
       g_free(l_dst_dir);
       gimp_progress_update (1.0);
     }
     
   }

   if(l_rc != 0)
   {
      if(global_errlist == NULL)
      {
         p_xanim_info("ERROR: could not execute xanim");
      }
      else
      {
         p_xanim_info(global_errlist);
      }
      l_rc = -1;
   }
   else
   {
     if(autoload)
     {
        /* load first frame and add a display */
        p_build_gap_framename(l_first_to_laod, first_frame, basename, &extension2[1]);
        l_rc = p_load_image(l_first_to_laod);

	if(l_rc >= 0) gimp_display_new(l_rc);
     }
   }

   return(l_rc);    
}	/* end  gap_xanim_decode */
