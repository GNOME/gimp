#include "gimpobjectP.h"

static void
gimp_object_init (GimpObject *gobject)
{
}

static void
gimp_object_class_init (GimpObjectClass *gobjectclass)
{
}




   									     
GtkType gimp_object_get_type (void)
{
	static GtkType type;
	GIMP_TYPE_INIT(type,
		       GimpObject,
		       GimpObjectClass,
		       gimp_object_init,
		       gimp_object_class_init,
		       GTK_TYPE_OBJECT);
	return type;
}

