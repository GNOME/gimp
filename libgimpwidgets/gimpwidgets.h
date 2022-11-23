/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmawidgets.h
 * Copyright (C) 2000 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_WIDGETS_H__
#define __LIGMA_WIDGETS_H__

#define __LIGMA_WIDGETS_H_INSIDE__

#include <libligmawidgets/ligmawidgetstypes.h>

#include <libligmawidgets/ligmabrowser.h>
#include <libligmawidgets/ligmabusybox.h>
#include <libligmawidgets/ligmabutton.h>
#include <libligmawidgets/ligmacairo-utils.h>
#include <libligmawidgets/ligmacellrenderercolor.h>
#include <libligmawidgets/ligmacellrenderertoggle.h>
#include <libligmawidgets/ligmachainbutton.h>
#include <libligmawidgets/ligmacolorarea.h>
#include <libligmawidgets/ligmacolorbutton.h>
#include <libligmawidgets/ligmacolordisplay.h>
#include <libligmawidgets/ligmacolordisplaystack.h>
#include <libligmawidgets/ligmacolorhexentry.h>
#include <libligmawidgets/ligmacolornotebook.h>
#include <libligmawidgets/ligmacolorprofilechooserdialog.h>
#include <libligmawidgets/ligmacolorprofilecombobox.h>
#include <libligmawidgets/ligmacolorprofilestore.h>
#include <libligmawidgets/ligmacolorprofileview.h>
#include <libligmawidgets/ligmacolorscale.h>
#include <libligmawidgets/ligmacolorscaleentry.h>
#include <libligmawidgets/ligmacolorscales.h>
#include <libligmawidgets/ligmacolorselector.h>
#include <libligmawidgets/ligmacolorselect.h>
#include <libligmawidgets/ligmacolorselection.h>
#include <libligmawidgets/ligmadialog.h>
#include <libligmawidgets/ligmaenumcombobox.h>
#include <libligmawidgets/ligmaenumlabel.h>
#include <libligmawidgets/ligmaenumstore.h>
#include <libligmawidgets/ligmaenumwidgets.h>
#include <libligmawidgets/ligmafileentry.h>
#include <libligmawidgets/ligmaframe.h>
#include <libligmawidgets/ligmahelpui.h>
#include <libligmawidgets/ligmahintbox.h>
#include <libligmawidgets/ligmaicons.h>
#include <libligmawidgets/ligmaintcombobox.h>
#include <libligmawidgets/ligmaintradioframe.h>
#include <libligmawidgets/ligmaintstore.h>
#include <libligmawidgets/ligmalabelcolor.h>
#include <libligmawidgets/ligmalabeled.h>
#include <libligmawidgets/ligmalabelentry.h>
#include <libligmawidgets/ligmalabelintwidget.h>
#include <libligmawidgets/ligmalabelspin.h>
#include <libligmawidgets/ligmamemsizeentry.h>
#include <libligmawidgets/ligmanumberpairentry.h>
#include <libligmawidgets/ligmaoffsetarea.h>
#include <libligmawidgets/ligmapageselector.h>
#include <libligmawidgets/ligmapatheditor.h>
#include <libligmawidgets/ligmapickbutton.h>
#include <libligmawidgets/ligmapreview.h>
#include <libligmawidgets/ligmapreviewarea.h>
#include <libligmawidgets/ligmapropwidgets.h>
#include <libligmawidgets/ligmaquerybox.h>
#include <libligmawidgets/ligmaruler.h>
#include <libligmawidgets/ligmascaleentry.h>
#include <libligmawidgets/ligmascrolledpreview.h>
#include <libligmawidgets/ligmasizeentry.h>
#include <libligmawidgets/ligmaspinbutton.h>
#include <libligmawidgets/ligmaspinscale.h>
#include <libligmawidgets/ligmastringcombobox.h>
#include <libligmawidgets/ligmaunitcombobox.h>
#include <libligmawidgets/ligmaunitstore.h>
#include <libligmawidgets/ligmawidgets-error.h>
#include <libligmawidgets/ligmawidgetsutils.h>
#include <libligmawidgets/ligmazoommodel.h>

#undef __LIGMA_WIDGETS_H_INSIDE__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 *  Widget Constructors
 */

/* specify radio buttons as va_list:
 *  const gchar  *label,
 *  gint          item_data,
 *  GtkWidget   **widget_ptr,
 */
GtkWidget * ligma_int_radio_group_new (gboolean          in_frame,
                                      const gchar      *frame_title,
                                      GCallback         radio_button_callback,
                                      gpointer          radio_button_callback_data,
                                      GDestroyNotify    radio_button_callback_destroy,
                                      gint              initial, /* item_data */
                                      ...) G_GNUC_NULL_TERMINATED;

void        ligma_int_radio_group_set_active (GtkRadioButton *radio_button,
                                             gint            item_data);


/**
 * LIGMA_RANDOM_SEED_SPINBUTTON:
 * @hbox: The #GtkBox returned by ligma_random_seed_new().
 *
 * Returns: the random_seed's #GtkSpinButton.
 **/
#define LIGMA_RANDOM_SEED_SPINBUTTON(hbox) \
        (g_object_get_data (G_OBJECT (hbox), "spinbutton"))

/**
 * LIGMA_RANDOM_SEED_SPINBUTTON_ADJ:
 * @hbox: The #GtkBox returned by ligma_random_seed_new().
 *
 * Returns: the #GtkAdjustment of the random_seed's #GtkSpinButton.
 **/
#define LIGMA_RANDOM_SEED_SPINBUTTON_ADJ(hbox)       \
        gtk_spin_button_get_adjustment \
        (GTK_SPIN_BUTTON (g_object_get_data (G_OBJECT (hbox), "spinbutton")))

/**
 * LIGMA_RANDOM_SEED_TOGGLE:
 * @hbox: The #GtkBox returned by ligma_random_seed_new().
 *
 * Returns: the random_seed's #GtkToggleButton.
 **/
#define LIGMA_RANDOM_SEED_TOGGLE(hbox) \
        (g_object_get_data (G_OBJECT(hbox), "toggle"))

GtkWidget * ligma_random_seed_new   (guint32            *seed,
                                    gboolean           *random_seed);

/**
 * LIGMA_COORDINATES_CHAINBUTTON:
 * @sizeentry: The #LigmaSizeEntry returned by ligma_coordinates_new().
 *
 * Returns: the #LigmaChainButton which is attached to the
 *          #LigmaSizeEntry.
 **/
#define LIGMA_COORDINATES_CHAINBUTTON(sizeentry) \
        (g_object_get_data (G_OBJECT (sizeentry), "chainbutton"))

GtkWidget * ligma_coordinates_new   (LigmaUnit            unit,
                                    const gchar        *unit_format,
                                    gboolean            menu_show_pixels,
                                    gboolean            menu_show_percent,
                                    gint                spinbutton_width,
                                    LigmaSizeEntryUpdatePolicy  update_policy,

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


/*
 *  Standard Callbacks
 */

void ligma_toggle_button_update           (GtkWidget       *widget,
                                          gpointer         data);

void ligma_radio_button_update            (GtkWidget       *widget,
                                          gpointer         data);

void ligma_int_adjustment_update          (GtkAdjustment   *adjustment,
                                          gpointer         data);

void ligma_uint_adjustment_update         (GtkAdjustment   *adjustment,
                                          gpointer         data);

void ligma_float_adjustment_update        (GtkAdjustment   *adjustment,
                                          gpointer         data);

void ligma_double_adjustment_update       (GtkAdjustment   *adjustment,
                                          gpointer         data);


G_END_DECLS

#endif /* __LIGMA_WIDGETS_H__ */
