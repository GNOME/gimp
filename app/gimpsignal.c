#include "gimpsignal.h"

struct _GimpSignalType{
	GtkSignalMarshaller marshaller;
	GtkType return_type;
	guint nparams;
	const GtkType* param_types;
};

typedef const GtkType TypeArr[];

GimpSignalID gimp_signal_new(const gchar* name,
			     GtkSignalRunType signal_flags,
			     GtkType object_type,
			     guint function_offset,
			     GimpSignalType* sig_type){
	return gtk_signal_newv(name,
			       signal_flags,
			       object_type,
			       function_offset,
			       sig_type->marshaller,
			       sig_type->return_type,
			       sig_type->nparams,
			       /* Bah. We try to be const correct, but 
				  gtk isn't.. */
			       (GtkType*)sig_type->param_types);
}

static GimpSignalType sigtype_void={
	gtk_signal_default_marshaller,
	GTK_TYPE_NONE,
	0,
	NULL
};

GimpSignalType* const gimp_sigtype_void=&sigtype_void;

static void
gimp_marshaller_pointer (GtkObject*	object,
			 GtkSignalFunc	func,
			 gpointer		func_data,
			 GtkArg*		args)
{
	(*(GimpHandlerPointer)func) (object,
			      GTK_VALUE_POINTER (args[0]),
			      func_data);
}

static TypeArr pointer_types={
	GTK_TYPE_POINTER
};

static GimpSignalType sigtype_pointer={
	gimp_marshaller_pointer,
	GTK_TYPE_NONE,
	1,
	pointer_types
};

GimpSignalType* const gimp_sigtype_pointer=&sigtype_pointer;

static void
gimp_marshaller_int (GtkObject*	object,
			      GtkSignalFunc	func,
			      gpointer		func_data,
			      GtkArg*		args)
{
  (*(GimpHandlerInt)func) (object,
			  GTK_VALUE_INT (args[0]),
			  func_data);
}

static TypeArr int_types={
	GTK_TYPE_INT
};

static GimpSignalType sigtype_int={
	gimp_marshaller_int,
	GTK_TYPE_NONE,
	1,
	int_types
};

GimpSignalType* const gimp_sigtype_int=&sigtype_int;

static void
gimp_marshaller_int_int_int_int (GtkObject*	object,
				      GtkSignalFunc	func,
				      gpointer		func_data,
				      GtkArg*		args)
{
  (*(GimpHandlerIntIntIntInt)func) (object,
			  GTK_VALUE_INT (args[0]),
			  GTK_VALUE_INT (args[1]),
			  GTK_VALUE_INT (args[2]),
			  GTK_VALUE_INT (args[3]),
			  func_data);
}

static TypeArr int_int_int_int_types={
	GTK_TYPE_INT,
	GTK_TYPE_INT,
	GTK_TYPE_INT,
	GTK_TYPE_INT
};
	
static GimpSignalType sigtype_int_int_int_int={
	gimp_marshaller_int_int_int_int,
	GTK_TYPE_NONE,
	4,
	int_int_int_int_types
};

GimpSignalType* const gimp_sigtype_int_int_int_int=&sigtype_int_int_int_int;
