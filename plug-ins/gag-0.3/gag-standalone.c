#include <gtk/gtk.h>
#include "gag.h"

char *gag_library_file= "./gag-library";
void (*gag_render_picture_ptr)(GtkWidget *, gpointer)= NULL;

int main (int argc, char *argv[])
{
    gtk_init (&argc, &argv);

    expr_init();

    gag_load_library(gag_library_file);

    gag_create_ui();

    gtk_main ();

    return 0;
}

