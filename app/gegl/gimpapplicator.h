/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpapplicator.h
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_APPLICATOR_H__
#define __GIMP_APPLICATOR_H__


#define GIMP_TYPE_APPLICATOR            (gimp_applicator_get_type ())
#define GIMP_APPLICATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_APPLICATOR, GimpApplicator))
#define GIMP_APPLICATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_APPLICATOR, GimpApplicatorClass))
#define GIMP_IS_APPLICATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_APPLICATOR))
#define GIMP_IS_APPLICATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_APPLICATOR))
#define GIMP_APPLICATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_APPLICATOR, GimpApplicatorClass))


typedef struct _GimpApplicatorClass GimpApplicatorClass;

struct _GimpApplicator
{
  GObject        parent_instance;

  GeglNode      *node;
  GeglNode      *mode_node;
  GeglNode      *src_node;
  GeglNode      *apply_src_node;
  GeglNode      *apply_offset_node;
  GeglNode      *dest_node;
  GeglProcessor *processor;
};

struct _GimpApplicatorClass
{
  GObjectClass  parent_class;
};


GType            gimp_applicator_get_type (void) G_GNUC_CONST;

GimpApplicator * gimp_applicator_new      (GeglBuffer           *dest_buffer,
                                           GimpComponentMask     affect,
                                           GeglBuffer           *mask_buffer,
                                           gint                  mask_offset_x,
                                           gint                  mask_offset_y);
void             gimp_applicator_apply    (GimpApplicator       *applicator,
                                           GeglBuffer           *src_buffer,
                                           GeglBuffer           *apply_buffer,
                                           gint                  apply_buffer_x,
                                           gint                  apply_buffer_y,
                                           gdouble               opacity,
                                           GimpLayerModeEffects  paint_mode);


#endif  /*  __GIMP_APPLICATOR_H__  */
