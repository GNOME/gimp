#include <ex/file_ostream_i.h>

int main(void){
	Ostream* x = OSTREAM(file_ostream_open("foo"));
	ostream_putstring(x, "Whammo!\n");
	ostream_close(x);
	return 0;
}
