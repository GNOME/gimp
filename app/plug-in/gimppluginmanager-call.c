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

#include "config.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <gtk/gtk.h>

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
#define STRICT
#include <windows.h>
#include <process.h>

#ifdef G_OS_WIN32
#include <fcntl.h>
#include <io.h>
#endif

#ifdef G_WITH_CYGWIN
#define O_TEXT		0x0100	/* text file */
#define _O_TEXT		0x0100	/* text file */
#define O_BINARY	0x0200	/* binary file */
#define _O_BINARY	0x0200	/* binary file */
#endif

#endif

#ifdef __EMX__
#include <fcntl.h>
#include <process.h>
#define _O_BINARY O_BINARY
#define _P_NOWAIT P_NOWAIT
#endif

#ifdef HAVE_IPC_H
#include <sys/ipc.h>
#endif

#ifdef HAVE_SHM_H
#include <sys/shm.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "plug-in-types.h"

#include "base/tile.h"
#include "base/tile-manager.h"

#include "config/gimpcoreconfig.h"
#include "config/gimpconfig-path.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpenvirontable.h"
#include "core/gimpimage.h"

#include "gui/brush-select.h"
#include "gui/gradient-select.h"
#include "gui/palette-select.h"
#include "gui/pattern-select.h"

#include "plug-in.h"
#include "plug-ins.h"
#include "plug-in-def.h"
#include "plug-in-params.h"
#include "plug-in-proc.h"
#include "plug-in-progress.h"
#include "plug-in-rc.h"

#include "libgimp/gimpintl.h"


typedef struct _PlugInBlocked PlugInBlocked;

struct _PlugInBlocked
{
  PlugIn *plug_in;
  gchar  *proc_name;
};


typedef struct _PlugInHelpPathDef PlugInHelpPathDef;

struct _PlugInHelpPathDef
{
  gchar *prog_name;
  gchar *help_path;
};


static gboolean plug_in_write             (GIOChannel	     *channel,
					   guint8            *buf,
				           gulong             count,
                                           gpointer           user_data);
static gboolean plug_in_flush             (GIOChannel        *channel,
                                           gpointer           user_data);
static void     plug_in_push              (PlugIn            *plug_in);
static void     plug_in_pop               (void);
static gboolean plug_in_recv_message	  (GIOChannel	     *channel,
					   GIOCondition	      cond,
					   gpointer	      data);
static void plug_in_handle_message        (PlugIn            *plug_in,
                                           WireMessage       *msg);
static void plug_in_handle_quit           (PlugIn            *plug_in);
static void plug_in_handle_tile_req       (PlugIn            *plug_in,
                                           GPTileReq         *tile_req);
static void plug_in_handle_proc_run       (PlugIn            *plug_in,
                                           GPProcRun         *proc_run);
static void plug_in_handle_proc_return    (PlugIn            *plug_in,
                                           GPProcReturn      *proc_return);
static void plug_in_handle_proc_install   (PlugIn            *plug_in,
                                           GPProcInstall     *proc_install);
static void plug_in_handle_proc_uninstall (PlugIn            *plug_in,
                                           GPProcUninstall   *proc_uninstall);
static void plug_in_handle_extension_ack  (PlugIn            *plug_in);
static void plug_in_handle_has_init       (PlugIn            *plug_in);

static Argument * plug_in_temp_run       (ProcRecord         *proc_rec,
					  Argument           *args,
					  gint                argc);
static void       plug_in_init_shm       (void);

static gchar    * plug_in_search_in_path (gchar              *search_path,
					  gchar              *filename);

static void       plug_in_prep_for_exec  (gpointer            data);


PlugIn     *current_plug_in    = NULL;
ProcRecord *last_plug_in       = NULL;

static GSList     *open_plug_ins    = NULL;
static GSList     *blocked_plug_ins = NULL;

static GSList     *plug_in_stack              = NULL;
static Argument   *current_return_vals        = NULL;
static gint        current_return_nvals       = 0;

static gint    shm_ID   = -1;
static guchar *shm_addr = NULL;

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
static HANDLE shm_handle;
#endif


static void
plug_in_init_shm (void)
{
  /* allocate a piece of shared memory for use in transporting tiles
   *  to plug-ins. if we can't allocate a piece of shared memory then
   *  we'll fall back on sending the data over the pipe.
   */
  
#ifdef HAVE_SHM_H
  shm_ID = shmget (IPC_PRIVATE,
                   TILE_WIDTH * TILE_HEIGHT * 4,
                   IPC_CREAT | 0600);

  if (shm_ID == -1)
    g_message ("shmget() failed: Disabling shared memory tile transport.");
  else
    {
      shm_addr = (guchar *) shmat (shm_ID, NULL, 0);
      if (shm_addr == (guchar *) -1)
	{
	  g_message ("shmat() failed: Disabling shared memory tile transport.");
	  shmctl (shm_ID, IPC_RMID, NULL);
	  shm_ID = -1;
	}
      
#ifdef	IPC_RMID_DEFERRED_RELEASE
      if (shm_addr != (guchar *) -1)
	shmctl (shm_ID, IPC_RMID, NULL);
#endif
    }
#else
#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
  /* Use Win32 shared memory mechanisms for
   * transfering tile data.
   */
  gint  pid;
  gchar fileMapName[MAX_PATH];
  gint  tileByteSize = TILE_WIDTH * TILE_HEIGHT * 4;
  
  /* Our shared memory id will be our process ID */
  pid = GetCurrentProcessId ();
  
  /* From the id, derive the file map name */
  g_snprintf (fileMapName, sizeof (fileMapName), "GIMP%d.SHM", pid);

  /* Create the file mapping into paging space */
  shm_handle = CreateFileMapping ((HANDLE) 0xFFFFFFFF, NULL,
				  PAGE_READWRITE, 0,
				  tileByteSize, fileMapName);
  
  if (shm_handle)
    {
      /* Map the shared memory into our address space for use */
      shm_addr = (guchar *) MapViewOfFile(shm_handle,
					  FILE_MAP_ALL_ACCESS,
					  0, 0, tileByteSize);
      
      /* Verify that we mapped our view */
      if (shm_addr)
	shm_ID = pid;
      else
	{
	  g_warning ("MapViewOfFile error: %d... disabling shared memory transport\n", GetLastError());
	}
    }
  else
    {
      g_warning ("CreateFileMapping error: %d... disabling shared memory transport\n", GetLastError());
    }
#endif
#endif
}

void
plug_in_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /* initialize the gimp protocol library and set the read and
   *  write handlers.
   */
  gp_init ();
  wire_set_writer (plug_in_write);
  wire_set_flusher (plug_in_flush);

  /* allocate a piece of shared memory for use in transporting tiles
   *  to plug-ins. if we can't allocate a piece of shared memory then
   *  we'll fall back on sending the data over the pipe.
   */
  if (gimp->use_shm)
    plug_in_init_shm ();
}

void
plug_in_kill (Gimp *gimp)
{
  GSList *tmp;
  PlugIn *plug_in;
  
#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
  CloseHandle (shm_handle);
#else
#ifdef HAVE_SHM_H
#ifndef	IPC_RMID_DEFERRED_RELEASE
  if (shm_ID != -1)
    {
      shmdt ((gchar *) shm_addr);
      shmctl (shm_ID, IPC_RMID, NULL);
    }
#else	/* IPC_RMID_DEFERRED_RELEASE */
  if (shm_ID != -1)
    shmdt ((gchar *) shm_addr);
#endif
#endif
#endif
 
  tmp = open_plug_ins;
  while (tmp)
    {
      plug_in = tmp->data;
      tmp = tmp->next;

      plug_in_destroy (plug_in);
    }
}

void
plug_in_call_query (Gimp      *gimp,
                    PlugInDef *plug_in_def)
{
  PlugIn      *plug_in;
  WireMessage  msg;

  plug_in = plug_in_new (gimp, plug_in_def->prog);

  if (plug_in)
    {
      plug_in->query       = TRUE;
      plug_in->synchronous = TRUE;
      plug_in->user_data   = plug_in_def;

      if (plug_in_open (plug_in))
	{
	  plug_in_push (plug_in);

	  while (plug_in->open)
	    {
	      if (! wire_read_msg (plug_in->my_read, &msg, plug_in))
                {
                  plug_in_close (plug_in, TRUE);
                }
	      else 
		{
		  plug_in_handle_message (plug_in, &msg);
		  wire_destroy (&msg);
		}
	    }

	  plug_in_pop ();
	  plug_in_destroy (plug_in);
	}
    }
}

void
plug_in_call_init (Gimp      *gimp,
                   PlugInDef *plug_in_def)
{
  PlugIn      *plug_in;
  WireMessage  msg;

  plug_in = plug_in_new (gimp, plug_in_def->prog);

  if (plug_in)
    {
      plug_in->init        = TRUE;
      plug_in->synchronous = TRUE;
      plug_in->user_data   = plug_in_def;

      if (plug_in_open (plug_in))
	{
	  plug_in_push (plug_in);

	  while (plug_in->open)
	    {
	      if (! wire_read_msg (plug_in->my_read, &msg, plug_in))
                {
                  plug_in_close (plug_in, TRUE);
                }
	      else 
		{
		  plug_in_handle_message (plug_in, &msg);
		  wire_destroy (&msg);
		}
	    }

	  plug_in_pop ();
	  plug_in_destroy (plug_in);
	}
    }
}

PlugIn *
plug_in_new (Gimp  *gimp,
             gchar *name)
{
  PlugIn *plug_in;
  gchar  *path;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (! g_path_is_absolute (name))
    {
      gchar *plug_in_path;

      plug_in_path = gimp_config_path_expand (gimp->config->plug_in_path,
                                              FALSE, NULL); 
      path = plug_in_search_in_path (plug_in_path, name);
      g_free (plug_in_path);

      if (! path)
	{
	  g_message (_("Unable to locate Plug-In: \"%s\""), name);
	  return NULL;
	}
    }
  else
    {
      path = name;
    }

  plug_in = g_new0 (PlugIn, 1);

  plug_in->gimp               = gimp;

  plug_in->open               = FALSE;
  plug_in->query              = FALSE;
  plug_in->init               = FALSE;
  plug_in->synchronous        = FALSE;
  plug_in->recurse            = FALSE;
  plug_in->busy               = FALSE;
  plug_in->pid                = 0;
  plug_in->args[0]            = g_strdup (path);
  plug_in->args[1]            = g_strdup ("-gimp");
  plug_in->args[2]            = NULL;
  plug_in->args[3]            = NULL;
  plug_in->args[4]            = NULL;
  plug_in->args[5]            = NULL;
  plug_in->args[6]            = NULL;
  plug_in->my_read            = NULL;
  plug_in->my_write           = NULL;
  plug_in->his_read           = NULL;
  plug_in->his_write          = NULL;
  plug_in->input_id           = 0;
  plug_in->write_buffer_index = 0;
  plug_in->temp_proc_defs     = NULL;
  plug_in->progress           = NULL;
  plug_in->user_data          = NULL;

  return plug_in;
}

void
plug_in_destroy (PlugIn *plug_in)
{
  if (plug_in)
    {
      if (plug_in->open)
	plug_in_close (plug_in, TRUE);

      if (plug_in->args[0])
	g_free (plug_in->args[0]);
      if (plug_in->args[1])
	g_free (plug_in->args[1]);
      if (plug_in->args[2])
	g_free (plug_in->args[2]);
      if (plug_in->args[3])
	g_free (plug_in->args[3]);
      if (plug_in->args[4])
	g_free (plug_in->args[4]);
      if (plug_in->args[5])
	g_free (plug_in->args[5]);

      if (plug_in->progress)
	plug_in_progress_end (plug_in);

      if (plug_in == current_plug_in)
	plug_in_pop ();

      g_free (plug_in);
    }
}

static void
plug_in_prep_for_exec (gpointer data)
{
#if !defined(G_OS_WIN32) && !defined (G_WITH_CYGWIN) && !defined(__EMX__)
  PlugIn *plug_in = data;

  g_io_channel_unref (plug_in->my_read);
  plug_in->my_read  = NULL;

  g_io_channel_unref (plug_in->my_write);
  plug_in->my_write  = NULL;
#endif
}

gboolean
plug_in_open (PlugIn *plug_in)
{
  gint my_read[2];
  gint my_write[2];
  gchar **envp;
  GError *error = NULL;

  g_return_val_if_fail (plug_in != NULL, FALSE);

  if (plug_in)
    {
      /* Open two pipes. (Bidirectional communication).
       */
      if ((pipe (my_read) == -1) || (pipe (my_write) == -1))
	{
	  g_message ("pipe() failed: Unable to start Plug-In \"%s\"\n(%s)",
		     g_path_get_basename (plug_in->args[0]),
		     plug_in->args[0]);
	  return FALSE;
	}

#if defined(G_WITH_CYGWIN) || defined(__EMX__)
      /* Set to binary mode */
      setmode (my_read[0], _O_BINARY);
      setmode (my_write[0], _O_BINARY);
      setmode (my_read[1], _O_BINARY);
      setmode (my_write[1], _O_BINARY);
#endif

      plug_in->my_read   = g_io_channel_unix_new (my_read[0]);
      plug_in->my_write  = g_io_channel_unix_new (my_write[1]);
      plug_in->his_read  = g_io_channel_unix_new (my_write[0]);
      plug_in->his_write = g_io_channel_unix_new (my_read[1]);

      g_io_channel_set_encoding (plug_in->my_read, NULL, NULL);
      g_io_channel_set_encoding (plug_in->my_write, NULL, NULL);
      g_io_channel_set_encoding (plug_in->his_read, NULL, NULL);
      g_io_channel_set_encoding (plug_in->his_write, NULL, NULL);

      g_io_channel_set_buffered (plug_in->my_read, FALSE);
      g_io_channel_set_buffered (plug_in->my_write, FALSE);
      g_io_channel_set_buffered (plug_in->his_read, FALSE);
      g_io_channel_set_buffered (plug_in->his_write, FALSE);

      g_io_channel_set_close_on_unref (plug_in->my_read, TRUE);
      g_io_channel_set_close_on_unref (plug_in->my_write, TRUE);
      g_io_channel_set_close_on_unref (plug_in->his_read, TRUE);
      g_io_channel_set_close_on_unref (plug_in->his_write, TRUE);

      /* Remember the file descriptors for the pipes.
       */
      plug_in->args[2] =
	g_strdup_printf ("%d", g_io_channel_unix_get_fd (plug_in->his_read));
      plug_in->args[3] =
	g_strdup_printf ("%d", g_io_channel_unix_get_fd (plug_in->his_write));

      /* Set the rest of the command line arguments.
       * FIXME: this is ugly.  Pass in the mode as a separate argument?
       */
      if (plug_in->query)
	{
	  plug_in->args[4] = g_strdup ("-query");
	}
      else if (plug_in->init)
	{
	  plug_in->args[4] = g_strdup ("-init");
	}
      else  
	{
	  plug_in->args[4] = g_strdup ("-run");
	}

      plug_in->args[5] = g_strdup_printf ("%d", plug_in->gimp->stack_trace_mode);

#ifdef __EMX__
      fcntl (my_read[0], F_SETFD, 1);
      fcntl (my_write[1], F_SETFD, 1);
#endif

      /* Fork another process. We'll remember the process id
       *  so that we can later use it to kill the filter if
       *  necessary.
       */
      envp = gimp_environ_table_get_envp (plug_in->gimp->environ_table);
      if (! g_spawn_async (NULL, plug_in->args, envp,
                           G_SPAWN_LEAVE_DESCRIPTORS_OPEN |
                           G_SPAWN_DO_NOT_REAP_CHILD,
                           plug_in_prep_for_exec, plug_in,
                           &plug_in->pid,
                           &error))
	{
          g_message ("Unable to run Plug-In: \"%s\"\n(%s)\n%s",
		     g_path_get_basename (plug_in->args[0]),
		     plug_in->args[0],
		     error->message);
          g_error_free (error);

          plug_in_destroy (plug_in);
          return FALSE;
	}

      g_io_channel_unref (plug_in->his_read);
      plug_in->his_read  = NULL;

      g_io_channel_unref (plug_in->his_write);
      plug_in->his_write = NULL;

      if (!plug_in->synchronous)
	{
	  plug_in->input_id =
	    g_io_add_watch (plug_in->my_read,
			    G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
			    plug_in_recv_message,
			    plug_in);

	  open_plug_ins = g_slist_prepend (open_plug_ins, plug_in);
	}

      plug_in->open = TRUE;
      return TRUE;
    }

  return FALSE;
}

void
plug_in_close (PlugIn   *plug_in,
	       gboolean  kill_it)
{
  gint status;
#ifndef G_OS_WIN32
  struct timeval tv;
#endif

  g_return_if_fail (plug_in != NULL);
  g_return_if_fail (plug_in->open == TRUE);

  if (! plug_in->open)
    return;

  plug_in->open = FALSE;

  /*  Ask the filter to exit gracefully  */
  if (kill_it && plug_in->pid)
    {
      plug_in_push (plug_in);
      gp_quit_write (plug_in->my_write, plug_in);
      plug_in_pop ();

      /*  give the plug-in some time (10 ms)  */
#ifndef G_OS_WIN32
      tv.tv_sec  = 0;
      tv.tv_usec = 100;	/* But this is 0.1 ms? */
      select (0, NULL, NULL, NULL, &tv);
#else
      Sleep (10);
#endif
    }

  /* If necessary, kill the filter. */
#ifndef G_OS_WIN32
  if (kill_it && plug_in->pid)
    status = kill (plug_in->pid, SIGKILL);

  /* Wait for the process to exit. This will happen
   *  immediately if it was just killed.
   */
  if (plug_in->pid)
    waitpid (plug_in->pid, &status, 0);
#else
  if (kill_it && plug_in->pid)
    {
      /* Trying to avoid TerminateProcess (does mostly work).
       * Otherwise some of our needed DLLs may get into an unstable state
       * (see Win32 API docs).
       */
      DWORD dwExitCode = STILL_ACTIVE;
      DWORD dwTries  = 10;
      while ((STILL_ACTIVE == dwExitCode)
	     && GetExitCodeProcess((HANDLE) plug_in->pid, &dwExitCode)
	     && (dwTries > 0))
	{
	  Sleep(10);
	  dwTries--;
	}
      if (STILL_ACTIVE == dwExitCode)
	{
	  g_warning("Terminating %s ...", plug_in->args[0]);
	  TerminateProcess ((HANDLE) plug_in->pid, 0);
	}
    }
#endif

  plug_in->pid = 0;

  /* Remove the input handler. */
  if (plug_in->input_id)
    {
      g_source_remove (plug_in->input_id);
      plug_in->input_id = 0;
    }

  /* Close the pipes. */
  if (plug_in->my_read != NULL)
    {
      g_io_channel_unref (plug_in->my_read);
      plug_in->my_read = NULL;
    }
  if (plug_in->my_write != NULL)
    {
      g_io_channel_unref (plug_in->my_write);
      plug_in->my_write = NULL;
    }
  if (plug_in->his_read != NULL)
    {
      g_io_channel_unref (plug_in->his_read);
      plug_in->his_read = NULL;
    }
  if (plug_in->his_write != NULL)
    {
      g_io_channel_unref (plug_in->his_write);
      plug_in->his_write = NULL;
    }

  wire_clear_error ();

  if (plug_in->progress)
    plug_in_progress_end (plug_in);

  if (plug_in->recurse)
    {
      gimp_main_loop_quit (plug_in->gimp);

      plug_in->recurse = FALSE;
    }

  plug_in->synchronous = FALSE;

  /* Unregister any temporary procedures. */
  if (plug_in->temp_proc_defs)
    {
      g_slist_foreach (plug_in->temp_proc_defs,
		       (GFunc) plug_ins_proc_def_remove,
		       plug_in->gimp);
      g_slist_foreach (plug_in->temp_proc_defs,
                       (GFunc) g_free,
                       NULL);
      g_slist_free (plug_in->temp_proc_defs);
      plug_in->temp_proc_defs = NULL;
    }

  /* Close any dialogs that this plugin might have opened */
  brush_select_dialogs_check ();
  gradient_select_dialogs_check ();
  palette_select_dialogs_check ();
  pattern_select_dialogs_check ();

  open_plug_ins = g_slist_remove (open_plug_ins, plug_in);
}

static Argument *
plug_in_get_current_return_vals (ProcRecord *proc_rec)
{
  Argument *return_vals;
  gint      nargs;

  /* Return the status code plus the current return values. */
  nargs = proc_rec->num_values + 1;
  if (current_return_vals && current_return_nvals == nargs)
    {
      return_vals = current_return_vals;
    }
  else if (current_return_vals)
    {
      /* Allocate new return values of the correct size. */
      return_vals = procedural_db_return_args (proc_rec, FALSE);

      /* Copy all of the arguments we can. */
      memcpy (return_vals, current_return_vals,
	      sizeof (Argument) * MIN (current_return_nvals, nargs));

      /* Free the old argument pointer.  This will cause a memory leak
	 only if there were more values returned than we need (which
	 shouldn't ever happen). */
      g_free (current_return_vals);
    }
  else
    {
      /* Just return a dummy set of values. */
      return_vals = procedural_db_return_args (proc_rec, FALSE);
    }

  /* We have consumed any saved values, so clear them. */
  current_return_nvals = 0;
  current_return_vals  = NULL;

  return return_vals;
}

Argument *
plug_in_run (Gimp       *gimp,
             ProcRecord *proc_rec,
	     Argument   *args,
	     gint        argc,
	     gboolean    synchronous,   
	     gboolean    destroy_values,
	     gint        gdisp_ID)
{
  GPConfig   config;
  GPProcRun  proc_run;
  Argument  *return_vals;
  PlugIn    *plug_in;

  return_vals = NULL;

  if (proc_rec->proc_type == GIMP_TEMPORARY)
    {
      return_vals = plug_in_temp_run (proc_rec, args, argc);
      goto done;
    }

  plug_in = plug_in_new (gimp, proc_rec->exec_method.plug_in.filename);

  if (plug_in)
    {
      if (plug_in_open (plug_in))
	{
	  plug_in->recurse = synchronous;

	  plug_in_push (plug_in);

	  config.version      = GP_VERSION;
	  config.tile_width   = TILE_WIDTH;
	  config.tile_height  = TILE_HEIGHT;
	  config.shm_ID       = shm_ID;
	  config.gamma        = gimp->config->gamma_val;
	  config.install_cmap = gimp->config->install_cmap;
          config.unused       = 0;
          config.min_colors   = CLAMP (gimp->config->min_colors, 27, 256);
	  config.gdisp_ID     = gdisp_ID;

	  proc_run.name    = proc_rec->name;
	  proc_run.nparams = argc;
	  proc_run.params  = plug_in_args_to_params (args, argc, FALSE);

	  if (! gp_config_write (plug_in->my_write, &config, plug_in)     ||
	      ! gp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
	      ! wire_flush (plug_in->my_write, plug_in))
	    {
	      return_vals = procedural_db_return_args (proc_rec, FALSE);
	      goto done;
	    }

	  plug_in_pop ();

	  plug_in_params_destroy (proc_run.params, proc_run.nparams, FALSE);

	  /*  If this is an automatically installed extension, wait for an
	   *  installation-confirmation message
	   */
	  if ((proc_rec->proc_type == GIMP_EXTENSION) &&
	      (proc_rec->num_args == 0))
            {
              gimp_main_loop (gimp);
            }

	  if (plug_in->recurse)
	    {
              gimp_main_loop (gimp);

	      return_vals = plug_in_get_current_return_vals (proc_rec);
	    }
	}
    }

 done:
  if (return_vals && destroy_values)
    {
      procedural_db_destroy_args (return_vals, proc_rec->num_values);
      return_vals = NULL;
    }
  return return_vals;
}

void
plug_in_repeat (Gimp    *gimp,
                gint     display_ID,
                gint     image_ID,
                gint     drawable_ID,
                gboolean with_interface)
{
  Argument    *args;
  gint         i;

  if (last_plug_in)
    {
      /* construct the procedures arguments */
      args = g_new (Argument, 3);

      /* initialize the first three argument types */
      for (i = 0; i < 3; i++)
	args[i].arg_type = last_plug_in->args[i].arg_type;

      /* initialize the first three plug-in arguments  */
      args[0].value.pdb_int = (with_interface ? 
                               GIMP_RUN_INTERACTIVE : GIMP_RUN_WITH_LAST_VALS);
      args[1].value.pdb_int = image_ID;
      args[2].value.pdb_int = drawable_ID;

      /* run the plug-in procedure */
      plug_in_run (gimp, last_plug_in, args, 3, FALSE, TRUE, display_ID);

      g_free (args);
    }
}

static gboolean
plug_in_recv_message (GIOChannel   *channel,
		      GIOCondition  cond,
		      gpointer	    data)
{
  PlugIn   *plug_in;
  gboolean  got_message = FALSE;

  plug_in = (PlugIn *) data;

  if (plug_in != current_plug_in)
    plug_in_push (plug_in);

  if (plug_in->my_read == NULL)
    return TRUE;

  if (cond & (G_IO_IN | G_IO_PRI))
    {
      WireMessage msg;

      memset (&msg, 0, sizeof (WireMessage));

      if (! wire_read_msg (plug_in->my_read, &msg, plug_in))
	{
	  plug_in_close (plug_in, TRUE);
	}
      else
	{
	  plug_in_handle_message (plug_in, &msg);
	  wire_destroy (&msg);
	  got_message = TRUE;
	}
    }

  if (cond & (G_IO_ERR | G_IO_HUP))
    {
      if (plug_in->open)
	{
	  plug_in_close (plug_in, TRUE);
	}
    }

  if (! got_message)
    g_message (_("Plug-In crashed: \"%s\"\n(%s)\n\n"
		 "The dying Plug-In may have messed up GIMP's internal state.\n"
		 "You may want to save your images and restart GIMP\n"
		 "to be on the safe side."),
	       g_path_get_basename (plug_in->args[0]),
	       plug_in->args[0]);

  if (! plug_in->open)
    plug_in_destroy (plug_in);
  else
    plug_in_pop ();

  return TRUE;
}

static void
plug_in_handle_message (PlugIn      *plug_in,
                        WireMessage *msg)
{
  switch (msg->type)
    {
    case GP_QUIT:
      plug_in_handle_quit (plug_in);
      break;

    case GP_CONFIG:
      g_warning ("plug_in_handle_message(): "
		 "received a config message (should not happen)");
      plug_in_close (plug_in, TRUE);
      break;

    case GP_TILE_REQ:
      plug_in_handle_tile_req (plug_in, msg->data);
      break;

    case GP_TILE_ACK:
      g_warning ("plug_in_handle_message(): "
		 "received a tile ack message (should not happen)");
      plug_in_close (plug_in, TRUE);
      break;

    case GP_TILE_DATA:
      g_warning ("plug_in_handle_message(): "
		 "received a tile data message (should not happen)");
      plug_in_close (plug_in, TRUE);
      break;

    case GP_PROC_RUN:
      plug_in_handle_proc_run (plug_in, msg->data);
      break;

    case GP_PROC_RETURN:
      plug_in_handle_proc_return (plug_in, msg->data);
      plug_in_close (plug_in, FALSE);
      break;

    case GP_TEMP_PROC_RUN:
      g_warning ("plug_in_handle_message(): "
		 "received a temp proc run message (should not happen)");
      plug_in_close (plug_in, TRUE);
      break;

    case GP_TEMP_PROC_RETURN:
      plug_in_handle_proc_return (plug_in, msg->data);
      gimp_main_loop_quit (plug_in->gimp);
      break;

    case GP_PROC_INSTALL:
      plug_in_handle_proc_install (plug_in, msg->data);
      break;

    case GP_PROC_UNINSTALL:
      plug_in_handle_proc_uninstall (plug_in, msg->data);
      break;

    case GP_EXTENSION_ACK:
      plug_in_handle_extension_ack (plug_in);
      break;

    case GP_HAS_INIT:
      plug_in_handle_has_init (plug_in);
      break;
    }
}

static void
plug_in_handle_quit (PlugIn *plug_in)
{
  plug_in_close (plug_in, FALSE);
}

static void
plug_in_handle_tile_req (PlugIn    *plug_in,
                         GPTileReq *tile_req)
{
  GPTileData   tile_data;
  GPTileData  *tile_info;
  WireMessage  msg;
  TileManager *tm;
  Tile        *tile;

  if (tile_req->drawable_ID == -1)
    {
      /*  this branch communicates with libgimp/gimptile.c:gimp_tile_put()  */

      tile_data.drawable_ID = -1;
      tile_data.tile_num    = 0;
      tile_data.shadow      = 0;
      tile_data.bpp         = 0;
      tile_data.width       = 0;
      tile_data.height      = 0;
      tile_data.use_shm     = (shm_ID == -1) ? FALSE : TRUE;
      tile_data.data        = NULL;

      if (! gp_tile_data_write (plug_in->my_write, &tile_data, plug_in))
	{
	  g_warning ("plug_in_handle_tile_req: ERROR");
	  plug_in_close (plug_in, TRUE);
	  return;
	}

      if (! wire_read_msg (plug_in->my_read, &msg, plug_in))
	{
	  g_warning ("plug_in_handle_tile_req: ERROR");
	  plug_in_close (plug_in, TRUE);
	  return;
	}

      if (msg.type != GP_TILE_DATA)
	{
	  g_warning ("expected tile data and received: %d", msg.type);
	  plug_in_close (plug_in, TRUE);
	  return;
	}

      tile_info = msg.data;

      if (tile_info->shadow)
	tm = gimp_drawable_shadow ((GimpDrawable *)
                                   gimp_item_get_by_ID (plug_in->gimp,
                                                        tile_info->drawable_ID));
      else
	tm = gimp_drawable_data ((GimpDrawable *)
                                 gimp_item_get_by_ID (plug_in->gimp,
                                                      tile_info->drawable_ID));

      if (!tm)
	{
	  g_warning ("plug-in requested invalid drawable (killing)");
	  plug_in_close (plug_in, TRUE);
	  return;
	}

      tile = tile_manager_get (tm, tile_info->tile_num, TRUE, TRUE);
      if (!tile)
	{
	  g_warning ("plug-in requested invalid tile (killing)");
	  plug_in_close (plug_in, TRUE);
	  return;
	}

      if (tile_data.use_shm)
	memcpy (tile_data_pointer (tile, 0, 0), shm_addr, tile_size (tile));
      else
	memcpy (tile_data_pointer (tile, 0, 0), tile_info->data, tile_size (tile));

      tile_release (tile, TRUE);

      wire_destroy (&msg);
      if (! gp_tile_ack_write (plug_in->my_write, plug_in))
	{
	  g_warning ("plug_in_handle_tile_req: ERROR");
	  plug_in_close (plug_in, TRUE);
	  return;
	}
    }
  else
    {
      /*  this branch communicates with libgimp/gimptile.c:gimp_tile_get()  */

      if (tile_req->shadow)
	tm = gimp_drawable_shadow ((GimpDrawable *)
                                   gimp_item_get_by_ID (plug_in->gimp,
                                                        tile_req->drawable_ID));
      else
	tm = gimp_drawable_data ((GimpDrawable *)
                                 gimp_item_get_by_ID (plug_in->gimp,
                                                      tile_req->drawable_ID));

      if (! tm)
	{
	  g_warning ("plug-in requested invalid drawable (killing)");
	  plug_in_close (plug_in, TRUE);
	  return;
	}

      tile = tile_manager_get (tm, tile_req->tile_num, TRUE, FALSE);
      if (! tile)
	{
	  g_warning ("plug-in requested invalid tile (killing)");
	  plug_in_close (plug_in, TRUE);
	  return;
	}

      tile_data.drawable_ID = tile_req->drawable_ID;
      tile_data.tile_num    = tile_req->tile_num;
      tile_data.shadow      = tile_req->shadow;
      tile_data.bpp         = tile_bpp (tile);
      tile_data.width       = tile_ewidth (tile);
      tile_data.height      = tile_eheight (tile);
      tile_data.use_shm     = (shm_ID == -1) ? FALSE : TRUE;

      if (tile_data.use_shm)
	memcpy (shm_addr, tile_data_pointer (tile, 0, 0), tile_size (tile));
      else
	tile_data.data = tile_data_pointer (tile, 0, 0);

      if (! gp_tile_data_write (plug_in->my_write, &tile_data, plug_in))
	{
	  g_message ("plug_in_handle_tile_req: ERROR");
	  plug_in_close (plug_in, TRUE);
	  return;
	}

      tile_release (tile, FALSE);

      if (! wire_read_msg (plug_in->my_read, &msg, plug_in))
	{
	  g_message ("plug_in_handle_tile_req: ERROR");
	  plug_in_close (plug_in, TRUE);
	  return;
	}

      if (msg.type != GP_TILE_ACK)
	{
	  g_warning ("expected tile ack and received: %d", msg.type);
	  plug_in_close (plug_in, TRUE);
	  return;
	}

      wire_destroy (&msg);
    }
}

static void
plug_in_handle_proc_run (PlugIn    *plug_in,
                         GPProcRun *proc_run)
{
  GPProcReturn   proc_return;
  ProcRecord    *proc_rec;
  Argument      *args;
  Argument      *return_vals;
  PlugInBlocked *blocked;

  args = plug_in_params_to_args (proc_run->params, proc_run->nparams, FALSE);
  proc_rec = procedural_db_lookup (plug_in->gimp, proc_run->name);

  if (proc_rec)
    {
      return_vals = procedural_db_execute (plug_in->gimp, proc_run->name, args);
    }
  else
    {
      /*  if the name lookup failed, construct a 
       *  dummy "executiuon error" return value --Michael
       */
      return_vals = g_new (Argument, 1);
      return_vals[0].arg_type      = GIMP_PDB_STATUS;
      return_vals[0].value.pdb_int = GIMP_PDB_EXECUTION_ERROR;
    }

  if (return_vals)
    {
      proc_return.name = proc_run->name;

      if (proc_rec)
	{
	  proc_return.nparams = proc_rec->num_values + 1;
	  proc_return.params = plug_in_args_to_params (return_vals, proc_rec->num_values + 1, FALSE);
	}
      else
	{
	  proc_return.nparams = 1;
	  proc_return.params = plug_in_args_to_params (return_vals, 1, FALSE);
	}

      if (! gp_proc_return_write (plug_in->my_write, &proc_return, plug_in))
	{
	  g_warning ("plug_in_handle_proc_run: ERROR");
	  plug_in_close (plug_in, TRUE);
	  return;
	}

      plug_in_args_destroy (args, proc_run->nparams, FALSE);
      plug_in_args_destroy (return_vals, (proc_rec ? (proc_rec->num_values + 1) : 1), TRUE);
      plug_in_params_destroy (proc_return.params, proc_return.nparams, FALSE);
    }
  else
    {
      blocked = g_new0 (PlugInBlocked, 1);

      blocked->plug_in   = plug_in;
      blocked->proc_name = g_strdup (proc_run->name);

      blocked_plug_ins = g_slist_prepend (blocked_plug_ins, blocked);
    }
}

static void
plug_in_handle_proc_return (PlugIn       *plug_in,
                            GPProcReturn *proc_return)
{
  PlugInBlocked *blocked;
  GSList        *tmp;

  if (plug_in->recurse)
    {
      current_return_vals = plug_in_params_to_args (proc_return->params,
						    proc_return->nparams,
						    TRUE);
      current_return_nvals = proc_return->nparams;
    }
  else
    {
      for (tmp = blocked_plug_ins; tmp; tmp = g_slist_next (tmp))
	{
	  blocked = tmp->data;

	  if (blocked->proc_name && proc_return->name && 
	      strcmp (blocked->proc_name, proc_return->name) == 0)
	    {
	      plug_in_push (blocked->plug_in);

	      if (! gp_proc_return_write (blocked->plug_in->my_write,
                                          proc_return,
                                          blocked->plug_in))
		{
		  g_message ("plug_in_handle_proc_run: ERROR");
		  plug_in_close (blocked->plug_in, TRUE);
		  return;
		}

	      plug_in_pop ();

	      blocked_plug_ins = g_slist_remove (blocked_plug_ins, blocked);
	      g_free (blocked->proc_name);
	      g_free (blocked);
	      break;
	    }
	}
    }
}

static void
plug_in_handle_proc_install (PlugIn        *plug_in,
                             GPProcInstall *proc_install)
{
  PlugInDef     *plug_in_def = NULL;
  PlugInProcDef *proc_def;
  ProcRecord    *proc = NULL;
  GSList        *tmp = NULL;
  gchar         *prog = NULL;
  gboolean       add_proc_def;
  gboolean       valid;
  gint           i;

  /*  Argument checking
   *   --only sanity check arguments when the procedure requests a menu path
   */

  if (proc_install->menu_path)
    {
      if (strncmp (proc_install->menu_path, "<Toolbox>", 9) == 0)
	{
	  if ((proc_install->nparams < 1) ||
	      (proc_install->params[0].type != GIMP_PDB_INT32))
	    {
	      g_message ("Plug-In \"%s\"\n(%s)\n"
			 "attempted to install procedure \"%s\"\n"
			 "which does not take the standard Plug-In args.",
			 g_path_get_basename (plug_in->args[0]),
			 plug_in->args[0],
			 proc_install->name);
	      return;
	    }
	}
      else if (strncmp (proc_install->menu_path, "<Image>", 7) == 0)
	{
	  if ((proc_install->nparams < 3) ||
	      (proc_install->params[0].type != GIMP_PDB_INT32) ||
	      (proc_install->params[1].type != GIMP_PDB_IMAGE) ||
	      (proc_install->params[2].type != GIMP_PDB_DRAWABLE))
	    {
	      g_message ("Plug-In \"%s\"\n(%s)\n"
			 "attempted to install procedure \"%s\"\n"
			 "which does not take the standard Plug-In args.",
			 g_path_get_basename (plug_in->args[0]),
			 plug_in->args[0],
			 proc_install->name);
	      return;
	    }
	}
      else if (strncmp (proc_install->menu_path, "<Load>", 6) == 0)
	{
	  if ((proc_install->nparams < 3) ||
	      (proc_install->params[0].type != GIMP_PDB_INT32) ||
	      (proc_install->params[1].type != GIMP_PDB_STRING) ||
	      (proc_install->params[2].type != GIMP_PDB_STRING))
	    {
	      g_message ("Plug-In \"%s\"\n(%s)\n"
			 "attempted to install procedure \"%s\"\n"
			 "which does not take the standard Plug-In args.",
			 g_path_get_basename (plug_in->args[0]),
			 plug_in->args[0],
			 proc_install->name);
	      return;
	    }
	}
      else if (strncmp (proc_install->menu_path, "<Save>", 6) == 0)
	{
	  if ((proc_install->nparams < 5) ||
	      (proc_install->params[0].type != GIMP_PDB_INT32)    ||
	      (proc_install->params[1].type != GIMP_PDB_IMAGE)    ||
	      (proc_install->params[2].type != GIMP_PDB_DRAWABLE) ||
	      (proc_install->params[3].type != GIMP_PDB_STRING)   ||
	      (proc_install->params[4].type != GIMP_PDB_STRING))
	    {
	      g_message ("Plug-In \"%s\"\n(%s)\n"
			 "attempted to install procedure \"%s\"\n"
			 "which does not take the standard Plug-In args.",
			 g_path_get_basename (plug_in->args[0]),
			 plug_in->args[0],
			 proc_install->name);
	      return;
	    }
	}
      else
	{
	  g_message ("Plug-In \"%s\"\n(%s)\n"
		     "attempted to install procedure \"%s\"\n"
		     "in an invalid menu location.\n"
		     "Use either \"<Toolbox>\", \"<Image>\", "
		     "\"<Load>\", or \"<Save>\".",
		     g_path_get_basename (plug_in->args[0]),
		     plug_in->args[0],
		     proc_install->name);
	  return;
	}
    }

  /*  Sanity check for array arguments  */

  for (i = 1; i < proc_install->nparams; i++) 
    {
      if ((proc_install->params[i].type == GIMP_PDB_INT32ARRAY ||
	   proc_install->params[i].type == GIMP_PDB_INT8ARRAY  ||
	   proc_install->params[i].type == GIMP_PDB_FLOATARRAY ||
	   proc_install->params[i].type == GIMP_PDB_STRINGARRAY) 
          &&
	  proc_install->params[i-1].type != GIMP_PDB_INT32) 
	{
	  g_message ("Plug-In \"%s\"\n(%s)\n"
		     "attempted to install procedure \"%s\"\n"
		     "which fails to comply with the array parameter\n"
		     "passing standard.  Argument %d is noncompliant.", 
		     g_path_get_basename (plug_in->args[0]),
		     plug_in->args[0],
		     proc_install->name, i);
	  return;
	}
    }

  /*  Sanity check strings for UTF-8 validity  */
  valid = FALSE;

  if ((proc_install->menu_path == NULL || 
       g_utf8_validate (proc_install->menu_path, -1, NULL)) &&
      (g_utf8_validate (proc_install->name, -1, NULL))      &&
      (proc_install->blurb == NULL || 
       g_utf8_validate (proc_install->blurb, -1, NULL))     &&
      (proc_install->help == NULL || 
       g_utf8_validate (proc_install->help, -1, NULL))      &&
      (proc_install->author == NULL ||
       g_utf8_validate (proc_install->author, -1, NULL))    &&
      (proc_install->copyright == NULL || 
       g_utf8_validate (proc_install->copyright, -1, NULL)) &&
      (proc_install->date == NULL ||
       g_utf8_validate (proc_install->date, -1, NULL)))
    {
      valid = TRUE;

      for (i = 0; i < proc_install->nparams && valid; i++)
        {
          if (! (g_utf8_validate (proc_install->params[i].name, -1, NULL) &&
                 (proc_install->params[i].description == NULL || 
                  g_utf8_validate (proc_install->params[i].description, -1, NULL))))
            valid = FALSE;
        }
      for (i = 0; i < proc_install->nreturn_vals && valid; i++)
        {
          if (! (g_utf8_validate (proc_install->return_vals[i].name, -1, NULL) &&
                 (proc_install->return_vals[i].description == NULL ||
                  g_utf8_validate (proc_install->return_vals[i].description, -1, NULL))))
            valid = FALSE;
        }
    }
  
  if (!valid)
    {
      g_message ("Plug-In \"%s\"\n(%s)\n"
                 "attempted to install a procedure with invalid UTF-8 strings.\n", 
                 g_path_get_basename (plug_in->args[0]),
                 plug_in->args[0]);
      return;
    }

  /*  Initialization  */

  proc_def = NULL;

  switch (proc_install->type)
    {
    case GIMP_PLUGIN:
    case GIMP_EXTENSION:
      plug_in_def = plug_in->user_data;
      prog = plug_in_def->prog;

      tmp = plug_in_def->proc_defs;
      break;

    case GIMP_TEMPORARY:
      prog = "none";

      tmp = plug_in->temp_proc_defs;
      break;
    }

  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (strcmp (proc_def->db_info.name, proc_install->name) == 0)
	{
	  if (proc_install->type == GIMP_TEMPORARY)
	    plug_ins_proc_def_remove (proc_def, plug_in->gimp);
	  else
	    plug_in_proc_def_destroy (proc_def, TRUE); /* destroys data_only */

	  break;
	}

      proc_def = NULL;
    }

  add_proc_def = FALSE;
  if (!proc_def)
    {
      add_proc_def = TRUE;
      proc_def = g_new0 (PlugInProcDef, 1);
    }

  proc_def->prog = g_strdup (prog);

  proc_def->menu_path       = g_strdup (proc_install->menu_path);
  proc_def->accelerator     = NULL;
  proc_def->extensions      = NULL;
  proc_def->prefixes        = NULL;
  proc_def->magics          = NULL;
  proc_def->image_types     = g_strdup (proc_install->image_types);
  proc_def->image_types_val = plug_ins_image_types_parse (proc_def->image_types);
  /* Install temp one use todays time */
  proc_def->mtime           = time (NULL);

  proc = &proc_def->db_info;

  /*  The procedural database procedure  */

  proc->name      = g_strdup (proc_install->name);
  proc->blurb     = g_strdup (proc_install->blurb);
  proc->help      = g_strdup (proc_install->help);
  proc->author    = g_strdup (proc_install->author);
  proc->copyright = g_strdup (proc_install->copyright);
  proc->date      = g_strdup (proc_install->date);
  proc->proc_type = proc_install->type;

  proc->num_args   = proc_install->nparams;
  proc->num_values = proc_install->nreturn_vals;

  proc->args   = g_new (ProcArg, proc->num_args);
  proc->values = g_new (ProcArg, proc->num_values);

  for (i = 0; i < proc->num_args; i++)
    {
      proc->args[i].arg_type    = proc_install->params[i].type;
      proc->args[i].name        = g_strdup (proc_install->params[i].name);
      proc->args[i].description = g_strdup (proc_install->params[i].description);
    }

  for (i = 0; i < proc->num_values; i++)
    {
      proc->values[i].arg_type    = proc_install->return_vals[i].type;
      proc->values[i].name        = g_strdup (proc_install->return_vals[i].name);
      proc->values[i].description = g_strdup (proc_install->return_vals[i].description);
    }

  switch (proc_install->type)
    {
    case GIMP_PLUGIN:
    case GIMP_EXTENSION:
      if (add_proc_def)
	plug_in_def->proc_defs = g_slist_prepend (plug_in_def->proc_defs, proc_def);
      break;

    case GIMP_TEMPORARY:
      if (add_proc_def)
	plug_in->temp_proc_defs = g_slist_prepend (plug_in->temp_proc_defs,
                                                   proc_def);

      proc->exec_method.temporary.plug_in = plug_in;

      plug_ins_proc_def_add (proc_def, plug_in->gimp,
                             plug_in_def ? plug_in_def->locale_domain : NULL,
                             plug_in_def ? plug_in_def->help_path     : NULL);
      break;
    }
}

static void
plug_in_handle_proc_uninstall (PlugIn          *plug_in,
                               GPProcUninstall *proc_uninstall)
{
  PlugInProcDef *proc_def;
  GSList        *tmp;

  for (tmp = plug_in->temp_proc_defs; tmp; tmp = g_slist_next (tmp))
    {
      proc_def = tmp->data;

      if (! strcmp (proc_def->db_info.name, proc_uninstall->name))
	{
	  plug_in->temp_proc_defs = g_slist_remove (plug_in->temp_proc_defs,
                                                    proc_def);
	  plug_ins_proc_def_remove (proc_def, plug_in->gimp);
          g_free (proc_def);
	  break;
	}
    }
}

static void
plug_in_handle_extension_ack (PlugIn *plug_in)
{
  gimp_main_loop_quit (plug_in->gimp);
}

static void
plug_in_handle_has_init (PlugIn *plug_in)
{
  plug_in->user_data->has_init = TRUE;
}

static gboolean
plug_in_write (GIOChannel *channel,
	       guint8      *buf,
	       gulong       count,
               gpointer     user_data)
{
  PlugIn *plug_in;
  gulong  bytes;

  plug_in = (PlugIn *) user_data;

  while (count > 0)
    {
      if ((plug_in->write_buffer_index + count) >= WRITE_BUFFER_SIZE)
	{
	  bytes = WRITE_BUFFER_SIZE - plug_in->write_buffer_index;
	  memcpy (&plug_in->write_buffer[plug_in->write_buffer_index],
                  buf, bytes);
	  plug_in->write_buffer_index += bytes;
	  if (! wire_flush (channel, plug_in))
	    return FALSE;
	}
      else
	{
	  bytes = count;
	  memcpy (&plug_in->write_buffer[plug_in->write_buffer_index],
                  buf, bytes);
	  plug_in->write_buffer_index += bytes;
	}

      buf += bytes;
      count -= bytes;
    }

  return TRUE;
}

static gboolean
plug_in_flush (GIOChannel *channel,
               gpointer    user_data)
{
  PlugIn    *plug_in;
  GIOStatus  status;
  GError    *error = NULL;
  gint       count;
  guint      bytes;

  plug_in = (PlugIn *) user_data;

  if (plug_in->write_buffer_index > 0)
    {
      count = 0;
      while (count != plug_in->write_buffer_index)
        {
	  do
	    {
	      bytes = 0;
	      status = g_io_channel_write_chars (channel,
						 &plug_in->write_buffer[count],
						 (plug_in->write_buffer_index - count),
						 &bytes,
						 &error);
	    }
	  while (status == G_IO_STATUS_AGAIN);

	  if (status != G_IO_STATUS_NORMAL)
	    {
	      if (error)
		{
		  g_warning ("%s: plug_in_flush(): error: %s",
			     g_get_prgname (), error->message);
		  g_error_free (error);
		}
	      else
		{
		  g_warning ("%s: plug_in_flush(): error",
			     g_get_prgname ());
		}

	      return FALSE;
	    }

          count += bytes;
        }

      plug_in->write_buffer_index = 0;
    }

  return TRUE;
}

static void
plug_in_push (PlugIn *plug_in)
{
  g_return_if_fail (plug_in != NULL);

  current_plug_in = plug_in;

  plug_in_stack = g_slist_prepend (plug_in_stack, current_plug_in);
}

static void
plug_in_pop (void)
{
  GSList *tmp;

  if (current_plug_in)
    {
      tmp = plug_in_stack;
      plug_in_stack = plug_in_stack->next;
      tmp->next = NULL;
      g_slist_free (tmp);
    }

  if (plug_in_stack)
    {
      current_plug_in = plug_in_stack->data;
    }
  else
    {
      current_plug_in = NULL;
    }
}

static Argument *
plug_in_temp_run (ProcRecord *proc_rec,
		  Argument   *args,
		  gint        argc)
{
  Argument  *return_vals;
  PlugIn    *plug_in;
  GPProcRun  proc_run;
  gint       old_recurse;

  return_vals = NULL;

  plug_in = (PlugIn *) proc_rec->exec_method.temporary.plug_in;

  if (plug_in)
    {
      if (plug_in->busy)
	{
	  return_vals = procedural_db_return_args (proc_rec, FALSE);
	  goto done;
	}

      plug_in->busy = TRUE;
      plug_in_push (plug_in);

      proc_run.name    = proc_rec->name;
      proc_run.nparams = argc;
      proc_run.params  = plug_in_args_to_params (args, argc, FALSE);

      if (! gp_temp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
	  ! wire_flush (plug_in->my_write, plug_in))
	{
	  return_vals = procedural_db_return_args (proc_rec, FALSE);
	  goto done;
	}

      plug_in_pop ();

      plug_in_params_destroy (proc_run.params, proc_run.nparams, FALSE);

      old_recurse = plug_in->recurse;
      plug_in->recurse = TRUE;

/*       gtk_main (); */
      
/*       return_vals = plug_in_get_current_return_vals (proc_rec); */
      return_vals = procedural_db_return_args (proc_rec, TRUE);
      plug_in->recurse = old_recurse;
      plug_in->busy = FALSE;
    }

 done:
  return return_vals;
}

static gchar *
plug_in_search_in_path (gchar *search_path,
			gchar *filename)
{
  gchar *local_path;
  gchar *token;
  gchar *next_token;
  gchar *path;

  local_path = g_strdup (search_path);
  next_token = local_path;
  token      = strtok (next_token, G_SEARCHPATH_SEPARATOR_S);

  while (token)
    {
      path = g_build_filename (token, filename, NULL);

      if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
	{
	  token = path;
	  break;
	}

      g_free (path);

      token = strtok (NULL, G_SEARCHPATH_SEPARATOR_S);
    }

  g_free (local_path);

  return token;
}
