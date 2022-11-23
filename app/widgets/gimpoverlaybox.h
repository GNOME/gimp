/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaOverlayBox
 * Copyright (C) 2009 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_OVERLAY_BOX_H__
#define __LIGMA_OVERLAY_BOX_H__


#define LIGMA_TYPE_OVERLAY_BOX            (ligma_overlay_box_get_type ())
#define LIGMA_OVERLAY_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OVERLAY_BOX, LigmaOverlayBox))
#define LIGMA_OVERLAY_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_OVERLAY_BOX, LigmaOverlayBoxClass))
#define LIGMA_IS_OVERLAY_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OVERLAY_BOX))
#define LIGMA_IS_OVERLAY_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_OVERLAY_BOX))
#define LIGMA_OVERLAY_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_OVERLAY_BOX, LigmaOverlayBoxClass))


typedef struct _LigmaOverlayBoxClass LigmaOverlayBoxClass;

struct _LigmaOverlayBox
{
  GtkContainer  parent_instance;

  GList        *children;
};

struct _LigmaOverlayBoxClass
{
  GtkContainerClass  parent_class;
};


GType       ligma_overlay_box_get_type            (void) G_GNUC_CONST;

GtkWidget * ligma_overlay_box_new                 (void);

void        ligma_overlay_box_add_child           (LigmaOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         xalign,
                                                  gdouble         yalign);
void        ligma_overlay_box_set_child_alignment (LigmaOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         xalign,
                                                  gdouble         yalign);
void        ligma_overlay_box_set_child_position  (LigmaOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         x,
                                                  gdouble         y);
void        ligma_overlay_box_set_child_angle     (LigmaOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         angle);
void        ligma_overlay_box_set_child_opacity   (LigmaOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         opacity);

void        ligma_overlay_box_scroll              (LigmaOverlayBox *box,
                                                  gint            offset_x,
                                                  gint            offset_y);


#endif /*  __LIGMA_OVERLAY_BOX_H__  */
