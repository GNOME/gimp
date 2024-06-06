/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpintradioframe.h
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

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#include <libgimpwidgets/gimpframe.h>

#ifndef __GIMP_INT_RADIO_FRAME_H__
#define __GIMP_INT_RADIO_FRAME_H__

G_BEGIN_DECLS


#define GIMP_TYPE_INT_RADIO_FRAME            (gimp_int_radio_frame_get_type ())
#define GIMP_INT_RADIO_FRAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_INT_RADIO_FRAME, GimpIntRadioFrame))
#define GIMP_INT_RADIO_FRAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_INT_RADIO_FRAME, GimpIntRadioFrameClass))
#define GIMP_IS_INT_RADIO_FRAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_INT_RADIO_FRAME))
#define GIMP_IS_INT_RADIO_FRAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_INT_RADIO_FRAME))
#define GIMP_INT_RADIO_FRAME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_INT_RADIO_FRAME, GimpIntRadioFrameClass))


typedef struct _GimpIntRadioFrameClass   GimpIntRadioFrameClass;

struct _GimpIntRadioFrame
{
  GimpFrame       parent_instance;
};

struct _GimpIntRadioFrameClass
{
  GimpFrameClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


/**
 * GimpIntRadioFrameSensitivityFunc:
 * @value: the value associated with a radio button.
 * @user_data: the data associated with a radio button.
 * @new_value: the value to check instead if the function returns %FALSE.
 * @data: (closure): the data set in gimp_int_radio_frame_set_sensitivity()
 *
 * Signature for a function called on each radio button value and data,
 * each time the %GimpIntRadioFrame is drawn, to make some radio button
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
typedef  gboolean (* GimpIntRadioFrameSensitivityFunc) (gint      value,
                                                        gpointer  user_data,
                                                        gint     *new_value,
                                                        gpointer  data);



GType         gimp_int_radio_frame_get_type        (void) G_GNUC_CONST;

GtkWidget   * gimp_int_radio_frame_new_from_store  (const gchar       *title,
                                                    GimpIntStore      *store);
GtkWidget   * gimp_int_radio_frame_new             (const gchar       *first_label,
                                                    gint               first_value,
                                                    ...) G_GNUC_NULL_TERMINATED;
GtkWidget   * gimp_int_radio_frame_new_valist      (const gchar       *first_label,
                                                    gint               first_value,
                                                    va_list            values);

GtkWidget   * gimp_int_radio_frame_new_array       (const gchar       *labels[]);

void          gimp_int_radio_frame_prepend         (GimpIntRadioFrame *radio_frame,
                                                    ...);
void          gimp_int_radio_frame_append          (GimpIntRadioFrame *radio_frame,
                                                    ...);

void          gimp_int_radio_frame_set_title       (GimpIntRadioFrame *frame,
                                                    const gchar       *title,
                                                    gboolean           with_mnemonic);

gboolean      gimp_int_radio_frame_set_active      (GimpIntRadioFrame *radio_frame,
                                                    gint               value);
gint          gimp_int_radio_frame_get_active      (GimpIntRadioFrame *radio_frame);

gboolean
      gimp_int_radio_frame_set_active_by_user_data (GimpIntRadioFrame *radio_frame,
                                                    gpointer           user_data);
gboolean
      gimp_int_radio_frame_get_active_user_data    (GimpIntRadioFrame *radio_frame,
                                                    gpointer          *user_data);

void          gimp_int_radio_frame_set_sensitivity (GimpIntRadioFrame *radio_frame,
                                                    GimpIntRadioFrameSensitivityFunc  func,
                                                    gpointer           data,
                                                    GDestroyNotify     destroy);


G_END_DECLS

#endif  /* __GIMP_INT_RADIO_FRAME_H__ */
