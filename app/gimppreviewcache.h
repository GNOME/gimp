/* The GIMP -- an image manipulation program
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

#ifndef __GIMPPREVIEWCACHE_H__
#define __GIMPPREVIEWCACHE_H__

#include "temp_buf.h"

typedef struct _PreviewCache {
  TempBuf* preview;
  gint     width;
  gint     height;
} PreviewCache;

typedef struct _PreviewNearest {
  PreviewCache *pc;
  gint   width;
  gint   height;
} PreviewNearest;

#define MAX_CACHE_PREVIEWS 5
TempBuf * gimp_preview_cache_get(GSList **,gint,gint);
void      gimp_preview_cache_add(GSList **,TempBuf *);
void      gimp_preview_cache_invalidate(GSList **);


#endif /* __GIMPPREVIEWCACHE_H__ */
