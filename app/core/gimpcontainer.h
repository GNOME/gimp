/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_CONTAINER_H__
#define __GIMP_CONTAINER_H__


#include "gimpobject.h"


typedef enum
{
  GIMP_CONTAINER_POLICY_STRONG,
  GIMP_CONTAINER_POLICY_WEAK
} GimpContainerPolicy;


#define GIMP_TYPE_CONTAINER            (gimp_container_get_type ())
#define GIMP_CONTAINER(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_CONTAINER, GimpContainer))
#define GIMP_CONTAINER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER, GimpContainerClass))
#define GIMP_IS_CONTAINER(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_CONTAINER))
#define GIMP_IS_CONTAINER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER))


typedef struct _GimpContainerClass GimpContainerClass;

struct _GimpContainer
{
  GimpObject           parent_instance;

  /*  public, read-only  */
  GtkType              children_type;
  GimpContainerPolicy  policy;
  gint                 num_children;

  GList               *children;

  /*  private  */
  GList               *handlers;
};

struct _GimpContainerClass
{
  GimpObjectClass  parent_class;

  void (* add)    (GimpContainer *container,
		   GimpObject    *object);
  void (* remove) (GimpContainer *container,
		   GimpObject    *object);
};


GtkType         gimp_container_get_type       (void);
GimpContainer * gimp_container_new            (GtkType              children_type,
					       GimpContainerPolicy  policy);

GtkType         gimp_container_children_type  (const GimpContainer *container);
GimpContainerPolicy gimp_container_policy     (const GimpContainer *container);
gint            gimp_container_num_children   (const GimpContainer *container);

gboolean        gimp_container_add            (GimpContainer       *container,
					       GimpObject          *object);
gboolean        gimp_container_remove         (GimpContainer       *container,
					       GimpObject          *object);

const GList   * gimp_container_lookup         (const GimpContainer *container,
					       const GimpObject    *object);
void            gimp_container_foreach        (GimpContainer       *container,
					       GFunc                func,
					       gpointer             user_data);

GQuark          gimp_container_add_handler    (GimpContainer       *container,
					       const gchar         *signame,
					       GtkSignalFunc        handler,
					       gpointer             user_data);
void            gimp_container_remove_handler (GimpContainer       *container,
					       GQuark               id);


#endif  /* __GIMP_CONTAINER_H__ */
