#ifndef __GIMPSET_H__
#define __GIMPSET_H__

#include <glib.h>
#include "gimpsetF.h"


/* GimpSet - a typed set of objects with signals for adding and
   removing of stuff. If it is weak, destroyed objects get removed
   automatically. If it is not, it refs them so they won't be freed
   till they are removed. (Though they can be destroyed, of course) */

#define GIMP_TYPE_SET gimp_set_get_type()

#define GIMP_SET(obj) GTK_CHECK_CAST (obj, GIMP_TYPE_SET, GimpSet)


     
#define GIMP_IS_SET(obj) GTK_CHECK_TYPE (obj, gimp_set_get_type())
     
/* Signals:
   add
   remove
*/


guint gimp_set_get_type (void);

GimpSet*	gimp_set_new	(GtkType type, gboolean weak);
GtkType		gimp_set_type	(GimpSet* set);
gboolean       	gimp_set_add	(GimpSet* gimpset, gpointer ob);
gboolean       	gimp_set_remove	(GimpSet* gimpset, gpointer ob);
gboolean	gimp_set_have	(GimpSet* gimpset, gpointer ob);
void		gimp_set_foreach(GimpSet* gimpset, GFunc func,
				 gpointer user_data);
gint		gimp_set_size	(GimpSet* gimpset);

#endif
