/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help Browser
 * Copyright (C) 1999 Sven Neumann <sven@gimp.org>
 *                    Michael Natterer <mitschel@cs.tu-berlin.de>
 *
 * queue.h - a history queue
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

#ifndef _GIMP_HELP_QUEUE_H_
#define _GIMP_HELP_QUEUE_H_

typedef struct _Queue Queue;

Queue       * queue_new       (void);
void          queue_free      (Queue       *queue);
void          queue_add       (Queue       *queue,
                               const gchar *ref);
const gchar * queue_prev      (Queue       *queue);
const gchar * queue_next      (Queue       *queue);
void          queue_move_prev (Queue       *queue);
void          queue_move_next (Queue       *queue);
gboolean      queue_has_next  (Queue       *queue);
gboolean      queue_has_prev  (Queue       *queue);

#endif /* _GIMP_HELP_QUEUE_H_ */
