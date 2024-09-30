/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_RESOURCE_SELECT_H__
#define __GIMP_RESOURCE_SELECT_H__

G_BEGIN_DECLS

/**
 * GimpResourceChoosedCallback:
 * @resource: Chosen resource
 * @is_dialog_closing: Did user click Close button of dialog?
 * @owner_data: (closure): Owner's data
 *
 * Callback from libgimp GimpResourceSelect adapter to owner.
 **/
typedef void (* GimpResourceChoosedCallback) (GimpResource *resource,
                                              gboolean      is_dialog_closing,
                                              gpointer      owner_data);

const gchar * gimp_resource_select_new       (const gchar                 *title,
                                              GBytes                      *parent_handle,
                                              GimpResource                *resource,
                                              GType                        resource_type,
                                              GimpResourceChoosedCallback  callback,
                                              gpointer                     owner_data,
                                              GDestroyNotify               data_destroy);

void          gimp_resource_select_set       (const gchar                 *callback_name,
                                              GimpResource                *resource);

G_END_DECLS

#endif /* __GIMP_RESOURCE_SELECT_H__ */
