#include "output.h"

void output_def(Def* d){
	TypeName* t=&d->type->name;
	/* GTK_TYPE_FOO macro */
	pr(type_hdr, "\n\n");
	pr(source, "\n");
	pr(prot_hdr, "\n\n");
	pr(source_head, "\n");
	
	pr(type_hdr,
	   "#define %3 \\\n"
	   " (%2 ? %2 : %2())\n",
	   pr_macro_name, t, "TYPE", NULL,
	   pr_internal_varname, t, "type",
	   pr_internal_varname, t, "type",
	   pr_internal_varname, t, "init_type");
	output_var(d, type_gtk_type, "type", type_hdr, TRUE);
}
