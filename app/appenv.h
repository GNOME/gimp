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
#ifndef __APPENV_H__
#define __APPENV_H__

#include "gdk/gdkx.h"
#include "gtk/gtk.h"
#include "gimpsetF.h"

#define DISPLAY              ((Display *) GDK_DISPLAY())

/*   important macros  */
#define BOUNDS(a,x,y)  ((a < x) ? x : ((a > y) ? y : a))
#define MINIMUM(x,y) ((x < y) ? x : y)
#define MAXIMUM(x,y) ((x > y) ? x : y)

typedef enum {
  MESSAGE_BOX,
  CONSOLE
} MessageHandlerType;

extern int no_interface;
extern int no_splash;
extern int no_splash_image;
extern int no_data;
extern int be_verbose;
extern int use_debug_handler;
extern int console_messages;
extern GimpSet* image_context;

extern MessageHandlerType message_handler;

#endif /*  APPENV_H  */
