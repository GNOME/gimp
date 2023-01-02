/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpbacktrace-windows.c
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


#include "config.h"

#include <gio/gio.h>

#include "gimpbacktrace-backend.h"


#ifdef GIMP_BACKTRACE_BACKEND_WINDOWS


#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <dbghelp.h>

#include <string.h>

#include "core-types.h"

#include "gimpbacktrace.h"


#define MAX_N_THREADS               256
#define MAX_N_FRAMES                256
#define THREAD_ENUMERATION_INTERVAL G_TIME_SPAN_SECOND


typedef struct _Thread              Thread;
typedef struct _GimpBacktraceThread GimpBacktraceThread;


struct _Thread
{
  DWORD  tid;

  union
  {
    gchar   *name;
    guint64  time;
  };
};

struct _GimpBacktraceThread
{
  DWORD        tid;
  const gchar *name;
  guint64      time;
  guint64      last_time;

  guintptr     frames[MAX_N_FRAMES];
  gint         n_frames;
};

struct _GimpBacktrace
{
  GimpBacktraceThread *threads;
  gint                 n_threads;
};


/*  local function prototypes  */

static inline gint   gimp_backtrace_normalize_frame   (GimpBacktrace       *backtrace,
                                                       gint                 thread,
                                                       gint                 frame);

static void          gimp_backtrace_set_thread_name   (DWORD                tid,
                                                       const gchar         *name);

static gboolean      gimp_backtrace_enumerate_threads (void);

static LONG WINAPI   gimp_backtrace_exception_handler (PEXCEPTION_POINTERS  info);


/*  static variables  */

static GMutex    mutex;
static gint      n_initializations;
static gboolean  initialized;
Thread           threads[MAX_N_THREADS];
gint             n_threads;
gint64           last_thread_enumeration_time;
Thread           thread_names[MAX_N_THREADS];
gint             n_thread_names;
gint             thread_names_spinlock;
Thread           thread_times[MAX_N_THREADS];
gint             n_thread_times;

DWORD   WINAPI (* gimp_backtrace_SymSetOptions)        (DWORD            SymOptions);
BOOL    WINAPI (* gimp_backtrace_SymInitialize)        (HANDLE           hProcess,
                                                        PCSTR            UserSearchPath,
                                                        BOOL             fInvadeProcess);
BOOL    WINAPI (* gimp_backtrace_SymCleanup)           (HANDLE           hProcess);
BOOL    WINAPI (* gimp_backtrace_SymFromAddr)          (HANDLE           hProcess,
                                                        DWORD64          Address,
                                                        PDWORD64         Displacement,
                                                        PSYMBOL_INFO     Symbol);
BOOL    WINAPI (* gimp_backtrace_SymGetLineFromAddr64) (HANDLE           hProcess,
                                                        DWORD64          qwAddr,
                                                        PDWORD           pdwDisplacement,
                                                        PIMAGEHLP_LINE64 Line64);


/*  private functions  */


static inline gint
gimp_backtrace_normalize_frame (GimpBacktrace *backtrace,
                                gint           thread,
                                gint           frame)
{
  if (frame >= 0)
    return frame;
  else
    return backtrace->threads[thread].n_frames + frame;
}

static void
gimp_backtrace_set_thread_name (DWORD        tid,
                                const gchar *name)
{
  while (! g_atomic_int_compare_and_exchange (&thread_names_spinlock,
                                              0, 1));

  if (n_thread_names < MAX_N_THREADS)
    {
      Thread *thread = &thread_names[n_thread_names++];

      thread->tid  = tid;
      thread->name = g_strdup (name);
    }

  g_atomic_int_set (&thread_names_spinlock, 0);
}

static gboolean
gimp_backtrace_enumerate_threads (void)
{
  HANDLE        hThreadSnap;
  THREADENTRY32 te32;
  DWORD         pid;
  gint64        time;

  time = g_get_monotonic_time ();

  if (time - last_thread_enumeration_time < THREAD_ENUMERATION_INTERVAL)
    return n_threads > 0;

  last_thread_enumeration_time = time;

  n_threads = 0;

  hThreadSnap = CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, 0);

  if (hThreadSnap == INVALID_HANDLE_VALUE)
    return FALSE;

  te32.dwSize = sizeof (te32);

  if (! Thread32First (hThreadSnap, &te32))
    {
      CloseHandle (hThreadSnap);

      return FALSE;
    }

  pid = GetCurrentProcessId ();

  while (! g_atomic_int_compare_and_exchange (&thread_names_spinlock, 0, 1));

  do
    {
      if (n_threads == MAX_N_THREADS)
        break;

      if (te32.th32OwnerProcessID == pid)
        {
          Thread *thread = &threads[n_threads++];
          gint    i;

          thread->tid  = te32.th32ThreadID;
          thread->name = NULL;

          for (i = n_thread_names - 1; i >= 0; i--)
            {
              if (thread->tid == thread_names[i].tid)
                {
                  thread->name = thread_names[i].name;

                  break;
                }
            }
        }
    }
  while (Thread32Next (hThreadSnap, &te32));

  g_atomic_int_set (&thread_names_spinlock, 0);

  CloseHandle (hThreadSnap);

  return n_threads > 0;
}

static LONG WINAPI
gimp_backtrace_exception_handler (PEXCEPTION_POINTERS info)
{
  #define EXCEPTION_SET_THREAD_NAME ((DWORD) 0x406D1388)

  typedef struct _THREADNAME_INFO
  {
    DWORD  dwType;	    /* must be 0x1000                        */
    LPCSTR szName;	    /* pointer to name (in user addr space)  */
    DWORD  dwThreadID;	/* thread ID (-1=caller thread)          */
    DWORD  dwFlags;	    /* reserved for future use, must be zero */
  } THREADNAME_INFO;

  if (info->ExceptionRecord                   != NULL                      &&
      info->ExceptionRecord->ExceptionCode    == EXCEPTION_SET_THREAD_NAME &&
      info->ExceptionRecord->NumberParameters * 
      sizeof (ULONG_PTR)                      == sizeof (THREADNAME_INFO))
    {
      THREADNAME_INFO name_info;

      memcpy (&name_info, info->ExceptionRecord->ExceptionInformation,
              sizeof (name_info));

      if (name_info.dwType == 0x1000)
        {
          DWORD tid = name_info.dwThreadID;

          if (tid == -1)
            tid = GetCurrentThreadId ();

          gimp_backtrace_set_thread_name (tid, name_info.szName);

          return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

  return EXCEPTION_CONTINUE_SEARCH;

  #undef EXCEPTION_SET_THREAD_NAME
}


/*  public functions  */


void
gimp_backtrace_init (void)
{
  gimp_backtrace_set_thread_name (GetCurrentThreadId (), g_get_prgname ());

  AddVectoredExceptionHandler (TRUE, gimp_backtrace_exception_handler);
}

gboolean
gimp_backtrace_start (void)
{
  g_mutex_lock (&mutex);

  if (n_initializations == 0)
    {
      HMODULE hModule;
      DWORD   options;

      hModule = LoadLibraryW (L"mgwhelp.dll");

      #define INIT_PROC(name)                                    \
        G_STMT_START                                             \
          {                                                      \
            gimp_backtrace_##name = name;                        \
                                                                 \
            if (hModule)                                         \
              {                                                  \
                gpointer proc = GetProcAddress (hModule, #name); \
                                                                 \
                if (proc)                                        \
                  gimp_backtrace_##name = proc;                  \
              }                                                  \
          }                                                      \
        G_STMT_END

      INIT_PROC (SymSetOptions);
      INIT_PROC (SymInitialize);
      INIT_PROC (SymCleanup);
      INIT_PROC (SymFromAddr);
      INIT_PROC (SymGetLineFromAddr64);

      #undef INIT_PROC

      options = SymGetOptions ();

      options &= ~SYMOPT_UNDNAME;
      options |= SYMOPT_OMAP_FIND_NEAREST |
                 SYMOPT_DEFERRED_LOADS    |
                 SYMOPT_DEBUG;

#ifdef ARCH_X86_64
      options |= SYMOPT_INCLUDE_32BIT_MODULES;
#endif

      gimp_backtrace_SymSetOptions (options);

      if (gimp_backtrace_SymInitialize (GetCurrentProcess (), NULL, TRUE))
        {
          n_threads                    = 0;
          last_thread_enumeration_time = 0;
          n_thread_times               = 0;

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

  if (n_initializations == 0)
    {
      if (initialized)
        {
          gimp_backtrace_SymCleanup (GetCurrentProcess ());

          initialized = FALSE;
        }
    }

  g_mutex_unlock (&mutex);
}

GimpBacktrace *
gimp_backtrace_new (gboolean include_current_thread)
{
  GimpBacktrace *backtrace;
  HANDLE         hProcess;
  DWORD          tid;
  gint           i;

  if (! initialized)
    return NULL;

  g_mutex_lock (&mutex);

  if (! gimp_backtrace_enumerate_threads ())
    {
      g_mutex_unlock (&mutex);

      return NULL;
    }

  hProcess = GetCurrentProcess ();
  tid      = GetCurrentThreadId ();

  backtrace = g_slice_new (GimpBacktrace);

  backtrace->threads   = g_new (GimpBacktraceThread, n_threads);
  backtrace->n_threads = 0;

  for (i = 0; i < n_threads; i++)
    {
      GimpBacktraceThread *thread  = &backtrace->threads[backtrace->n_threads];
      HANDLE               hThread;
      CONTEXT              context = {};
      STACKFRAME64         frame   = {};
      DWORD                machine_type;
      FILETIME             creation_time;
      FILETIME             exit_time;
      FILETIME             kernel_time;
      FILETIME             user_time;

      if (! include_current_thread && threads[i].tid == tid)
        continue;

      hThread = OpenThread (THREAD_QUERY_INFORMATION |
                            THREAD_GET_CONTEXT       |
                            THREAD_SUSPEND_RESUME,
                            FALSE,
                            threads[i].tid);

      if (hThread == INVALID_HANDLE_VALUE)
        continue;

      if (threads[i].tid != tid && SuspendThread (hThread) == (DWORD) -1)
        {
          CloseHandle (hThread);

          continue;
        }

      context.ContextFlags = CONTEXT_FULL;

      if (! GetThreadContext (hThread, &context))
        {
          if (threads[i].tid != tid)
            ResumeThread (hThread);

          CloseHandle (hThread);

          continue;
        }

#ifdef ARCH_X86_64
      machine_type = IMAGE_FILE_MACHINE_AMD64;
      frame.AddrPC.Offset    = context.Rip;
      frame.AddrPC.Mode      = AddrModeFlat;
      frame.AddrStack.Offset = context.Rsp;
      frame.AddrStack.Mode   = AddrModeFlat;
      frame.AddrFrame.Offset = context.Rbp;
      frame.AddrFrame.Mode   = AddrModeFlat;
#elif defined (ARCH_X86)
      machine_type = IMAGE_FILE_MACHINE_I386;
      frame.AddrPC.Offset    = context.Eip;
      frame.AddrPC.Mode      = AddrModeFlat;
      frame.AddrStack.Offset = context.Esp;
      frame.AddrStack.Mode   = AddrModeFlat;
      frame.AddrFrame.Offset = context.Ebp;
      frame.AddrFrame.Mode   = AddrModeFlat;
#else
#error unsupported architecture
#endif

      thread->tid       = threads[i].tid;
      thread->name      = threads[i].name;
      thread->last_time = 0;
      thread->time      = 0;

      thread->n_frames = 0;

      while (thread->n_frames < MAX_N_FRAMES &&
             StackWalk64 (machine_type, hProcess, hThread, &frame, &context,
                          NULL,
                          SymFunctionTableAccess64,
                          SymGetModuleBase64,
                          NULL))
        {
          thread->frames[thread->n_frames++] = frame.AddrPC.Offset;

          if (frame.AddrPC.Offset == frame.AddrReturn.Offset)
            break;
        }

      if (GetThreadTimes (hThread,
                          &creation_time, &exit_time,
                          &kernel_time, &user_time))
        {
          thread->time = (((guint64) kernel_time.dwHighDateTime << 32) |
                          ((guint64) kernel_time.dwLowDateTime))       +
                         (((guint64) user_time.dwHighDateTime   << 32) |
                          ((guint64) user_time.dwLowDateTime));

          if (i < n_thread_times && thread->tid == thread_times[i].tid)
            {
              thread->last_time = thread_times[i].time;
            }
          else
            {
              gint j;

              for (j = 0; j < n_thread_times; j++)
                {
                  if (thread->tid == thread_times[j].tid)
                    {
                      thread->last_time = thread_times[j].time;

                      break;
                    }
                }
            }
        }

      if (threads[i].tid != tid)
        ResumeThread (hThread);

      CloseHandle (hThread);

      if (thread->n_frames > 0)
        backtrace->n_threads++;
    }

  n_thread_times = backtrace->n_threads;

  for (i = 0; i < backtrace->n_threads; i++)
    {
      thread_times[i].tid  = backtrace->threads[i].tid;
      thread_times[i].time = backtrace->threads[i].time;
    }

  g_mutex_unlock (&mutex);

  if (backtrace->n_threads == 0)
    {
      gimp_backtrace_free (backtrace);

      return NULL;
    }

  return backtrace;
}

void
gimp_backtrace_free (GimpBacktrace *backtrace)
{
  if (backtrace)
    {
      g_free (backtrace->threads);

      g_slice_free (GimpBacktrace, backtrace);
    }
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

  return backtrace->threads[thread].name;
}

gboolean
gimp_backtrace_is_thread_running (GimpBacktrace *backtrace,
                                  gint           thread)
{
  g_return_val_if_fail (backtrace != NULL, FALSE);
  g_return_val_if_fail (thread >= 0 && thread < backtrace->n_threads, FALSE);

  return backtrace->threads[thread].time >
         backtrace->threads[thread].last_time;
}

gint
gimp_backtrace_find_thread_by_id (GimpBacktrace *backtrace,
                                  guintptr       thread_id,
                                  gint           thread_hint)
{
  DWORD tid = thread_id;
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

  return backtrace->threads[thread].n_frames;
}

guintptr
gimp_backtrace_get_frame_address (GimpBacktrace *backtrace,
                                  gint           thread,
                                  gint           frame)
{
  g_return_val_if_fail (backtrace != NULL, 0);
  g_return_val_if_fail (thread >= 0 && thread < backtrace->n_threads, 0);

  frame = gimp_backtrace_normalize_frame (backtrace, thread, frame);

  g_return_val_if_fail (frame >= 0 &&
                        frame <  backtrace->threads[thread].n_frames, 0);

  return backtrace->threads[thread].frames[frame];
}

#define IN_MIDDLE_OF_UTF8_CODEPOINT(c) \
((c & 0xc0) == 0x80)

static void
utf8_copy_sized (char       *dest,
                 const char *src,
                 size_t      size)
{
  if (size == 0)
    return;

  strncpy (dest, src, size);

  if (dest[size - 1] != 0)
    {
      char *p = &dest[size - 1];

      /* Checking for p > dest is not actually needed, but
       * it's useful in case of malformed source string. */
      while (IN_MIDDLE_OF_UTF8_CODEPOINT (*p) && G_LIKELY (p > dest))
        *p-- = 0;

      *p = 0;
    }
}

#undef IN_MIDDLE_OF_UTF8_CODEPOINT

gboolean
gimp_backtrace_get_address_info (guintptr                  address,
                                 GimpBacktraceAddressInfo *info)
{
  SYMBOL_INFO     *symbol_info;
  HANDLE           hProcess;
  HMODULE          hModule;
  DWORD64          offset      = 0;
  IMAGEHLP_LINE64  line        = {};
  DWORD            line_offset = 0;
  gboolean         result      = FALSE;
  wchar_t          buf[MAX_PATH];
  DWORD            wchars_count;

  hProcess = GetCurrentProcess ();
  hModule  = (HMODULE) (guintptr) SymGetModuleBase64 (hProcess, address);

  info->object_name[0] = '\0';
  wchars_count = sizeof (buf) / sizeof (buf[0]);
  if (hModule && GetModuleFileNameExW (hProcess, hModule, buf, wchars_count))
    {
      char *object_name;

      if ((object_name = g_utf16_to_utf8 (buf, -1, NULL, NULL, NULL)))
        {
          utf8_copy_sized (info->object_name, object_name, sizeof (info->object_name));

          result = TRUE;
          g_free (object_name);
        }
    }

  symbol_info = g_malloc (sizeof (SYMBOL_INFO)       +
                          sizeof (info->symbol_name) - 1);

  symbol_info->SizeOfStruct = sizeof (SYMBOL_INFO);
  symbol_info->MaxNameLen   = sizeof (info->symbol_name);

  if (gimp_backtrace_SymFromAddr (hProcess, address,
                                  &offset, symbol_info))
    {
      g_strlcpy (info->symbol_name, symbol_info->Name,
                 sizeof (info->symbol_name));

      info->symbol_address = offset ? address - offset : 0;

      result = TRUE;
    }
  else
    {
      info->symbol_name[0] = '\0';
      info->symbol_address = 0;
    }

  g_free (symbol_info);

  if (gimp_backtrace_SymGetLineFromAddr64 (hProcess, address,
                                           &line_offset, &line))
    {
      g_strlcpy (info->source_file, line.FileName,
                 sizeof (info->source_file));

      info->source_line = line.LineNumber;

      result = TRUE;
    }
  else
    {
      info->source_file[0] = '\0';
      info->source_line    = 0;
    }

  return result;
}


#endif /* GIMP_BACKTRACE_BACKEND_WINDOWS */
