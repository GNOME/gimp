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

#ifndef __PROCEDURE_COMMANDS_H__
#define __PROCEDURE_COMMANDS_H__


LigmaValueArray * procedure_commands_get_run_mode_arg (LigmaProcedure  *procedure);
LigmaValueArray * procedure_commands_get_data_args    (LigmaProcedure  *procedure,
                                                      LigmaObject     *object);
LigmaValueArray * procedure_commands_get_image_args   (LigmaProcedure  *procedure,
                                                      LigmaImage      *image);
LigmaValueArray * procedure_commands_get_items_args   (LigmaProcedure  *procedure,
                                                      LigmaImage      *image,
                                                      GList          *items);
LigmaValueArray * procedure_commands_get_display_args (LigmaProcedure  *procedure,
                                                      LigmaDisplay    *display,
                                                      LigmaObject     *settings);

gboolean      procedure_commands_run_procedure       (LigmaProcedure  *procedure,
                                                      Ligma           *ligma,
                                                      LigmaProgress   *progress,
                                                      LigmaValueArray *args);
gboolean      procedure_commands_run_procedure_async (LigmaProcedure  *procedure,
                                                      Ligma           *ligma,
                                                      LigmaProgress   *progress,
                                                      LigmaRunMode     run_mode,
                                                      LigmaValueArray *args,
                                                      LigmaDisplay    *display);


#endif /* __PROCEDURE_COMMANDS_H__ */
