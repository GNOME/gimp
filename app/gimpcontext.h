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
#ifndef __GIMP_CONTEXT_H__
#define __GIMP_CONTEXT_H__

#include "gimpobjectP.h"

#define GIMP_TYPE_CONTEXT            (gimp_context_get_type ())
#define GIMP_CONTEXT(obj)            (GIMP_CHECK_CAST ((obj), GIMP_TYPE_CONTEXT, GimpContext))
#define GIMP_CONTEXT_CLASS(klass)    (GIMP_CHECK_CLASS_CAST (klass, GIMP_TYPE_CONTEXT, GimpContextClass))
#define GIMP_IS_CONTEXT(obj)         (GIMP_CHECK_TYPE ((obj), GIMP_TYPE_CONTEXT))
#define GIMP_IS_CONTEXT_CLASS(klass) (GIMP_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTEXT))

typedef struct _GimpContext GimpContext;
typedef struct _GimpContextClass GimpContextClass;

struct _GimpContext
{
  GimpObject object;

  gdouble    opacity;
  gint       paint_mode;
};

struct _GimpContextClass
{
  GimpObjectClass parent_class;

  void (* opacity_changed)    (GimpContext *context);
  void (* paint_mode_changed) (GimpContext *context);
};

GtkType       gimp_context_get_type       (void);
GimpContext * gimp_context_new            (void);

/*  to be used by the context management system only
 */
GimpContext * gimp_context_get_current    (void);
void          gimp_context_set_current    (GimpContext *context);

/*  functions for manipulating a single context
 */
gdouble       gimp_context_get_opacity    (GimpContext *context);
void          gimp_context_set_opacity    (GimpContext *context,
					   gdouble      opacity);

gint          gimp_context_get_paint_mode (GimpContext *context);
void          gimp_context_set_paint_mode (GimpContext *context,
					   gint         paint_mode);

#endif /* __GIMP_CONTEXT_H__ */
