/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in.c
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

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <glib-object.h>

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)

#define STRICT
#include <windows.h>
#include <process.h>

#ifdef G_OS_WIN32
#include <fcntl.h>
#include <io.h>
#endif

#ifdef G_WITH_CYGWIN
#define O_TEXT                0x0100        /* text file */
#define _O_TEXT                0x0100        /* text file */
#define O_BINARY        0x0200        /* binary file */
#define _O_BINARY        0x0200        /* binary file */
#endif

#endif /* G_OS_WIN32 || G_WITH_CYGWIN */

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "plug-in-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpenvirontable.h"
#include "core/gimpinterpreterdb.h"
#include "core/gimpprogress.h"

#include "pdb/gimptemporaryprocedure.h"

#include "plug-in.h"
#include "plug-in-debug.h"
#include "plug-in-def.h"
#include "plug-in-locale-domain.h"
#include "plug-in-message.h"
#include "plug-in-params.h"
#include "plug-in-progress.h"
#include "plug-in-shm.h"
#include "plug-ins.h"

#include "gimp-intl.h"


/*  local funcion prototypes  */

static gboolean   plug_in_write         (GIOChannel   *channel,
                                         const guint8 *buf,
                                         gulong        count,
                                         gpointer      user_data);
static gboolean   plug_in_flush         (GIOChannel   *channel,
                                         gpointer      user_data);

static gboolean   plug_in_recv_message  (GIOChannel   *channel,
                                         GIOCondition  cond,
                                         gpointer      data);

#if !defined(G_OS_WIN32) && !defined (G_WITH_CYGWIN)
static void       plug_in_prep_for_exec (gpointer      data);
#endif


void
plug_in_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /* initialize the gimp protocol library and set the read and
   *  write handlers.
   */
  gp_init ();

  gimp_wire_set_writer (plug_in_write);
  gimp_wire_set_flusher (plug_in_flush);

  /* allocate a piece of shared memory for use in transporting tiles
   *  to plug-ins. if we can't allocate a piece of shared memory then
   *  we'll fall back on sending the data over the pipe.
   */
  if (gimp->use_shm)
    plug_in_shm_init (gimp);

  plug_in_debug_init (gimp);
}

void
plug_in_exit (Gimp *gimp)
{
  GSList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  plug_in_debug_exit (gimp);

  if (gimp->use_shm)
    plug_in_shm_exit (gimp);

  list = gimp->open_plug_ins;
  while (list)
    {
      PlugIn *plug_in = list->data;

      list = list->next;

      if (plug_in->open)
        plug_in_close (plug_in, TRUE);

      plug_in_unref (plug_in);
    }
}

void
plug_in_call_query (Gimp        *gimp,
                    GimpContext *context,
                    PlugInDef   *plug_in_def)
{
  PlugIn *plug_in;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (plug_in_def != NULL);

  plug_in = plug_in_new (gimp, context, NULL, NULL, plug_in_def->prog);

  if (plug_in)
    {
      plug_in->query       = TRUE;
      plug_in->synchronous = TRUE;
      plug_in->plug_in_def = plug_in_def;

      if (plug_in_open (plug_in))
        {
          while (plug_in->open)
            {
              GimpWireMessage msg;

              if (! gimp_wire_read_msg (plug_in->my_read, &msg, plug_in))
                {
                  plug_in_close (plug_in, TRUE);
                }
              else
                {
                  plug_in_handle_message (plug_in, &msg);
                  gimp_wire_destroy (&msg);
                }
            }
        }

      plug_in_unref (plug_in);
    }
}

void
plug_in_call_init (Gimp        *gimp,
                   GimpContext *context,
                   PlugInDef   *plug_in_def)
{
  PlugIn *plug_in;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (plug_in_def != NULL);

  plug_in = plug_in_new (gimp, context, NULL, NULL, plug_in_def->prog);

  if (plug_in)
    {
      plug_in->init        = TRUE;
      plug_in->synchronous = TRUE;
      plug_in->plug_in_def = plug_in_def;

      if (plug_in_open (plug_in))
        {
          while (plug_in->open)
            {
              GimpWireMessage msg;

              if (! gimp_wire_read_msg (plug_in->my_read, &msg, plug_in))
                {
                  plug_in_close (plug_in, TRUE);
                }
              else
                {
                  plug_in_handle_message (plug_in, &msg);
                  gimp_wire_destroy (&msg);
                }
            }
        }

      plug_in_unref (plug_in);
    }
}

PlugIn *
plug_in_new (Gimp          *gimp,
             GimpContext   *context,
             GimpProgress  *progress,
             GimpProcedure *procedure,
             const gchar   *prog)
{
  PlugIn *plug_in;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (prog != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (prog), NULL);

  plug_in = g_new0 (PlugIn, 1);

  plug_in->gimp               = gimp;

  plug_in->ref_count          = 1;

  plug_in->open               = FALSE;
  plug_in->query              = FALSE;
  plug_in->init               = FALSE;
  plug_in->synchronous        = FALSE;
  plug_in->pid                = 0;

  plug_in->name               = g_path_get_basename (prog);
  plug_in->prog               = g_strdup (prog);

  plug_in->my_read            = NULL;
  plug_in->my_write           = NULL;
  plug_in->his_read           = NULL;
  plug_in->his_write          = NULL;

  plug_in->input_id           = 0;
  plug_in->write_buffer_index = 0;

  plug_in->temp_procedures    = NULL;

  plug_in->ext_main_loop      = NULL;

  plug_in_proc_frame_init (&plug_in->main_proc_frame,
                           context, progress, procedure);

  plug_in->temp_proc_frames   = NULL;

  plug_in->plug_in_def        = NULL;

  return plug_in;
}

void
plug_in_ref (PlugIn *plug_in)
{
  g_return_if_fail (plug_in != NULL);

  plug_in->ref_count++;
}

void
plug_in_unref (PlugIn *plug_in)
{
  g_return_if_fail (plug_in != NULL);

  plug_in->ref_count--;

  if (plug_in->ref_count < 1)
    {
      if (plug_in->open)
        plug_in_close (plug_in, TRUE);

      g_free (plug_in->name);
      g_free (plug_in->prog);

      plug_in_proc_frame_dispose (&plug_in->main_proc_frame, plug_in);

      g_free (plug_in);
    }
}

#if !defined(G_OS_WIN32) && !defined (G_WITH_CYGWIN)

static void
plug_in_prep_for_exec (gpointer data)
{
  PlugIn *plug_in = data;

  g_io_channel_unref (plug_in->my_read);
  plug_in->my_read  = NULL;

  g_io_channel_unref (plug_in->my_write);
  plug_in->my_write  = NULL;
}

#else

#define plug_in_prep_for_exec NULL

#endif

gboolean
plug_in_open (PlugIn *plug_in)
{
  gint       my_read[2];
  gint       my_write[2];
  gchar    **envp;
  gchar     *args[9], **argv, **debug_argv;
  gint       argc;
  gchar     *interp, *interp_arg;
  gchar     *read_fd, *write_fd;
  gchar     *mode, *stm;
  GError    *error = NULL;
  Gimp      *gimp;
  gboolean   debug;
  guint      debug_flag;
  guint      spawn_flags;

  g_return_val_if_fail (plug_in != NULL, FALSE);

  gimp = plug_in->gimp;

  /* Open two pipes. (Bidirectional communication).
   */
  if ((pipe (my_read) == -1) || (pipe (my_write) == -1))
    {
      g_message ("Unable to run plug-in \"%s\"\n(%s)\n\npipe() failed: %s",
                 gimp_filename_to_utf8 (plug_in->name),
                 gimp_filename_to_utf8 (plug_in->prog),
                 g_strerror (errno));
      return FALSE;
    }

#if defined(G_WITH_CYGWIN)
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
  read_fd  = g_strdup_printf ("%d",
                              g_io_channel_unix_get_fd (plug_in->his_read));
  write_fd = g_strdup_printf ("%d",
                              g_io_channel_unix_get_fd (plug_in->his_write));

  /* Set the rest of the command line arguments.
   * FIXME: this is ugly.  Pass in the mode as a separate argument?
   */
  if (plug_in->query)
    {
      mode = "-query";
      debug_flag = GIMP_DEBUG_WRAP_QUERY;
    }
  else if (plug_in->init)
    {
      mode = "-init";
      debug_flag = GIMP_DEBUG_WRAP_INIT;
    }
  else
    {
      mode = "-run";
      debug_flag = GIMP_DEBUG_WRAP_RUN;
    }

  stm = g_strdup_printf ("%d", plug_in->gimp->stack_trace_mode);

  interp = gimp_interpreter_db_resolve (plug_in->gimp->interpreter_db,
                                        plug_in->prog, &interp_arg);

  argc = 0;

  if (interp)
    args[argc++] = interp;

  if (interp_arg)
    args[argc++] = interp_arg;

  args[argc++] = plug_in->prog;
  args[argc++] = "-gimp";
  args[argc++] = read_fd;
  args[argc++] = write_fd;
  args[argc++] = mode;
  args[argc++] = stm;
  args[argc++] = NULL;

  argv = args;
  envp = gimp_environ_table_get_envp (plug_in->gimp->environ_table);
  spawn_flags = (G_SPAWN_LEAVE_DESCRIPTORS_OPEN |
                 G_SPAWN_DO_NOT_REAP_CHILD      |
                 G_SPAWN_CHILD_INHERITS_STDIN);

  debug = FALSE;

  if (gimp->plug_in_debug)
    {
      debug_argv = plug_in_debug_argv (gimp, plug_in->name, debug_flag, args);

      if (debug_argv)
        {
          debug = TRUE;
          argv = debug_argv;
          spawn_flags |= G_SPAWN_SEARCH_PATH;
        }
    }

  /* Fork another process. We'll remember the process id so that we
   * can later use it to kill the filter if necessary.
   */
  if (! g_spawn_async (NULL, argv, envp, spawn_flags,
                       plug_in_prep_for_exec, plug_in,
                       &plug_in->pid,
                       &error))
    {
      g_message ("Unable to run plug-in \"%s\"\n(%s)\n\n%s",
                 gimp_filename_to_utf8 (plug_in->name),
                 gimp_filename_to_utf8 (plug_in->prog),
                 error->message);
      g_error_free (error);
      goto cleanup;
    }

  g_io_channel_unref (plug_in->his_read);
  plug_in->his_read = NULL;

  g_io_channel_unref (plug_in->his_write);
  plug_in->his_write = NULL;

  if (! plug_in->synchronous)
    {
      GSource *source;

      source = g_io_create_watch (plug_in->my_read,
                                  G_IO_IN  | G_IO_PRI | G_IO_ERR | G_IO_HUP);

      g_source_set_callback (source,
                             (GSourceFunc) plug_in_recv_message, plug_in,
                             NULL);

      g_source_set_can_recurse (source, TRUE);

      plug_in->input_id = g_source_attach (source, NULL);
      g_source_unref (source);

      gimp->open_plug_ins = g_slist_prepend (gimp->open_plug_ins, plug_in);
    }

  plug_in->open = TRUE;

cleanup:

  if (debug)
    g_free (argv);

  g_free (read_fd);
  g_free (write_fd);
  g_free (stm);
  g_free (interp);
  g_free (interp_arg);

  return plug_in->open;
}

void
plug_in_close (PlugIn   *plug_in,
               gboolean  kill_it)
{
  Gimp          *gimp;
#ifndef G_OS_WIN32
  gint           status;
  struct timeval tv;
#endif
  GList         *list;

  g_return_if_fail (plug_in != NULL);
  g_return_if_fail (plug_in->open == TRUE);

  gimp = plug_in->gimp;

  if (! plug_in->open)
    return;

  plug_in->open = FALSE;

  if (plug_in->pid)
    {
      /*  Ask the filter to exit gracefully  */
      if (kill_it)
        {
          gp_quit_write (plug_in->my_write, plug_in);

          /*  give the plug-in some time (10 ms)  */
#ifndef G_OS_WIN32
          tv.tv_sec  = 0;
          tv.tv_usec = 10 * 1000;
          select (0, NULL, NULL, NULL, &tv);
#else
          Sleep (10);
#endif
        }

      /* If necessary, kill the filter. */
#ifndef G_OS_WIN32

      if (kill_it)
        {
          if (gimp->be_verbose)
            g_print (_("Terminating plug-in: '%s'\n"),
                     gimp_filename_to_utf8 (plug_in->prog));

          status = kill (plug_in->pid, SIGKILL);
        }

      /* Wait for the process to exit. This will happen
       *  immediately if it was just killed.
       */
      waitpid (plug_in->pid, &status, 0);

#else /* G_OS_WIN32 */

      if (kill_it)
        {
          /* Trying to avoid TerminateProcess (does mostly work).
           * Otherwise some of our needed DLLs may get into an
           * unstable state (see Win32 API docs).
           */
          DWORD dwExitCode = STILL_ACTIVE;
          DWORD dwTries    = 10;

          while (dwExitCode == dwExitCode &&
                 GetExitCodeProcess ((HANDLE) plug_in->pid, &dwExitCode) &&
                 (dwTries > 0))
            {
              Sleep (10);
              dwTries--;
            }

          if (dwExitCode == STILL_ACTIVE)
            {
              if (gimp->be_verbose)
                g_print (_("Terminating plug-in: '%s'\n"),
                         gimp_filename_to_utf8 (plug_in->prog));

              TerminateProcess ((HANDLE) plug_in->pid, 0);
            }
        }

#endif /* G_OS_WIN32 */

      g_spawn_close_pid (plug_in->pid);
      plug_in->pid = 0;
    }

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

  gimp_wire_clear_error ();

  for (list = plug_in->temp_proc_frames; list; list = g_list_next (list))
    {
      PlugInProcFrame *proc_frame = list->data;

#ifdef GIMP_UNSTABLE
      g_printerr ("plug_in_close: plug-in aborted before sending its "
                  "temporary procedure return values\n");
#endif

      if (proc_frame->main_loop &&
          g_main_loop_is_running (proc_frame->main_loop))
        {
          g_main_loop_quit (proc_frame->main_loop);
        }
    }

  if (plug_in->main_proc_frame.main_loop &&
      g_main_loop_is_running (plug_in->main_proc_frame.main_loop))
    {
#ifdef GIMP_UNSTABLE
      g_printerr ("plug_in_close: plug-in aborted before sending its "
                  "procedure return values\n");
#endif

      g_main_loop_quit (plug_in->main_proc_frame.main_loop);
    }

  plug_in_proc_frame_dispose (&plug_in->main_proc_frame, plug_in);

  if (plug_in->ext_main_loop &&
      g_main_loop_is_running (plug_in->ext_main_loop))
    {
#ifdef GIMP_UNSTABLE
      g_printerr ("plug_in_close: extension aborted before sending its "
                  "extension_ack message\n");
#endif

      g_main_loop_quit (plug_in->ext_main_loop);
    }

  plug_in->synchronous = FALSE;

  /* Unregister any temporary procedures. */
  while (plug_in->temp_procedures)
    plug_in_remove_temp_proc (plug_in, plug_in->temp_procedures->data);

  /* Close any dialogs that this plugin might have opened */
  gimp_pdb_dialogs_check (plug_in->gimp);

  gimp->open_plug_ins = g_slist_remove (gimp->open_plug_ins, plug_in);
}

static gboolean
plug_in_recv_message (GIOChannel   *channel,
                      GIOCondition  cond,
                      gpointer            data)
{
  PlugIn   *plug_in     = data;
  gboolean  got_message = FALSE;

#ifdef G_OS_WIN32
  /* Workaround for GLib bug #137968: sometimes we are called for no
   * reason...
   */
  if (cond == 0)
    return TRUE;
#endif

  if (plug_in->my_read == NULL)
    return TRUE;

  if (cond & (G_IO_IN | G_IO_PRI))
    {
      GimpWireMessage msg;

      memset (&msg, 0, sizeof (GimpWireMessage));

      if (! gimp_wire_read_msg (plug_in->my_read, &msg, plug_in))
        {
          plug_in_close (plug_in, TRUE);
        }
      else
        {
          plug_in_handle_message (plug_in, &msg);
          gimp_wire_destroy (&msg);
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
    g_message (_("Plug-in crashed: \"%s\"\n(%s)\n\n"
                 "The dying plug-in may have messed up GIMP's internal state. "
                 "You may want to save your images and restart GIMP "
                 "to be on the safe side."),
               gimp_filename_to_utf8 (plug_in->name),
               gimp_filename_to_utf8 (plug_in->prog));

  if (! plug_in->open)
    plug_in_unref (plug_in);

  return TRUE;
}

static gboolean
plug_in_write (GIOChannel   *channel,
               const guint8 *buf,
               gulong        count,
               gpointer      user_data)
{
  PlugIn *plug_in = user_data;
  gulong  bytes;

  while (count > 0)
    {
      if ((plug_in->write_buffer_index + count) >= WRITE_BUFFER_SIZE)
        {
          bytes = WRITE_BUFFER_SIZE - plug_in->write_buffer_index;
          memcpy (&plug_in->write_buffer[plug_in->write_buffer_index],
                  buf, bytes);
          plug_in->write_buffer_index += bytes;
          if (! gimp_wire_flush (channel, plug_in))
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
  PlugIn *plug_in = user_data;

  if (plug_in->write_buffer_index > 0)
    {
      GIOStatus  status;
      GError    *error = NULL;
      gint       count;
      gsize      bytes;

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
                             gimp_filename_to_utf8 (g_get_prgname ()),
                             error->message);
                  g_error_free (error);
                }
              else
                {
                  g_warning ("%s: plug_in_flush(): error",
                             gimp_filename_to_utf8 (g_get_prgname ()));
                }

              return FALSE;
            }

          count += bytes;
        }

      plug_in->write_buffer_index = 0;
    }

  return TRUE;
}

void
plug_in_push (Gimp   *gimp,
              PlugIn *plug_in)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (plug_in != NULL);

  gimp->current_plug_in = plug_in;

  gimp->plug_in_stack = g_slist_prepend (gimp->plug_in_stack,
                                         gimp->current_plug_in);
}

void
plug_in_pop (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->current_plug_in)
    gimp->plug_in_stack = g_slist_remove (gimp->plug_in_stack,
                                          gimp->plug_in_stack->data);

  if (gimp->plug_in_stack)
    gimp->current_plug_in = gimp->plug_in_stack->data;
  else
    gimp->current_plug_in = NULL;
}

PlugInProcFrame *
plug_in_get_proc_frame (PlugIn *plug_in)
{
  g_return_val_if_fail (plug_in != NULL, NULL);

  if (plug_in->temp_proc_frames)
    return plug_in->temp_proc_frames->data;
  else
    return &plug_in->main_proc_frame;
}

PlugInProcFrame *
plug_in_proc_frame_push (PlugIn        *plug_in,
                         GimpContext   *context,
                         GimpProgress  *progress,
                         GimpProcedure *procedure)
{
  PlugInProcFrame *proc_frame;

  g_return_val_if_fail (plug_in != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  proc_frame = plug_in_proc_frame_new (context, progress, procedure);

  plug_in->temp_proc_frames = g_list_prepend (plug_in->temp_proc_frames,
                                              proc_frame);

  return proc_frame;
}

void
plug_in_proc_frame_pop (PlugIn *plug_in)
{
  PlugInProcFrame *proc_frame;

  g_return_if_fail (plug_in != NULL);
  g_return_if_fail (plug_in->temp_proc_frames != NULL);

  proc_frame = (PlugInProcFrame *) plug_in->temp_proc_frames->data;

  plug_in_proc_frame_unref (proc_frame, plug_in);

  plug_in->temp_proc_frames = g_list_remove (plug_in->temp_proc_frames,
                                             proc_frame);
}

void
plug_in_main_loop (PlugIn *plug_in)
{
  PlugInProcFrame *proc_frame;

  g_return_if_fail (plug_in != NULL);
  g_return_if_fail (plug_in->temp_proc_frames != NULL);

  proc_frame = (PlugInProcFrame *) plug_in->temp_proc_frames->data;

  g_return_if_fail (proc_frame->main_loop == NULL);

  proc_frame->main_loop = g_main_loop_new (NULL, FALSE);

  gimp_threads_leave (plug_in->gimp);
  g_main_loop_run (proc_frame->main_loop);
  gimp_threads_enter (plug_in->gimp);

  g_main_loop_unref (proc_frame->main_loop);
  proc_frame->main_loop = NULL;
}

void
plug_in_main_loop_quit (PlugIn *plug_in)
{
  PlugInProcFrame *proc_frame;

  g_return_if_fail (plug_in != NULL);
  g_return_if_fail (plug_in->temp_proc_frames != NULL);

  proc_frame = (PlugInProcFrame *) plug_in->temp_proc_frames->data;

  g_return_if_fail (proc_frame->main_loop != NULL);

  g_main_loop_quit (proc_frame->main_loop);
}

gchar *
plug_in_get_undo_desc (PlugIn *plug_in)
{
  PlugInProcFrame     *proc_frame = NULL;
  GimpPlugInProcedure *proc       = NULL;
  gchar               *undo_desc  = NULL;

  g_return_val_if_fail (plug_in != NULL, NULL);

  proc_frame = plug_in_get_proc_frame (plug_in);

  if (proc_frame)
    proc = GIMP_PLUG_IN_PROCEDURE (proc_frame->procedure);

  if (proc)
    {
      const gchar *domain = plug_in_locale_domain (plug_in->gimp,
                                                   plug_in->prog, NULL);

      undo_desc = gimp_plug_in_procedure_get_label (proc, domain);
    }

  if (! undo_desc)
    undo_desc = g_filename_display_name (plug_in->name);

  return undo_desc;
}

/*  called from the PDB (gimp_plugin_menu_register)  */
gboolean
plug_in_menu_register (PlugIn      *plug_in,
                       const gchar *proc_name,
                       const gchar *menu_path)
{
  GimpPlugInProcedure *proc  = NULL;
  GError              *error = NULL;

  g_return_val_if_fail (plug_in != NULL, FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);
  g_return_val_if_fail (menu_path != NULL, FALSE);

  if (plug_in->plug_in_def)
    proc = gimp_plug_in_procedure_find (plug_in->plug_in_def->procedures,
                                        proc_name);

  if (! proc)
    proc = gimp_plug_in_procedure_find (plug_in->temp_procedures, proc_name);

  if (! proc)
    {
      g_message ("Plug-in \"%s\"\n(%s)\n"
                 "attempted to register the menu item \"%s\" "
                 "for the procedure \"%s\".\n"
                 "It has however not installed that procedure.  This "
                 "is not allowed.",
                 gimp_filename_to_utf8 (plug_in->name),
                 gimp_filename_to_utf8 (plug_in->prog),
                 menu_path, proc_name);

      return FALSE;
    }

  switch (GIMP_PROCEDURE (proc)->proc_type)
    {
    case GIMP_INTERNAL:
      return FALSE;

    case GIMP_PLUGIN:
    case GIMP_EXTENSION:
      if (! plug_in->query && ! plug_in->init)
        return FALSE;

    case GIMP_TEMPORARY:
      break;
    }

  if (! proc->menu_label)
    {
      g_message ("Plug-in \"%s\"\n(%s)\n"
                 "attempted to register the menu item \"%s\" "
                 "for procedure \"%s\".\n"
                 "The menu label given in gimp_install_procedure() "
                 "already contained a path.  To make this work, "
                 "pass just the menu's label to "
                 "gimp_install_procedure().",
                 gimp_filename_to_utf8 (plug_in->name),
                 gimp_filename_to_utf8 (plug_in->prog),
                 menu_path, proc_name);

      return FALSE;
    }

  if (! gimp_plug_in_procedure_add_menu_path (proc, menu_path, &error))
    {
      g_message (error->message);
      g_clear_error (&error);

      return FALSE;
    }

  if (GIMP_IS_TEMPORARY_PROCEDURE (proc) && ! plug_in->gimp->no_interface)
    {
      gimp_menus_create_item (plug_in->gimp, proc, menu_path);
    }

  return TRUE;
}

void
plug_in_add_temp_proc (PlugIn                 *plug_in,
                       GimpTemporaryProcedure *proc)
{
  g_return_if_fail (plug_in != NULL);
  g_return_if_fail (GIMP_IS_TEMPORARY_PROCEDURE (proc));

  plug_in->temp_procedures = g_slist_prepend (plug_in->temp_procedures,
                                              g_object_ref (proc));
  plug_ins_temp_procedure_add (plug_in->gimp, proc);
}

void
plug_in_remove_temp_proc (PlugIn                 *plug_in,
                          GimpTemporaryProcedure *proc)
{
  g_return_if_fail (plug_in != NULL);
  g_return_if_fail (GIMP_IS_TEMPORARY_PROCEDURE (proc));

  plug_in->temp_procedures = g_slist_remove (plug_in->temp_procedures,
                                             proc);
  plug_ins_temp_procedure_remove (plug_in->gimp, proc);
  g_object_unref (proc);
}
