/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimp-debug.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <sys/types.h>

#include <glib.h>

#ifndef G_OS_WIN32
#include "libgimpbase/gimpsignal.h"

#else

#ifdef HAVE_EXCHNDL
#include <time.h>
#include <exchndl.h>
#endif

#include <signal.h>
#endif

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
#  ifdef STRICT
#  undef STRICT
#  endif
#  define STRICT

#  include <windows.h>
#  include <tlhelp32.h>
#  include <processthreadsapi.h>
#  undef RGB
#endif

#include "gimp.h"
#include "gimp-debug.h"

static GLogLevelFlags create_log_level_flags        (void);
static void           make_visible_libgimp_messages (void);

static void   gimp_message_func (const gchar    *log_domain,
                                 GLogLevelFlags  log_level,
                                 const gchar    *message,
                                 gpointer        data);
static void   gimp_fatal_func   (const gchar    *log_domain,
                                 GLogLevelFlags  flags,
                                 const gchar    *message,
                                 gpointer        data);



static const GDebugKey gimp_debug_keys[] =
{
  { "pid",            GIMP_DEBUG_PID            },
  { "fatal-warnings", GIMP_DEBUG_FATAL_WARNINGS },
  { "fatal-criticals", GIMP_DEBUG_FATAL_CRITICALS },
  { "fw",             GIMP_DEBUG_FATAL_WARNINGS },
  { "query",          GIMP_DEBUG_QUERY          },
  { "init",           GIMP_DEBUG_INIT           },
  { "run",            GIMP_DEBUG_RUN            },
  { "quit",           GIMP_DEBUG_QUIT           }
};

/* Set by gimp_debug_configure() to partial parameterize gimp_fatal_handler(). */
static GimpStackTraceMode  _stack_trace_mode   = GIMP_STACK_TRACE_NEVER;
/* Set by _gimp_debug_init(). Exported by _gimp_get_debug_flags() */
static guint               _gimp_debug_flags   = 0;


/*
 * Set _gimp_debug_flags, from env var GIMP_PLUGIN_DEBUG if it exists.
 * Also ensure GLib default handler prints all log messages for domain LibGimp.
 */
void
_gimp_debug_init (const gchar *basename)
{
  const gchar *env_string = g_getenv ("GIMP_PLUGIN_DEBUG");
  const gchar *debug_options;
  gint         plugin_name_len;
  gboolean     is_debug_name_match_basename;

  if (!env_string) return;

  debug_options = strchr (env_string, ',');

  /* Does name match basename or name match "all"
   * Use strlen to allow basename to have prefix "all" without matching.
   */
  /* Safe subtraction of pointers into the same string. */
  plugin_name_len = debug_options - env_string;
  is_debug_name_match_basename = (
      ((strlen (basename) == plugin_name_len) &&
       (strncmp (basename, env_string, plugin_name_len) == 0)) ||
      (strncmp (env_string, "all", plugin_name_len) == 0)
    );


  if (is_debug_name_match_basename && debug_options)
    {
      _gimp_debug_flags =
        g_parse_debug_string (debug_options + 1,
                              gimp_debug_keys,
                              G_N_ELEMENTS (gimp_debug_keys));
    }
  /* Else assert _gimp_debug_flags==0 .
   * Only ERROR will be fatal, and fatal handler installed
   * (to print according to stack-trace-mode)
   */

  make_visible_libgimp_messages();
}


guint
_gimp_get_debug_flags (void)
{
  return _gimp_debug_flags;
}


/*
 * Configure GLib logging according to GIMP_PLUGIN_DEBUG
 */
void
_gimp_debug_configure (GimpStackTraceMode stack_trace_mode)
{
  const gchar * const gimp_log_domains[] =
  {
    "LibGimp",
    "LibGimpBase",
    "LibGimpColor",
    "LibGimpConfig",
    "LibGimpMath",
    "LibGimpModule",
    "LibGimpThumb",
    "LibGimpWidgets"
  };
  const gchar * const glib_log_domains[] =
  {
    "GLib",
    "GLib-GObject",
    "GLib-GIO",
  };
  gint i;
  GLogLevelFlags fatal_mask;

  _stack_trace_mode = stack_trace_mode;

  /* Set handler for Gimp domains, for MESSAGE level.
   * i.e. from g_message(), but not from g_info() (rarely used?).
   */
  for (i = 0; i < G_N_ELEMENTS (gimp_log_domains); i++)
    g_log_set_handler (gimp_log_domains[i],
                       G_LOG_LEVEL_MESSAGE,
                       gimp_message_func,
                       NULL);

  /*  Also set handler for "" i.e. app domain. */
  g_log_set_handler (NULL,
                     G_LOG_LEVEL_MESSAGE,
                     gimp_message_func,
                     NULL);

  /* Make fatal a subset of log levels (ERROR is always fatal)
   * as specified by GIMP_PLUGIN_DEBUG now in _gimp_debug_flags
   */
  fatal_mask = create_log_level_flags();

  g_log_set_always_fatal (fatal_mask);

  /* set our custom handler for fatal messages, for many domains. */

  /* For the null i.e. "" i.e. app i.e. plugin domain */
  g_log_set_handler (NULL,
                     fatal_mask,
                     gimp_fatal_func,
                     NULL);

  for (i = 0; i < G_N_ELEMENTS (gimp_log_domains); i++)
    g_log_set_handler (gimp_log_domains[i],
                       fatal_mask,
                       gimp_fatal_func,
                       NULL);
  for (i = 0; i < G_N_ELEMENTS (glib_log_domains); i++)
    g_log_set_handler (glib_log_domains[i],
                      fatal_mask,
                      gimp_fatal_func,
                      NULL);
}



/*
 * Suspend i.e. pause the plugin process.
 */
void
_gimp_debug_stop (void)
{
#ifndef G_OS_WIN32

  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Waiting for debugger...");
  raise (SIGSTOP);

#else

  HANDLE        hThreadSnap = NULL;
  THREADENTRY32 te32        = { 0 };
  DWORD         opid        = GetCurrentProcessId();

  g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
         "Debugging (restart externally): %ld",
         (long int) opid);

  hThreadSnap = CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, 0);
  if (hThreadSnap == INVALID_HANDLE_VALUE)
    {
      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
             "error getting threadsnap - debugging impossible");
      return;
    }

  te32.dwSize = sizeof (THREADENTRY32);

  if (Thread32First (hThreadSnap, &te32))
    {
      do
        {
          if (te32.th32OwnerProcessID == opid)
            {
              HANDLE hThread = OpenThread (THREAD_SUSPEND_RESUME, FALSE,
                                           te32.th32ThreadID);
              SuspendThread (hThread);
              CloseHandle (hThread);
            }
        }
      while (Thread32Next (hThreadSnap, &te32));
    }
  else
    {
      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "error getting threads");
    }

  CloseHandle (hThreadSnap);

#endif
}


/*  private functions  */

/*
 * Append domain LibGimp to env var G_MESSAGES_DEBUG.
 * Per GLib docs, that makes default handler print all levels of log messages,
 * from domain LibGimp.

 * We may yet install non-default handler gimp_fatal_func(), for some levels,
 * which prints the message and also may generate stack trace.
 *
 * This does not depend on GIMP_PLUGIN_DEBUG or any plugin names therein.
 * This does not affect the Gimp app, whose environment is distinct.
 * This does not make a plugin's own logged messages visible,
 * since the plugin is in a distinct domain.
 */
static void
make_visible_libgimp_messages (void)
{
  const gchar *debug_messages = g_getenv ("G_MESSAGES_DEBUG");

  if (debug_messages)
    {
      gchar *tmp = g_strconcat (debug_messages, ",LibGimp", NULL);
      g_setenv ("G_MESSAGES_DEBUG", tmp, TRUE);
      g_free (tmp);
    }
  else
    {
      g_setenv ("G_MESSAGES_DEBUG", "LibGimp", TRUE);
    }
}


/* Create GLogLevelFlags for fatal,
 * from current fatal flags and from GIMP_PLUGIN_DEBUG env var.
 */
static GLogLevelFlags
create_log_level_flags(void)
{
  GLogLevelFlags result;

  result = g_log_set_always_fatal (G_LOG_FATAL_MASK);
  /* result is the old one, and may have flags set already
   * via the GLib G_DEBUG environment variable.
   */

  /* Ensure ERROR remains fatal.
   * Even if GIMP_PLUGIN_DEBUG is not defined, or defined "=run"
   * we install fatal handler for ERROR for all plugins.
   */
  result |= G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL;

  if (_gimp_debug_flags & GIMP_DEBUG_FATAL_WARNINGS)
    result |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
  if (_gimp_debug_flags & GIMP_DEBUG_FATAL_CRITICALS)
    result |= G_LOG_LEVEL_CRITICAL;
  return result;
}


/*
 * Handler for GLib log events of MESSAGE level.
 * Signature is GLogFunc
 */
static void
gimp_message_func (const gchar    *log_domain,
                   GLogLevelFlags  log_level,
                   const gchar    *message,
                   gpointer        data)
{
  gimp_message (message);
}


/*
 * Handler for fatal GLib logging events.
 * Signature is GLogFunc
 */
static void
gimp_fatal_func (const gchar    *log_domain,
                 GLogLevelFlags  flags,
                 const gchar    *message,
                 gpointer        data)
{
  const gchar *level;

  switch (flags & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_WARNING:
      level = "WARNING";
      break;
    case G_LOG_LEVEL_CRITICAL:
      level = "CRITICAL";
      break;
    case G_LOG_LEVEL_ERROR:
      level = "ERROR";
      break;
    default:
      level = "FATAL";
      break;
    }

  /* Earlier, gimp.c g_set_prgname to short basename; progname is full path. */

  /*
   * Print message canonical to what GLib's default handler would.
   * Except prefix with which plugin using short name i.e.  basename
   * so reader can distinguish interleaved messages from plugin or app process.
   * The log_domain is which library logged the event.
   */
  g_printerr ("Plugin %s: %s: %s: %s\n",
              g_get_prgname(), log_domain, level, message);

#ifndef G_OS_WIN32
  switch (_stack_trace_mode)
    {
    case GIMP_STACK_TRACE_NEVER:
      break;

    case GIMP_STACK_TRACE_QUERY:
        {
          sigset_t sigset;

          sigemptyset (&sigset);
          sigprocmask (SIG_SETMASK, &sigset, NULL);
          gimp_stack_trace_query (log_domain);
        }
      break;

    case GIMP_STACK_TRACE_ALWAYS:
        {
          sigset_t sigset;

          sigemptyset (&sigset);
          sigprocmask (SIG_SETMASK, &sigset, NULL);
          gimp_stack_trace_print (log_domain, stdout, NULL);
        }
      break;
    }
#endif /* ! G_OS_WIN32 */

  /* Do not end with gimp_quit().
   * We want the plug-in to continue its normal crash course, otherwise
   * we won't get the "Plug-in crashed" error in GIMP.
   */
  exit (EXIT_FAILURE);
}
