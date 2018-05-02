/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugin.c
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#ifndef _WIN32
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)

#define STRICT
#include <windows.h>
#include <process.h>

#ifdef G_OS_WIN32
#include <fcntl.h>
#include <io.h>

#ifndef pipe
#define pipe(fds) _pipe(fds, 4096, _O_BINARY)
#endif

#endif

#ifdef G_WITH_CYGWIN
#define O_TEXT    0x0100  /* text file   */
#define _O_TEXT   0x0100  /* text file   */
#define O_BINARY  0x0200  /* binary file */
#define _O_BINARY 0x0200  /* binary file */
#endif

#endif /* G_OS_WIN32 || G_WITH_CYGWIN */

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "plug-in-types.h"

#include "core/gimp.h"
#include "core/gimp-spawn.h"
#include "core/gimpprogress.h"

#include "pdb/gimppdbcontext.h"

#include "gimpenvirontable.h"
#include "gimpinterpreterdb.h"
#include "gimpplugin.h"
#include "gimpplugin-message.h"
#include "gimpplugin-progress.h"
#include "gimpplugindebug.h"
#include "gimpplugindef.h"
#include "gimppluginmanager.h"
#include "gimppluginmanager-help-domain.h"
#include "gimppluginmanager-locale-domain.h"
#include "gimptemporaryprocedure.h"
#include "plug-in-params.h"

#include "gimp-intl.h"


static void       gimp_plug_in_finalize      (GObject      *object);

static gboolean   gimp_plug_in_write         (GIOChannel   *channel,
                                              const guint8 *buf,
                                              gulong        count,
                                              gpointer      data);
static gboolean   gimp_plug_in_flush         (GIOChannel   *channel,
                                              gpointer      data);

static gboolean   gimp_plug_in_recv_message  (GIOChannel   *channel,
                                              GIOCondition  cond,
                                              gpointer      data);



G_DEFINE_TYPE (GimpPlugIn, gimp_plug_in, GIMP_TYPE_OBJECT)

#define parent_class gimp_plug_in_parent_class


static void
gimp_plug_in_class_init (GimpPlugInClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_plug_in_finalize;

  /* initialize the gimp protocol library and set the read and
   *  write handlers.
   */
  gp_init ();
  gimp_wire_set_writer (gimp_plug_in_write);
  gimp_wire_set_flusher (gimp_plug_in_flush);
}

static void
gimp_plug_in_init (GimpPlugIn *plug_in)
{
  plug_in->manager            = NULL;
  plug_in->file               = NULL;

  plug_in->call_mode          = GIMP_PLUG_IN_CALL_NONE;
  plug_in->open               = FALSE;
  plug_in->hup                = FALSE;
  plug_in->pid                = 0;

  plug_in->my_read            = NULL;
  plug_in->my_write           = NULL;
  plug_in->his_read           = NULL;
  plug_in->his_write          = NULL;

  plug_in->input_id           = 0;
  plug_in->write_buffer_index = 0;

  plug_in->temp_procedures    = NULL;

  plug_in->ext_main_loop      = NULL;

  plug_in->temp_proc_frames   = NULL;

  plug_in->plug_in_def        = NULL;
}

static void
gimp_plug_in_finalize (GObject *object)
{
  GimpPlugIn *plug_in = GIMP_PLUG_IN (object);

  g_clear_object (&plug_in->file);

  gimp_plug_in_proc_frame_dispose (&plug_in->main_proc_frame, plug_in);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

GimpPlugIn *
gimp_plug_in_new (GimpPlugInManager   *manager,
                  GimpContext         *context,
                  GimpProgress        *progress,
                  GimpPlugInProcedure *procedure,
                  GFile               *file)
{
  GimpPlugIn *plug_in;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (GIMP_IS_PDB_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (procedure == NULL ||
                        GIMP_IS_PLUG_IN_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (file == NULL || G_IS_FILE (file), NULL);
  g_return_val_if_fail ((procedure != NULL || file != NULL) &&
                        ! (procedure != NULL && file != NULL), NULL);

  plug_in = g_object_new (GIMP_TYPE_PLUG_IN, NULL);

  if (! file)
    file = gimp_plug_in_procedure_get_file (procedure);

  gimp_object_take_name (GIMP_OBJECT (plug_in),
                         g_path_get_basename (gimp_file_get_utf8_name (file)));

  plug_in->manager = manager;
  plug_in->file    = g_object_ref (file);

  gimp_plug_in_proc_frame_init (&plug_in->main_proc_frame,
                                context, progress, procedure);

  return plug_in;
}

gboolean
gimp_plug_in_open (GimpPlugIn         *plug_in,
                   GimpPlugInCallMode  call_mode,
                   gboolean            synchronous)
{
  gchar        *progname;
  gint          my_read[2];
  gint          my_write[2];
  gchar       **envp;
  const gchar  *args[10];
  gchar       **argv;
  gint          argc;
  gchar         protocol_version[8];
  gchar        *interp, *interp_arg;
  gchar        *his_read_fd, *his_write_fd;
  const gchar  *mode;
  gchar        *stm;
  GError       *error = NULL;
  gboolean      debug;
  guint         debug_flag;
  guint         spawn_flags;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (plug_in->call_mode == GIMP_PLUG_IN_CALL_NONE, FALSE);

  /* Open two pipes. (Bidirectional communication).
   */
  if ((pipe (my_read) == -1) || (pipe (my_write) == -1))
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Unable to run plug-in \"%s\"\n(%s)\n\npipe() failed: %s",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file),
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

  /* Prevent the plug-in from inheriting our end of the pipes */
  gimp_spawn_set_cloexec (my_read[0]);
  gimp_spawn_set_cloexec (my_write[1]);

#ifdef G_OS_WIN32
  plug_in->my_read   = g_io_channel_win32_new_fd (my_read[0]);
  plug_in->my_write  = g_io_channel_win32_new_fd (my_write[1]);
  plug_in->his_read  = g_io_channel_win32_new_fd (my_write[0]);
  plug_in->his_write = g_io_channel_win32_new_fd (my_read[1]);
#else
  plug_in->my_read   = g_io_channel_unix_new (my_read[0]);
  plug_in->my_write  = g_io_channel_unix_new (my_write[1]);
  plug_in->his_read  = g_io_channel_unix_new (my_write[0]);
  plug_in->his_write = g_io_channel_unix_new (my_read[1]);
#endif

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
  his_read_fd  = g_strdup_printf ("%d",
                                  g_io_channel_unix_get_fd (plug_in->his_read));
  his_write_fd = g_strdup_printf ("%d",
                                  g_io_channel_unix_get_fd (plug_in->his_write));

  switch (call_mode)
    {
    case GIMP_PLUG_IN_CALL_QUERY:
      mode = "-query";
      debug_flag = GIMP_DEBUG_WRAP_QUERY;
      break;

    case GIMP_PLUG_IN_CALL_INIT:
      mode = "-init";
      debug_flag = GIMP_DEBUG_WRAP_INIT;
      break;

    case GIMP_PLUG_IN_CALL_RUN:
      mode = "-run";
      debug_flag = GIMP_DEBUG_WRAP_RUN;
      break;

    default:
      gimp_assert_not_reached ();
    }

  stm = g_strdup_printf ("%d", plug_in->manager->gimp->stack_trace_mode);

  progname = g_file_get_path (plug_in->file);

  interp = gimp_interpreter_db_resolve (plug_in->manager->interpreter_db,
                                        progname, &interp_arg);

  argc = 0;

  if (interp)
    args[argc++] = interp;

  if (interp_arg)
    args[argc++] = interp_arg;

  g_snprintf (protocol_version, sizeof (protocol_version),
              "%d", GIMP_PROTOCOL_VERSION);

  args[argc++] = progname;
  args[argc++] = "-gimp";
  args[argc++] = protocol_version;
  args[argc++] = his_read_fd;
  args[argc++] = his_write_fd;
  args[argc++] = mode;
  args[argc++] = stm;
  args[argc++] = NULL;

  argv = (gchar **) args;
  envp = gimp_environ_table_get_envp (plug_in->manager->environ_table);
  spawn_flags = (G_SPAWN_LEAVE_DESCRIPTORS_OPEN |
                 G_SPAWN_DO_NOT_REAP_CHILD      |
                 G_SPAWN_CHILD_INHERITS_STDIN);

  debug = FALSE;

  if (plug_in->manager->debug)
    {
      gchar **debug_argv = gimp_plug_in_debug_argv (plug_in->manager->debug,
                                                    progname,
                                                    debug_flag, args);

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
  if (! gimp_spawn_async (argv, envp, spawn_flags, &plug_in->pid, &error))
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Unable to run plug-in \"%s\"\n(%s)\n\n%s",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file),
                    error->message);
      g_clear_error (&error);
      goto cleanup;
    }

  g_clear_pointer (&plug_in->his_read,  g_io_channel_unref);
  g_clear_pointer (&plug_in->his_write, g_io_channel_unref);

  if (! synchronous)
    {
      GSource *source;

      source = g_io_create_watch (plug_in->my_read,
                                  G_IO_IN  | G_IO_PRI | G_IO_ERR | G_IO_HUP);

      g_source_set_callback (source,
                             (GSourceFunc) gimp_plug_in_recv_message, plug_in,
                             NULL);

      g_source_set_can_recurse (source, TRUE);

      plug_in->input_id = g_source_attach (source, NULL);
      g_source_unref (source);
    }

  plug_in->open      = TRUE;
  plug_in->call_mode = call_mode;

  gimp_plug_in_manager_add_open_plug_in (plug_in->manager, plug_in);

 cleanup:

  if (debug)
    g_free (argv);

  g_free (his_read_fd);
  g_free (his_write_fd);
  g_free (stm);
  g_free (interp);
  g_free (interp_arg);
  g_free (progname);

  return plug_in->open;
}

void
gimp_plug_in_close (GimpPlugIn *plug_in,
                    gboolean    kill_it)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (plug_in->open);

  plug_in->open = FALSE;

  if (plug_in->pid)
    {
#ifndef G_OS_WIN32
      gint status;
#endif

      /*  Ask the filter to exit gracefully,
          but not if it is closed because of a broken pipe.  */
      if (kill_it && ! plug_in->hup)
        {
          gp_quit_write (plug_in->my_write, plug_in);

          /*  give the plug-in some time (10 ms)  */
          g_usleep (10000);
        }

      /* If necessary, kill the filter. */

#ifndef G_OS_WIN32

      if (kill_it)
        {
          if (plug_in->manager->gimp->be_verbose)
            g_print ("Terminating plug-in: '%s'\n",
                     gimp_file_get_utf8_name (plug_in->file));

          /*  If the plug-in opened a process group, kill the group instead
           *  of only the plug-in, so we kill the plug-in's children too
           */
          if (getpgid (0) != getpgid (plug_in->pid))
            status = kill (- plug_in->pid, SIGKILL);
          else
            status = kill (plug_in->pid, SIGKILL);
        }

      /* Wait for the process to exit. This will happen
       * immediately if it was just killed.
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

          while (dwExitCode == STILL_ACTIVE &&
                 GetExitCodeProcess ((HANDLE) plug_in->pid, &dwExitCode) &&
                 (dwTries > 0))
            {
              Sleep (10);
              dwTries--;
            }

          if (dwExitCode == STILL_ACTIVE)
            {
              if (plug_in->manager->gimp->be_verbose)
                g_print ("Terminating plug-in: '%s'\n",
                         gimp_file_get_utf8_name (plug_in->file));

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
  g_clear_pointer (&plug_in->my_read,   g_io_channel_unref);
  g_clear_pointer (&plug_in->my_write,  g_io_channel_unref);
  g_clear_pointer (&plug_in->his_read,  g_io_channel_unref);
  g_clear_pointer (&plug_in->his_write, g_io_channel_unref);

  gimp_wire_clear_error ();

  while (plug_in->temp_proc_frames)
    {
      GimpPlugInProcFrame *proc_frame = plug_in->temp_proc_frames->data;

#ifdef GIMP_UNSTABLE
      g_printerr ("plug-in '%s' aborted before sending its "
                  "temporary procedure return values\n",
                  gimp_object_get_name (plug_in));
#endif

      if (proc_frame->main_loop &&
          g_main_loop_is_running (proc_frame->main_loop))
        {
          g_main_loop_quit (proc_frame->main_loop);
        }

      /* pop the frame here, because normally this only happens in
       * gimp_plug_in_handle_temp_proc_return(), which can't
       * be called after plug_in_close()
       */
      gimp_plug_in_proc_frame_pop (plug_in);
    }

  if (plug_in->main_proc_frame.main_loop &&
      g_main_loop_is_running (plug_in->main_proc_frame.main_loop))
    {
#ifdef GIMP_UNSTABLE
      g_printerr ("plug-in '%s' aborted before sending its "
                  "procedure return values\n",
                  gimp_object_get_name (plug_in));
#endif

      g_main_loop_quit (plug_in->main_proc_frame.main_loop);
    }

  if (plug_in->ext_main_loop &&
      g_main_loop_is_running (plug_in->ext_main_loop))
    {
#ifdef GIMP_UNSTABLE
      g_printerr ("extension '%s' aborted before sending its "
                  "extension_ack message\n",
                  gimp_object_get_name (plug_in));
#endif

      g_main_loop_quit (plug_in->ext_main_loop);
    }

  /* Unregister any temporary procedures. */
  while (plug_in->temp_procedures)
    gimp_plug_in_remove_temp_proc (plug_in, plug_in->temp_procedures->data);

  gimp_plug_in_manager_remove_open_plug_in (plug_in->manager, plug_in);
}

static gboolean
gimp_plug_in_recv_message (GIOChannel   *channel,
                           GIOCondition  cond,
                           gpointer      data)
{
  GimpPlugIn *plug_in     = data;
  gboolean    got_message = FALSE;

#ifdef G_OS_WIN32
  /* Workaround for GLib bug #137968: sometimes we are called for no
   * reason...
   */
  if (cond == 0)
    return TRUE;
#endif

  if (plug_in->my_read == NULL)
    return TRUE;

  g_object_ref (plug_in);

  if (cond & (G_IO_IN | G_IO_PRI))
    {
      GimpWireMessage msg;

      memset (&msg, 0, sizeof (GimpWireMessage));

      if (! gimp_wire_read_msg (plug_in->my_read, &msg, plug_in))
        {
          gimp_plug_in_close (plug_in, TRUE);
        }
      else
        {
          gimp_plug_in_handle_message (plug_in, &msg);
          gimp_wire_destroy (&msg);
          got_message = TRUE;
        }
    }

  if (cond & (G_IO_ERR | G_IO_HUP))
    {
      if (cond & G_IO_HUP)
        plug_in->hup = TRUE;

      if (plug_in->open)
        gimp_plug_in_close (plug_in, TRUE);
    }

  if (! got_message)
    {
      GimpPlugInProcFrame *frame    = gimp_plug_in_get_proc_frame (plug_in);
      GimpProgress        *progress = frame ? frame->progress : NULL;

      gimp_message (plug_in->manager->gimp, G_OBJECT (progress),
                    GIMP_MESSAGE_ERROR,
                    _("Plug-in crashed: \"%s\"\n(%s)\n\n"
                      "The dying plug-in may have messed up GIMP's internal "
                      "state. You may want to save your images and restart "
                      "GIMP to be on the safe side."),
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file));
    }

  g_object_unref (plug_in);

  return TRUE;
}

static gboolean
gimp_plug_in_write (GIOChannel   *channel,
                    const guint8 *buf,
                    gulong        count,
                    gpointer      data)
{
  GimpPlugIn *plug_in = data;
  gulong      bytes;

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
gimp_plug_in_flush (GIOChannel *channel,
                    gpointer    data)
{
  GimpPlugIn *plug_in = data;

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

GimpPlugInProcFrame *
gimp_plug_in_get_proc_frame (GimpPlugIn *plug_in)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  if (plug_in->temp_proc_frames)
    return plug_in->temp_proc_frames->data;
  else
    return &plug_in->main_proc_frame;
}

GimpPlugInProcFrame *
gimp_plug_in_proc_frame_push (GimpPlugIn             *plug_in,
                              GimpContext            *context,
                              GimpProgress           *progress,
                              GimpTemporaryProcedure *procedure)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (GIMP_IS_PDB_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (GIMP_IS_TEMPORARY_PROCEDURE (procedure), NULL);

  proc_frame = gimp_plug_in_proc_frame_new (context, progress,
                                            GIMP_PLUG_IN_PROCEDURE (procedure));

  plug_in->temp_proc_frames = g_list_prepend (plug_in->temp_proc_frames,
                                              proc_frame);

  return proc_frame;
}

void
gimp_plug_in_proc_frame_pop (GimpPlugIn *plug_in)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (plug_in->temp_proc_frames != NULL);

  proc_frame = (GimpPlugInProcFrame *) plug_in->temp_proc_frames->data;

  gimp_plug_in_proc_frame_unref (proc_frame, plug_in);

  plug_in->temp_proc_frames = g_list_remove (plug_in->temp_proc_frames,
                                             proc_frame);
}

void
gimp_plug_in_main_loop (GimpPlugIn *plug_in)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (plug_in->temp_proc_frames != NULL);

  proc_frame = (GimpPlugInProcFrame *) plug_in->temp_proc_frames->data;

  g_return_if_fail (proc_frame->main_loop == NULL);

  proc_frame->main_loop = g_main_loop_new (NULL, FALSE);

  gimp_threads_leave (plug_in->manager->gimp);
  g_main_loop_run (proc_frame->main_loop);
  gimp_threads_enter (plug_in->manager->gimp);

  g_clear_pointer (&proc_frame->main_loop, g_main_loop_unref);
}

void
gimp_plug_in_main_loop_quit (GimpPlugIn *plug_in)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (plug_in->temp_proc_frames != NULL);

  proc_frame = (GimpPlugInProcFrame *) plug_in->temp_proc_frames->data;

  g_return_if_fail (proc_frame->main_loop != NULL);

  g_main_loop_quit (proc_frame->main_loop);
}

const gchar *
gimp_plug_in_get_undo_desc (GimpPlugIn *plug_in)
{
  GimpPlugInProcFrame *proc_frame;
  const gchar         *undo_desc = NULL;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);

  if (proc_frame && proc_frame->procedure)
    undo_desc = gimp_procedure_get_label (proc_frame->procedure);

  return undo_desc ? undo_desc : gimp_object_get_name (plug_in);
}

/*  called from the PDB (gimp_plugin_menu_register)  */
gboolean
gimp_plug_in_menu_register (GimpPlugIn  *plug_in,
                            const gchar *proc_name,
                            const gchar *menu_path)
{
  GimpPlugInProcedure *proc  = NULL;
  GError              *error = NULL;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (proc_name != NULL, FALSE);
  g_return_val_if_fail (menu_path != NULL, FALSE);

  if (plug_in->plug_in_def)
    proc = gimp_plug_in_procedure_find (plug_in->plug_in_def->procedures,
                                        proc_name);

  if (! proc)
    proc = gimp_plug_in_procedure_find (plug_in->temp_procedures, proc_name);

  if (! proc)
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n"
                    "attempted to register the menu item \"%s\" "
                    "for the procedure \"%s\".\n"
                    "It has however not installed that procedure.  This "
                    "is not allowed.",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file),
                    menu_path, proc_name);

      return FALSE;
    }

  switch (GIMP_PROCEDURE (proc)->proc_type)
    {
    case GIMP_INTERNAL:
      return FALSE;

    case GIMP_PLUGIN:
    case GIMP_EXTENSION:
      if (plug_in->call_mode != GIMP_PLUG_IN_CALL_QUERY &&
          plug_in->call_mode != GIMP_PLUG_IN_CALL_INIT)
        return FALSE;

    case GIMP_TEMPORARY:
      break;
    }

  if (! proc->menu_label)
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n"
                    "attempted to register the menu item \"%s\" "
                    "for procedure \"%s\".\n"
                    "The menu label given in gimp_install_procedure() "
                    "already contained a path.  To make this work, "
                    "pass just the menu's label to "
                    "gimp_install_procedure().",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file),
                    menu_path, proc_name);

      return FALSE;
    }

  if (! strlen (proc->menu_label))
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n"
                    "attempted to register the procedure \"%s\" "
                    "in the menu \"%s\", but the procedure has no label.  "
                    "This is not allowed.",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file),
                    proc_name, menu_path);

      return FALSE;
    }

  if (! gimp_plug_in_procedure_add_menu_path (proc, menu_path, &error))
    {
      gimp_message_literal (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);

      return FALSE;
    }

  return TRUE;
}

void
gimp_plug_in_set_error_handler (GimpPlugIn          *plug_in,
                                GimpPDBErrorHandler  handler)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);

  if (proc_frame)
    proc_frame->error_handler = handler;
}

GimpPDBErrorHandler
gimp_plug_in_get_error_handler (GimpPlugIn *plug_in)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in),
                        GIMP_PDB_ERROR_HANDLER_INTERNAL);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);

  if (proc_frame)
    return proc_frame->error_handler;

  return GIMP_PDB_ERROR_HANDLER_INTERNAL;
}

void
gimp_plug_in_add_temp_proc (GimpPlugIn             *plug_in,
                            GimpTemporaryProcedure *proc)
{
  GimpPlugInProcedure *overridden;
  const gchar         *locale_domain;
  const gchar         *help_domain;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (GIMP_IS_TEMPORARY_PROCEDURE (proc));

  overridden = gimp_plug_in_procedure_find (plug_in->temp_procedures,
                                            gimp_object_get_name (proc));

  if (overridden)
    gimp_plug_in_remove_temp_proc (plug_in,
                                   GIMP_TEMPORARY_PROCEDURE (overridden));

  locale_domain = gimp_plug_in_manager_get_locale_domain (plug_in->manager,
                                                          plug_in->file,
                                                          NULL);
  help_domain = gimp_plug_in_manager_get_help_domain (plug_in->manager,
                                                      plug_in->file,
                                                      NULL);

  gimp_plug_in_procedure_set_locale_domain (GIMP_PLUG_IN_PROCEDURE (proc),
                                            locale_domain);
  gimp_plug_in_procedure_set_help_domain (GIMP_PLUG_IN_PROCEDURE (proc),
                                          help_domain);

  plug_in->temp_procedures = g_slist_prepend (plug_in->temp_procedures,
                                              g_object_ref (proc));
  gimp_plug_in_manager_add_temp_proc (plug_in->manager, proc);
}

void
gimp_plug_in_remove_temp_proc (GimpPlugIn             *plug_in,
                               GimpTemporaryProcedure *proc)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (GIMP_IS_TEMPORARY_PROCEDURE (proc));

  plug_in->temp_procedures = g_slist_remove (plug_in->temp_procedures, proc);

  gimp_plug_in_manager_remove_temp_proc (plug_in->manager, proc);
  g_object_unref (proc);
}

void
gimp_plug_in_enable_precision (GimpPlugIn *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  plug_in->precision = TRUE;
}

gboolean
gimp_plug_in_precision_enabled (GimpPlugIn *plug_in)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);

  return plug_in->precision;
}
