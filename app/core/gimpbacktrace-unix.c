/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpbacktrace-unix.c
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


#ifdef GIMP_BACKTRACE_BACKEND_UNIX


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
#ifdef __APPLE__
#include <pthread.h>
#include <mach/mach.h>
#endif

#if defined(HAVE_LIBBACKTRACE) && !defined(__APPLE__)
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
#if defined(__gnu_linux__)
  pid_t     tid;
#elif defined (__APPLE__)
  pthread_t tid;
#endif
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

#if defined(__gnu_linux__)
static gint          gimp_backtrace_enumerate_threads (gboolean       include_current_thread,
                                                       pid_t         *threads,
                                                       gint           size);
static void          gimp_backtrace_read_thread_name  (pid_t          tid,
                                                       gchar         *name,
                                                       gint           size);
static gchar         gimp_backtrace_read_thread_state (pid_t          tid);
#elif defined(__APPLE__)
static gint          gimp_backtrace_enumerate_threads (gboolean       include_current_thread,
                                                       pthread_t     *threads,
                                                       gint           size);
static void          gimp_backtrace_read_thread_name  (pthread_t      tid,
                                                       gchar         *name,
                                                       gint           size);
static gchar         gimp_backtrace_read_thread_state (pthread_t      tid);
#endif

static void          gimp_backtrace_signal_handler    (gint           signum);


/*  static variables  */

static GMutex            mutex;
static gint              n_initializations;
static gboolean          initialized;
static struct sigaction  orig_action;
#if defined(__gnu_linux__)
static pid_t             blacklisted_threads[MAX_N_THREADS];
#elif defined(__APPLE__)
static pthread_t         blacklisted_threads[MAX_N_THREADS];
#endif
static gint              n_blacklisted_threads;
static GimpBacktrace    *handler_backtrace;
static gint              handler_n_remaining_threads;
static gint              handler_lock;

#if defined(HAVE_LIBBACKTRACE) && !defined(__APPLE__)
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
#if defined(__gnu_linux__)
                                  pid_t    *threads,
#elif defined(__APPLE__)
                                  pthread_t *threads,
#endif
                                  gint      size)
{
#if defined(__gnu_linux__)
  DIR           *dir;
  struct dirent *dirent;
  pid_t          tid;
#elif defined(__APPLE__)
  kern_return_t          dir;
  thread_act_array_t     thread_list;
  mach_msg_type_number_t thread_count;
  mach_msg_type_number_t j;
  pthread_t              tid;
#endif
  gint           n_threads;

#if defined(__gnu_linux__)
  dir = opendir ("/proc/self/task");
#elif defined(__APPLE__)
  dir = task_threads (mach_task_self (), &thread_list, &thread_count);
#endif

#if defined(__gnu_linux__)
  if (! dir)
#elif defined(__APPLE__)
  if (dir != KERN_SUCCESS)
#endif
    return 0;

#if defined(__gnu_linux__)
  tid = syscall (SYS_gettid);
#elif defined(__APPLE__)
  tid = pthread_self ();
#endif

  n_threads = 0;

#if defined(__gnu_linux__)
  while (n_threads < size && (dirent = readdir (dir)))
#elif defined(__APPLE__)
  for (j = 0; j < thread_count && n_threads < size; j++)
#endif
    {
#if defined(__gnu_linux__)
      pid_t id = g_ascii_strtoull (dirent->d_name, NULL, 10);
#elif defined(__APPLE__)
      pthread_t id = pthread_from_mach_thread_np (thread_list[j]);
#endif

      if (id)
        {
#if defined(__gnu_linux__)
          if (! include_current_thread && id == tid)
            id = 0;
#elif defined(__APPLE__)
          if (! include_current_thread && pthread_equal (id, tid))
            id = NULL;
#endif
        }

      if (id)
        {
          gint i;

          for (i = 0; i < n_blacklisted_threads; i++)
            {
#if defined(__gnu_linux__)
              if (id == blacklisted_threads[i])
#elif defined(__APPLE__)
              if (pthread_equal (id, blacklisted_threads[i]))
#endif
                {
#if defined(__gnu_linux__)
                  id = 0;
#elif defined(__APPLE__)
                  id = NULL;
#endif

                  break;
                }
            }
        }

      if (id)
        threads[n_threads++] = id;
    }

#if defined(__gnu_linux__)
    closedir (dir);
#elif defined(__APPLE__)
  for (j = 0; j < thread_count; j++)
    {
      mach_port_deallocate (mach_task_self (), thread_list[j]);
    }
  vm_deallocate (mach_task_self (), (vm_address_t) thread_list, thread_count * sizeof (thread_t));
#endif

  return n_threads;
}

static void
#if defined(__gnu_linux__)
gimp_backtrace_read_thread_name (pid_t  tid,
                                 gchar *name,
                                 gint   size)
#elif defined(__APPLE__)
gimp_backtrace_read_thread_name (pthread_t tid,
                                 gchar    *name,
                                 gint      size)
#endif
{
#if defined(__gnu_linux__)
  gchar filename[64];
  gint  fd;
#endif

  if (size <= 0)
    return;

  name[0] = '\0';

#if defined(__gnu_linux__)
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
#elif defined(__APPLE__)
  pthread_getname_np (tid, name, size);
#endif
}

static gchar
#if defined(__gnu_linux__)
gimp_backtrace_read_thread_state (pid_t tid)
#elif defined(__APPLE__)
gimp_backtrace_read_thread_state (pthread_t tid)
#endif
{
#if defined(__gnu_linux__)
  gchar buffer[64];
  gint  fd;
#elif defined(__APPLE__)
  mach_port_t               fd;
  thread_basic_info_data_t info;
  mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
#endif
  gchar state = '\0';

#if defined(__gnu_linux__)
  g_snprintf (buffer, sizeof (buffer),
              "/proc/self/task/%llu/stat",
              (unsigned long long) tid);

  fd = open (buffer, O_RDONLY);
#elif defined(__APPLE__)
  fd = pthread_mach_thread_np (tid);
#endif

#if defined(__gnu_linux__)
  if (fd >= 0)
    {
      gint n = read (fd, buffer, sizeof (buffer));

      if (n > 0)
        buffer[n - 1] = '\0';

      sscanf (buffer, "%*d %*s %c", &state);

      close (fd);
    }
#elif defined(__APPLE__)
  if (thread_info (fd, THREAD_BASIC_INFO,
                   (thread_info_t) &info, &count) == KERN_SUCCESS)
    {
      switch (info.run_state)
        {
        case TH_STATE_RUNNING:
          state = 'R';
          break;
        case TH_STATE_STOPPED:
          state = 'T';
          break;
        case TH_STATE_WAITING:
        case TH_STATE_UNINTERRUPTIBLE:
          state = 'S';
          break;
        default:
          state = '\0';
          break;
        }
    }
#endif

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
#if defined(__gnu_linux__)
      pid_t tid = syscall (SYS_gettid);
#elif defined(__APPLE__)
      pthread_t tid = pthread_self ();
#endif
      gint  i;

      for (i = 0; i < curr_backtrace->n_threads; i++)
        {
          GimpBacktraceThread *thread = &curr_backtrace->threads[i];

#if defined(__gnu_linux__)
          if (thread->tid == tid)
#elif defined(__APPLE__)
          if (pthread_equal (thread->tid, tid))
#endif
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
#if defined(HAVE_LIBBACKTRACE) && !defined(__APPLE__)
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
#if defined(__gnu_linux__)
          pid_t *threads;
#elif defined(__APPLE__)
          pthread_t *threads;
#endif
          gint   n_threads;
          gint   i;

          n_blacklisted_threads = 0;

#if defined(__gnu_linux__)
          threads = g_new (pid_t, MAX_N_THREADS);
#elif defined(__APPLE__)
          threads = g_new (pthread_t, MAX_N_THREADS);
#endif

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
#if defined(__gnu_linux__)
  pid_t          pid;
  pid_t         *threads;
#elif defined(__APPLE__)
  pthread_t     *threads;
#endif
  gint           n_threads;
  gint64         start_time;
  gint           i;

  if (! initialized)
    return NULL;

#if defined(__gnu_linux__)
  pid = getpid ();

  threads = g_new (pid_t, MAX_N_THREADS);
#elif defined(__APPLE__)
  threads = g_new (pthread_t, MAX_N_THREADS);
#endif

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

#if defined(__gnu_linux__)
      syscall (SYS_tgkill, pid, threads[i], BACKTRACE_SIGNAL);
#elif defined(__APPLE__)
      pthread_kill (threads[i], BACKTRACE_SIGNAL);
#endif
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

#if defined(__gnu_linux__)
  return backtrace->threads[thread].tid;
#elif defined(__APPLE__)
  return (guintptr) backtrace->threads[thread].tid;
#endif
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
#if defined(__gnu_linux__)
  pid_t tid = thread_id;
#elif defined(__APPLE__)
  pthread_t tid = (pthread_t) thread_id;
#endif
  gint  i;

  g_return_val_if_fail (backtrace != NULL, -1);

#if defined(__gnu_linux__)
  if (thread_hint >= 0                    &&
      thread_hint <  backtrace->n_threads &&
      backtrace->threads[thread_hint].tid == tid)
#elif defined(__APPLE__)
  if (thread_hint >= 0                    &&
      thread_hint <  backtrace->n_threads &&
      pthread_equal (backtrace->threads[thread_hint].tid, tid))
#endif
    {
      return thread_hint;
    }

  for (i = 0; i < backtrace->n_threads; i++)
    {
#if defined(__gnu_linux__)
      if (backtrace->threads[i].tid == tid)
#elif defined(__APPLE__)
      if (pthread_equal (backtrace->threads[i].tid, tid))
#endif
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

#if defined(HAVE_LIBBACKTRACE) && !defined(__APPLE__)
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
#endif /* defined(HAVE_LIBBACKTRACE) && !defined(__APPLE__) */

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

#if defined(HAVE_LIBBACKTRACE) && !defined(__APPLE__)
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
#endif /* defined(HAVE_LIBBACKTRACE) && !defined(__APPLE__) */

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


#endif /* GIMP_BACKTRACE_BACKEND_UNIX */
