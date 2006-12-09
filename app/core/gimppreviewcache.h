/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1999 Andy Thomas alt@gimp.org
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

#ifndef __GIMP_PREVIEW_CACHE_H__
#define __GIMP_PREVIEW_CACHE_H__


#define PREVIEW_CACHE_PRIME_WIDTH  112
#define PREVIEW_CACHE_PRIME_HEIGHT 112


TempBuf * gimp_preview_cache_get         (GSList  **plist,
                                          gint      width,
                                          gint      height);
void      gimp_preview_cache_add         (GSList  **plist,
                                          TempBuf  *buf);
void      gimp_preview_cache_invalidate  (GSList  **plist);

gsize     gimp_preview_cache_get_memsize (GSList   *cache);


#endif /* __GIMP_PREVIEW_CACHE_H__ */
