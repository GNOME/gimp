#include <stdio.h>

static void ex_file_ostream_init_real(ExFileOstream* str){
	str->file = NULL;
}

static void put_char(ExOstream* s, gchar c){
	ExFileOstream* str = EX_FILE_OSTREAM(s);
	fputc(c, str->file);
}

static void close(ExOstream* s){
	ExFileOstream* str = EX_FILE_OSTREAM(s);
	fclose(str->file);
	str->file = NULL;
}

static void ex_file_ostream_class_init_real(ExFileOstreamClass* klass){
	((ExOstreamClass*)klass)->putchar = put_char;
	((ExOstreamClass*)klass)->close = close;
}

static ExFileOstream* file_ostream_open_real(gchar* filename){
	ExFileOstream* str;
	FILE* f = fopen(filename, "w+");
	if(!f)
		return NULL;
	str = gtk_type_new(EX_TYPE_FILE_OSTREAM);
	str->file = f;
	return str;
}
