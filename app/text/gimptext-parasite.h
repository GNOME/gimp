/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaText
 * Copyright (C) 2003  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_TEXT_PARASITE_H__
#define __LIGMA_TEXT_PARASITE_H__


const gchar  * ligma_text_parasite_name          (void) G_GNUC_CONST;
LigmaParasite * ligma_text_to_parasite            (LigmaText      *text);
LigmaText     * ligma_text_from_parasite          (const LigmaParasite  *parasite,
                                                 Ligma                *ligma,
                                                 GError             **error);

const gchar  * ligma_text_gdyntext_parasite_name (void) G_GNUC_CONST;
LigmaText     * ligma_text_from_gdyntext_parasite (const LigmaParasite  *parasite);


#endif /* __LIGMA_TEXT_PARASITE_H__ */
