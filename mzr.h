#ifndef MZR_H
#define MZR_H

#include <gtk/gtk.h>

extern struct mzr_s {
        GtkWidget *win;
        GtkWidget *vte;
        char *argv[3];

        GPid pid;
        glong col;
        glong row;
        struct {
                gchar *font;
                gchar *title;
                gchar *geom;
                gchar *cwd;
                gchar *exec;
                gchar **xexec;
                gboolean version;
        } opt;

        bool vte_resize_request;
} mzr;

#endif
