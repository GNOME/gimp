/*
 * "$Id$"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu). and Steve Miller (smiller@rni.net
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#ifndef __PRINT_GIMP_H__
#define __PRINT_GIMP_H__

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print.h"

/*
 * All Gimp-specific code is in this file.
 */

#define PLUG_IN_VERSION		VERSION " - " RELEASE_DATE
#define PLUG_IN_NAME		"Print"

/*
 * Constants for GUI...
 */
#define PREVIEW_SIZE_VERT  240 /* Assuming max media size of 24" A2 */
#define PREVIEW_SIZE_HORIZ 240 /* Assuming max media size of 24" A2 */

#if !defined(GIMP_MINOR_VERSION) || (GIMP_MAJOR_VERSION == 1 && GIMP_MINOR_VERSION == 0) || (GIMP_MAJOR_VERSION == 1 && GIMP_MINOR_VERSION == 1 && GIMP_MICRO_VERSION < 21)
#define GIMP_1_0
#endif

/*
 * Function prototypes
 */

/* How to create an Image wrapping a Gimp drawable */
extern Image Image_GDrawable_new(GDrawable *drawable);

#endif  /* __PRINT_GIMP_H__ */
