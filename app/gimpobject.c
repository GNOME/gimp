#include <gtk/gtkobject.h>
#include "gimpobjectP.h"
#include "gimpobject.h"

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
	if(!type){
		GtkTypeInfo info={
			"GimpObject",
			sizeof(GimpObject),
			sizeof(GimpObjectClass),
			gimp_object_class_init,
			gimp_object_init,
			NULL,
			NULL};
		type=gtk_type_unique(gtk_object_get_type(), &info);
	}
	return type;
}

