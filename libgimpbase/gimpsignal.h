/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_SIGNAL_H__
#define __GIMP_SIGNAL_H__

#include <signal.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GimpSignalHandlerFunc:
 * @signum: The number of the signal. Useful if different signals are
 *          handled by a single handler.
 *
 * A prototype for a reference to a signal handler functions. Note
 * that each function which takes or returns a variable of this type
 * also accepts or may return special values defined by your system's
 * signal.h header file (like @SIG_DFL or @SIG_IGN).
 **/
typedef void (* GimpSignalHandlerFunc) (gint signum);

GimpSignalHandlerFunc  gimp_signal_private (gint                   signum,
                                            GimpSignalHandlerFunc  handler,
                                            gint                   flags);


G_END_DECLS

#endif /* __GIMP_SIGNAL_H__ */
