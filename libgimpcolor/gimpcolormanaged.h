/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * LigmaColorManaged interface
 * Copyright (C) 2007  Sven Neumann <sven@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_COLOR_H_INSIDE__) && !defined (LIGMA_COLOR_COMPILATION)
#error "Only <libligmacolor/ligmacolor.h> can be included directly."
#endif

#ifndef __LIGMA_COLOR_MANAGED_H__
#define __LIGMA_COLOR_MANAGED_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_COLOR_MANAGED (ligma_color_managed_get_type ())
G_DECLARE_INTERFACE (LigmaColorManaged, ligma_color_managed, LIGMA, COLOR_MANAGED, GObject)

/**
 * LigmaColorManagedInterface:
 * @base_iface: The parent interface
 * @get_icc_profile: Returns the ICC profile of the pixels managed by
 *                   the object
 * @profile_changed: This signal is emitted when the object's color profile
 *                   has changed
 * @get_color_profile: Returns the #LigmaColorProfile of the pixels managed
 *                     by the object
 * @get_simulation_profile: Returns the simulation #LigmaColorProfile of the
 *                          pixels managed by the object
 * @get_simulation_rendering_intent: Returns the simulation #LigmaColorRenderingIntent
 *                                   of the pixels managed by the object
 * @get_simulation_bpc: Returns whether black point compensation is enabled for the
 *                      simulation of the pixels managed by the object
 **/
struct _LigmaColorManagedInterface
{
  GTypeInterface  base_iface;

  /**
   * LigmaColorManagedInterface::get_icc_profile:
   * @managed: an object the implements the #LigmaColorManaged interface
   * @len: (out): return location for the number of bytes in the profile data
   *
   * Returns: (array length=len): A blob of data that represents an ICC color
   *                              profile.
   *
   * Since: 2.4
   */
  const guint8     * (* get_icc_profile)                     (LigmaColorManaged *managed,
                                                              gsize            *len);

  /*  signals  */
  void               (* profile_changed)                     (LigmaColorManaged *managed);

  void               (* simulation_profile_changed)          (LigmaColorManaged *managed);

  void               (* simulation_intent_changed)           (LigmaColorManaged *managed);

  void               (* simulation_bpc_changed)              (LigmaColorManaged *managed);

  /*  virtual functions  */
  LigmaColorProfile * (* get_color_profile)               (LigmaColorManaged *managed);
  LigmaColorProfile * (* get_simulation_profile)          (LigmaColorManaged *managed);
  LigmaColorRenderingIntent
                     (* get_simulation_intent)           (LigmaColorManaged *managed);
  gboolean           (* get_simulation_bpc)              (LigmaColorManaged *managed);
};


const guint8     *       ligma_color_managed_get_icc_profile            (LigmaColorManaged *managed,
                                                                        gsize            *len);
LigmaColorProfile *       ligma_color_managed_get_color_profile          (LigmaColorManaged *managed);

LigmaColorProfile *       ligma_color_managed_get_simulation_profile     (LigmaColorManaged *managed);

LigmaColorRenderingIntent ligma_color_managed_get_simulation_intent      (LigmaColorManaged *managed);

gboolean                 ligma_color_managed_get_simulation_bpc         (LigmaColorManaged *managed);

void                     ligma_color_managed_profile_changed            (LigmaColorManaged *managed);

void                     ligma_color_managed_simulation_profile_changed (LigmaColorManaged *managed);

void                     ligma_color_managed_simulation_intent_changed  (LigmaColorManaged *managed);

void                     ligma_color_managed_simulation_bpc_changed     (LigmaColorManaged *managed);


G_END_DECLS

#endif  /* __LIGMA_COLOR_MANAGED_IFACE_H__ */
