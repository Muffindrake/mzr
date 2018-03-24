#define _POSIX_C_SOURCE 200809L
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <pango/pango.h>
#include <vte/vte.h>

#include "mzr.h"

#include "config.h"

#define HCF(fmt, ...) \
        fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, \
                        __VA_ARGS__)
#define HCF0(fmt) \
        fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__)

static char *mzr_env[2] = {"TERM="MZR_TERM};
struct mzr_s mzr;

#define ENT(ln, sn, flag, ptr) { ln, sn, 0, flag, ptr, 0, 0 }
static GOptionEntry mzr_opt[] = {
        ENT("version", 'v', G_OPTION_ARG_NONE, &mzr.opt.version),
        ENT("font", 'f', G_OPTION_ARG_STRING, &mzr.opt.font),
        ENT("working-directory", 'd', G_OPTION_ARG_STRING, &mzr.opt.cwd),
        ENT("exec", 'x', G_OPTION_ARG_STRING, &mzr.opt.exec),
        ENT("", 0, G_OPTION_ARG_STRING_ARRAY, &mzr.opt.xexec),
        ENT("title", 't', G_OPTION_ARG_STRING, &mzr.opt.title),
        ENT("geometry", 'g', G_OPTION_ARG_STRING, &mzr.opt.geom),
        {0}
};
#undef ENT

static
void
mzr_beep(GtkWidget *w, void *data)
{
        (void) w;
        (void) data;

        gtk_window_set_urgency_hint(GTK_WINDOW(mzr.win), 0);
        if (MZR_VTE_BELL_URGENT)
                gtk_window_set_urgency_hint(GTK_WINDOW(mzr.win), 1);
}

static
void
mzr_eof(GtkWidget *w, void *data)
{
        (void) w;
        (void) data;

        g_spawn_close_pid(mzr.pid);
        gtk_main_quit();
}

static
void
mzr_title_change(GtkWidget *w, void *data)
{
        (void) w;
        (void) data;

        gtk_window_set_title(GTK_WINDOW(mzr.win),
                        vte_terminal_get_window_title(VTE_TERMINAL(mzr.vte)));
}

static
void
mzr_size_reset(long col, long row)
{
        GtkBorder pad;
        glong width;
        glong height;

        gtk_style_context_get_padding(gtk_widget_get_style_context(mzr.vte),
                        gtk_widget_get_state_flags(mzr.vte), &pad);
        width = pad.left + pad.right;
        height = pad.top + pad.bottom;
        if (mzr.vte_resize_request) {
                width += vte_terminal_get_char_width(VTE_TERMINAL(mzr.vte))
                                * (col ? col : vte_terminal_get_column_count(
                                VTE_TERMINAL(mzr.vte)));
                height += vte_terminal_get_char_height(VTE_TERMINAL(mzr.vte))
                                * (row ? row : vte_terminal_get_row_count(
                                VTE_TERMINAL(mzr.vte)));
                mzr.vte_resize_request = 0;
        }
        GdkWindow *gdk;
        if ((gdk = gtk_widget_get_window(GTK_WIDGET(mzr.win)))
                && ((gdk_window_get_state(gdk) & GDK_WINDOW_STATE_MAXIMIZED)))
                return;
        gtk_window_resize(GTK_WINDOW(mzr.win), width, height);
}

static
void
mzr_vte_callback(VteTerminal *t, GPid pid, GError *err, gpointer user_data)
{
        (void) t;

        GError **ptr = user_data;
        mzr.pid = pid;
        if (pid == -1)
                *ptr = err;
}

static
void
mzr_exec_args(int argc, char **argv)
{
        gchar *path;
        GError *err;

        if (!argc)
                return;
        path = g_find_program_in_path(argv[0]);
        if (!path) {
                HCF("command not found: %s", argv[0]);
                exit(1);
        }
        g_free(path);
        vte_terminal_spawn_async(VTE_TERMINAL(mzr.vte), VTE_PTY_DEFAULT, 0,
                        argv, mzr_env, G_SPAWN_SEARCH_PATH, 0, 0, 0, -1, 0,
                        mzr_vte_callback, &err);
        if (mzr.pid == -1) {
                HCF("error spawning vte terminal: %s", err->message);
                exit(1);
        }
}

static
char *
mzr_term_cwd(void)
{
        char *ret = 0;
        char *file;
        char *buf;
        struct stat sb;
        ssize_t len;

        if (mzr.pid < 0)
                return 0;
        file = g_strdup_printf("/proc/%d/cwd", mzr.pid);
        if (g_stat(file, &sb) == -1)
                goto file_free;
        buf = malloc(sb.st_size + 1);
        if (!buf)
                goto file_free;
        len = readlinkat(AT_FDCWD, file, buf, sb.st_size + 1);
        if (len > 0 && buf[0] == '/') {
                buf[len] = 0;
                ret = g_strdup(buf);
        }
        g_free(buf);
file_free:
        g_free(file);
        return ret;
}

static
void
mzr_show_callback(GtkWidget *w, void *data)
{
        (void) w;
        (void) data;

        mzr_size_reset(0, 0);
}

static
gboolean
mzr_resize_callback(GtkWidget *w, GdkEventConfigure *ev, void *data)
{
        (void) w;
        (void) ev;
        (void) data;

        mzr.vte_resize_request = 1;
        return 0;
}

static
void
mzr_init(int argc, char **argv)
{
        PangoFontDescription *font;
        GError *err = 0;
        gchar *path;
        gchar *path_join = 0;
        char *cwd;

        g_object_set(G_OBJECT(gtk_settings_get_default()),
                        "gtk-dialogs-use-header", 1, NULL);
        mzr.win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(mzr.win),
                        mzr.opt.title ? mzr.opt.title : "mzr");
        g_signal_connect(G_OBJECT(mzr.win), "configure-event",
                        G_CALLBACK(mzr_resize_callback), NULL);
        g_signal_connect(G_OBJECT(mzr.win), "show",
                        G_CALLBACK(mzr_show_callback), NULL);
        g_signal_connect(G_OBJECT(mzr.win), "delete-event",
                        G_CALLBACK(gtk_main_quit), NULL);
        mzr.vte = vte_terminal_new();
        if (mzr.opt.font)
                font = pango_font_description_from_string(mzr.opt.font);
        else
                font = pango_font_description_from_string(MZR_FONT);
#define OPT(fn, opt) fn(VTE_TERMINAL(mzr.vte), opt)
        OPT(vte_terminal_set_font, font);
        pango_font_description_free(font);
        OPT(vte_terminal_set_scrollback_lines, 0);
        OPT(vte_terminal_set_mouse_autohide, MZR_VTE_MOUSE_HIDE);
        OPT(vte_terminal_set_backspace_binding, MZR_VTE_BACKSPACE_CHAR);
        OPT(vte_terminal_set_word_char_exceptions, MZR_VTE_WORD_CHARS);
        OPT(vte_terminal_set_allow_bold, MZR_VTE_BOLD_ALLOW);
        OPT(vte_terminal_set_audible_bell, MZR_VTE_BELL_SOUND);
        OPT(vte_terminal_set_cursor_blink_mode, MZR_VTE_CURSOR_BLINK);
        OPT(vte_terminal_set_cursor_shape, MZR_VTE_CURSOR_SHAPE);
        OPT(vte_terminal_set_color_cursor_foreground, &mzr_color_cursor_fg);
        OPT(vte_terminal_set_color_cursor, &mzr_color_cursor_bg);
        OPT(vte_terminal_set_color_foreground, &mzr_color_fg);
        OPT(vte_terminal_set_color_background, &mzr_color_bg);
#undef OPT
        vte_terminal_set_colors(VTE_TERMINAL(mzr.vte), &mzr_color_fg,
                        &mzr_color_bg, mzr_palette, MZR_PALETTE_SZ);
        gtk_container_add(GTK_CONTAINER(mzr.win), mzr.vte);
        g_signal_connect(G_OBJECT(mzr.vte), "bell", G_CALLBACK(mzr_beep), 0);
        g_signal_connect(G_OBJECT(mzr.vte), "eof", G_CALLBACK(mzr_eof), 0);
        g_signal_connect(G_OBJECT(mzr.vte), "child-exited", G_CALLBACK(mzr_eof),
                        0);
        g_signal_connect(G_OBJECT(mzr.vte), "window-title-changed",
                        G_CALLBACK(mzr_title_change), 0);
        mzr_size_reset(mzr.col, mzr.row);
        gtk_widget_show_all(mzr.win);
        if (mzr.opt.exec)
                path = mzr.opt.exec;
        else if (mzr.opt.xexec)
                path = path_join = g_strjoinv(" ", mzr.opt.xexec);
        else
                path = 0;
        if (path) {
                if (!g_shell_parse_argv(path, &argc, &argv, &err))
                        switch (err->code) {
                        case G_SHELL_ERROR_EMPTY_STRING:
                                HCF0("-x: empty exec string");
                                exit(1);
                        case G_SHELL_ERROR_BAD_QUOTING:
                                HCF0("-x: unparsable: quotes mangled");
                                exit(1);
                        case G_SHELL_ERROR_FAILED:
                                HCF0("-x: failure parsing arguments");
                                exit(1);
                        }
                if (err)
                        g_error_free(err);
                if (path_join && path == path_join)
                        g_free(path_join);
                mzr_exec_args(argc, argv);
                g_strfreev(mzr.opt.xexec);
                mzr.opt.xexec = 0;
                return;
        }
        cwd = mzr_term_cwd();
        if (!cwd)
                cwd = g_get_current_dir();
        vte_terminal_spawn_async(VTE_TERMINAL(mzr.vte),
                        VTE_PTY_DEFAULT, cwd, mzr.argv, mzr_env,
                        G_SPAWN_SEARCH_PATH | G_SPAWN_FILE_AND_ARGV_ZERO,
                        0, 0, 0, -1, 0, mzr_vte_callback, &err);
        free(cwd);
        if (mzr.pid == -1) {
                HCF("error spawning vte terminal: %s", err->message);
                exit(1);
        }
}

int
main(int argc, char **argv)
{
        GOptionContext *opt_ctx;
        GError *err = 0;
        long col;
        long row;

        setlocale(LC_ALL, "");
        opt_ctx = g_option_context_new("- vte-based terminal emulator");
        g_option_context_add_main_entries(opt_ctx, mzr_opt, 0);;
        if (!g_option_context_parse(opt_ctx, &argc, &argv, &err)) {
                HCF("%s", err->message);
                return 1;
        }
        if (err)
                g_error_free(err);
        g_option_context_free(opt_ctx);
        if (mzr.opt.cwd && chdir(mzr.opt.cwd)) {
                HCF0("unable to change working directory.");
                return 1;
        }
        if (mzr.opt.version) {
                printf("mzr version: %s\n", VERSION);
                return 0;
        }
        if (mzr.opt.geom) {
                if (sscanf(mzr.opt.geom, "%ldx%ld", &col, &row) != 2) {
                        HCF("invalid geometry option: %s", mzr.opt.geom);
                        return 1;
                }
                mzr.col = col;
                mzr.row = row;
        } else {
                mzr.col = MZR_DEFAULT_COL;
                mzr.row = MZR_DEFAULT_ROW;
        }
        mzr.argv[0] = g_strdup(g_getenv("SHELL"));
        mzr.argv[1] = mzr.argv[0];
        mzr.vte_resize_request = 1;
        gtk_init(&argc, &argv);
        mzr_init(argc, argv);
        gtk_main();
        g_free(mzr.argv[0]);
        return 0;
}
