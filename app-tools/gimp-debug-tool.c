#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "app/widgets/gimpcriticaldialog.h"

void show_debug_dialog(const gchar *program, const gchar *pid, const gchar *reason, const gchar *message,
                       const gchar *bt_file, const gchar *last_version, const gchar *release_date) {
    const gchar *error;
    gchar *trace = NULL;
    GtkWidget *dialog;

    error = g_strdup_printf("%s: %s", reason, message);

    g_file_get_contents(bt_file, &trace, NULL, NULL);

    if (trace == NULL || strlen(trace) == 0) {
        g_free(error);
        return;
    }

    gtk_init(NULL, NULL);

    dialog = gimp_critical_dialog_new(_("GIMP Crash Debug"), last_version,
                                       release_date ? g_ascii_strtoll(release_date, NULL, 10) : -1);
    gimp_critical_dialog_add(dialog, error, trace, TRUE, program, g_ascii_strtoull(pid, NULL, 10));
    g_free(error);
    g_free(trace);

    g_signal_connect(dialog, "delete-event", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(dialog, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show(dialog);
    gtk_main();
}

int main(int argc, char **argv) {
    if (argc != 6 && argc != 8) {
        g_print("Usage: gimp-debug-tool-2.0 [PROGRAM] [PID] [REASON] [MESSAGE] [BT_FILE] "
                "([LAST_VERSION] [RELEASE_TIMESTAMP])\n");
        return EXIT_FAILURE;
    }

    const gchar *program = argv[1];
    const gchar *pid = argv[2];
    const gchar *reason = argv[3];
    const gchar *message = argv[4];
    const gchar *bt_file = argv[5];
    const gchar *last_version = NULL;
    const gchar *release_date = NULL;

    if (argc == 8) {
        last_version = argv[6];
        release_date = argv[7];
    }

    show_debug_dialog(program, pid, reason, message, bt_file, last_version, release_date);

    return EXIT_SUCCESS;
}
