/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoverlaydialog.h
 * Copyright (C) 2009-2010  Michael Natterer <mitch@gimp.org>
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

#pragma once

#include "gimpoverlayframe.h"


#define GIMP_RESPONSE_DETACH 100


#define GIMP_TYPE_OVERLAY_DIALOG            (gimp_overlay_dialog_get_type ())
#define GIMP_OVERLAY_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OVERLAY_DIALOG, GimpOverlayDialog))
#define GIMP_OVERLAY_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_OVERLAY_DIALOG, GimpOverlayDialogClass))
#define GIMP_IS_OVERLAY_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OVERLAY_DIALOG))
#define GIMP_IS_OVERLAY_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_OVERLAY_DIALOG))
#define GIMP_OVERLAY_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_OVERLAY_DIALOG, GimpOverlayDialogClass))


typedef struct _GimpOverlayDialog      GimpOverlayDialog;
typedef struct _GimpOverlayDialogClass GimpOverlayDialogClass;

struct _GimpOverlayDialog
{
  GimpOverlayFrame  parent_instance;

  gchar            *title;
  gchar            *icon_name;

  GtkWidget        *header;
  GtkWidget        *icon_image;
  GtkWidget        *title_label;
  GtkWidget        *detach_button;
  GtkWidget        *close_button;
  GtkWidget        *action_area;
};

struct _GimpOverlayDialogClass
{
  GimpOverlayFrameClass  parent_class;

  void (* response) (GimpOverlayDialog *overlay,
                     gint               response_id);

  void (* detach)   (GimpOverlayDialog *overlay);
  void (* close)    (GimpOverlayDialog *overlay);
};


GType       gimp_overlay_dialog_get_type               (void) G_GNUC_CONST;

GtkWidget * gimp_overlay_dialog_new                    (GimpToolInfo      *tool_info,
                                                        const gchar       *desc,
                                                        ...) G_GNUC_NULL_TERMINATED;

void        gimp_overlay_dialog_response               (GimpOverlayDialog *overlay,
                                                        gint               response_id);
void        gimp_overlay_dialog_add_buttons_valist     (GimpOverlayDialog *overlay,
                                                        va_list            args);
GtkWidget * gimp_overlay_dialog_add_button             (GimpOverlayDialog *overlay,
                                                        const gchar       *button_text,
                                                        gint               response_id);
void        gimp_overlay_dialog_set_alternative_button_order
                                                       (GimpOverlayDialog *overlay,
                                                        gint               n_ids,
                                                        gint              *ids);
void        gimp_overlay_dialog_set_default_response   (GimpOverlayDialog *overlay,
                                                        gint               response_id);
void        gimp_overlay_dialog_set_response_sensitive (GimpOverlayDialog *overlay,
                                                        gint               response_id,
                                                        gboolean           sensitive);
