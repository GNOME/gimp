/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoverlayframe.h
 * Copyright (C) 2010  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_OVERLAY_FRAME_H__
#define __LIGMA_OVERLAY_FRAME_H__


#define LIGMA_TYPE_OVERLAY_FRAME            (ligma_overlay_frame_get_type ())
#define LIGMA_OVERLAY_FRAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OVERLAY_FRAME, LigmaOverlayFrame))
#define LIGMA_OVERLAY_FRAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_OVERLAY_FRAME, LigmaOverlayFrameClass))
#define LIGMA_IS_OVERLAY_FRAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OVERLAY_FRAME))
#define LIGMA_IS_OVERLAY_FRAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_OVERLAY_FRAME))
#define LIGMA_OVERLAY_FRAME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_OVERLAY_FRAME, LigmaOverlayFrameClass))


typedef struct _LigmaOverlayFrame      LigmaOverlayFrame;
typedef struct _LigmaOverlayFrameClass LigmaOverlayFrameClass;

struct _LigmaOverlayFrame
{
  GtkBin  parent_instance;
};

struct _LigmaOverlayFrameClass
{
  GtkBinClass  parent_class;
};


GType       ligma_overlay_frame_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_overlay_frame_new      (void);


#endif /* __LIGMA_OVERLAY_FRAME_H__ */
