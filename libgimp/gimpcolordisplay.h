/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Manish Singh <yosh@gimp.org>
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

#ifndef __GIMP_COLOR_DISPLAY_H__
#define __GIMP_COLOR_DISPLAY_H__

#include <glib.h>

#include <libgimp/parasite.h>

typedef void       (*GimpColorDisplayInit)      (void);
typedef gpointer   (*GimpColorDisplayNew)       (int       type);
typedef void       (*GimpColorDisplayConvert)   (gpointer  cd_ID,
						 guchar    *buf,
						 int        width,
						 int        height,
						 int        bpp);
typedef void       (*GimpColorDisplayDestroy)   (gpointer   cd_ID);
typedef void       (*GimpColorDisplayFinalize)  (void);
typedef void       (*GimpColorDisplayLoadState) (gpointer   cd_ID,
						 Parasite  *state);
typedef Parasite * (*GimpColorDisplaySaveState) (gpointer   cd_ID);
typedef void       (*GimpColorDisplayConfigure) (gpointer   cd_ID);

typedef struct _GimpColorDisplayMethods GimpColorDisplayMethods;

struct _GimpColorDisplayMethods
{
  GimpColorDisplayInit      init;
  GimpColorDisplayNew       new;
  GimpColorDisplayConvert   convert;
  GimpColorDisplayDestroy   destroy;
  GimpColorDisplayFinalize  finalize;
  GimpColorDisplayLoadState load;
  GimpColorDisplaySaveState save;
  GimpColorDisplayConfigure configure;
};

gboolean gimp_color_display_register   (const char              *name,
    				        GimpColorDisplayMethods *methods);
gboolean gimp_color_display_unregister (const char              *name);

#endif /* __GIMP_COLOR_DISPLAY_H__ */
