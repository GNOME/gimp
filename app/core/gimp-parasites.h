/* ligmaparasite.h: Copyright 1998 Jay Cox <jaycox@ligma.org>
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

#ifndef __LIGMA_PARASITES_H__
#define __LIGMA_PARASITES_H__


/* some wrappers to access ligma->parasites, mainly for the PDB */

gboolean              ligma_parasite_validate     (Ligma               *ligma,
                                                  const LigmaParasite *parasite,
                                                  GError            **error);
void                  ligma_parasite_attach       (Ligma               *ligma,
                                                  const LigmaParasite *parasite);
void                  ligma_parasite_detach       (Ligma               *ligma,
                                                  const gchar        *name);
const LigmaParasite  * ligma_parasite_find         (Ligma               *ligma,
                                                  const gchar        *name);
gchar              ** ligma_parasite_list         (Ligma               *ligma);

void                  ligma_parasite_shift_parent (LigmaParasite       *parasite);

void                  ligma_parasiterc_load       (Ligma               *ligma);
void                  ligma_parasiterc_save       (Ligma               *ligma);


#endif  /*  __LIGMA_PARASITES_H__  */
