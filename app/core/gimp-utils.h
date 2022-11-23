/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __APP_LIGMA_UTILS_H__
#define __APP_LIGMA_UTILS_H__


#define LIGMA_TIMER_START() \
  { GTimer *_timer = g_timer_new ();

#define LIGMA_TIMER_END(message) \
  g_printerr ("%s: %s took %0.4f seconds\n", \
              G_STRFUNC, message, g_timer_elapsed (_timer, NULL)); \
  g_timer_destroy (_timer); }


#define MIN4(a,b,c,d) MIN (MIN ((a), (b)), MIN ((c), (d)))
#define MAX4(a,b,c,d) MAX (MAX ((a), (b)), MAX ((c), (d)))


gint         ligma_get_pid                          (void);
guint64      ligma_get_physical_memory_size         (void);
gchar      * ligma_get_default_language             (const gchar     *category);
LigmaUnit     ligma_get_default_unit                 (void);

gchar     ** ligma_properties_append                (GType            object_type,
                                                    gint            *n_properties,
                                                    gchar          **names,
                                                    GValue         **values,
                                                    ...) G_GNUC_NULL_TERMINATED;
gchar     ** ligma_properties_append_valist         (GType            object_type,
                                                    gint            *n_properties,
                                                    gchar          **names,
                                                    GValue         **values,
                                                    va_list          args);
void         ligma_properties_free                  (gint             n_properties,
                                                    gchar          **names,
                                                    GValue          *values);

gchar      * ligma_markup_extract_text              (const gchar     *markup);

const gchar* ligma_enum_get_value_name              (GType            enum_type,
                                                    gint             value);

gboolean     ligma_get_fill_params                  (LigmaContext      *context,
                                                    LigmaFillType      fill_type,
                                                    LigmaRGB          *color,
                                                    LigmaPattern     **pattern,
                                                    GError          **error);

/* Common values for the n_snap_lines parameter of
 * ligma_constrain_line.
 */
#define LIGMA_CONSTRAIN_LINE_90_DEGREES 2
#define LIGMA_CONSTRAIN_LINE_45_DEGREES 4
#define LIGMA_CONSTRAIN_LINE_15_DEGREES 12

void         ligma_constrain_line                   (gdouble            start_x,
                                                    gdouble            start_y,
                                                    gdouble           *end_x,
                                                    gdouble           *end_y,
                                                    gint               n_snap_lines,
                                                    gdouble            offset_angle,
                                                    gdouble            xres,
                                                    gdouble            yres);

gint         ligma_file_compare                     (GFile             *file1,
                                                    GFile             *file2);
gboolean     ligma_file_is_executable               (GFile             *file);
gchar      * ligma_file_get_extension               (GFile             *file);
GFile      * ligma_file_with_new_extension          (GFile             *file,
                                                    GFile             *ext_file);
gboolean     ligma_file_delete_recursive            (GFile             *file,
                                                    GError           **error);

gchar      * ligma_data_input_stream_read_line_always
                                                   (GDataInputStream  *stream,
                                                    gsize             *length,
                                                    GCancellable      *cancellable,
                                                    GError           **error);

gboolean     ligma_ascii_strtoi                     (const gchar       *nptr,
                                                    gchar            **endptr,
                                                    gint               base,
                                                    gint              *result);
gboolean     ligma_ascii_strtod                     (const gchar       *nptr,
                                                    gchar            **endptr,
                                                    gdouble           *result);

gchar     *  ligma_appstream_to_pango_markup        (const gchar       *as_text);
void         ligma_appstream_to_pango_markups       (const gchar       *as_text,
                                                    gchar            **introduction,
                                                    GList            **release_items);

gint         ligma_g_list_compare                   (GList             *list1,
                                                    GList             *list2);

LigmaTRCType  ligma_suggest_trc_for_component_type   (LigmaComponentType  component_type,
                                                    LigmaTRCType        old_trc);

LigmaAsync  * ligma_idle_run_async                   (LigmaRunAsyncFunc func,
                                                    gpointer         user_data);
LigmaAsync  * ligma_idle_run_async_full              (gint             priority,
                                                    LigmaRunAsyncFunc func,
                                                    gpointer         user_data,
                                                    GDestroyNotify   user_data_destroy_func);

LigmaImage  * ligma_create_image_from_buffer         (Ligma              *ligma,
                                                    GeglBuffer        *buffer,
                                                    const gchar       *image_name);

gint         ligma_view_size_get_larger             (gint view_size);
gint         ligma_view_size_get_smaller            (gint view_size);

#ifdef G_OS_WIN32

gboolean     ligma_win32_have_wintab                (void);
gboolean     ligma_win32_have_windows_ink           (void);

#endif

#endif /* __APP_LIGMA_UTILS_H__ */
