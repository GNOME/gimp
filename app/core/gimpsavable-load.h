/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpsavable.h
 * Copyright (C) 2026 Jehan
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


#define GIMP_WLBR_ERROR (gimp_wlbr_error_quark ())

typedef enum
{
  GIMP_WLBR_ERROR_OPEN,   /*  Opening file failed   */
  GIMP_WLBR_ERROR_READ,   /*  Reading file failed   */
  GIMP_WLBR_ERROR_WRITE,  /*  Writing file failed   */
  GIMP_WLBR_ERROR_DELETE, /*  Deleting file failed  */
  GIMP_WLBR_ERROR_FORMAT, /*  Invalid format        */
  GIMP_WLBR_ERROR_DATA,   /*  Invalid data          */
} GimpWlbrError;


/* Return with %FALSE and @error set if the error is fatale.
 * Return with %TRUE and @error to display a non-fatal error.
 * Return with %TRUE and a %NULL @error for normal handling.
 */
typedef gboolean (* GimpEnterElementHandler) (GimpLoadState  *state,
                                              const gchar   **attribute_names,
                                              const gchar   **attribute_values,
                                              gpointer        user_data,
                                              GError         **error);
typedef gboolean (* GimpExitElementhandler)  (GimpLoadState  *state,
                                              const gchar     *text,
                                              gsize            len,
                                              gpointer         user_data,
                                              GError         **error);


void         gimp_savable_load                   (GType                    savable_type,
                                                  GimpLoadState           *state);
void         gimp_savable_config_load            (GType                    config_type,
                                                  const gchar             *element_name,
                                                  GimpLoadState           *state,
                                                  GimpExitElementhandler   secondary_exit_handler,
                                                  ...) G_GNUC_NULL_TERMINATED;

/* Functions to be used from inside GimpSavable's load() implementation */

void         gimp_savable_load_store_from_string  (GimpLoadState            *state,
                                                   ...) G_GNUC_NULL_TERMINATED;
void         gimp_savable_load_store_value        (GimpLoadState            *state,
                                                   const gchar              *key,
                                                   gpointer                  data,
                                                   GDestroyNotify            free_data);

gboolean     gimp_savable_load_get_values         (GimpLoadState            *state,
                                                   ...) G_GNUC_NULL_TERMINATED;
gboolean     gimp_savable_load_get_parent_values  (GimpLoadState            *state,
                                                   ...) G_GNUC_NULL_TERMINATED;
void         gimp_savable_load_bubble_up          (GimpLoadState            *state,
                                                   const gchar              *key);

void         gimp_savable_load_add_handlers       (GimpLoadState            *state,
                                                   const gchar              *element_name,
                                                   GimpEnterElementHandler   enter_callback,
                                                   GimpExitElementhandler    exit_callback,
                                                   gpointer                  user_data,
                                                   GDestroyNotify            free_data);

void         gimp_savable_load_add_simple_handler (GimpLoadState            *state,
                                                   const gchar              *element_name,
                                                   const gchar              *text_value_format,
                                                   gboolean                  all_attributes_needed,
                                                   gboolean                  fatal_on_missing,
                                                   ...) G_GNUC_NULL_TERMINATED;


/* Generic element handlers as args to gimp_savable_load_add_handlers(). */

gboolean     gimp_savable_enter_unit             (GimpLoadState            *state,
                                                  const gchar             **attribute_names,
                                                  const gchar             **attribute_values,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     gimp_savable_exit_unit              (GimpLoadState            *state,
                                                  const gchar              *text,
                                                  gsize                     len,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     gimp_savable_enter_color            (GimpLoadState            *state,
                                                  const gchar             **attribute_names,
                                                  const gchar             **attribute_values,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     gimp_savable_exit_color             (GimpLoadState            *state,
                                                  const gchar              *text,
                                                  gsize                     len,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     gimp_savable_enter_format           (GimpLoadState            *state,
                                                  const gchar             **attribute_names,
                                                  const gchar             **attribute_values,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     gimp_savable_exit_format            (GimpLoadState            *state,
                                                  const gchar              *text,
                                                  gsize                     len,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     gimp_savable_enter_space            (GimpLoadState            *state,
                                                  const gchar             **attribute_names,
                                                  const gchar             **attribute_values,
                                                  gpointer                  user_data,
                                                  GError                  **error);
gboolean     gimp_savable_exit_space             (GimpLoadState            *state,
                                                  const gchar              *text,
                                                  gsize                     len,
                                                  gpointer                  user_data,
                                                  GError                  **error);

/* Friend functions used only in gimpimage-savable.c */

gboolean      gimp_savable_load_parse            (GimpLoadState            *state,
                                                  Gimp                     *gimp,
                                                  GFile                    *backup_dir,
                                                  GError                  **error);
void          gimp_savable_load_free_state       (GimpLoadState            *state);
void          gimp_savable_load_append_text      (GimpLoadState            *state,
                                                  const gchar              *text,
                                                  gsize                     text_len);
const gchar * gimp_savable_load_get_text         (GimpLoadState            *state,
                                                  gsize                    *text_len);

gboolean      gimp_savable_enter_element         (GimpLoadState            *state,
                                                  const gchar              *element_name,
                                                  const gchar             **attribute_names,
                                                  const gchar             **attribute_values,
                                                  GError                  **error);
gboolean      gimp_savable_exit_element          (GimpLoadState            *state,
                                                  const gchar              *element_name,
                                                  const gchar              *text,
                                                  gsize                     text_len,
                                                  GError                  **error);


/*  The error domain associated with format loading. */

GQuark       gimp_wlbr_error_quark               (void) G_GNUC_CONST;
