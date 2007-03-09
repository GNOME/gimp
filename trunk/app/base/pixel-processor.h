/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * pixel_processor.h: Copyright (C) 1999 Jay Cox <jaycox@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __PIXEL_PROCESSOR_H__
#define __PIXEL_PROCESSOR_H__


#define GIMP_MAX_NUM_THREADS  16


typedef void (* PixelProcessorFunc)         (void);
typedef void (* PixelProcessorProgressFunc) (gpointer  progress_data,
                                             gdouble   fraction);


void  pixel_processor_init            (gint num_threads);
void  pixel_processor_set_num_threads (gint num_threads);
void  pixel_processor_exit            (void);

void  pixel_regions_process_parallel  (PixelProcessorFunc  func,
                                       gpointer            data,
                                       gint                num_regions,
                                       ...);

void  pixel_regions_process_parallel_progress
                                      (PixelProcessorFunc          func,
                                       gpointer                    data,
                                       PixelProcessorProgressFunc  progress_func,
                                       gpointer                    progress_data,
                                       gint                        num_regions,
                                       ...);


#endif /* __PIXEL_PROCESSOR_H__ */
