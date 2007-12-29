/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */
#ifndef _GEGL_BUFFER_TYPES_H
#define _GEGL_BUFFER_TYPES_H

typedef struct _GeglSampler              GeglSampler;

typedef struct _GeglTile                 GeglTile;
typedef struct _GeglTileClass            GeglTileClass;

typedef struct _GeglProvider             GeglProvider;
typedef struct _GeglProviderClass        GeglProviderClass;

typedef struct _GeglTileBackend          GeglTileBackend;
typedef struct _GeglTileBackendClass     GeglTileBackendClass;

typedef struct _GeglHandler              GeglHandler;
typedef struct _GeglHandlerClass         GeglHandlerClass;

typedef struct _GeglHandlers             GeglHandlers;
typedef struct _GeglHandlersClass        GeglHandlersClass;

typedef struct _GeglStorage              GeglStorage;
typedef struct _GeglStorageClass         GeglStorageClass;

#ifndef _GEGL_BUFFER_H
typedef struct _GeglBuffer               GeglBuffer;
#endif
typedef struct _GeglBufferClass          GeglBufferClass;

typedef struct _GeglCache                GeglCache;
typedef struct _GeglCacheClass           GeglCacheClass;

typedef struct _GeglBufferAllocator      GeglBufferAllocator;
typedef struct _GeglBufferAllocatorClass GeglBufferAllocatorClass;
typedef struct _GeglRegion               GeglRegion;

#endif
