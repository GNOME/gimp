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

#ifndef  __COLOR_PICKER_H__
#define  __COLOR_PICKER_H__

#include "tool.h"

#define GIMP_TYPE_COLOR_PICKER            (gimp_color_picker_get_type ())
#define GIMP_COLOR_PICKER(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_COLOR_PICKER, GimpColorPicker))
#define GIMP_IS_COLOR_PICKER(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_COLOR_PICKER))
#define GIMP_COLOR_PICKER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_PICKER, GimpColorPickerClass))
#define GIMP_IS_COLOR_PICKER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_PICKER))

GtkType gimp_color_picker_get_type (void);

struct _GimpColorPicker
{
	GimpTool parent_instance;

	DrawCore *core;       /*  Core select object  */

	gint      centerx;    /*  starting x coord    */
	gint      centery;    /*  starting y coord    */
	    
};

struct _GimpColorPickerClass
{
	GimpToolClass parent_class;
};

typedef struct _GimpColorPicker GimpColorPicker; /* This is one of the stupidest parts of the gnu coding standards */
typedef struct _GimpColorPickerClass GimpColorPickerClass; /* making the typedef and the struct one line like everyone else does confuses nobody */

extern gint col_value[5];


gboolean   pick_color              (GimpImage    *gimage,
				    GimpDrawable *drawable,
				    gint          x,
				    gint          y,
				    gboolean      sample_merged,
				    gboolean      sample_average,
				    double        average_radius,
				    gint          final);

GimpTool     * gimp_color_picker_new  (void);

#endif  /*  __COLOR_PICKER_H__  */
