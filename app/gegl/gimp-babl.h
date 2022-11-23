/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-babl.h
 * Copyright (C) 2012 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_BABL_H__
#define __LIGMA_BABL_H__


void                ligma_babl_init                         (void);
void                ligma_babl_init_fishes                  (LigmaInitStatusFunc  status_callback);

const gchar       * ligma_babl_format_get_description       (const Babl         *format);
LigmaColorProfile  * ligma_babl_format_get_color_profile     (const Babl         *format);
LigmaColorProfile  * ligma_babl_get_builtin_color_profile    (LigmaImageBaseType   base_type,
                                                            LigmaTRCType         trc);

LigmaImageBaseType   ligma_babl_format_get_base_type         (const Babl         *format);
LigmaComponentType   ligma_babl_format_get_component_type    (const Babl         *format);
LigmaPrecision       ligma_babl_format_get_precision         (const Babl         *format);
LigmaTRCType         ligma_babl_format_get_trc               (const Babl         *format);

LigmaComponentType   ligma_babl_component_type               (LigmaPrecision       precision);
LigmaTRCType         ligma_babl_trc                          (LigmaPrecision       precision);
LigmaPrecision       ligma_babl_precision                    (LigmaComponentType   component,
                                                            LigmaTRCType         trc);

gboolean            ligma_babl_is_valid                     (LigmaImageBaseType   base_type,
                                                            LigmaPrecision       precision);
LigmaComponentType   ligma_babl_is_bounded                   (LigmaPrecision       precision);

const Babl        * ligma_babl_format                       (LigmaImageBaseType   base_type,
                                                            LigmaPrecision       precision,
                                                            gboolean            with_alpha,
                                                            const Babl         *space);
const Babl        * ligma_babl_mask_format                  (LigmaPrecision       precision);
const Babl        * ligma_babl_component_format             (LigmaImageBaseType   base_type,
                                                            LigmaPrecision       precision,
                                                            gint                index);

const Babl        * ligma_babl_format_change_component_type (const Babl          *format,
                                                            LigmaComponentType    component);
const Babl        * ligma_babl_format_change_trc            (const Babl          *format,
                                                            LigmaTRCType          trc);

gchar            ** ligma_babl_print_pixel                  (const Babl          *format,
                                                            gpointer             pixel);


#endif /* __LIGMA_BABL_H__ */
