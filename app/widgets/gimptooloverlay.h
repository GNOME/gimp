/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptooloverlay.h
 * Copyright (C) 2009  Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_TOOL_OVERLAY_H__
#define __GIMP_TOOL_OVERLAY_H__


#define GIMP_TYPE_TOOL_OVERLAY            (gimp_tool_overlay_get_type ())
#define GIMP_TOOL_OVERLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_OVERLAY, GimpToolOverlay))
#define GIMP_TOOL_OVERLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_OVERLAY, GimpToolOverlayClass))
#define GIMP_IS_TOOL_OVERLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_OVERLAY))
#define GIMP_IS_TOOL_OVERLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_OVERLAY))
#define GIMP_TOOL_OVERLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_OVERLAY, GimpToolOverlayClass))


typedef struct _GimpToolOverlay      GimpToolOverlay;
typedef struct _GimpToolOverlayClass GimpToolOverlayClass;

struct _GimpToolOverlay
{
  GtkBin     parent_instance;

  GtkWidget *action_area;
};

struct _GimpToolOverlayClass
{
  GtkBinClass  parent_class;

  void (* response) (GimpToolOverlay *overlay,
                     gint             response_id);

  void (* close)    (GimpToolOverlay *overlay);
};


GType       gimp_tool_overlay_get_type           (void) G_GNUC_CONST;

GtkWidget * gimp_tool_overlay_new                (GimpToolInfo    *tool_info,
                                                  const gchar     *desc,
                                                  ...) G_GNUC_NULL_TERMINATED;

void        gimp_tool_overlay_response           (GimpToolOverlay *overlay,
                                                  gint             response_id);
void        gimp_tool_overlay_add_buttons_valist (GimpToolOverlay *overlay,
                                                  va_list          args);
GtkWidget * gimp_tool_overlay_add_button         (GimpToolOverlay *overlay,
                                                  const gchar     *button_text,
                                                  gint             response_id);


#endif /* __GIMP_TOOL_OVERLAY_H__ */
