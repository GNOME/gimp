/* Aye, _mondo_ slow default method.. */

static void putstring(ExOstream* str, gchar* string){
	gint i;
	for(i = 0; string[i]; i++)
		ex_ostream_putchar(str, string[i]);
}
	
	
static void ex_ostream_init_real(ExOstream* str){
}

static void ex_ostream_class_init_real(ExOstreamClass* klass){
	klass->putstring = putstring;
}

