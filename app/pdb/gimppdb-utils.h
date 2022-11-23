/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_PDB_UTILS_H__
#define __LIGMA_PDB_UTILS_H__


LigmaBrush     * ligma_pdb_get_brush              (Ligma               *ligma,
                                                 const gchar        *name,
                                                 LigmaPDBDataAccess   access,
                                                 GError            **error);
LigmaBrush     * ligma_pdb_get_generated_brush    (Ligma               *ligma,
                                                 const gchar        *name,
                                                 LigmaPDBDataAccess   access,
                                                 GError            **error);
LigmaDynamics  * ligma_pdb_get_dynamics           (Ligma               *ligma,
                                                 const gchar        *name,
                                                 LigmaPDBDataAccess   access,
                                                 GError            **error);
LigmaMybrush   * ligma_pdb_get_mybrush            (Ligma               *ligma,
                                                 const gchar        *name,
                                                 LigmaPDBDataAccess   access,
                                                 GError            **error);
LigmaPattern   * ligma_pdb_get_pattern            (Ligma               *ligma,
                                                 const gchar        *name,
                                                 GError            **error);
LigmaGradient  * ligma_pdb_get_gradient           (Ligma               *ligma,
                                                 const gchar        *name,
                                                 LigmaPDBDataAccess   access,
                                                 GError            **error);
LigmaPalette   * ligma_pdb_get_palette            (Ligma               *ligma,
                                                 const gchar        *name,
                                                 LigmaPDBDataAccess   access,
                                                 GError            **error);
LigmaFont      * ligma_pdb_get_font               (Ligma               *ligma,
                                                 const gchar        *name,
                                                 GError            **error);
LigmaBuffer    * ligma_pdb_get_buffer             (Ligma               *ligma,
                                                 const gchar        *name,
                                                 GError            **error);
LigmaPaintInfo * ligma_pdb_get_paint_info         (Ligma               *ligma,
                                                 const gchar        *name,
                                                 GError            **error);

gboolean        ligma_pdb_item_is_attached       (LigmaItem           *item,
                                                 LigmaImage          *image,
                                                 LigmaPDBItemModify   modify,
                                                 GError            **error);
gboolean        ligma_pdb_item_is_in_tree        (LigmaItem           *item,
                                                 LigmaImage          *image,
                                                 LigmaPDBItemModify   modify,
                                                 GError            **error);
gboolean        ligma_pdb_item_is_in_same_tree   (LigmaItem           *item,
                                                 LigmaItem           *item2,
                                                 LigmaImage          *image,
                                                 GError            **error);
gboolean        ligma_pdb_item_is_not_ancestor   (LigmaItem           *item,
                                                 LigmaItem           *not_descendant,
                                                 GError            **error);
gboolean        ligma_pdb_item_is_floating       (LigmaItem           *item,
                                                 LigmaImage          *dest_image,
                                                 GError            **error);
gboolean        ligma_pdb_item_is_modifiable     (LigmaItem           *item,
                                                 LigmaPDBItemModify   modify,
                                                 GError            **error);
gboolean        ligma_pdb_item_is_group          (LigmaItem           *item,
                                                 GError            **error);
gboolean        ligma_pdb_item_is_not_group      (LigmaItem           *item,
                                                 GError            **error);

gboolean        ligma_pdb_layer_is_text_layer    (LigmaLayer          *layer,
                                                 LigmaPDBItemModify   modify,
                                                 GError            **error);

gboolean        ligma_pdb_image_is_base_type     (LigmaImage          *image,
                                                 LigmaImageBaseType   type,
                                                 GError            **error);
gboolean        ligma_pdb_image_is_not_base_type (LigmaImage          *image,
                                                 LigmaImageBaseType   type,
                                                 GError            **error);

gboolean        ligma_pdb_image_is_precision     (LigmaImage          *image,
                                                 LigmaPrecision       precision,
                                                 GError            **error);
gboolean        ligma_pdb_image_is_not_precision (LigmaImage          *image,
                                                 LigmaPrecision       precision,
                                                 GError            **error);

LigmaGuide     * ligma_pdb_image_get_guide        (LigmaImage          *image,
                                                 gint                guide_id,
                                                 GError            **error);
LigmaSamplePoint *
                ligma_pdb_image_get_sample_point (LigmaImage          *image,
                                                 gint                sample_point_id,
                                                 GError            **error);

LigmaStroke    * ligma_pdb_get_vectors_stroke     (LigmaVectors        *vectors,
                                                 gint                stroke_id,
                                                 LigmaPDBItemModify   modify,
                                                 GError            **error);

gboolean        ligma_pdb_is_canonical_procedure (const gchar        *procedure_name,
                                                 GError            **error);


#endif /* __LIGMA_PDB_UTILS_H__ */
