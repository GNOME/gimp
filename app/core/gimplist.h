#ifndef __GIMPLIST_H__
#define __GIMPLIST_H__

#include <glib.h>
#include "gimplistF.h"


/* GimpList - a typed list of objects with signals for adding and
   removing of stuff. If it is weak, destroyed objects get removed
   automatically. If it is not, it refs them so they won't be freed
   till they are removed. (Though they can be destroyed, of course) */

#define GIMP_TYPE_LIST gimp_list_get_type()

#define GIMP_LIST(obj) GTK_CHECK_CAST (obj, GIMP_TYPE_LIST, GimpList)


     
#define GIMP_IS_LIST(obj) GTK_CHECK_TYPE (obj, gimp_list_get_type())
     
/* Signals:
   add
   remove
*/


guint gimp_list_get_type (void);

GimpList*	gimp_list_new	 (GtkType type, gboolean weak);
GtkType		gimp_list_type	 (GimpList* list);
gboolean       	gimp_list_add	 (GimpList* gimplist, gpointer ob);
gboolean       	gimp_list_remove (GimpList* gimplist, gpointer ob);
gboolean	gimp_list_have	 (GimpList* gimplist, gpointer ob);
void		gimp_list_foreach(GimpList* gimplist, GFunc func,
				  gpointer user_data);
gint		gimp_list_size	 (GimpList* gimplist);

#endif
