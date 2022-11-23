/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_OBJECT_H__
#define __LIGMA_OBJECT_H__


#define LIGMA_TYPE_OBJECT            (ligma_object_get_type ())
#define LIGMA_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OBJECT, LigmaObject))
#define LIGMA_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_OBJECT, LigmaObjectClass))
#define LIGMA_IS_OBJECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OBJECT))
#define LIGMA_IS_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_OBJECT))
#define LIGMA_OBJECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_OBJECT, LigmaObjectClass))


typedef struct _LigmaObjectPrivate  LigmaObjectPrivate;
typedef struct _LigmaObjectClass    LigmaObjectClass;

struct _LigmaObject
{
  GObject            parent_instance;

  LigmaObjectPrivate *p;
};

struct _LigmaObjectClass
{
  GObjectClass  parent_class;

  /*  signals  */
  void    (* disconnect)   (LigmaObject *object);
  void    (* name_changed) (LigmaObject *object);

  /*  virtual functions  */
  gint64  (* get_memsize)  (LigmaObject *object,
                            gint64     *gui_size);
};


GType         ligma_object_get_type        (void) G_GNUC_CONST;

void          ligma_object_set_name        (LigmaObject       *object,
                                           const gchar      *name);
void          ligma_object_set_name_safe   (LigmaObject       *object,
                                           const gchar      *name);
void          ligma_object_set_static_name (LigmaObject       *object,
                                           const gchar      *name);
void          ligma_object_take_name       (LigmaObject       *object,
                                           gchar            *name);
const gchar * ligma_object_get_name        (gconstpointer     object);
void          ligma_object_name_changed    (LigmaObject       *object);
void          ligma_object_name_free       (LigmaObject       *object);

gint          ligma_object_name_collate    (LigmaObject       *object1,
                                           LigmaObject       *object2);
gint64        ligma_object_get_memsize     (LigmaObject       *object,
                                           gint64           *gui_size);


#endif  /* __LIGMA_OBJECT_H__ */
