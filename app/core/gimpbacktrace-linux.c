/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpbacktrace-linux.c
 * Copyright (C) 2018 Ell
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#define _GNU_SOURCE


#include "config.h"

#include <gio/gio.h>

#include "gimpbacktrace-backend.h"


#ifdef GIMP_BACKTRACE_BACKEND_LINUX


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_LIBBACKTRACE
#include <backtrace.h>
#endif

#ifdef HAVE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif

#include "core-types.h"

#include "gimpbacktrace.h"


#define MAX_N_THREADS        256
#define MAX_N_FRAMES         256
#define MAX_THREAD_NAME_SIZE 32
#define N_SKIPPED_FRAMES     2
#define MAX_WAIT_TIME        (G_TIME_SPAN_SECOND / 20)
#define BACKTRACE_SIGNAL     SIGUSR1


typedef struct _GimpBacktraceThread GimpBacktraceThread;


struct _GimpBacktraceThread
{
  pid_t    tid;
  gchar    name[MAX_THREAD_NAME_SIZE];
  gchar    state;

  guintptr frames[MAX_N_FRAMES];
  gint     n_frames;
};

struct _GimpBacktrace
{
  GimpBacktraceThread *threads;
  gint                 n_threads;
};


/*  local function prototypes  */

static inline gint   gimp_backtrace_normalize_frame   (GimpBacktrace *backtrace,
                                                       gint           thread,
                                                       gint           frame);

static gint          gimp_backtrace_enumerate_threads (gboolean       include_current_thread,
                                                       pid_t         *threads,
                                                       gint           size);
static void          gimp_backtrace_read_thread_name  (pid_t          tid,
                                                       gchar         *name,
                                                       gint           size);
static gchar         gimp_backtrace_read_thread_state (pid_t          tid);

static void          gimp_backtrace_signal_handler    (gint           signum);


/*  static variables  */

static GMutex            mutex;
static gint              n_initializations;
static gboolean          initialized;
static struct sigaction  orig_action;
static pid_t             blacklisted_threads[MAX_N_THREADS];
static gint              n_blacklisted_threads;
static GimpBacktrace    *handler_backtrace;
static gint              handler_n_remaining_threads;
static gint              handler_lock;

#ifdef HAVE_LIBBACKTRACE
static struct backtrace_state *backtrace_state;
#endif

static const gchar * const blacklisted_thread_names[] =
{
  "gmain",
  "threaded-ml"
};


/*  private functions  */


static inline gint
gimp_backtrace_normalize_frame (GimpBacktrace *backtrace,
                                gint           thread,
                                gint           frame)
{
  if (frame >= 0)
    return frame + N_SKIPPED_FRAMES;
  else
    return backtrace->threads[thread].n_frames + frame;
}

static gint
gimp_backtrace_enumerate_threads (gboolean  include_current_thread,
                                  pid_t    *threads,
                                  gint      size)
{
  DIR           *dir;
  struct dirent *dirent;
  pid_t          tid;
  gint           n_threads;

  dir = opendir ("/proc/self/task");

  if (! dir)
    return 0;

  tid = syscall (SYS_gettid);

  n_threads = 0;

  while (n_threads < size && (dirent = readdir (dir)))
    {
      pid_t id = g_ascii_strtoull (dirent->d_name, NULL, 10);

      if (id)
        {
          if (! include_current_thread && id == tid)
            id = 0;
        }

      if (id)
        {
          gint i;

          for (i = 0; i < n_blacklisted_threads; i++)
            {
              if (id == blacklisted_threads[i])
                {
                  id = 0;

                  break;
                }
            }
        }

      if (id)
        threads[n_threads++] = id;
    }

  closedir (dir);

  return n_threads;
}

static void
gimp_backtrace_read_thread_name (pid_t  tid,
                                 gchar *name,
                                 gint   size)
{
  gchar filename[64];
  gint  fd;

  if (size <= 0)
    return;

  name[0] = '\0';

  g_snprintf (filename, sizeof (filename),
              "/proc/self/task/%llu/comm",
              (unsigned long long) tid);

  fd = open (filename, O_RDONLY);

  if (fd >= 0)
    {
      gint n = read (fd, name, size);

      if (n > 0)
        name[n - 1] = '\0';

      close (fd);
    }
}

static gchar
gimp_backtrace_read_thread_state (pid_t tid)
{
  gchar buffer[64];
  gint  fd;
  gchar state = '\0';

  g_snprintf (buffer, sizeof (buffer),
              "/proc/self/task/%llu/stat",
              (unsigned long long) tid);

  fd = open (buffer, O_RDONLY);

  if (fd >= 0)
    {
      gint n = read (fd, buffer, sizeof (buffer));

      if (n > 0)
        buffer[n - 1] = '\0';

      sscanf (buffer, "%*d %*s %c", &state);

      close (fd);
    }

  return state;
}

static void
gimp_backtrace_signal_handler (gint signum)
{
  GimpBacktrace *curr_backtrace;
  gint           lock;

  do
    {
      lock = g_atomic_int_get (&handler_lock);

      if (lock < 0)
        continue;
    }
  while (! g_atomic_int_compare_and_exchange (&handler_lock, lock, lock + 1));

  curr_backtrace = g_atomic_pointer_get (&handler_backtrace);

  if (curr_backtrace)
    {
      pid_t tid = syscall (SYS_gettid);
      gint  i;

      for (i = 0; i < curr_backtrace->n_threads; i++)
        {
          GimpBacktraceThread *thread = &curr_backtrace->threads[i];

          if (thread->tid == tid)
            {
              thread->n_frames = backtrace ((gpointer *) thread->frames,
                                            MAX_N_FRAMES);

              g_atomic_int_dec_and_test (&handler_n_remaining_threads);

              break;
            }
        }
    }

  g_atomic_int_dec_and_test (&handler_lock);
}


/*  public functions  */


void
gimp_backtrace_init (void)
{
#ifdef HAVE_LIBBACKTRACE
  backtrace_state = backtrace_create_state (NULL, 0, NULL, NULL);
#endif
}

gboolean
gimp_backtrace_start (void)
{
  g_mutex_lock (&mutex);

  if (n_initializations == 0)
    {
      struct sigaction action = {};

      action.sa_handler = gimp_backtrace_signal_handler;
      action.sa_flags   = SA_RESTART;

      sigemptyset (&action.sa_mask);

      if (sigaction (BACKTRACE_SIGNAL, &action, &orig_action) == 0)
        {
          pid_t *threads;
          gint   n_threads;
          gint   i;

          n_blacklisted_threads = 0;

          threads = g_new (pid_t, MAX_N_THREADS);

          n_threads = gimp_backtrace_enumerate_threads (TRUE,
                                                        threads, MAX_N_THREADS);

          for (i = 0; i < n_threads; i++)
            {
              gchar name[MAX_THREAD_NAME_SIZE];
              gint  j;

              gimp_backtrace_read_thread_name (threads[i],
                                               name, MAX_THREAD_NAME_SIZE);

              for (j = 0; j < G_N_ELEMENTS (blacklisted_thread_names); j++)
                {
                  if (! strcmp (name, blacklisted_thread_names[j]))
                    {
                      blacklisted_threads[n_blacklisted_threads++] = threads[i];
                    }
                }
            }

          g_free (threads);

          initialized = TRUE;
        }
    }

  n_initializations++;

  g_mutex_unlock (&mutex);

  return initialized;
}

void
gimp_backtrace_stop (void)
{
  g_return_if_fail (n_initializations > 0);

  g_mutex_lock (&mutex);

  n_initializations--;

  if (n_initializations == 0 && initialized)
    {
      if (sigaction (BACKTRACE_SIGNAL, &orig_action, NULL) < 0)
        g_warning ("failed to restore original backtrace signal handler");

      initialized = FALSE;
    }

  g_mutex_unlock (&mutex);
}

GimpBacktrace *
gimp_backtrace_new (gboolean include_current_thread)
{
  GimpBacktrace *backtrace;
  pid_t          pid;
  pid_t         *threads;
  gint           n_threads;
  gint64         start_time;
  gint           i;

  if (! initialized)
    return NULL;

  pid = getpid ();

  threads = g_new (pid_t, MAX_N_THREADS);

  n_threads = gimp_backtrace_enumerate_threads (include_current_thread,
                                                threads, MAX_N_THREADS);

  if (n_threads == 0)
    {
      g_free (threads);

      return NULL;
    }

  g_mutex_lock (&mutex);

  backtrace = g_slice_new (GimpBacktrace);

  backtrace->threads   = g_new (GimpBacktraceThread, n_threads);
  backtrace->n_threads = n_threads;

  while (! g_atomic_int_compare_and_exchange (&handler_lock, 0, -1));

  g_atomic_pointer_set (&handler_backtrace,           backtrace);
  g_atomic_int_set     (&handler_n_remaining_threads, n_threads);

  g_atomic_int_set (&handler_lock, 0);

  for (i = 0; i < n_threads; i++)
    {
      GimpBacktraceThread *thread = &backtrace->threads[i];

      thread->tid      = threads[i];
      thread->n_frames = 0;

      gimp_backtrace_read_thread_name (thread->tid,
                                       thread->name, MAX_THREAD_NAME_SIZE);

      thread->state = gimp_backtrace_read_thread_state (thread->tid);

      syscall (SYS_tgkill, pid, threads[i], BACKTRACE_SIGNAL);
    }

  g_free (threads);

  start_time = g_get_monotonic_time ();

  while (g_atomic_int_get (&handler_n_remaining_threads) > 0)
    {
      gint64 time = g_get_monotonic_time ();

      if (time - start_time > MAX_WAIT_TIME)
        break;

      g_usleep (1000);
    }

  while (! g_atomic_int_compare_and_exchange (&handler_lock, 0, -1));

  g_atomic_pointer_set (&handler_backtrace, NULL);

  g_atomic_int_set (&handler_lock, 0);

#if 0
  if (handler_n_remaining_threads > 0)
    {
      gint j = 0;

      for (i = 0; i < n_threads; i++)
        {
          if (backtrace->threads[i].n_frames == 0)
            {
              if (n_blacklisted_threads < MAX_N_THREADS)
                {
                  blacklisted_threads[n_blacklisted_threads++] =
                    backtrace->threads[i].tid;
                }
            }
          else
            {
              if (j < i)
                backtrace->threads[j] = backtrace->threads[i];

              j++;
            }
        }

      n_threads = j;
    }
#endif

  g_mutex_unlock (&mutex);

  if (n_threads == 0)
    {
      gimp_backtrace_free (backtrace);

      return NULL;
    }

  return backtrace;
}

void
gimp_backtrace_free (GimpBacktrace *backtrace)
{
  if (! backtrace)
    return;

  g_free (backtrace->threads);

  g_slice_free (GimpBacktrace, backtrace);
}

gint
gimp_backtrace_get_n_threads (GimpBacktrace *backtrace)
{
  g_return_val_if_fail (backtrace != NULL, 0);

  return backtrace->n_threads;
}

guintptr
gimp_backtrace_get_thread_id (GimpBacktrace *backtrace,
                              gint           thread)
{
  g_return_val_if_fail (backtrace != NULL, 0);
  g_return_val_if_fail (thread >= 0 && thread < backtrace->n_threads, 0);

  return backtrace->threads[thread].tid;
}

const gchar *
gimp_backtrace_get_thread_name (GimpBacktrace *backtrace,
                                gint           thread)
{
  g_return_val_if_fail (backtrace != NULL, NULL);
  g_return_val_if_fail (thread >= 0 && thread < backtrace->n_threads, NULL);

  if (backtrace->threads[thread].name[0])
    return backtrace->threads[thread].name;
  else
    return NULL;
}

gboolean
gimp_backtrace_is_thread_running (GimpBacktrace *backtrace,
                                  gint           thread)
{
  g_return_val_if_fail (backtrace != NULL, FALSE);
  g_return_val_if_fail (thread >= 0 && thread < backtrace->n_threads, FALSE);

  return backtrace->threads[thread].state == 'R';
}

gint
gimp_backtrace_find_thread_by_id (GimpBacktrace *backtrace,
                                  guintptr       thread_id,
                                  gint           thread_hint)
{
  pid_t tid = thread_id;
  gint  i;

  g_return_val_if_fail (backtrace != NULL, -1);

  if (thread_hint >= 0                    &&
      thread_hint <  backtrace->n_threads &&
      backtrace->threads[thread_hint].tid == tid)
    {
      return thread_hint;
    }

  for (i = 0; i < backtrace->n_threads; i++)
    {
      if (backtrace->threads[i].tid == tid)
        return i;
    }

  return -1;
}

gint
gimp_backtrace_get_n_frames (GimpBacktrace *backtrace,
                             gint           thread)
{
  g_return_val_if_fail (backtrace != NULL, 0);
  g_return_val_if_fail (thread >= 0 && thread < backtrace->n_threads, 0);

  return MAX (backtrace->threads[thread].n_frames - N_SKIPPED_FRAMES, 0);
}

guintptr
gimp_backtrace_get_frame_address (GimpBacktrace *backtrace,
                                  gint           thread,
                                  gint           frame)
{
  g_return_val_if_fail (backtrace != NULL, 0);
  g_return_val_if_fail (thread >= 0 && thread < backtrace->n_threads, 0);

  frame = gimp_backtrace_normalize_frame (backtrace, thread, frame);

  g_return_val_if_fail (frame >= N_SKIPPED_FRAMES &&
                        frame <  backtrace->threads[thread].n_frames, 0);

  return backtrace->threads[thread].frames[frame];
}

#ifdef HAVE_LIBBACKTRACE
static void
gimp_backtrace_syminfo_callback (GimpBacktraceAddressInfo *info,
                                 guintptr                  pc,
                                 const gchar              *symname,
                                 guintptr                  symval,
                                 guintptr                  symsize)
{
  if (symname)
    g_strlcpy (info->symbol_name, symname, sizeof (info->symbol_name));

  info->symbol_address = symval;
}

static gint
gimp_backtrace_pcinfo_callback (GimpBacktraceAddressInfo *info,
                                guintptr                  pc,
                                const gchar              *filename,
                                gint                      lineno,
                                const gchar              *function)
{
  if (function)
    g_strlcpy (info->symbol_name, function, sizeof (info->symbol_name));

  if (filename)
    g_strlcpy (info->source_file, filename, sizeof (info->source_file));

  info->source_line = lineno;

  return 0;
}
#endif /* HAVE_LIBBACKTRACE */

gboolean
gimp_backtrace_get_address_info (guintptr                  address,
                                 GimpBacktraceAddressInfo *info)
{
  Dl_info  dl_info;
  gboolean result = FALSE;

  g_return_val_if_fail (info != NULL, FALSE);

  info->object_name[0] = '\0';

  info->symbol_name[0] = '\0';
  info->symbol_address = 0;

  info->source_file[0] = '\0';
  info->source_line    = 0;

  if (dladdr ((gpointer) address, &dl_info))
    {
      if (dl_info.dli_fname)
        {
          g_strlcpy (info->object_name, dl_info.dli_fname,
                     sizeof (info->object_name));
        }

      if (dl_info.dli_sname)
        {
          g_strlcpy (info->symbol_name, dl_info.dli_sname,
                     sizeof (info->symbol_name));
        }

      info->symbol_address = (guintptr) dl_info.dli_saddr;

      result = TRUE;
    }

#ifdef HAVE_LIBBACKTRACE
  if (backtrace_state)
    {
      backtrace_syminfo (
        backtrace_state, address,
        (backtrace_syminfo_callback) gimp_backtrace_syminfo_callback,
        NULL,
        info);

      backtrace_pcinfo (
        backtrace_state, address,
        (backtrace_full_callback) gimp_backtrace_pcinfo_callback,
        NULL,
        info);

      result = TRUE;
    }
#endif /* HAVE_LIBBACKTRACE */

#ifdef HAVE_LIBUNWIND
/* we use libunwind to get the symbol name, when available, even if dladdr() or
 * libbacktrace already found one, since it provides more descriptive names in
 * some cases, and, in particular, full symbol names for C++ lambdas.
 *
 * note that, in some cases, this can result in a discrepancy between the
 * symbol name, and the corresponding source location.
 */
#if 0
  if (! info->symbol_name[0])
#endif
    {
      unw_context_t context = {};
      unw_cursor_t  cursor;
      unw_word_t    offset;

      if (unw_init_local (&cursor, &context)         == 0 &&
          unw_set_reg (&cursor, UNW_REG_IP, address) == 0 &&
          unw_get_proc_name (&cursor,
                             info->symbol_name, sizeof (info->symbol_name),
                             &offset)                == 0)
        {
          info->symbol_address = address - offset;

          result = TRUE;
        }
    }
#endif /* HAVE_LIBUNWIND */

  return result;
}


#endif /* GIMP_BACKTRACE_BACKEND_LINUX */
