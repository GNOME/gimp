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

#ifndef __GIMP_DODGE_BURN_H__
#define __GIMP_DODGE_BURN_H__


#include "gimppaintcore.h"
#include "gimppaintoptions.h"


#define GIMP_TYPE_DODGEBURN            (gimp_dodgeburn_get_type ())
#define GIMP_DODGEBURN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DODGEBURN, GimpDodgeBurn))
#define GIMP_IS_DODGEBURN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DODGEBURN))
#define GIMP_DODGEBURN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DODGEBURN, GimpDodgeBurnClass))
#define GIMP_IS_DODGEBURN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DODGEBURN))


typedef struct _GimpDodgeBurn      GimpDodgeBurn;
typedef struct _GimpDodgeBurnClass GimpDodgeBurnClass;

struct _GimpDodgeBurn
{
  GimpPaintCore  parent_instance;

  GimpLut       *lut;
};

struct _GimpDodgeBurnClass
{
  GimpPaintCoreClass parent_class;
};


typedef struct _GimpDodgeBurnOptions GimpDodgeBurnOptions;

struct _GimpDodgeBurnOptions
{
  GimpPaintOptions   paint_options;

  GimpDodgeBurnType  type;
  GimpDodgeBurnType  type_d;
  GtkWidget         *type_w;

  GimpTransferMode   mode;     /*highlights, midtones, shadows*/
  GimpTransferMode   mode_d;
  GtkWidget         *mode_w;

  gdouble            exposure;
  gdouble            exposure_d;
  GtkObject         *exposure_w;
};


void    gimp_dodgeburn_register (Gimp                      *gimp,
                                 GimpPaintRegisterCallback  callback);

GType   gimp_dodgeburn_get_type (void) G_GNUC_CONST;


GimpDodgeBurnOptions * gimp_dodgeburn_options_new (void);


#endif  /*  __GIMP_DODGEBURN_H__  */
