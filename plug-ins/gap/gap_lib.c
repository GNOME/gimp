/* gap_lib.c
 * 1997.11.18 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * basic anim functions:
 *   Delete, Duplicate, Exchange, Shift 
 *   Next, Prev, First, Last, Goto
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
 * 1.1.9a;  1999/09/14   hof: handle frame filenames with framenumbers
 *                            that are not the 4digit style. (like frame1.xcf)
 * 1.1.8a;  1999/08/31   hof: for AnimFrame Filtypes != XCF:
 *                            p_decide_save_as does save INTERACTIVE at 1.st time
 *                            and uses RUN_WITH_LAST_VALS for subsequent calls
 *                            (this enables to set Fileformat specific save-Parameters
 *                            at least at the 1.st call, using the save dialog
 *                            of the selected (by gimp_file_save) file_save procedure.
 *                            in NONINTERACTIVE mode we have no access to
 *                            the Fileformat specific save-Parameters
 *          1999/07/22   hof: accept anim framenames without underscore '_'
 *                            (suggested by Samuel Meder)
 * 0.99.00; 1999/03/15   hof: prepared for win/dos filename conventions
 * 0.98.00; 1998/11/30   hof: started Port to GIMP 1.1:
 *                               exchange of Images (by next frame) is now handled in the
 *                               new module: gap_exchange_image.c
 * 0.96.02; 1998/07/30   hof: extended gap_dup (duplicate range instead of singele frame)
 *                            added gap_shift
 * 0.96.00               hof: - now using gap_arr_dialog.h
 * 0.95.00               hof:  increased duplicate frames limit from 50 to 99
 * 0.93.01               hof: fixup bug when frames are not in the current directory
 * 0.90.00;              hof: 1.st (pre) release
 */
#include "config.h"
 
/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

/* GIMP includes */
#include "gtk/gtk.h"
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
#include "gap_layer_copy.h"
#include "gap_lib.h"
#include "gap_arr_dialog.h"
#include "gap_mov_dialog.h"
#include "gap_exchange_image.h"

extern      int gap_debug; /* ==0  ... dont print debug infos */

static char g_errtxt[1024];  /* buffer for current message- or errortext */
 
/* ------------------------------------------ */
/* forward  working procedures */
/* ------------------------------------------ */

static int          p_save_old_frame(t_anim_info *ainfo_ptr);

static int   p_rename_frame(t_anim_info *ainfo_ptr, long from_nr, long to_nr);
static int   p_delete_frame(t_anim_info *ainfo_ptr, long nr);
static int   p_del(t_anim_info *ainfo_ptr, long cnt);
static int   p_decide_save_as(gint32 image_id, char *sav_name);

/* ============================================================================
 * p_strdup_*_underscore
 *   duplicate string and if last char is no underscore add one at end.
 *   duplicate string and delete last char if it is the underscore
 * ============================================================================
 */
char *
p_strdup_add_underscore(char *name)
{
  int   l_len;
  char *l_str;
  if(name == NULL)
  {
    return(g_strdup("\0"));
  }

  l_len = strlen(name);
  l_str = g_malloc(l_len+1);
  strcpy(l_str, name);
  if(l_len > 0)
  {
    if (name[l_len-1] != '_')
    {
       l_str[l_len    ] = '_';
       l_str[l_len +1 ] = '\0';
    }
      
  }
  return(l_str);
}

char *
p_strdup_del_underscore(char *name)
{
  int   l_len;
  char *l_str;
  if(name == NULL)
  {
    return(g_strdup("\0"));
  }

  l_len = strlen(name);
  l_str = g_strdup(name);
  if(l_len > 0)
  {
    if (l_str[l_len-1] == '_')
    {
       l_str[l_len -1 ] = '\0';
    }
      
  }
  return(l_str);
}

/* ============================================================================
 * p_msg_win
 *   print a message both to stderr
 *   and to a gimp info window with OK button (only when run INTERACTIVE)
 * ============================================================================
 */

void p_msg_win(GRunModeType run_mode, char *msg)
{
  static t_but_arg  l_argv[1];
  int               l_argc;  
  
  l_argv[0].but_txt  = "CANCEL";
  l_argv[0].but_val  = 0;
  l_argc             = 1;

  if(msg)
  {
    if(*msg)
    {
       fwrite(msg, 1, strlen(msg), stderr);
       fputc('\n', stderr);

       if(run_mode == RUN_INTERACTIVE) p_buttons_dialog  ("GAP Message", msg, l_argc, l_argv, -1);
    }
  }
}    /* end  p_msg_win */

/* ============================================================================
 * p_file_exists
 *
 * return 0  ... file does not exist, or is not accessable by user,
 *               or is empty,
 *               or is not a regular file
 *        1  ... file exists
 * ============================================================================
 */
int p_file_exists(char *fname)
{
  struct stat  l_stat_buf;
  long         l_len;

  /* File Laenge ermitteln */
  if (0 != stat(fname, &l_stat_buf))
  {
    /* stat error (file does not exist) */
    return(0);
  }
  
  if(!S_ISREG(l_stat_buf.st_mode))
  {
    return(0);
  }
  
  l_len = (long)l_stat_buf.st_size;
  if(l_len < 1)
  {
    /* File is empty*/
    return(0);
  }
  
  return(1);
}	/* end p_file_exists */

/* ============================================================================
 * p_file_copy
 * ============================================================================
 */
int p_file_copy(char *fname, char *fname_copy)
{
  FILE	      *l_fp;
  char                     *l_buffer;
  struct stat               l_stat_buf;
  long	       l_len;


  if(gap_debug) printf("p_file_copy src:%s dst:%s\n", fname, fname_copy);

  /* File Laenge ermitteln */
  if (0 != stat(fname, &l_stat_buf))
  {
    fprintf (stderr, "stat error on '%s'\n", fname);
    return -1;
  }
  l_len = (long)l_stat_buf.st_size;

  /* Buffer allocate */
  l_buffer=(char *)g_malloc0((size_t)l_len+1);
  if(l_buffer == NULL)
  {
    fprintf(stderr, "file_copy: calloc error (%ld Bytes not available)\n", l_len);
    return -1;
  }

  /* load File into Buffer */
  l_fp = fopen(fname, "r");		    /* open read */
  if(l_fp == NULL)
  {
    fprintf (stderr, "open(read) error on '%s'\n", fname);
    g_free(l_buffer);
    return -1;
  }
  fread(l_buffer, 1, (size_t)l_len, l_fp);
  fclose(l_fp);
  
  l_fp = fopen(fname_copy, "w");		    /* open write */
  if(l_fp == NULL)
  {
    fprintf (stderr, "file_copy: open(write) error on '%s' \n", fname_copy);
    g_free(l_buffer);
    return -1;
  }

  if(l_len > 0)
  {
     fwrite(l_buffer,  l_len, 1, l_fp);
  }

  fclose(l_fp);
  g_free(l_buffer);
  return 0;           /* all done OK */
}	/* end p_file_copy */


/* ============================================================================
 * p_delete_frame
 * ============================================================================
 */
int p_delete_frame(t_anim_info *ainfo_ptr, long nr)
{
   char          *l_fname;
   int            l_rc;
   
   l_fname = p_alloc_fname(ainfo_ptr->basename, nr, ainfo_ptr->extension);
   if(l_fname == NULL) { return(1); }
     
   if(gap_debug) fprintf(stderr, "\nDEBUG p_delete_frame: %s\n", l_fname);
   l_rc = remove(l_fname);
   
   g_free(l_fname);
   
   return(l_rc);
   
}    /* end p_delete_frame */

/* ============================================================================
 * p_rename_frame
 * ============================================================================
 */
int p_rename_frame(t_anim_info *ainfo_ptr, long from_nr, long to_nr)
{
   char          *l_from_fname;
   char          *l_to_fname;
   int            l_rc;
   
   l_from_fname = p_alloc_fname(ainfo_ptr->basename, from_nr, ainfo_ptr->extension);
   if(l_from_fname == NULL) { return(1); }
   
   l_to_fname = p_alloc_fname(ainfo_ptr->basename, to_nr, ainfo_ptr->extension);
   if(l_to_fname == NULL) { g_free(l_from_fname); return(1); }
   
     
   if(gap_debug) fprintf(stderr, "\nDEBUG p_rename_frame: %s ..to.. %s\n", l_from_fname, l_to_fname);
   l_rc = rename(l_from_fname, l_to_fname);
   
   g_free(l_from_fname);
   g_free(l_to_fname);
   
   return(l_rc);
   
}    /* end p_rename_frame */


/* ============================================================================
 * p_alloc_basename
 *
 * build the basename from an images name
 * Extension and trailing digits ("0000.xcf") are cut off.
 * return name or NULL (if malloc fails)
 * Output: number contains the integer representation of the stripped off
 *         number String. (or 0 if there was none)
 * ============================================================================
 */
char*  p_alloc_basename(char *imagename, long *number)
{
  char *l_fname;
  char *l_ptr;
  long  l_idx;

  *number = 0;
  if(imagename == NULL) return (NULL);
  l_fname = (char *)g_malloc(strlen(imagename) + 1);
  if(l_fname == NULL)   return (NULL);

  /* copy from imagename */
  strcpy(l_fname, imagename);

  if(gap_debug) fprintf(stderr, "DEBUG p_alloc_basename  source: '%s'\n", l_fname);
  /* cut off extension */
  l_ptr = &l_fname[strlen(l_fname)];
  while(l_ptr != l_fname)
  {
    if((*l_ptr == G_DIR_SEPARATOR) || (*l_ptr == DIR_ROOT))  { break; }  /* dont run into dir part */
    if(*l_ptr == '.')                                        { *l_ptr = '\0'; break; }
    l_ptr--;
  }
  if(gap_debug) fprintf(stderr, "DEBUG p_alloc_basename (ext_off): '%s'\n", l_fname);

  /* cut off trailing digits (0000) */
  l_ptr = &l_fname[strlen(l_fname)];
  if(l_ptr != l_fname) l_ptr--;
  l_idx = 1;
  while(l_ptr != l_fname)
  {
    if((*l_ptr >= '0') && (*l_ptr <= '9'))
    { 
      *number += ((*l_ptr - '0') * l_idx);
       l_idx = l_idx * 10;
      *l_ptr = '\0'; 
       l_ptr--; 
    }
    else
    {
      /* do not cut off underscore any longer */
      /*
       * if(*l_ptr == '_')
       * { 
       *    *l_ptr = '\0';
       * }
       */
       break;
    }
  }
  
  if(gap_debug) fprintf(stderr, "DEBUG p_alloc_basename  result:'%s'\n", l_fname);

  return(l_fname);
   
}    /* end p_alloc_basename */



/* ============================================================================
 * p_alloc_extension
 *
 * input: a filename
 * returns: a copy of the filename extension (incl "." )
 *          if the filename has no extension, a pointer to
 *          an empty string is returned ("\0")
 *          NULL if allocate mem for extension failed.
 * ============================================================================
 */
char*  p_alloc_extension(char *imagename)
{
  int   l_exlen;
  char *l_ext;
  char *l_ptr;
  
  l_exlen = 0;
  l_ptr = &imagename[strlen(imagename)];
  while(l_ptr != imagename)
  {
    if((*l_ptr == G_DIR_SEPARATOR) || (*l_ptr == DIR_ROOT))  { break; }     /* dont run into dir part */
    if(*l_ptr == '.')                                        { l_exlen = strlen(l_ptr); break; }
    l_ptr--;
  }
  
  l_ext = g_malloc0((size_t)(l_exlen + 1));
  if(l_ext == NULL)
      return (NULL);
  
  
  if(l_exlen > 0)
     strcpy(l_ext, l_ptr);
     
  return(l_ext);
}


/* ============================================================================
 * p_alloc_fname
 *
 * build the name of a frame using "basename_0000.ext"
 * 
 * return name or NULL (if malloc fails)
 * ============================================================================
 */
char*  p_alloc_fname(char *basename, long nr, char *extension)
{
  char *l_fname;
  int   l_leading_zeroes;
  long  l_nr_chk;
  
  if(basename == NULL) return (NULL);
  l_fname = (char *)g_malloc(strlen(basename)  + strlen(extension) + 8);
  if(l_fname != NULL)
  {
    l_leading_zeroes = TRUE;
    if(nr < 1000)
    {
       /* try to figure out if the frame numbers are in
        * 4-digit style, with leading zeroes  "frame_0001.xcf"
        * or not                              "frame_1.xcf"
        */
       l_nr_chk = nr;

       while(l_nr_chk >= 0)
       {
         /* check if frame is on disk with 4-digit style framenumber */
         sprintf(l_fname, "%s%04ld%s", basename, l_nr_chk, extension);
         if (p_file_exists(l_fname))
         {
            break;
         }

         /* now check for filename without leading zeroes in the framenumber */
         sprintf(l_fname, "%s%ld%s", basename, l_nr_chk, extension);
         if (p_file_exists(l_fname))
         {
            l_leading_zeroes = FALSE;
            break;
         }
         l_nr_chk--;
         
         /* if the frames nr and nr-1  were not found
          * try to check frames 1 and 0
          * to limit down the loop to max 4 cycles
          */
         if((l_nr_chk == nr -2) && (l_nr_chk > 1))
         {
           l_nr_chk = 1;
         }
      }
    }
    else
    {
      l_leading_zeroes = FALSE;
    }

    if(l_leading_zeroes)  sprintf(l_fname, "%s%04ld%s", basename, nr, extension);
    else                  sprintf(l_fname, "%s%ld%s", basename, nr, extension);
  }
  return(l_fname);
}    /* end p_alloc_fname */




/* ============================================================================
 * p_alloc_ainfo
 *
 * allocate and init an ainfo structure from the given image.
 * ============================================================================
 */
t_anim_info *p_alloc_ainfo(gint32 image_id, GRunModeType run_mode)
{
   t_anim_info   *l_ainfo_ptr;

   l_ainfo_ptr = (t_anim_info*)g_malloc(sizeof(t_anim_info));
   if(l_ainfo_ptr == NULL) return(NULL);
   
   l_ainfo_ptr->basename = NULL;
   l_ainfo_ptr->new_filename = NULL;
   l_ainfo_ptr->extension = NULL;
   l_ainfo_ptr->image_id = image_id;
 
   /* get current gimp images name  */   
   l_ainfo_ptr->old_filename = gimp_image_get_filename(image_id);
   if(l_ainfo_ptr->old_filename == NULL)
   {
     l_ainfo_ptr->old_filename = g_malloc(30);
     sprintf(l_ainfo_ptr->old_filename, "frame_0001.xcf");    /* assign a defaultname */
     gimp_image_set_filename (image_id, l_ainfo_ptr->old_filename);
   }

   l_ainfo_ptr->basename = p_alloc_basename(l_ainfo_ptr->old_filename, &l_ainfo_ptr->frame_nr);
   if(NULL == l_ainfo_ptr->basename)
   {
       p_free_ainfo(&l_ainfo_ptr);
       return(NULL);
   }

   l_ainfo_ptr->extension = p_alloc_extension(l_ainfo_ptr->old_filename);

   l_ainfo_ptr->curr_frame_nr = l_ainfo_ptr->frame_nr;
   l_ainfo_ptr->first_frame_nr = -1;
   l_ainfo_ptr->last_frame_nr = -1;
   l_ainfo_ptr->frame_cnt = 0;
   l_ainfo_ptr->run_mode = run_mode;


   return(l_ainfo_ptr);
 

}    /* end p_init_ainfo */

/* ============================================================================
 * p_dir_ainfo
 *
 * fill in more items into the given ainfo structure.
 * - first_frame_nr
 * - last_frame_nr
 * - frame_cnt
 *
 * to get this information, the directory entries have to be checked
 * ============================================================================
 */
int p_dir_ainfo(t_anim_info *ainfo_ptr)
{
   char          *l_dirname;
   char          *l_dirname_ptr;
   char          *l_ptr;
   char          *l_exptr;
   char          *l_dummy;
   /* int            l_cmp_len;   */
   DIR           *l_dirp;
   struct dirent *l_dp;
   long           l_nr;
   long           l_maxnr;
   long           l_minnr;
   short          l_dirflag;
   char           dirname_buff[1024];

   l_dirname = g_malloc(strlen(ainfo_ptr->basename) +1);
   if(l_dirname == NULL)
     return -1;

   ainfo_ptr->frame_cnt = 0;
   l_dirp = NULL;
   l_minnr = 99999999;
   l_maxnr = 0;
   strcpy(l_dirname, ainfo_ptr->basename);

   l_ptr = &l_dirname[strlen(l_dirname)];
   while(l_ptr != l_dirname)
   {
      if ((*l_ptr == G_DIR_SEPARATOR) || (*l_ptr == DIR_ROOT))
      { 
        *l_ptr = '\0';   /* split the string into dirpath 0 basename */
        l_ptr++;
        break;           /* stop at char behind the slash */
      }
      l_ptr--;
   }
   /* l_cmp_len = strlen(l_ptr); */

   if(gap_debug) fprintf(stderr, "DEBUG p_dir_ainfo: BASENAME:%s\n", l_ptr);
   
   if      (l_ptr == l_dirname)   { l_dirname_ptr = ".";  l_dirflag = 0; }
   else if (*l_dirname == '\0')   { l_dirname_ptr = G_DIR_SEPARATOR_S ; l_dirflag = 1; }
   else                           { l_dirname_ptr = l_dirname; l_dirflag = 2; }

   if(gap_debug) fprintf(stderr, "DEBUG p_dir_ainfo: DIRNAME:%s\n", l_dirname_ptr);
   l_dirp = opendir( l_dirname_ptr );  
   
   if(!l_dirp) fprintf(stderr, "ERROR p_dir_ainfo: cant read directory %s\n", l_dirname_ptr);
   else
   {
     while ( (l_dp = readdir( l_dirp )) != NULL )
     {

       /* if(gap_debug) fprintf(stderr, "DEBUG p_dir_ainfo: l_dp->d_name:%s\n", l_dp->d_name); */
       
       
       /* findout extension of the directory entry name */
       l_exptr = &l_dp->d_name[strlen(l_dp->d_name)];
       while(l_exptr != l_dp->d_name)
       {
         if(*l_exptr == G_DIR_SEPARATOR) { break; }                 /* dont run into dir part */
         if(*l_exptr == '.')       { break; }
         l_exptr--;
       }
       /* l_exptr now points to the "." of the direntry (or to its begin if has no ext) */
       /* now check for equal extension */
       if((*l_exptr == '.') && (0 == strcmp(l_exptr, ainfo_ptr->extension)))
       {
         /* build full pathname (to check if file exists) */
         switch(l_dirflag)
         {
           case 0:
            sprintf(dirname_buff, "%s", l_dp->d_name);
            break;
           case 1:
            sprintf(dirname_buff, "%c%s",  G_DIR_SEPARATOR, l_dp->d_name);
            break;
           default:
            /* UNIX:  "/dir/file"
             * DOS:   "drv:\dir\file"
             */
            sprintf(dirname_buff, "%s%c%s", l_dirname_ptr,  G_DIR_SEPARATOR,  l_dp->d_name);
            break;
         }
         
         if(1 == p_file_exists(dirname_buff)) /* check for regular file */
         {
           /* get basename and frame number of the directory entry */
           l_dummy = p_alloc_basename(l_dp->d_name, &l_nr);
           if(l_dummy != NULL)
           { 
               /* check for files, with equal basename (frames)
                * (length must be greater than basepart+extension
                * because of the frame_nr part "0000")
                */
               if((0 == strcmp(l_ptr, l_dummy))
               && ( strlen(l_dp->d_name) > strlen(l_dummy) + strlen(l_exptr)  ))
               {
                 ainfo_ptr->frame_cnt++;


                 if(gap_debug) fprintf(stderr, "DEBUG p_dir_ainfo:  %s NR=%ld\n", l_dp->d_name, l_nr);

                 if (l_nr > l_maxnr)
                    l_maxnr = l_nr;
                 if (l_nr < l_minnr)
                    l_minnr = l_nr;
               }

               g_free(l_dummy);
           }
         }
       }
     }
     closedir( l_dirp );
   }

  g_free(l_dirname);

  /* set first_frame_nr and last_frame_nr (found as "_0099" in diskfile namepart) */
  ainfo_ptr->last_frame_nr = l_maxnr;
  ainfo_ptr->first_frame_nr = l_minnr;

  return 0;           /* OK */
}	/* end p_dir_ainfo */
 

/* ============================================================================
 * p_free_ainfo
 *
 * free ainfo and its allocated stuff
 * ============================================================================
 */

void p_free_ainfo(t_anim_info **ainfo)
{
  t_anim_info *aptr;
  
  if((aptr = *ainfo) == NULL)
     return;
  
  if(aptr->basename)
     g_free(aptr->basename);
  
  if(aptr->extension)
     g_free(aptr->extension);
   
  if(aptr->new_filename)
     g_free(aptr->new_filename);

  if(aptr->old_filename)
     g_free(aptr->old_filename);
     
  g_free(aptr);
}


/* ============================================================================
 * p_chk_framechange
 *
 * check if the current frame nr has changed. 
 * useful after dialogs, because the user may have renamed the current image
 * (using save as)
 * while the gap-plugin's dialog was open.
 * return: 0 .. OK
 *        -1 .. Changed (or error occured)
 * ============================================================================
 */
int p_chk_framechange(t_anim_info *ainfo_ptr)
{
  int l_rc;
  t_anim_info *l_ainfo_ptr;
 
  l_rc = -1;
  l_ainfo_ptr = p_alloc_ainfo(ainfo_ptr->image_id, ainfo_ptr->run_mode);
  if(l_ainfo_ptr != NULL)
  {
    if(ainfo_ptr->curr_frame_nr == l_ainfo_ptr->curr_frame_nr )
    { 
       l_rc = 0;
    }
    else
    {
       p_msg_win(ainfo_ptr->run_mode,
         "OPERATION CANCELLED\n(current frame changed while dialog was open)\n");
    }
    p_free_ainfo(&l_ainfo_ptr);
  }

  return l_rc;   
}	/* end p_chk_framechange */

/* ============================================================================
 * p_chk_framerange
 *
 * check ainfo struct if there is a framerange (of at least one frame)
 * return: 0 .. OK
 *        -1 .. No frames there (print error)
 * ============================================================================
 */
int p_chk_framerange(t_anim_info *ainfo_ptr)
{
  if(ainfo_ptr->frame_cnt == 0)
  {
     p_msg_win(ainfo_ptr->run_mode,
       "OPERATION CANCELLED\nGAP-plugins works only with filenames\nthat ends with _0001.xcf\n==> rename your image, then try again");
     return -1;
  }
  return 0;
}	/* end p_chk_framerange */

/* ============================================================================
 * p_gzip
 *   gzip or gunzip the file to a temporary file.
 *   zip == "zip"    compress
 *   zip == "unzip"  decompress
 *   return a pointer to the temporary created (by gzip) file.
 *          NULL  in case of errors
 * ============================================================================
 */
char * p_gzip (char *orig_name, char *new_name, char *zip)
{
  char*   l_cmd;
  char*   l_tmpname;
  int     l_rc, l_rc2;

  if(zip == NULL) return NULL;
  
  l_cmd = NULL;
  l_tmpname = new_name;
  
  l_cmd = g_malloc((strlen(l_tmpname) + strlen(orig_name) + 20));
  if(l_cmd == NULL)
  {
    return NULL;
  }

  /* used gzip options:
   *     -c --stdout --to-stdout
   *          Write  output  on  standard  output
   *     -d --decompress --uncompress
   *          Decompress.
   *     -f --force
   *           Force compression or decompression even if the file
   */

  if(*zip == 'u')
  {
    sprintf(l_cmd, "gzip -cfd <\"%s\"  >\"%s\"", orig_name, l_tmpname);
  }
  else
  {
    sprintf(l_cmd, "gzip -cf  <\"%s\"  >\"%s\"", orig_name, l_tmpname);
  }

  if(gap_debug) fprintf(stderr, "system: %s\n", l_cmd);
  
  l_rc = system(l_cmd);
  if(l_rc != 0)
  {
     /* Shift 8 Bits gets Retcode of the executed Program */
     l_rc2 = l_rc >> 8;
     fprintf(stderr, "ERROR system: %s\nreturncodes %d %d", l_cmd, l_rc, l_rc2);
     l_tmpname = NULL;
  }
  g_free(l_cmd);
  return l_tmpname;
  
}	/* end p_gzip */

/* ============================================================================
 * p_decide_save_as
 *   decide what to to, when attempt to save a frame in any image format 
 *  (other than xcf)
 *   Let the user decide if the frame is to save "as it is" or "flattened"
 *   ("as it is" will save only the backround layer in most fileformat types)
 *   The SAVE_AS_MODE is stored , and reused
 *   (without displaying the dialog) on subsequent calls.
 *
 *   return -1  ... CANCEL (do not save)
 *           0  ... save the image (may be flattened)
 * ============================================================================
 */
int p_decide_save_as(gint32 image_id, char *sav_name)
{
  static char *l_msg;


  static char l_save_as_name[80];
  
  static t_but_arg  l_argv[3];
  int               l_argc;  
  int               l_save_as_mode;
  GRunModeType      l_run_mode;  

  l_msg = _("You are using a fileformat != xcf\nSave Operations may result\nin loss of layerinformation");
  /* check if there are SAVE_AS_MODE settings (from privious calls within one gimp session) */
  l_save_as_mode = -1;
  /* sprintf(l_save_as_name, "plug_in_gap_plugins_SAVE_AS_MODE_%d", (int)image_id);*/
  sprintf(l_save_as_name, "plug_in_gap_plugins_SAVE_AS_MODE");
  gimp_get_data (l_save_as_name, &l_save_as_mode);

  if(l_save_as_mode == -1)
  {
    /* no defined value found (this is the 1.st call for this image_id)
     * ask what to do with a 3 Button dialog
     */
    l_argv[0].but_txt  = _("CANCEL");
    l_argv[0].but_val  = -1;
    l_argv[1].but_txt  = _("SAVE Flattened");
    l_argv[1].but_val  = 1;
    l_argv[2].but_txt  = _("SAVE As Is");
    l_argv[2].but_val  = 0;
    l_argc             = 3;

    l_save_as_mode =  p_buttons_dialog  (_("GAP Question"), l_msg, l_argc, l_argv, -1);
    
    if(gap_debug) fprintf(stderr, "DEBUG: decide SAVE_AS_MODE %d\n", (int)l_save_as_mode);
    
    if(l_save_as_mode < 0) return -1;
    l_run_mode = RUN_INTERACTIVE;
  }
  else
  {
    l_run_mode = RUN_WITH_LAST_VALS;
  }

  gimp_set_data (l_save_as_name, &l_save_as_mode, sizeof(l_save_as_mode));
   
  
  if(l_save_as_mode == 1)
  {
      gimp_image_flatten (image_id);
  }

  return(p_save_named_image(image_id, sav_name, l_run_mode));
}	/* end p_decide_save_as */



/* ============================================================================
 * p_save_named_image
 * ============================================================================
 */
gint32 p_save_named_image(gint32 image_id, char *sav_name, GRunModeType run_mode)
{
  GDrawable  *l_drawable;
  gint        l_nlayers;
  gint32     *l_layers_list;
  GParam     *l_params;
  gint        l_retvals;

  if(gap_debug) fprintf(stderr, "DEBUG: before   p_save_named_image: '%s'\n", sav_name);

  l_layers_list = gimp_image_get_layers(image_id, &l_nlayers);
  if(l_layers_list == NULL)
     return -1;

  l_drawable =  gimp_drawable_get(l_layers_list[l_nlayers -1]);  /* use the background layer */
  if(l_drawable == NULL)
  {
     fprintf(stderr, "ERROR: p_save_named_image gimp_drawable_get failed '%s' nlayers=%d\n",
                     sav_name, (int)l_nlayers);
     g_free (l_layers_list);
     return -1;
  }
  
  l_params = gimp_run_procedure ("gimp_file_save",
			       &l_retvals,
			       PARAM_INT32,    run_mode,
			       PARAM_IMAGE,    image_id,
			       PARAM_DRAWABLE, l_drawable->id,
			       PARAM_STRING, sav_name,
			       PARAM_STRING, sav_name, /* raw name ? */
			       PARAM_END);

  if(gap_debug) fprintf(stderr, "DEBUG: after    p_save_named_image: '%s' nlayers=%d image=%d drw=%d run_mode=%d\n", sav_name, (int)l_nlayers, (int)image_id, (int)l_drawable->id, (int)run_mode);

  g_free (l_layers_list);
  g_free (l_drawable);


  if (l_params[0].data.d_status == FALSE)
  {
    fprintf(stderr, "ERROR: p_save_named_image  gimp_file_save failed '%s'\n", sav_name);
    g_free(l_params);
    return -1;
  }
  else
  {
    g_free(l_params);
    return image_id;
  }
   
}	/* end p_save_named_image */



/* ============================================================================
 * p_save_named_frame
 *  save frame into temporary image,
 *  on success rename it to desired framename.
 *  (this is done, to avoid corrupted frames on disk in case of
 *   crash in one of the save procedures)
 * ============================================================================
 */
int p_save_named_frame(gint32 image_id, char *sav_name)
{
  GParam* l_params;
  char  *l_ext;
  char  *l_tmpname;
  gint   l_retvals;
  int    l_gzip;
  int    l_xcf;
  int    l_rc;

  l_tmpname = NULL;
  l_rc   = -1;
  l_gzip = 0;          /* dont zip */
  l_xcf  = 0;          /* assume no xcf format */
  
  /* check extension to decide if savd file will be zipped */      
  l_ext = p_alloc_extension(sav_name);
  if(l_ext == NULL)  return -1;
  
  if(0 == strcmp(l_ext, ".xcf"))
  { 
    l_xcf  = 1;
  }
  else if(0 == strcmp(l_ext, ".xcfgz"))
  { 
    l_gzip = 1;          /* zip it */
    l_xcf  = 1;
  }
  else if(0 == strcmp(l_ext, ".gz"))
  { 
    l_gzip = 1;          /* zip it */
  }

  /* find a temp name 
   * that resides on the same filesystem as sav_name
   * and has the same extension as the original sav_name 
   */
  l_tmpname = (char *)g_malloc(strlen(sav_name) + strlen(".gtmp") + strlen(l_ext) +2);
  if(l_tmpname == NULL)
  {
    return -1;
  }
  sprintf(l_tmpname, "%s.gtmp%s", sav_name, l_ext);
  if(1 == p_file_exists(l_tmpname))
  {
      /* FILE exists: let gimp find another temp name */
      l_params = gimp_run_procedure ("gimp_temp_name",
                                &l_retvals,
                                PARAM_STRING, 
                                &l_ext[1],
                                PARAM_END);

      if(l_params[1].data.d_string != NULL)
      {
         l_tmpname = l_params[1].data.d_string;
      }
      g_free(l_params);
  }

  g_free(l_ext);


   if(gap_debug)
   {
     l_ext = g_getenv("GAP_NO_SAVE");
     if(l_ext != NULL)
     {
       fprintf(stderr, "DEBUG: GAP_NO_SAVE is set: save is skipped: '%s'\n", l_tmpname);
       g_free(l_tmpname);  /* free if it was a temporary name */
       return 0;
     }
   }

  if(gap_debug) fprintf(stderr, "DEBUG: before   p_save_named_frame: '%s'\n", l_tmpname);

  if(l_xcf != 0)
  {
    /* save current frame as xcf image
     * xcf_save does operate on the complete image,
     * the drawable is ignored. (we can supply a dummy value)
     */
    l_params = gimp_run_procedure ("gimp_xcf_save",
			         &l_retvals,
			         PARAM_INT32,    RUN_NONINTERACTIVE,
			         PARAM_IMAGE,    image_id,
			         PARAM_DRAWABLE, 0,
			         PARAM_STRING, l_tmpname,
			         PARAM_STRING, l_tmpname, /* raw name ? */
			         PARAM_END);
    if(gap_debug) fprintf(stderr, "DEBUG: after   xcf  p_save_named_frame: '%s'\n", l_tmpname);

    if (l_params[0].data.d_status != FALSE)
    {
       l_rc = image_id;
    }
    g_free(l_params);
  }
  else
  {
     /* let gimp try to save (and detect filetype by extension)
      * Note: the most imagefileformats do not support multilayer
      *       images, and extra channels
      *       the result may not contain the whole imagedata
      *
      * To Do: Should warn the user at 1.st attempt to do this.
      */
      
     l_rc = p_decide_save_as(image_id, l_tmpname);
  } 

  if(l_rc < 0)
  {
     remove(l_tmpname);
     g_free(l_tmpname);  /* free temporary name */
     return l_rc;
  }

  if(l_gzip == 0)
  {
     /* remove sav_name, then rename tmpname ==> sav_name */
     remove(sav_name);
     if (0 != rename(l_tmpname, sav_name))
     {
        /* if tempname is located on another filesystem (errno == EXDEV)
         * rename will not work.
         * so lets try a  copy ; remove sequence
         */
         if(gap_debug) fprintf(stderr, "DEBUG: p_save_named_frame: RENAME 2nd try\n");
         if(0 == p_file_copy(l_tmpname, sav_name))  remove(l_tmpname);
         else
         {
            fprintf(stderr, "ERROR in p_save_named_frame: cant rename %s to %s\n",
                            l_tmpname, sav_name);
            return -1;
         }
     }
  }
  else
  {
    /* ZIP tmpname ==> sav_name */
    if(NULL != p_gzip(l_tmpname, sav_name, "zip"))
    {
       /* OK zip created compressed file named sav_name
        * now delete the uncompressed l_tempname
        */
       remove(l_tmpname);
    }
  }

  g_free(l_tmpname);  /* free temporary name */

  return l_rc;
   
}	/* end p_save_named_frame */


/* ============================================================================
 * p_save_old_frame
 * ============================================================================
 */
int p_save_old_frame(t_anim_info *ainfo_ptr)
{
  /* (here we should check if image has unsaved changes
   * before save)
   */ 
  if(1)
  {
    return (p_save_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename));
  }
  else
  {
    return -1;
  }
}	/* end p_save_old_frame */



/* ============================================================================
 * p_load_image
 * load image of any type by filename, and return its image id
 * (or -1 in case of errors)
 * ============================================================================
 */
gint32 p_load_image (char *lod_name)
{
  GParam* l_params;
  char  *l_ext;
  char  *l_tmpname;
  gint   l_retvals;
  gint32 l_tmp_image_id;
  int    l_rc;
  
  l_rc = 0;
  l_tmpname = NULL;
  l_ext = p_alloc_extension(lod_name);
  if(l_ext != NULL)
  {
    if((0 == strcmp(l_ext, ".xcfgz"))
    || (0 == strcmp(l_ext, ".gz")))
    { 

      /* find a temp name */
      l_params = gimp_run_procedure ("gimp_temp_name",
			           &l_retvals,
			           PARAM_STRING, 
			           &l_ext[1],           /* extension */
			           PARAM_END);

      if(l_params[1].data.d_string != NULL)
      {
         /* try to unzip file, before loading it */
         l_tmpname = p_gzip(lod_name, l_params[1].data.d_string, "unzip");
      }
 
      g_free(l_params);
    }
    else l_tmpname = lod_name;
    g_free(l_ext);
  }

  if(l_tmpname == NULL)
  {
    return -1;
  }


  if(gap_debug) fprintf(stderr, "DEBUG: before   p_load_image: '%s'\n", l_tmpname);

  l_params = gimp_run_procedure ("gimp_file_load",  /* "gimp_xcf_load" */
			       &l_retvals,
			       PARAM_INT32,  RUN_NONINTERACTIVE,
			       PARAM_STRING, l_tmpname,
			       PARAM_STRING, l_tmpname, /* raw name ? */
			       PARAM_END);

  l_tmp_image_id = l_params[1].data.d_int32;
  
  if(gap_debug) fprintf(stderr, "DEBUG: after    p_load_image: '%s' id=%d\n\n",
                               l_tmpname, (int)l_tmp_image_id);

  if(l_tmpname != lod_name)
  {
    remove(l_tmpname);
    g_free(l_tmpname);  /* free if it was a temporary name */
  }
  

  g_free(l_params);
  return l_tmp_image_id;

}	/* end p_load_image */



/* ============================================================================
 * p_load_named_frame
 * load new frame, replacing the existing image
 * file must be of same type and size
 * ============================================================================
 */
int p_load_named_frame (gint32 image_id, char *lod_name)
{
  gint32 l_tmp_image_id;
  int    l_rc;
  
  l_rc = 0;

  l_tmp_image_id = p_load_image(lod_name);
  
  if(gap_debug) fprintf(stderr, "DEBUG: after    p_load_named_frame: '%s' id=%d  new_id=%d\n\n",
                               lod_name, (int)image_id, (int)l_tmp_image_id);

  if(l_tmp_image_id < 0)
      return -1;


   /* replace image_id with the content of l_tmp_image_id */
   if(p_exchange_image (image_id, l_tmp_image_id) < 0)
   {
      /* in case of errors the image will be trashed */
      image_id = -1;
   }

   /* delete the temporary image (old content of the original image) */
   gimp_image_delete(l_tmp_image_id);

   /* use the original lod_name */
   gimp_image_set_filename (image_id, lod_name);
   return image_id;

}	/* end p_load_named_frame */



/* ============================================================================
 * p_replace_image
 *
 * make new_filename of next file to load, check if that file does exist on disk
 * then save current image and replace it, by loading the new_filename
 * ============================================================================
 */
int p_replace_image(t_anim_info *ainfo_ptr)
{
  if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
  ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                      ainfo_ptr->frame_nr,
                                      ainfo_ptr->extension);
  if(ainfo_ptr->new_filename == NULL)
     return -1;

  if(0 == p_file_exists(ainfo_ptr->new_filename ))
     return -1;

  if(p_save_old_frame(ainfo_ptr) <0)
  {
    return -1;
  }
  else
  {
    return (p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->new_filename));
  }
}	/* end p_replace_image */



/* ============================================================================
 * p_del
 *
 * delete cnt frames starting at current
 * all following frames are renamed (renumbered down by cnt) 
 * ============================================================================
 */
int p_del(t_anim_info *ainfo_ptr, long cnt)
{
   long  l_lo, l_hi, l_curr, l_idx;

   if(gap_debug) fprintf(stderr, "DEBUG  p_del\n");

   if(cnt < 1) return -1;

   l_curr =  ainfo_ptr->curr_frame_nr;
   if((1 + ainfo_ptr->last_frame_nr - l_curr) < cnt)
   {
      /* limt cnt to last existing frame */
      cnt = 1 + ainfo_ptr->frame_cnt - l_curr;
   }

   if(cnt >= ainfo_ptr->frame_cnt)
   {
      /* dont want to delete all frames
       * so we have to leave a rest of one frame
       */
      cnt--;
   }

   
   l_idx   = l_curr;
   while(l_idx < (l_curr + cnt))
   {
      p_delete_frame(ainfo_ptr, l_idx);
      l_idx++;
   }
   
   /* rename (renumber) all frames with number greater than current
    */
   l_lo   = l_curr;
   l_hi   = l_curr + cnt;
   while(l_hi <= ainfo_ptr->last_frame_nr)
   {
     if(0 != p_rename_frame(ainfo_ptr, l_hi, l_lo))
     {
        sprintf(g_errtxt, "Error: could not rename frame %ld to %ld\n"
               , l_hi, l_lo);
        p_msg_win(ainfo_ptr->run_mode, g_errtxt);
        return -1;
     }
     l_lo++;
     l_hi++;
   }

   /* calculate how much frames are left */
   ainfo_ptr->frame_cnt -= cnt;
   ainfo_ptr->last_frame_nr = ainfo_ptr->first_frame_nr + ainfo_ptr->frame_cnt -1;
   
   /* set current position to previous frame (if there is one) */
   if(l_curr > ainfo_ptr->first_frame_nr) 
   { 
     ainfo_ptr->frame_nr = l_curr -1;
   }
   else
   { 
      ainfo_ptr->frame_nr = ainfo_ptr->first_frame_nr;
   }

   /* make filename, then load the new current frame */
   if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
   ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                      ainfo_ptr->frame_nr,
                                      ainfo_ptr->extension);

   if(ainfo_ptr->new_filename != NULL)
      return (p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->new_filename));
   else
      return -1;
      
}        /* end p_del */

/* ============================================================================
 * p_dup
 *
 * all following frames are renamed (renumbered up by cnt) 
 * current frame is duplicated (cnt) times
 * ============================================================================
 */
int p_dup(t_anim_info *ainfo_ptr, long cnt, long range_from, long range_to)
{
   long  l_lo, l_hi;
   long  l_cnt2;
   long  l_step;
   long  l_src_nr, l_src_nr_min, l_src_nr_max;
   char  *l_dup_name;
   char  *l_curr_name;
   gdouble    l_percentage, l_percentage_step;  

   if(gap_debug) fprintf(stderr, "DEBUG  p_dup fr:%d to:%d cnt:%d\n",
                         (int)range_from, (int)range_to, (int)cnt);

   if(cnt < 1) return -1;

   l_curr_name = p_alloc_fname(ainfo_ptr->basename, ainfo_ptr->curr_frame_nr, ainfo_ptr->extension);
   /* save current frame  */   
   p_save_named_frame(ainfo_ptr->image_id, l_curr_name);

   /* use a new name (0001.xcf Konvention) */ 
   gimp_image_set_filename (ainfo_ptr->image_id, l_curr_name);
   g_free(l_curr_name);


   if((range_from <0 ) && (range_to < 0 ))
   {
      /* set range to one single current frame
       * (used for the old non_interactive PDB-interface without range params)
       */
      range_from = ainfo_ptr->curr_frame_nr;
      range_to   = ainfo_ptr->curr_frame_nr;
   }

   /* clip range */
   if(range_from > ainfo_ptr->last_frame_nr)  range_from = ainfo_ptr->last_frame_nr;
   if(range_to   > ainfo_ptr->last_frame_nr)  range_to   = ainfo_ptr->last_frame_nr;
   if(range_from < ainfo_ptr->first_frame_nr) range_from = ainfo_ptr->first_frame_nr;
   if(range_to   < ainfo_ptr->first_frame_nr) range_to   = ainfo_ptr->first_frame_nr;

   if(range_to < range_from)
   {
       /* invers range */
      l_cnt2 = ((range_from - range_to ) + 1) * cnt;
      l_step = -1;
      l_src_nr_max = range_from;
      l_src_nr_min = range_to;
   }
   else
   {
      l_cnt2 = ((range_to - range_from ) + 1) * cnt;  
      l_step = 1;
      l_src_nr_max = range_to;
      l_src_nr_min = range_from;
   }    

   if(gap_debug) fprintf(stderr, "DEBUG  p_dup fr:%d to:%d cnt2:%d l_src_nr_max:%d\n",
                         (int)range_from, (int)range_to, (int)l_cnt2, (int)l_src_nr_max);
   

   l_percentage = 0.0;  
   if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
   { 
     gimp_progress_init( _("Duplicating frames .."));
   }

   /* rename (renumber) all frames with number greater than current
    */
   l_lo   = ainfo_ptr->last_frame_nr;
   l_hi   = l_lo + l_cnt2;
   while(l_lo > l_src_nr_max)
   {
     if(0 != p_rename_frame(ainfo_ptr, l_lo, l_hi))
     {
        sprintf(g_errtxt, "Error: could not rename frame %ld to %ld\n", l_lo, l_hi);
        p_msg_win(ainfo_ptr->run_mode, g_errtxt);
        return -1;
     }
     l_lo--;
     l_hi--;
   }


   l_percentage_step = 1.0 / ((1.0 + l_hi) - l_src_nr_max);
   if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
   { 
      l_percentage += l_percentage_step;
      gimp_progress_update (l_percentage);
   }

   /* copy cnt duplicates */   
   l_src_nr = range_to;
   while(l_hi > l_src_nr_max)
   {
      l_curr_name = p_alloc_fname(ainfo_ptr->basename, l_src_nr, ainfo_ptr->extension);  
      l_dup_name = p_alloc_fname(ainfo_ptr->basename, l_hi, ainfo_ptr->extension);
      if((l_dup_name != NULL) && (l_curr_name != NULL))
      {
         p_file_copy(l_curr_name, l_dup_name);
         g_free(l_dup_name);
         g_free(l_curr_name);
      }
      if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
      { 
         l_percentage += l_percentage_step;
         gimp_progress_update (l_percentage);
      }
      
      
      l_src_nr -= l_step;
      if(l_src_nr < l_src_nr_min) l_src_nr = l_src_nr_max;
      if(l_src_nr > l_src_nr_max) l_src_nr = l_src_nr_min;
      
      l_hi--;
   }

   /* restore current position */
   ainfo_ptr->frame_cnt += l_cnt2;
   ainfo_ptr->last_frame_nr = ainfo_ptr->first_frame_nr + ainfo_ptr->frame_cnt -1;

   /* load from the "new" current frame */   
   if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
   ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                      ainfo_ptr->curr_frame_nr,
                                      ainfo_ptr->extension);
   return (p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->new_filename));
}        /* end p_dup */

/* ============================================================================
 * p_exchg
 *
 * save current frame, exchange its name with destination frame on disk 
 * and reload current frame (now has contents of dest. and vice versa)
 * ============================================================================
 */
int p_exchg(t_anim_info *ainfo_ptr, long dest)
{
   long  l_tmp_nr;

   l_tmp_nr = ainfo_ptr->last_frame_nr + 4;  /* use a free frame_nr for temp name */

   if((dest < 1) || (dest == ainfo_ptr->curr_frame_nr)) 
      return -1;

   if(p_save_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename) < 0)
      return -1;
   

   /* rename (renumber) frames */
   if(0 != p_rename_frame(ainfo_ptr, dest, l_tmp_nr))
   {
        sprintf(g_errtxt, "Error: could not rename frame %ld to %ld\n", dest, l_tmp_nr);
        p_msg_win(ainfo_ptr->run_mode, g_errtxt);
        return -1;
   }
   if(0 != p_rename_frame(ainfo_ptr, ainfo_ptr->curr_frame_nr, dest))
   {
        sprintf(g_errtxt, "Error: could not rename frame %ld to %ld\n", ainfo_ptr->curr_frame_nr, dest);
        p_msg_win(ainfo_ptr->run_mode, g_errtxt);
        return -1;
   }
   if(0 != p_rename_frame(ainfo_ptr, l_tmp_nr, ainfo_ptr->curr_frame_nr))
   {
        sprintf(g_errtxt, "Error: could not rename frame %ld to %ld\n", l_tmp_nr, ainfo_ptr->curr_frame_nr);
        p_msg_win(ainfo_ptr->run_mode, g_errtxt);
        return -1;
   }

   /* load from the "new" current frame */   
   if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
   ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                      ainfo_ptr->curr_frame_nr,
                                      ainfo_ptr->extension);
   return (p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->new_filename));

}        /* end p_exchg */

/* ============================================================================
 * p_shift
 *
 * all frmaes in the given range are renumbered (shifted)
 * according to cnt:
 *  example:  cnt == 1 :  range before 3, 4, 5, 6, 7
 *                        range after  4, 5, 6, 7, 3
 * ============================================================================
 */
static int
p_shift(t_anim_info *ainfo_ptr, long cnt, long range_from, long range_to)
{
   long  l_lo, l_hi, l_curr, l_dst;
   long  l_upper;
   long  l_shift;
   char  *l_curr_name;
   gdouble    l_percentage, l_percentage_step;  

   if(gap_debug) fprintf(stderr, "DEBUG  p_shift fr:%d to:%d cnt:%d\n",
                         (int)range_from, (int)range_to, (int)cnt);

   if(range_from == range_to) return -1;

   /* clip range */
   if(range_from > ainfo_ptr->last_frame_nr)  range_from = ainfo_ptr->last_frame_nr;
   if(range_to   > ainfo_ptr->last_frame_nr)  range_to   = ainfo_ptr->last_frame_nr;
   if(range_from < ainfo_ptr->first_frame_nr) range_from = ainfo_ptr->first_frame_nr;
   if(range_to   < ainfo_ptr->first_frame_nr) range_to   = ainfo_ptr->first_frame_nr;

   if(range_to < range_from)
   {
      l_lo = range_to;
      l_hi = range_from;
   }
   else
   {
      l_lo = range_from;
      l_hi = range_to;
   }
   
   /* limit shift  amount to number of frames in range */
   l_shift = cnt % (l_hi - l_lo);
   if(gap_debug) fprintf(stderr, "DEBUG  p_shift shift:%d\n",
                         (int)l_shift);
   if(l_shift == 0) return -1;

   l_curr_name = p_alloc_fname(ainfo_ptr->basename, ainfo_ptr->curr_frame_nr, ainfo_ptr->extension);
   /* save current frame  */   
   p_save_named_frame(ainfo_ptr->image_id, l_curr_name);
   g_free(l_curr_name);

   l_percentage = 0.0;  
   if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
   { 
     gimp_progress_init( _("Renumber Framesequence .."));
   }

   /* rename (renumber) all frames (using high numbers)
    */

   l_upper = ainfo_ptr->last_frame_nr +100;
   l_percentage_step = 0.5 / ((1.0 + l_lo) - l_hi);
   for(l_curr = l_lo; l_curr <= l_hi; l_curr++)
   {
     if(0 != p_rename_frame(ainfo_ptr, l_curr, l_curr + l_upper))
     {
        sprintf(g_errtxt, "Error: could not rename frame %ld to %ld\n", l_lo, l_hi);
        p_msg_win(ainfo_ptr->run_mode, g_errtxt);
        return -1;
     }
     if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
     { 
       l_percentage += l_percentage_step;
       gimp_progress_update (l_percentage);
     }
   }

   /* rename (renumber) all frames (using desied destination numbers)
    */
   l_dst = l_lo + l_shift;
   if (l_dst > l_hi) { l_dst -= (l_lo -1); }
   if (l_dst < l_lo) { l_dst += ((l_hi - l_lo) +1); }
   for(l_curr = l_upper + l_lo; l_curr <= l_upper + l_hi; l_curr++)
   {
     if (l_dst > l_hi) { l_dst = l_lo; }
     if(0 != p_rename_frame(ainfo_ptr, l_curr, l_dst))
     {
        sprintf(g_errtxt, "Error: could not rename frame %ld to %ld\n", l_lo, l_hi);
        p_msg_win(ainfo_ptr->run_mode, g_errtxt);
        return -1;
     }
     if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
     { 
       l_percentage += l_percentage_step;
       gimp_progress_update (l_percentage);
     }

     l_dst ++;     
   }


   /* load from the "new" current frame */   
   if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
   ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                      ainfo_ptr->curr_frame_nr,
                                      ainfo_ptr->extension);
   return (p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->new_filename));
}  /* end p_shift */





/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */ 
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */ 
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */ 


/* ============================================================================
 * gap_next gap_prev
 *
 * store the current Gimp Image to the current anim Frame
 * and load it from the next/prev anim Frame on disk.
 * ============================================================================
 */
int gap_next(GRunModeType run_mode, gint32 image_id)
{
  int rc;
  t_anim_info *ainfo_ptr;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    ainfo_ptr->frame_nr = ainfo_ptr->curr_frame_nr + 1;
    rc = p_replace_image(ainfo_ptr);
  
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    
}	/* end gap_next */

int gap_prev(GRunModeType run_mode, gint32 image_id)
{
  int rc;
  t_anim_info *ainfo_ptr;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    ainfo_ptr->frame_nr = ainfo_ptr->curr_frame_nr - 1;
    rc = p_replace_image(ainfo_ptr);
  
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    
}	/* end gap_prev */

/* ============================================================================
 * gap_first  gap_last
 *
 * store the current Gimp Image to the current anim Frame
 * and load it from the first/last anim Frame on disk.
 * ============================================================================
 */

int gap_first(GRunModeType run_mode, gint32 image_id)
{
  int rc;
  t_anim_info *ainfo_ptr;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      ainfo_ptr->frame_nr = ainfo_ptr->first_frame_nr;
      rc = p_replace_image(ainfo_ptr);
    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    
}	/* end gap_first */

int gap_last(GRunModeType run_mode, gint32 image_id)
{
  int rc;
  t_anim_info *ainfo_ptr;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      ainfo_ptr->frame_nr = ainfo_ptr->last_frame_nr;
      rc = p_replace_image(ainfo_ptr);
    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    
}	/* end gap_last */

/* ============================================================================
 * gap_goto
 * 
 * store the current Gimp Image to disk
 * and load it from the anim Frame on disk that has the specified frame Nr.
 * RUN_INTERACTIVE:
 *    show dialogwindow where user can enter the destination frame Nr.
 * ============================================================================
 */

int gap_goto(GRunModeType run_mode, gint32 image_id, int nr)
{
  int rc;
  t_anim_info *ainfo_ptr;

  long           l_dest;
  char           l_hline[50];
  char           l_title[50];

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(0 != p_chk_framerange(ainfo_ptr))   return -1;

      if(run_mode == RUN_INTERACTIVE)
      {
        sprintf(l_title, _("Goto Frame (%ld/%ld)")
                , ainfo_ptr->curr_frame_nr
                , ainfo_ptr->frame_cnt);
        sprintf(l_hline, _("Destination Frame Number (%ld  - %ld)")
                , ainfo_ptr->first_frame_nr
                , ainfo_ptr->last_frame_nr);

        l_dest = p_slider_dialog(l_title, l_hline, _("Number :"), NULL
                , ainfo_ptr->first_frame_nr
                , ainfo_ptr->last_frame_nr
                , ainfo_ptr->curr_frame_nr
                , TRUE);
                
        if(l_dest < 0)
        {
           /* Cancel button: go back to current frame */
           l_dest = ainfo_ptr->curr_frame_nr;
        }  
        if(0 != p_chk_framechange(ainfo_ptr))
        {
           l_dest = -1;
        }
      }
      else
      {
        l_dest = nr;
      }

      if(l_dest >= 0)
      {
        ainfo_ptr->frame_nr = l_dest;
        rc = p_replace_image(ainfo_ptr);
      }

    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    
}	/* end gap_goto */

/* ============================================================================
 * gap_del
 * ============================================================================
 */
int gap_del(GRunModeType run_mode, gint32 image_id, int nr)
{
  int rc;
  t_anim_info *ainfo_ptr;

  long           l_cnt;
  long           l_max;
  char           l_hline[50];
  char           l_title[50];

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(0 != p_chk_framerange(ainfo_ptr))   return -1;
      
      if(run_mode == RUN_INTERACTIVE)
      {
        sprintf(l_title, _("Delete Frames (%ld/%ld)")
                , ainfo_ptr->curr_frame_nr
                , ainfo_ptr->frame_cnt);
        sprintf(l_hline, _("Delete Frames from %ld to (Number)")
                , ainfo_ptr->curr_frame_nr);

        l_max = ainfo_ptr->last_frame_nr;
        if(l_max == ainfo_ptr->curr_frame_nr)
        { 
          /* bugfix: the slider needs a maximum > minimum
           *         (if not an error is printed, and
           *          a default range 0 to 100 is displayed)
           */
          l_max++;
        }
        
        l_cnt = p_slider_dialog(l_title, l_hline, _("Number :"), NULL
              , ainfo_ptr->curr_frame_nr
              , l_max
              , ainfo_ptr->curr_frame_nr
              , TRUE);
                
        if(l_cnt >= 0)
        {
           l_cnt = 1 + l_cnt - ainfo_ptr->curr_frame_nr;
        } 
        if(0 != p_chk_framechange(ainfo_ptr))
        {
           l_cnt = -1;
        }
      }
      else
      {
        l_cnt = nr;
      }
 
      if(l_cnt >= 0)
      {
         /* delete l_cnt number of frames (-1 CANCEL button) */

         rc = p_del(ainfo_ptr, l_cnt);
      }


    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    

}	/* end gap_del */


/* ============================================================================
 * p_dup_dialog
 *
 * ============================================================================
 */
int p_dup_dialog(t_anim_info *ainfo_ptr, long *range_from, long *range_to)
{
  static t_arr_arg  argv[3];
  char           l_title[50];

  sprintf(l_title, _("Duplicate Frames (%ld/%ld)")
          , ainfo_ptr->curr_frame_nr
          , ainfo_ptr->frame_cnt);

  p_init_arr_arg(&argv[0], WGT_INT_PAIR);
  argv[0].label_txt = _("From :");
  argv[0].constraint = TRUE;
  argv[0].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[0].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[0].int_ret   = (gint)ainfo_ptr->curr_frame_nr;
  argv[0].help_txt  = _("Source Range starts at this framenumber");

  p_init_arr_arg(&argv[1], WGT_INT_PAIR);
  argv[1].label_txt = _("To :");
  argv[1].constraint = TRUE;
  argv[1].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[1].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[1].int_ret   = (gint)ainfo_ptr->curr_frame_nr;
  argv[1].help_txt  = _("Source Range ends at this framenumber");
    
  p_init_arr_arg(&argv[2], WGT_INT_PAIR);
  argv[2].label_txt = _("N-Times :");
  argv[2].constraint = FALSE;
  argv[2].int_min   = 0;
  argv[2].int_max   = 99;
  argv[2].int_ret   = 1;
  argv[2].help_txt  = _("Copy selected Range n-times  \n(you may type in Values > 99)");
  
  if(TRUE == p_array_dialog(l_title, _("Duplicate Framerange"),  3, argv))
  {   *range_from = (long)(argv[0].int_ret);
      *range_to   = (long)(argv[1].int_ret);
       return (int)(argv[2].int_ret);
  }
  else
  {
     return -1;
  }
   

}	/* end p_dup_dialog */


/* ============================================================================
 * gap_dup
 * ============================================================================
 */
int gap_dup(GRunModeType run_mode, gint32 image_id, int nr,
            long range_from, long range_to)
{
  int rc;
  t_anim_info *ainfo_ptr;

  long           l_cnt, l_from, l_to;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(run_mode == RUN_INTERACTIVE)
      {
         if(0 != p_chk_framechange(ainfo_ptr)) { l_cnt = -1; }
         else { l_cnt = p_dup_dialog(ainfo_ptr, &l_from, &l_to); }

         if((0 != p_chk_framechange(ainfo_ptr)) || (l_cnt < 1))
         {
            l_cnt = -1;
         }
                
      }
      else
      {
        l_cnt  = nr;
        l_from = range_from;
        l_to   = range_to;
      }
 
      if(l_cnt > 0)
      {
         /* make l_cnt duplicate frames (on disk) */
         rc = p_dup(ainfo_ptr, l_cnt, l_from, l_to);
      }


    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    

 
}	/* end gap_dup */


/* ============================================================================
 * gap_exchg
 * ============================================================================
 */

int gap_exchg(GRunModeType run_mode, gint32 image_id, int nr)
{
  int rc;
  t_anim_info *ainfo_ptr;

  long           l_dest;
  long           l_initial;
  char           l_hline[50];
  char           l_title[50];

  rc = -1;
  l_initial = 1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(0 != p_chk_framerange(ainfo_ptr))   return -1;
      
      if(run_mode == RUN_INTERACTIVE)
      {
         if(ainfo_ptr->curr_frame_nr < ainfo_ptr->last_frame_nr)
         {
           l_initial = ainfo_ptr->curr_frame_nr + 1;
         }
         else
         {
           l_initial = ainfo_ptr->last_frame_nr; 
         }
         sprintf(l_title, _("Exchange current Frame (%ld)")
                 , ainfo_ptr->curr_frame_nr);
         sprintf(l_hline, _("With Frame (Number)"));

         l_dest = p_slider_dialog(l_title, l_hline, _("Number :"), NULL
              , ainfo_ptr->first_frame_nr 
              , ainfo_ptr->last_frame_nr
              , l_initial
              , TRUE);
                
         if(0 != p_chk_framechange(ainfo_ptr))
         {
            l_dest = -1;
         }
      }
      else
      {
        l_dest = nr;
      }
 
      if((l_dest >= ainfo_ptr->first_frame_nr ) && (l_dest <= ainfo_ptr->last_frame_nr ))
      {
         /* excange current frames with destination frame (on disk) */
         rc = p_exchg(ainfo_ptr, l_dest);
      }


    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    
}	/* end gap_exchg */

/* ============================================================================
 * p_shift_dialog
 *
 * ============================================================================
 */
int p_shift_dialog(t_anim_info *ainfo_ptr, long *range_from, long *range_to)
{
  static t_arr_arg  argv[3];
  char           l_title[50];

  sprintf(l_title, _("Framesequence Shift (%ld/%ld)")
          , ainfo_ptr->curr_frame_nr
          , ainfo_ptr->frame_cnt);

  p_init_arr_arg(&argv[0], WGT_INT_PAIR);
  argv[0].label_txt = _("From :");
  argv[0].constraint = TRUE;
  argv[0].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[0].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[0].int_ret   = (gint)ainfo_ptr->curr_frame_nr;
  argv[0].help_txt  = _("Affected Range starts at this framenumber");

  p_init_arr_arg(&argv[1], WGT_INT_PAIR);
  argv[1].label_txt = _("To :");
  argv[1].constraint = TRUE;
  argv[1].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[1].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[1].int_ret   = (gint)ainfo_ptr->last_frame_nr;
  argv[1].help_txt  = _("Affected Range ends at this framenumber");
    
  p_init_arr_arg(&argv[2], WGT_INT_PAIR);
  argv[2].label_txt = _("N-Shift :");
  argv[2].constraint = FALSE;
  argv[2].int_min   = -1 * (gint)ainfo_ptr->last_frame_nr;
  argv[2].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[2].int_ret   = 1;
  argv[2].help_txt  = _("Renumber the affected framesequence     \n(numbers are shifted in circle by N)");
  
  if(TRUE == p_array_dialog(l_title, _("Framesequence shift"),  3, argv))
  {   *range_from = (long)(argv[0].int_ret);
      *range_to   = (long)(argv[1].int_ret);
       return (int)(argv[2].int_ret);
  }
  else
  {
     return 0;
  }
   

}	/* end p_shift_dialog */


/* ============================================================================
 * gap_shift
 * ============================================================================
 */
int gap_shift(GRunModeType run_mode, gint32 image_id, int nr,
            long range_from, long range_to)
{
  int rc;
  t_anim_info *ainfo_ptr;

  long           l_cnt, l_from, l_to;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(run_mode == RUN_INTERACTIVE)
      {
         l_cnt = 1;
         if(0 != p_chk_framechange(ainfo_ptr)) { l_cnt = 0; }
         else { l_cnt = p_shift_dialog(ainfo_ptr, &l_from, &l_to); }

         if((0 != p_chk_framechange(ainfo_ptr)) || (l_cnt == 0))
         {
            l_cnt = 0;
         }
                
      }
      else
      {
        l_cnt  = nr;
        l_from = range_from;
        l_to   = range_to;
      }
 
      if(l_cnt != 0)
      {
         /* shift framesquence by l_cnt frames 
          * (rename all frames in the given range on disk)
          */
         rc = p_shift(ainfo_ptr, l_cnt, l_from, l_to);
      }


    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    

 
}	/* end gap_shift */

