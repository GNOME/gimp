#ifndef __GIMP_OBJECT_H__
#define __GIMP_OBJECT_H__

#include <gtk/gtktypeutils.h>
#include "gimpobjectF.h"

#define GIMP_TYPE_OBJECT gimp_object_get_type()

#define GIMP_OBJECT(obj) GTK_CHECK_CAST (obj, GIMP_TYPE_OBJECT, GimpObject)

#define GIMP_IS_OBJECT(obj) GTK_CHECK_TYPE (obj, GIMP_TYPE_OBJECT)


guint gimp_object_get_type(void);

/* hacks to fake a gimp object lib */
#define GIMP_CHECK_CAST GTK_CHECK_CAST
#define GIMP_CHECK_TYPE GTK_CHECK_TYPE
#define gimp_type_new gtk_type_new
#define gimp_object_destroy(obj) gtk_object_destroy(GTK_OBJECT(obj))
#define gimp_object_ref(obj) gtk_object_ref(GTK_OBJECT(obj))
#define gimp_object_unref(obj) gtk_object_unref(GTK_OBJECT(obj))

#endif




	
