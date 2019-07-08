/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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
 *
 * $Revision$
 */

#include "config.h"

#define _GNU_SOURCE  /* for the sigaction stuff */

#include <glib.h>

#include "gimpsignal.h"


/**
 * SECTION: gimpsignal
 * @title: gimpsignal
 * @short_description: Portable signal handling.
 * @see_also: signal(2), signal(5 or 7), sigaction(2).
 *
 * Portable signal handling.
 **/


/* Courtesy of Austin Donnelly 06-04-2000 to address bug #2742 */

/**
 * gimp_signal_private:
 * @signum: Selects signal to be handled see man 5 signal (or man 7 signal)
 * @handler: Handler that maps to signum. Invoked by O/S.
 *           Handler gets signal that caused invocation. Corresponds
 *           to the @sa_handler field of the @sigaction struct.
 * @flags: Preferences. OR'ed SA_&lt;xxx&gt;. See man sigaction. Corresponds
 *         to the @sa_flags field of the @sigaction struct.
 *
 * This function furnishes a workalike for signal(2) but
 * which internally invokes sigaction(2) after certain
 * sa_flags are set; these primarily to ensure restarting
 * of interrupted system calls. See sigaction(2)  It is a
 * aid to transition and not new development: that effort
 * should employ sigaction directly. [gosgood 18.04.2000]
 *
 * Cause @handler to be run when @signum is delivered.  We
 * use sigaction(2) rather than signal(2) so that we can control the
 * signal handler's environment completely via @flags: some signal(2)
 * implementations differ in their semantics, so we need to nail down
 * exactly what we want. [austin 06.04.2000]
 *
 * Returns: A reference to the signal handling function which was
 *          active before the call to gimp_signal_private().
 */
GimpSignalHandlerFunc
gimp_signal_private (gint                   signum,
                     GimpSignalHandlerFunc  handler,
                     gint                   flags)
{
#ifndef G_OS_WIN32
  gint ret;
  struct sigaction sa;
  struct sigaction osa;

  /*  The sa_handler (mandated by POSIX.1) and sa_sigaction (a
   *  common extension) are often implemented by the OS as members
   *  of a union.  This means you CAN NOT set both, you set one or
   *  the other.  Caveat programmer!
   */

  /*  Passing gimp_signal_private a gimp_sighandler of NULL is not
   *  an error, and generally results in the action for that signal
   *  being set to SIG_DFL (default behavior).  Many OSes define
   *  SIG_DFL as (void (*)()0, so setting sa_handler to NULL is
   *  the same thing as passing SIG_DFL to it.
   */
  sa.sa_handler = handler;

  /*  Mask all signals while handler runs to avoid re-entrancy
   *  problems.
   */
  sigfillset (&sa.sa_mask);

  sa.sa_flags = flags;

  ret = sigaction (signum, &sa, &osa);

  if (ret < 0)
    g_error ("unable to set handler for signal %d\n", signum);

  return (GimpSignalHandlerFunc) osa.sa_handler;
#else
  return NULL;                  /* Or g_error()? Should all calls to
                                 * this function really be inside
                                 * #ifdef G_OS_UNIX?
                                 */
#endif
}
