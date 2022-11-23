/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainerentry.h
 * Copyright (C) 2004  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_CONTAINER_ENTRY_H__
#define __LIGMA_CONTAINER_ENTRY_H__


#define LIGMA_TYPE_CONTAINER_ENTRY            (ligma_container_entry_get_type ())
#define LIGMA_CONTAINER_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTAINER_ENTRY, LigmaContainerEntry))
#define LIGMA_CONTAINER_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTAINER_ENTRY, LigmaContainerEntryClass))
#define LIGMA_IS_CONTAINER_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTAINER_ENTRY))
#define LIGMA_IS_CONTAINER_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTAINER_ENTRY))
#define LIGMA_CONTAINER_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTAINER_ENTRY, LigmaContainerEntryClass))


typedef struct _LigmaContainerEntryClass  LigmaContainerEntryClass;

struct _LigmaContainerEntry
{
  GtkEntry        parent_instance;

  LigmaViewable   *viewable;
};

struct _LigmaContainerEntryClass
{
  GtkEntryClass   parent_class;
};


GType       ligma_container_entry_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_container_entry_new      (LigmaContainer *container,
                                           LigmaContext   *context,
                                           gint           view_size,
                                           gint           view_border_width);


#endif  /*  __LIGMA_CONTAINER_ENTRY_H__  */
