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

#ifndef __LIGMA_BRUSH_CORE_LOOPS_H__
#define __LIGMA_BRUSH_CORE_LOOPS_H__


const LigmaTempBuf * ligma_brush_core_subsample_mask  (LigmaBrushCore     *core,
                                                     const LigmaTempBuf *mask,
                                                     gdouble            x,
                                                     gdouble            y);
const LigmaTempBuf * ligma_brush_core_pressurize_mask (LigmaBrushCore     *core,
                                                     const LigmaTempBuf *brush_mask,
                                                     gdouble            x,
                                                     gdouble            y,
                                                     gdouble            pressure);
const LigmaTempBuf * ligma_brush_core_solidify_mask   (LigmaBrushCore     *core,
                                                     const LigmaTempBuf *brush_mask,
                                                     gdouble            x,
                                                     gdouble            y);


#endif  /*  __LIGMA_BRUSH_CORE_LOOPS_H__  */
