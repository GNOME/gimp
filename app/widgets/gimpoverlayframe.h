/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoverlayframe.h
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_OVERLAY_FRAME_H__
#define __GIMP_OVERLAY_FRAME_H__


#define GIMP_TYPE_OVERLAY_FRAME            (gimp_overlay_frame_get_type ())
#define GIMP_OVERLAY_FRAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OVERLAY_FRAME, GimpOverlayFrame))
#define GIMP_OVERLAY_FRAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_OVERLAY_FRAME, GimpOverlayFrameClass))
#define GIMP_IS_OVERLAY_FRAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OVERLAY_FRAME))
#define GIMP_IS_OVERLAY_FRAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_OVERLAY_FRAME))
#define GIMP_OVERLAY_FRAME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_OVERLAY_FRAME, GimpOverlayFrameClass))


typedef struct _GimpOverlayFrame      GimpOverlayFrame;
typedef struct _GimpOverlayFrameClass GimpOverlayFrameClass;

struct _GimpOverlayFrame
{
  GtkBin  parent_instance;
};

struct _GimpOverlayFrameClass
{
  GtkBinClass  parent_class;
};


GType       gimp_overlay_frame_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_overlay_frame_new      (void);


#endif /* __GIMP_OVERLAY_FRAME_H__ */
