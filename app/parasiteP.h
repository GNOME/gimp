/* parasiteP.h
 * Copyright (C) 1998 Jay Cox <jaycox@earthlink.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _PARASITEP_H_
#define _PARASITEP_H_

#include <glib.h>

struct _Parasite
{
  guchar creator[4];    /* the creator code of the plug-in/author   */
  guchar type[4];       /* the data type of the parasite            */
  guint32 flags;	/* save Parasite in XCF file, etc.          */
  guint32 size;         /* amount of data                           */
  void *data;           /* a pointer to the data.  plugin is        *
			 * responsible for tracking byte order      */
};

#endif _PARASITEP_H_
