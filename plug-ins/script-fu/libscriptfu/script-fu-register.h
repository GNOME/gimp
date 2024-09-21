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

#ifndef __SCRIPT_FU_REGISTER_H__
#define __SCRIPT_FU_REGISTER_H__

pointer   script_fu_script_create_formal_args       (scheme   *sc,
                                                     pointer  *handle,
                                                     SFScript *script);
SFScript *script_fu_script_new_from_metadata_args   (scheme   *sc,
                                                     pointer  *handle);
SFScript *script_fu_script_new_from_metadata_regular
                                                    (scheme   *sc,
                                                     pointer  *handle);
pointer   script_fu_script_parse_drawable_arity_arg (scheme   *sc,
                                                     pointer  *handle,
                                                     SFScript *script);

#endif /* __SCRIPT_FU_REGISTER_H__ */
