/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoverlaydialog.h
 * Copyright (C) 2009-2010  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_OVERLAY_DIALOG_H__
#define __LIGMA_OVERLAY_DIALOG_H__


#include "ligmaoverlayframe.h"


#define LIGMA_RESPONSE_DETACH 100


#define LIGMA_TYPE_OVERLAY_DIALOG            (ligma_overlay_dialog_get_type ())
#define LIGMA_OVERLAY_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OVERLAY_DIALOG, LigmaOverlayDialog))
#define LIGMA_OVERLAY_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_OVERLAY_DIALOG, LigmaOverlayDialogClass))
#define LIGMA_IS_OVERLAY_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OVERLAY_DIALOG))
#define LIGMA_IS_OVERLAY_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_OVERLAY_DIALOG))
#define LIGMA_OVERLAY_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_OVERLAY_DIALOG, LigmaOverlayDialogClass))


typedef struct _LigmaOverlayDialog      LigmaOverlayDialog;
typedef struct _LigmaOverlayDialogClass LigmaOverlayDialogClass;

struct _LigmaOverlayDialog
{
  LigmaOverlayFrame  parent_instance;

  gchar            *title;
  gchar            *icon_name;

  GtkWidget        *header;
  GtkWidget        *icon_image;
  GtkWidget        *title_label;
  GtkWidget        *detach_button;
  GtkWidget        *close_button;
  GtkWidget        *action_area;
};

struct _LigmaOverlayDialogClass
{
  LigmaOverlayFrameClass  parent_class;

  void (* response) (LigmaOverlayDialog *overlay,
                     gint               response_id);

  void (* detach)   (LigmaOverlayDialog *overlay);
  void (* close)    (LigmaOverlayDialog *overlay);
};


GType       ligma_overlay_dialog_get_type               (void) G_GNUC_CONST;

GtkWidget * ligma_overlay_dialog_new                    (LigmaToolInfo      *tool_info,
                                                        const gchar       *desc,
                                                        ...) G_GNUC_NULL_TERMINATED;

void        ligma_overlay_dialog_response               (LigmaOverlayDialog *overlay,
                                                        gint               response_id);
void        ligma_overlay_dialog_add_buttons_valist     (LigmaOverlayDialog *overlay,
                                                        va_list            args);
GtkWidget * ligma_overlay_dialog_add_button             (LigmaOverlayDialog *overlay,
                                                        const gchar       *button_text,
                                                        gint               response_id);
void        ligma_overlay_dialog_set_alternative_button_order
                                                       (LigmaOverlayDialog *overlay,
                                                        gint               n_ids,
                                                        gint              *ids);
void        ligma_overlay_dialog_set_default_response   (LigmaOverlayDialog *overlay,
                                                        gint               response_id);
void        ligma_overlay_dialog_set_response_sensitive (LigmaOverlayDialog *overlay,
                                                        gint               response_id,
                                                        gboolean           sensitive);


#endif /* __LIGMA_OVERLAY_DIALOG_H__ */
