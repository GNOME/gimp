/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Daniel Risacher
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

/* bzip2 plug-in for the gimp */
/* this is almost exactly the same as the gz(ip) plugin by */
/* Dan Risacher & Josh, so feel free to go look there. */
/* GZ plugin adapted to BZ2 by Adam. I've left all other */
/* Error checking added by srn. */
/* credits intact since it was only a super-wussy mod. */

/* This is reads and writes bzip2ed image files for the Gimp
 *
 * You need to have bzip2 installed for it to work.
 *
 * It should work with file names of the form
 * filename.foo.bz2 where foo is some already-recognized extension
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "libgimp/gimp.h"


static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);
static gint32 load_image (char *filename, gint32 run_mode);

static gint save_image (char   *filename,
			gint32  image_ID,
			gint32  drawable_ID,
			gint32 run_mode);

static int valid_file (char* filename) ;
static char* find_extension (char* filename) ;

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MAIN ()

static void
query ()
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name entered" },
  };

  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };

  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" }
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_bz2_load",
                          "loads files compressed with bzip2",
                          "You need to have bzip2 installed.",
                          "Daniel Risacher",
                          "Daniel Risacher, Spencer Kimball and Peter Mattis",
                          "1995-1997",
                          "<Load>/bzip2",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_bz2_save",
                          "saves files compressed with bzip2",
                          "You need to have bzip2 installed",
                          "Daniel Risacher",
                          "Daniel Risacher, Spencer Kimball and Peter Mattis",
                          "1995-1997",
                          "<Save>/bzip2",
			  "RGB*, GRAY*, INDEXED*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_load_handler ("file_bz2_load", "xcf.bz2,bz2,xcfbz2", "");
  gimp_register_save_handler ("file_bz2_save", "xcf.bz2,bz2,xcfbz2", "");

}


static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gint32 image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_bz2_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string,
			     param[0].data.d_int32);
      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[0].data.d_status = STATUS_SUCCESS;
	  values[1].type = PARAM_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  values[0].data.d_status = STATUS_EXECUTION_ERROR;
	  g_assert (FALSE);
	}
    }
  else if (strcmp (name, "file_bz2_save") == 0)
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  break;
	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 4)
	    status = STATUS_CALLING_ERROR;

	case RUN_WITH_LAST_VALS:
	  break;

	default:
	  break;
	}

      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string,
		      param[1].data.d_int32,
		      param[2].data.d_int32,
		      param[0].data.d_int32 ))
	{
	  values[0].data.d_status = STATUS_SUCCESS;
	}
      else
	values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }
  else
    g_assert (FALSE);
}

static gint
save_image (char   *filename,
	    gint32  image_ID,
	    gint32  drawable_ID,
	    gint32  run_mode)
{
  FILE* f;
  GParam* params;
  gint retvals;
  char* ext;
  char* tmpname;
  int pid;
  int status;

  if (NULL == (ext = find_extension(filename))) return -1;

  /* get a temp name with the right extension and save into it. */

  params = gimp_run_procedure ("gimp_temp_name",
			       &retvals,
			       PARAM_STRING, ext + 1,
			       PARAM_END);

  tmpname = params[1].data.d_string;

  params = gimp_run_procedure ("gimp_file_save",
			       &retvals,
 			       PARAM_INT32, run_mode,
			       PARAM_IMAGE, image_ID,
			       PARAM_DRAWABLE, drawable_ID,
			       PARAM_STRING, tmpname,
			       PARAM_STRING, tmpname,
			       PARAM_END);

  if (params[0].data.d_status == FALSE || !valid_file(tmpname)) {
    unlink (tmpname);
    return -1;
  }

/*   if (! file_save(image_ID, tmpname, tmpname)) { */
/*     unlink (tmpname); */
/*     return -1; */
/*   } */

  /* fork off a bzip2 process */
  if ((pid = fork()) < 0)
    {
      g_message ("bz2: fork failed: %s\n", g_strerror(errno));
      return -1;
    }
  else if (pid == 0)
    {

      if (!(f = fopen(filename,"w"))){
	      g_message("bz2: fopen failed: %s\n", g_strerror(errno));
	      _exit(127);
      }

      /* make stdout for this process be the output file */
      if (-1 == dup2(fileno(f),fileno(stdout)))
	g_message ("bz2: dup2 failed: %s\n", g_strerror(errno));

      /* and bzip2 into it */
      execlp ("bzip2", "bzip2", "-cf", tmpname, NULL);
      g_message ("bz2: exec failed: bzip2: %s\n", g_strerror(errno));
      _exit(127);
    }
  else
    {
      waitpid (pid, &status, 0);

      if (!WIFEXITED(status) ||
	  WEXITSTATUS(status) != 0)
	{
	  g_message ("bz2: bzip2 exited abnormally on file %s\n", tmpname);
	  return 0;
	}
    }

  unlink (tmpname);

  return TRUE;
}

static gint32
load_image (char *filename, gint32 run_mode)
{
  GParam* params;
  gint retvals;
  char* ext;
  char* tmpname;
  int pid;
  int status;

  if (NULL == (ext = find_extension(filename))) return -1;

  /* find a temp name */
  params = gimp_run_procedure ("gimp_temp_name",
			       &retvals,
			       PARAM_STRING, ext + 1,
			       PARAM_END);

  tmpname = params[1].data.d_string;

  /* fork off a g(un)zip and wait for it */
  if ((pid = fork()) < 0)
    {
      g_message ("bz2: fork failed: %s\n", g_strerror(errno));
      return -1;
    }
  else if (pid == 0)  /* child process */
    {
      FILE* f;
       if (!(f = fopen(tmpname,"w"))){
	      g_message("bz2: fopen failed: %s\n", g_strerror(errno));
	      _exit(127);
      }

      /* make stdout for this child process be the temp file */
      if (-1 == dup2(fileno(f),fileno(stdout)))
	g_message ("bz2: dup2 failed: %s\n", g_strerror(errno));

      /* and unzip into it */
      execlp ("bzip2", "bzip2", "-cfd", filename, NULL);
      g_message ("bz2: exec failed: bunzip2: %s\n", g_strerror(errno));
      _exit(127);
    }
  else  /* parent process */
    {
      waitpid (pid, &status, 0);

      if (!WIFEXITED(status) ||
	  WEXITSTATUS(status) != 0)
	{
	  g_message ("bz2: bzip2 exited abnormally on file %s\n", filename);
	  return -1;
	}
    }

  /* now that we un-bzip2ed it, load the temp file */

  params = gimp_run_procedure ("gimp_file_load",
			       &retvals,
			       PARAM_INT32, run_mode,
			       PARAM_STRING, tmpname,
			       PARAM_STRING, tmpname,
			       PARAM_END);

  unlink (tmpname);

  if (params[0].data.d_status == FALSE)
    return -1;
  else
    {
      gimp_image_set_filename (params[1].data.d_int32, filename);
      return params[1].data.d_int32;
    }
}


static int valid_file (char* filename)
{
  int stat_res;
  struct stat buf;

  stat_res = stat(filename, &buf);

  if ((0 == stat_res) && (buf.st_size > 0))
    return 1;
  else
    return 0;
}

static char* find_extension (char* filename)
{
  char* filename_copy;
  char* ext;

  /* we never free this copy - aren't we evil! */
  filename_copy = malloc(strlen(filename)+1);
  strcpy(filename_copy, filename);

  /* find the extension, boy! */
  ext = strrchr (filename_copy, '.');

  while (1) {
    if (!ext || ext[1] == 0 || strchr(ext, '/'))
      {
	g_message ("bz2: can't open bzip2ed file without a sensible extension\n");
	return NULL;
      }
    if (0 == strcmp(ext, ".xcfbz2")) {
      return ".xcf";  /* we've found it */
    }
    if (0 != strcmp(ext,".bz2")) {
      return ext;
    } else {
      /* we found ".bz2" so strip it, loop back, and look again */
      *ext = 0;
      ext = strrchr (filename_copy, '.');
    }
  }
}
