/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * parasiteP.h
 * Copyright (C) 1998 Jay Cox <jaycox@earthlink.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __PARASITEP_H__
#define __PARASITEP_H__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _Parasite
{
  char *name;           /* The name of the parasite. USE A UNIQUE PREFIX! */
  guint32 flags;	/* save Parasite in XCF file, etc.                */
  guint32 size;         /* amount of data                                 */
  void *data;           /* a pointer to the data.  plugin is              *
			 * responsible for tracking byte order            */
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PARASITEP_H__ */
