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

#ifndef __GIMP_SAMPLE_POINT_H__
#define __GIMP_SAMPLE_POINT_H__


#define GIMP_SAMPLE_POINT_DRAW_SIZE 10


#define GIMP_TYPE_SAMPLE_POINT (gimp_sample_point_get_type ())


struct _GimpSamplePoint
{
  gint     ref_count;
  guint32  sample_point_ID;
  gint     x;
  gint     y;
};


GType             gimp_sample_point_get_type (void) G_GNUC_CONST;

GimpSamplePoint * gimp_sample_point_new      (guint32          sample_point_ID);

GimpSamplePoint * gimp_sample_point_ref      (GimpSamplePoint *sample_point);
void              gimp_sample_point_unref    (GimpSamplePoint *sample_point);


#endif /* __GIMP_SAMPLE_POINT_H__ */
