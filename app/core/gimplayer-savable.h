/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattisbvf
 *
 * GimpImage-save
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

#include "gimpsavable.h"


/* Protected functions for subclasses of GimpLayer */

void     gimp_subclass_layer_enter (GimpLoadState  *state,
                                    const gchar   **attribute_names,
                                    const gchar   **attribute_values,
                                    gpointer        user_data,
                                    GError        **error);
gboolean gimp_subclass_layer_exit  (GimpLoadState  *state,
                                    const gchar    *text,
                                    gsize           len,
                                    gpointer        user_data,
                                    GError        **error);
