#include <stdio.h>

#define FILE_OSTREAM_INIT fo_init

static void fo_init(ExFileOstream* str){
	str->file = NULL;
}

static void fo_putchar(ExOstream* s, gchar c){
	ExFileOstream* str = EX_FILE_OSTREAM(s);
	fputc(c, str->file);
}

static void fo_close(ExOstream* s){
	ExFileOstream* str = EX_FILE_OSTREAM(s);
	fclose(str->file);
	str->file = NULL;
}

#define FILE_OSTREAM_CLASS_INIT fo_cinit

static void fo_cinit(ExFileOstreamClass* klass){
	((ExOstreamClass*)klass)->putchar = fo_putchar;
	((ExOstreamClass*)klass)->close = fo_close;
}


#define FILE_OSTREAM_OPEN fo_open

static ExFileOstream* fo_open(gchar* filename){
	ExFileOstream* str;
	FILE* f = fopen(filename, "w+");
	if(!f)
		return NULL;
	str = gtk_type_new(EX_TYPE_FILE_OSTREAM);
	str->file = f;
	return str;
}


