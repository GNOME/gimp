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

#ifndef __GIMP_AIRBRUSH_H__
#define __GIMP_AIRBRUSH_H__


#include "gimppaintcore.h"
#include "gimppaintoptions.h"


#define GIMP_TYPE_AIRBRUSH            (gimp_airbrush_get_type ())
#define GIMP_AIRBRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_AIRBRUSH, GimpAirbrush))
#define GIMP_AIRBRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_AIRBRUSH, GimpAirbrushClass))
#define GIMP_IS_AIRBRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_AIRBRUSH))
#define GIMP_IS_AIRBRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_AIRBRUSH))
#define GIMP_AIRBRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_AIRBRUSH, GimpAirbrushClass))


typedef struct _GimpAirbrush      GimpAirbrush;
typedef struct _GimpAirbrushClass GimpAirbrushClass;

struct _GimpAirbrush
{
  GimpPaintCore parent_instance;
};

struct _GimpAirbrushClass
{
  GimpPaintCoreClass parent_class;
};


typedef struct _GimpAirbrushOptions GimpAirbrushOptions;

struct _GimpAirbrushOptions
{
  GimpPaintOptions  paint_options;

  gdouble           rate;
  gdouble           rate_d;
  GtkObject        *rate_w;

  gdouble           pressure;
  gdouble           pressure_d;
  GtkObject        *pressure_w;
};


void    gimp_airbrush_register (Gimp                      *gimp,
                                GimpPaintRegisterCallback  callback);

GType   gimp_airbrush_get_type (void) G_GNUC_CONST;


GimpAirbrushOptions * gimp_airbrush_options_new (GimpContext *context);


#endif  /*  __GIMP_AIRBRUSH_H__  */
