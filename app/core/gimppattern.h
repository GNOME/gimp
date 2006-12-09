/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_PATTERN_H__
#define __GIMP_PATTERN_H__


#include "gimpdata.h"


#define GIMP_TYPE_PATTERN            (gimp_pattern_get_type ())
#define GIMP_PATTERN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PATTERN, GimpPattern))
#define GIMP_PATTERN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PATTERN, GimpPatternClass))
#define GIMP_IS_PATTERN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PATTERN))
#define GIMP_IS_PATTERN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PATTERN))
#define GIMP_PATTERN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PATTERN, GimpPatternClass))


typedef struct _GimpPatternClass GimpPatternClass;

struct _GimpPattern
{
  GimpData  parent_instance;

  TempBuf  *mask;
};

struct _GimpPatternClass
{
  GimpDataClass  parent_class;
};


GType      gimp_pattern_get_type     (void) G_GNUC_CONST;

GimpData * gimp_pattern_new          (const gchar       *name);
GimpData * gimp_pattern_get_standard (void);

TempBuf  * gimp_pattern_get_mask     (const GimpPattern *pattern);


#endif /* __GIMP_PATTERN_H__ */
