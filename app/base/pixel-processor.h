/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * pixel_processor.h: Copyright (C) 1999 Jay Cox <jaycox@earthlink.net>
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


typedef struct _PixelProcessor PixelProcessor;


typedef void (* p_func)             (void);
typedef gint (* ProgressReportFunc) (gpointer ,
				     gint     ,
				     gint     ,
				     gint     ,
				     gint     );


void             pixel_regions_process_parallel (p_func             f,
						 gpointer           data,
						 gint               num_regions,
						 ...);
PixelProcessor * pixel_process_progress         (p_func             f,
						 gpointer           data,
						 ProgressReportFunc progress_func,
						 gpointer           progress_data,
						 gint               num_regions,
						 ...);

void             pixel_processor_free           (PixelProcessor    *pp);
void             pixel_processor_stop           (PixelProcessor    *pp);
PixelProcessor * pixel_processor_cont           (PixelProcessor    *pp);


#endif /* __PIXEL_PROCESSOR_H__ */
