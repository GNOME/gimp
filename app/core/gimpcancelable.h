/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmacancelable.h
 * Copyright (C) 2018 Ell
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

#ifndef __LIGMA_CANCELABLE_H__
#define __LIGMA_CANCELABLE_H__


#define LIGMA_TYPE_CANCELABLE (ligma_cancelable_get_type ())
G_DECLARE_INTERFACE (LigmaCancelable, ligma_cancelable, LIGMA, CANCELABLE, GObject)


struct _LigmaCancelableInterface
{
  GTypeInterface base_iface;

  /*  signals  */
  void   (* cancel) (LigmaCancelable *cancelable);
};


void    ligma_cancelable_cancel   (LigmaCancelable *cancelable);


#endif  /* __LIGMA_CANCELABLE_H__ */
