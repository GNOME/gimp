/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaquerybox.h
 * Copyright (C) 1999-2000 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_QUERY_BOX_H__
#define __LIGMA_QUERY_BOX_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * LigmaQueryStringCallback:
 * @query_box: The query box.
 * @string:    The entered string.
 * @data: (closure): user data.
 *
 * Note that you must not g_free() the passed string.
 **/
typedef void (* LigmaQueryStringCallback)  (GtkWidget   *query_box,
                                           const gchar *string,
                                           gpointer     data);

/**
 * LigmaQueryIntCallback:
 * @query_box: The query box.
 * @value:     The entered integer value.
 * @data: (closure): user data.
 *
 * The callback for an int query box.
 **/
typedef void (* LigmaQueryIntCallback)     (GtkWidget   *query_box,
                                           gint         value,
                                           gpointer     data);

/**
 * LigmaQueryDoubleCallback:
 * @query_box: The query box.
 * @value:     The entered double value.
 * @data: (closure): user data.
 *
 * The callback for a double query box.
 **/
typedef void (* LigmaQueryDoubleCallback)  (GtkWidget   *query_box,
                                           gdouble      value,
                                           gpointer     data);

/**
 * LigmaQuerySizeCallback:
 * @query_box: The query box.
 * @size:      The entered size in pixels.
 * @unit:      The selected unit from the #LigmaUnitMenu.
 * @data: (closure): user data.
 *
 * The callback for a size query box.
 **/
typedef void (* LigmaQuerySizeCallback)    (GtkWidget   *query_box,
                                           gdouble      size,
                                           LigmaUnit     unit,
                                           gpointer     data);

/**
 * LigmaQueryBooleanCallback:
 * @query_box: The query box.
 * @value:     The entered boolean value.
 * @data: (closure): user data.
 *
 * The callback for a boolean query box.
 **/
typedef void (* LigmaQueryBooleanCallback) (GtkWidget   *query_box,
                                           gboolean     value,
                                           gpointer     data);


/**
 * LIGMA_QUERY_BOX_VBOX:
 * @qbox: The query box.
 *
 * A macro to access the vertical #GtkBox in a #libligmawidgets-ligmaquerybox.
 * Useful if you want to add more widgets.
 **/
#define LIGMA_QUERY_BOX_VBOX(qbox) g_object_get_data (G_OBJECT (qbox), \
                                                     "ligma-query-box-vbox")


/*  some simple query dialogs  */
GtkWidget * ligma_query_string_box  (const gchar              *title,
                                    GtkWidget                *parent,
                                    LigmaHelpFunc              help_func,
                                    const gchar              *help_id,
                                    const gchar              *message,
                                    const gchar              *initial,
                                    GObject                  *object,
                                    const gchar              *signal,
                                    LigmaQueryStringCallback   callback,
                                    gpointer                  data,
                                    GDestroyNotify            data_destroy);

GtkWidget * ligma_query_int_box     (const gchar              *title,
                                    GtkWidget                *parent,
                                    LigmaHelpFunc              help_func,
                                    const gchar              *help_id,
                                    const gchar              *message,
                                    gint                      initial,
                                    gint                      lower,
                                    gint                      upper,
                                    GObject                  *object,
                                    const gchar              *signal,
                                    LigmaQueryIntCallback      callback,
                                    gpointer                  data,
                                    GDestroyNotify            data_destroy);

GtkWidget * ligma_query_double_box  (const gchar              *title,
                                    GtkWidget                *parent,
                                    LigmaHelpFunc              help_func,
                                    const gchar              *help_id,
                                    const gchar              *message,
                                    gdouble                   initial,
                                    gdouble                   lower,
                                    gdouble                   upper,
                                    gint                      digits,
                                    GObject                  *object,
                                    const gchar              *signal,
                                    LigmaQueryDoubleCallback   callback,
                                    gpointer                  data,
                                    GDestroyNotify            data_destroy);

GtkWidget * ligma_query_size_box    (const gchar              *title,
                                    GtkWidget                *parent,
                                    LigmaHelpFunc              help_func,
                                    const gchar              *help_id,
                                    const gchar              *message,
                                    gdouble                   initial,
                                    gdouble                   lower,
                                    gdouble                   upper,
                                    gint                      digits,
                                    LigmaUnit                  unit,
                                    gdouble                   resolution,
                                    gboolean                  dot_for_dot,
                                    GObject                  *object,
                                    const gchar              *signal,
                                    LigmaQuerySizeCallback     callback,
                                    gpointer                  data,
                                    GDestroyNotify            data_destroy);

GtkWidget * ligma_query_boolean_box (const gchar              *title,
                                    GtkWidget                *parent,
                                    LigmaHelpFunc              help_func,
                                    const gchar              *help_id,
                                    const gchar              *icon_name,
                                    const gchar              *message,
                                    const gchar              *true_button,
                                    const gchar              *false_button,
                                    GObject                  *object,
                                    const gchar              *signal,
                                    LigmaQueryBooleanCallback  callback,
                                    gpointer                  data,
                                    GDestroyNotify            data_destroy);


G_END_DECLS

#endif /* __LIGMA_QUERY_BOX_H__ */
