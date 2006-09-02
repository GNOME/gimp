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

#ifndef __GIMP_HEAL_OPTIONS_H__
#define __GIMP_HEAL_OPTIONS_H__


#include "gimppaintoptions.h"


#define GIMP_TYPE_HEAL_OPTIONS            (gimp_heal_options_get_type ())
#define GIMP_HEAL_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_HEAL_OPTIONS, GimpHealOptions))
#define GIMP_HEAL_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_HEAL_OPTIONS, GimpHealOptionsClass))
#define GIMP_IS_HEAL_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_HEAL_OPTIONS))
#define GIMP_IS_HEAL_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_HEAL_OPTIONS))
#define GIMP_HEAL_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_HEAL_OPTIONS, GimpHealOptionsClass))


typedef struct _GimpHealOptions       GimpHealOptions;
typedef struct _GimpPaintOptionsClass GimpHealOptionsClass;

struct _GimpHealOptions
{
  GimpPaintOptions  paint_options;

  GimpHealAlignMode align_mode;
  gboolean          sample_merged;
};


GType   gimp_heal_options_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_HEAL_OPTIONS_H__  */
