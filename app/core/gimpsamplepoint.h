/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_SAMPLE_POINT_H__
#define __GIMP_SAMPLE_POINT_H__


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
