/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpOverlayBox
 * Copyright (C) 2009 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_OVERLAY_BOX_H__
#define __GIMP_OVERLAY_BOX_H__


#define GIMP_TYPE_OVERLAY_BOX            (gimp_overlay_box_get_type ())
#define GIMP_OVERLAY_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OVERLAY_BOX, GimpOverlayBox))
#define GIMP_OVERLAY_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_OVERLAY_BOX, GimpOverlayBoxClass))
#define GIMP_IS_OVERLAY_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OVERLAY_BOX))
#define GIMP_IS_OVERLAY_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_OVERLAY_BOX))
#define GIMP_OVERLAY_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_OVERLAY_BOX, GimpOverlayBoxClass))


typedef struct _GimpOverlayBoxClass GimpOverlayBoxClass;

struct _GimpOverlayBox
{
  GtkContainer  parent_instance;

  GList        *children;
};

struct _GimpOverlayBoxClass
{
  GtkContainerClass  parent_class;
};


GType       gimp_overlay_box_get_type            (void) G_GNUC_CONST;

GtkWidget * gimp_overlay_box_new                 (void);

void        gimp_overlay_box_add_child           (GimpOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         xalign,
                                                  gdouble         yalign);
void        gimp_overlay_box_set_child_alignment (GimpOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         xalign,
                                                  gdouble         yalign);
void        gimp_overlay_box_set_child_position  (GimpOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         x,
                                                  gdouble         y);
void        gimp_overlay_box_set_child_angle     (GimpOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         angle);
void        gimp_overlay_box_set_child_opacity   (GimpOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         opacity);

void        gimp_overlay_box_scroll              (GimpOverlayBox *box,
                                                  gint            offset_x,
                                                  gint            offset_y);


#endif /*  __GIMP_OVERLAY_BOX_H__  */
