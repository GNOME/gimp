#include <ex/file_ostream.i.h>

void putstring_handler(Ostream* o, gchar* msg, gpointer data){
	gchar* s = data;
	g_print("Putstring: %s (%p), \"%s\"..\n", s, o, msg);
}

void close_handler(Ostream* o, gpointer data){
	gchar* s = data;
	g_print("Closed: %s (%p)!\n", s, o);
}

int main(void){
	Ostream* x;
	gtk_type_init();
	x = OSTREAM(file_ostream_open("foo"));
	ostream_connect_putstring(putstring_handler, "foo", x);
	ostream_connect_close(close_handler, "foo", x);
	ostream_putstring(x, "Whammo!\n");
	ostream_close(x);
	return 0;
}
