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

#ifndef  __GIMP_CROP_TOOL_H__
#define  __GIMP_CROP_TOOL_H__


#include "gimpdrawtool.h"


/* XXX Used? */
typedef enum
{
  CROP_CROP,
  RESIZE_CROP
} CropType;


#define GIMP_TYPE_CROP_TOOL            (gimp_crop_tool_get_type ())
#define GIMP_CROP_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_CROP_TOOL, GimpCropTool))
#define GIMP_IS_CROP_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_CROP_TOOL))
#define GIMP_CROP_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CROP_TOOL, GimpCropToolClass))


typedef struct _GimpCropTool      GimpCropTool;
typedef struct _GimpCropToolClass GimpCropToolClass;

struct _GimpCropTool
{
  GimpDrawTool  parent_instance;

  gint          startx;     /*  starting x coord            */
  gint          starty;     /*  starting y coord            */

  gint          lastx;      /*  previous x coord            */
  gint          lasty;      /*  previous y coord            */

  gint          x1, y1;     /*  upper left hand coordinate  */
  gint          x2, y2;     /*  lower right hand coords     */

  gint          srw, srh;   /*  width and height of corners */

  gint          tx1, ty1;   /*  transformed coords          */
  gint          tx2, ty2;   /*                              */

  guint         function;   /*  moving or resizing          */
  guint         context_id; /*  for the statusbar           */
};

struct _GimpCropToolClass
{
  GimpDrawToolClass parent_class;
};


void      gimp_crop_tool_register (Gimp      *gimp);

GtkType   gimp_crop_tool_get_type (void);


#endif  /*  __GIMP_CROP_TOOL_H__  */
