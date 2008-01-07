/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2003 Calvin Williamson
 *           2006 Øyvind Kolås
 */

#ifndef __GEGL_TYPES_H__
#define __GEGL_TYPES_H__

G_BEGIN_DECLS

typedef struct _GeglConnection       GeglConnection;
#ifndef __GEGL_H__
typedef struct _GeglColor            GeglColor;
typedef struct _GeglCurve            GeglCurve;
#endif
typedef struct _GeglCRVisitor        GeglCRVisitor;
typedef struct _GeglDebugRectVisitor GeglDebugRectVisitor;
typedef struct _GeglEvalMgr          GeglEvalMgr;
typedef struct _GeglEvalVisitor      GeglEvalVisitor;
typedef struct _GeglFinishVisitor    GeglFinishVisitor;
typedef struct _GeglGraph            GeglGraph;
typedef struct _GeglHaveVisitor      GeglHaveVisitor;
typedef struct _GeglNeedVisitor      GeglNeedVisitor;
#ifndef __GEGL_H__
typedef struct _GeglNode             GeglNode;
#endif
typedef struct _GeglNodeContext      GeglNodeContext;
typedef struct _GeglOperation        GeglOperation;
typedef struct _GeglPad              GeglPad;
#ifndef __GEGL_H__
typedef struct _GeglVector           GeglVector;
typedef struct _GeglProcessor        GeglProcessor;
#endif
typedef struct _GeglPrepareVisitor   GeglPrepareVisitor;
typedef struct _GeglVisitable        GeglVisitable; /* dummy typedef */
typedef struct _GeglVisitor          GeglVisitor;

#ifndef __GEGL_H__
typedef struct _GeglRectangle        GeglRectangle;
#endif
typedef struct _GeglPoint            GeglPoint;
typedef struct _GeglDimension        GeglDimension;

#ifndef __GEGL_H__
struct _GeglRectangle
{
  gint x;
  gint y;
  gint width;
  gint height;
};
#endif

struct _GeglPoint
{
  gint x;
  gint y;
};

struct _GeglDimension
{
  gint width;
  gint height;
};

G_END_DECLS

#endif /* __GEGL_TYPES_H__ */
