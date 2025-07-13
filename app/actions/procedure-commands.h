/* GIMP - The GNU Image Manipulation Program
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

#pragma once


GimpValueArray * procedure_commands_get_run_mode_arg (GimpProcedure  *procedure);
GimpValueArray * procedure_commands_get_data_args    (GimpProcedure  *procedure,
                                                      GimpObject     *object);
GimpValueArray * procedure_commands_get_image_args   (GimpProcedure  *procedure,
                                                      GimpImage      *image);
GimpValueArray * procedure_commands_get_items_args   (GimpProcedure  *procedure,
                                                      GimpImage      *image,
                                                      GList          *items);
GimpValueArray * procedure_commands_get_display_args (GimpProcedure  *procedure,
                                                      GimpDisplay    *display,
                                                      GimpObject     *settings);

gboolean      procedure_commands_run_procedure       (GimpProcedure  *procedure,
                                                      Gimp           *gimp,
                                                      GimpProgress   *progress,
                                                      GimpValueArray *args);
gboolean      procedure_commands_run_procedure_async (GimpProcedure  *procedure,
                                                      Gimp           *gimp,
                                                      GimpProgress   *progress,
                                                      GimpRunMode     run_mode,
                                                      GimpValueArray *args,
                                                      GimpDisplay    *display);
