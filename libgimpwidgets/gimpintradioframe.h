/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaintradioframe.h
 * Copyright (C) 2022 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#include <libligmawidgets/ligmaframe.h>

#ifndef __LIGMA_INT_RADIO_FRAME_H__
#define __LIGMA_INT_RADIO_FRAME_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_INT_RADIO_FRAME            (ligma_int_radio_frame_get_type ())
#define LIGMA_INT_RADIO_FRAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_INT_RADIO_FRAME, LigmaIntRadioFrame))
#define LIGMA_INT_RADIO_FRAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_INT_RADIO_FRAME, LigmaIntRadioFrameClass))
#define LIGMA_IS_INT_RADIO_FRAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_INT_RADIO_FRAME))
#define LIGMA_IS_INT_RADIO_FRAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_INT_RADIO_FRAME))
#define LIGMA_INT_RADIO_FRAME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_INT_RADIO_FRAME, LigmaIntRadioFrameClass))


typedef struct _LigmaIntRadioFrameClass   LigmaIntRadioFrameClass;

struct _LigmaIntRadioFrame
{
  LigmaFrame       parent_instance;
};

struct _LigmaIntRadioFrameClass
{
  LigmaFrameClass  parent_class;

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


/**
 * LigmaIntRadioFrameSensitivityFunc:
 * @value: the value associated with a radio button.
 * @user_data: the data associated with a radio button.
 * @new_value: the value to check instead if the function returns %FALSE.
 * @data: (closure): the data set in ligma_int_radio_frame_set_sensitivity()
 *
 * Signature for a function called on each radio button value and data,
 * each time the %LigmaIntRadioFrame is drawn, to make some radio button
 * insensitive.
 * If the function returns %FALSE, it usually means that the value is
 * not a valid choice in current situation. In this case, you might want
 * to toggle instead another value automatically. Set @new_value to the
 * value to toggle. If you leave this untouched, the radio button will
 * stay toggled despite being insensitive. This is up to you to decide
 * whether this is meaningful.
 *
 * Returns: %TRUE if the button stays sensitive, %FALSE otherwise.
 */
typedef  gboolean (* LigmaIntRadioFrameSensitivityFunc) (gint      value,
                                                        gpointer  user_data,
                                                        gint     *new_value,
                                                        gpointer  data);



GType         ligma_int_radio_frame_get_type        (void) G_GNUC_CONST;

GtkWidget   * ligma_int_radio_frame_new_from_store  (const gchar       *title,
                                                    LigmaIntStore      *store);
GtkWidget   * ligma_int_radio_frame_new             (const gchar       *first_label,
                                                    gint               first_value,
                                                    ...) G_GNUC_NULL_TERMINATED;
GtkWidget   * ligma_int_radio_frame_new_valist      (const gchar       *first_label,
                                                    gint               first_value,
                                                    va_list            values);

GtkWidget   * ligma_int_radio_frame_new_array       (const gchar       *labels[]);

void          ligma_int_radio_frame_prepend         (LigmaIntRadioFrame *radio_frame,
                                                    ...);
void          ligma_int_radio_frame_append          (LigmaIntRadioFrame *radio_frame,
                                                    ...);

gboolean      ligma_int_radio_frame_set_active      (LigmaIntRadioFrame *radio_frame,
                                                    gint               value);
gint          ligma_int_radio_frame_get_active      (LigmaIntRadioFrame *radio_frame);

gboolean
      ligma_int_radio_frame_set_active_by_user_data (LigmaIntRadioFrame *radio_frame,
                                                    gpointer           user_data);
gboolean
      ligma_int_radio_frame_get_active_user_data    (LigmaIntRadioFrame *radio_frame,
                                                    gpointer          *user_data);

void          ligma_int_radio_frame_set_sensitivity (LigmaIntRadioFrame *radio_frame,
                                                    LigmaIntRadioFrameSensitivityFunc  func,
                                                    gpointer           data,
                                                    GDestroyNotify     destroy);


G_END_DECLS

#endif  /* __LIGMA_INT_RADIO_FRAME_H__ */
