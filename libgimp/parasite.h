/* parasite.h
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

#ifndef _PARASITE_H_
#define _PARASITE_H_

#include <glib.h>
#include <libgimp/parasiteF.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PARASITE_PERSISTANT 1

Parasite *parasite_new      (const char *name, guint32 flags,
			     guint32 size, const void *data);
void      parasite_free     (Parasite *parasite);

int       parasite_is_type  (const Parasite *parasite, const char *name);
Parasite *parasite_copy     (const Parasite *parasite);

Parasite *parasite_error    ();

int       parasite_is_error      (const Parasite *p);

int       parasite_is_persistant (const Parasite *p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif _PARASITE_H_
