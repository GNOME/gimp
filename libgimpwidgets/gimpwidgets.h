/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpwidgets.h
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_WIDGETS_H__
#define __GIMP_WIDGETS_H__


#include <libgimpwidgets/gimpwidgetstypes.h>

#include <libgimpwidgets/gimpbrowser.h>
#include <libgimpwidgets/gimpbutton.h>
#include <libgimpwidgets/gimpcellrenderercolor.h>
#include <libgimpwidgets/gimpcellrenderertoggle.h>
#include <libgimpwidgets/gimpchainbutton.h>
#include <libgimpwidgets/gimpcolorarea.h>
#include <libgimpwidgets/gimpcolorbutton.h>
#include <libgimpwidgets/gimpcolordisplay.h>
#include <libgimpwidgets/gimpcolordisplaystack.h>
#include <libgimpwidgets/gimpcolorhexentry.h>
#include <libgimpwidgets/gimpcolornotebook.h>
#include <libgimpwidgets/gimpcolorscale.h>
#include <libgimpwidgets/gimpcolorscales.h>
#include <libgimpwidgets/gimpcolorselector.h>
#include <libgimpwidgets/gimpcolorselect.h>
#include <libgimpwidgets/gimpcolorselection.h>
#include <libgimpwidgets/gimpdialog.h>
#include <libgimpwidgets/gimpenumcombobox.h>
#include <libgimpwidgets/gimpenumlabel.h>
#include <libgimpwidgets/gimpenumstore.h>
#include <libgimpwidgets/gimpenumwidgets.h>
#include <libgimpwidgets/gimpfileentry.h>
#include <libgimpwidgets/gimpframe.h>
#include <libgimpwidgets/gimphelpui.h>
#include <libgimpwidgets/gimphintbox.h>
#include <libgimpwidgets/gimpintcombobox.h>
#include <libgimpwidgets/gimpintstore.h>
#include <libgimpwidgets/gimpmemsizeentry.h>
#include <libgimpwidgets/gimpoffsetarea.h>
#include <libgimpwidgets/gimppageselector.h>
#include <libgimpwidgets/gimppatheditor.h>
#include <libgimpwidgets/gimppickbutton.h>
#include <libgimpwidgets/gimppixmap.h>
#include <libgimpwidgets/gimppreview.h>
#include <libgimpwidgets/gimppreviewarea.h>
#include <libgimpwidgets/gimppropwidgets.h>
#include <libgimpwidgets/gimpquerybox.h>
#include <libgimpwidgets/gimpratioentry.h>
#include <libgimpwidgets/gimpscrolledpreview.h>
#include <libgimpwidgets/gimpsizeentry.h>
#include <libgimpwidgets/gimpstock.h>
#include <libgimpwidgets/gimpstringcombobox.h>
#include <libgimpwidgets/gimpunitmenu.h>
#include <libgimpwidgets/gimpzoommodel.h>

#include <libgimpwidgets/gimpoldwidgets.h>


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 *  Widget Constructors
 */

GtkWidget * gimp_int_radio_group_new (gboolean          in_frame,
                                      const gchar      *frame_title,
                                      GCallback         radio_button_callback,
                                      gpointer          radio_button_callback_data,
                                      gint              initial, /* item_data */

                                      /* specify radio buttons as va_list:
                                       *  const gchar  *label,
                                       *  gint          item_data,
                                       *  GtkWidget   **widget_ptr,
                                       */

                                      ...) G_GNUC_NULL_TERMINATED;

void        gimp_int_radio_group_set_active (GtkRadioButton *radio_button,
                                             gint            item_data);


GtkWidget * gimp_radio_group_new   (gboolean            in_frame,
                                    const gchar        *frame_title,

                                    /* specify radio buttons as va_list:
                                     *  const gchar    *label,
                                     *  GCallback       callback,
                                     *  gpointer        callback_data,
                                     *  gpointer        item_data,
                                     *  GtkWidget     **widget_ptr,
                                     *  gboolean        active,
                                     */

                                    ...) G_GNUC_NULL_TERMINATED;
GtkWidget * gimp_radio_group_new2  (gboolean            in_frame,
                                    const gchar        *frame_title,
                                    GCallback           radio_button_callback,
                                    gpointer            radio_button_callback_data,
                                    gpointer            initial, /* item_data */

                                    /* specify radio buttons as va_list:
                                     *  const gchar    *label,
                                     *  gpointer        item_data,
                                     *  GtkWidget     **widget_ptr,
                                     */

                                    ...) G_GNUC_NULL_TERMINATED;

void   gimp_radio_group_set_active (GtkRadioButton     *radio_button,
                                    gpointer            item_data);


GtkWidget * gimp_spin_button_new   (/* return value: */
                                    GtkObject         **adjustment,

                                    gdouble             value,
                                    gdouble             lower,
                                    gdouble             upper,
                                    gdouble             step_increment,
                                    gdouble             page_increment,
                                    gdouble             page_size,
                                    gdouble             climb_rate,
                                    guint               digits);

#define GIMP_SCALE_ENTRY_LABEL(adj) \
        (g_object_get_data (G_OBJECT (adj), "label"))

#define GIMP_SCALE_ENTRY_SCALE(adj) \
        (g_object_get_data (G_OBJECT (adj), "scale"))
#define GIMP_SCALE_ENTRY_SCALE_ADJ(adj) \
        gtk_range_get_adjustment \
        (GTK_RANGE (g_object_get_data (G_OBJECT (adj), "scale")))

#define GIMP_SCALE_ENTRY_SPINBUTTON(adj) \
        (g_object_get_data (G_OBJECT (adj), "spinbutton"))
#define GIMP_SCALE_ENTRY_SPINBUTTON_ADJ(adj) \
        gtk_spin_button_get_adjustment \
        (GTK_SPIN_BUTTON (g_object_get_data (G_OBJECT (adj), "spinbutton")))

GtkObject * gimp_scale_entry_new       (GtkTable           *table,
                                        gint                column,
                                        gint                row,
                                        const gchar        *text,
                                        gint                scale_width,
                                        gint                spinbutton_width,
                                        gdouble             value,
                                        gdouble             lower,
                                        gdouble             upper,
                                        gdouble             step_increment,
                                        gdouble             page_increment,
                                        guint               digits,
                                        gboolean            constrain,
                                        gdouble             unconstrained_lower,
                                        gdouble             unconstrained_upper,
                                        const gchar        *tooltip,
                                        const gchar        *help_id);


GtkObject * gimp_color_scale_entry_new (GtkTable           *table,
                                        gint                column,
                                        gint                row,
                                        const gchar        *text,
                                        gint                scale_width,
                                        gint                spinbutton_width,
                                        gdouble             value,
                                        gdouble             lower,
                                        gdouble             upper,
                                        gdouble             step_increment,
                                        gdouble             page_increment,
                                        guint               digits,
                                        const gchar        *tooltip,
                                        const gchar        *help_id);

void     gimp_scale_entry_set_sensitive   (GtkObject       *adjustment,
                                           gboolean         sensitive);

void     gimp_scale_entry_set_logarithmic (GtkObject       *adjustment,
                                           gboolean         logarithmic);
gboolean gimp_scale_entry_get_logarithmic (GtkObject       *adjustment);


#define GIMP_RANDOM_SEED_SPINBUTTON(hbox) \
        (g_object_get_data (G_OBJECT (hbox), "spinbutton"))
#define GIMP_RANDOM_SEED_SPINBUTTON_ADJ(hbox) \
        gtk_spin_button_get_adjustment \
        (GTK_SPIN_BUTTON (g_object_get_data (G_OBJECT (hbox), "spinbutton")))
#define GIMP_RANDOM_SEED_TOGGLE(hbox) \
        (g_object_get_data (G_OBJECT(hbox), "toggle"))

GtkWidget * gimp_random_seed_new   (guint32            *seed,
                                    gboolean           *random_seed);

#define GIMP_COORDINATES_CHAINBUTTON(sizeentry) \
        (g_object_get_data (G_OBJECT (sizeentry), "chainbutton"))

GtkWidget * gimp_coordinates_new   (GimpUnit            unit,
                                    const gchar        *unit_format,
                                    gboolean            menu_show_pixels,
                                    gboolean            menu_show_percent,
                                    gint                spinbutton_width,
                                    GimpSizeEntryUpdatePolicy  update_policy,

                                    gboolean            chainbutton_active,
                                    gboolean            chain_constrains_ratio,

                                    const gchar        *xlabel,
                                    gdouble             x,
                                    gdouble             xres,
                                    gdouble             lower_boundary_x,
                                    gdouble             upper_boundary_x,
                                    gdouble             xsize_0,   /* % */
                                    gdouble             xsize_100, /* % */

                                    const gchar        *ylabel,
                                    gdouble             y,
                                    gdouble             yres,
                                    gdouble             lower_boundary_y,
                                    gdouble             upper_boundary_y,
                                    gdouble             ysize_0,   /* % */
                                    gdouble             ysize_100  /* % */);


#ifndef GIMP_DISABLE_DEPRECATED

GtkWidget * gimp_pixmap_button_new  (gchar             **xpm_data,
                                     const gchar        *text);

#endif


/*
 *  Standard Callbacks
 */

void gimp_toggle_button_sensitive_update (GtkToggleButton *toggle_button);

void gimp_toggle_button_update           (GtkWidget       *widget,
                                          gpointer         data);

void gimp_radio_button_update            (GtkWidget       *widget,
                                          gpointer         data);

void gimp_int_adjustment_update          (GtkAdjustment   *adjustment,
                                          gpointer         data);

void gimp_uint_adjustment_update         (GtkAdjustment   *adjustment,
                                          gpointer         data);

void gimp_float_adjustment_update        (GtkAdjustment   *adjustment,
                                          gpointer         data);

void gimp_double_adjustment_update       (GtkAdjustment   *adjustment,
                                          gpointer         data);

void gimp_unit_menu_update               (GtkWidget       *widget,
                                          gpointer         data);


/*
 *  Helper Functions
 */

GtkWidget * gimp_table_attach_aligned    (GtkTable        *table,
                                          gint             column,
                                          gint             row,
                                          const gchar     *label_text,
                                          gfloat           xalign,
                                          gfloat           yalign,
                                          GtkWidget       *widget,
                                          gint             colspan,
                                          gboolean         left_align);


void        gimp_label_set_attributes    (GtkLabel        *label,
                                          ...);


G_END_DECLS

#endif /* __GIMP_WIDGETS_H__ */
