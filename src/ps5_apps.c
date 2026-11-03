#include "ps5_apps.h"
#include "launcher.h"
#include "log.h"

#include <dlfcn.h>
#include <stddef.h>
#include <sys/stat.h>

extern const unsigned char icon_png_data[];
extern const unsigned int icon_png_data_len;

int sceAppInstUtilInitialize(void);
int sceAppInstUtilTerminate(void);
typedef int (*install_title_dir_fn)(const char *, const char *, void *);
int sceAppInstUtilAppUnInstall(const char *title_id);

int apps_install_launcher(void)
{
    struct stat st;
    int w, r;
    install_title_dir_fn install = (install_title_dir_fn)dlsym(RTLD_DEFAULT,
                                        "sceAppInstUtilAppInstallTitleDir");

    /* This optional export is absent on 13.60. A direct ELF import prevents
     * main() from running at all, even when the launcher is disabled. */
    if (!install) {
        log_line("launcher: install export unavailable; use http://<ps5-ip>:8095/");
        return -1;
    }

    w = launcher_write(LAUNCHER_APP_ROOT, icon_png_data, icon_png_data_len);
    if (w < 0) {
        log_line("launcher: could not write the files under %s", LAUNCHER_APP_ROOT);
        return -1;
    }
    if (w == 0 && stat(LAUNCHER_APP_ROOT "/" LAUNCHER_TITLE_ID "/sce_sys/icon0.png", &st) == 0) {
        log_line("launcher: already installed and up to date");
        return 0;
    }
    if ((r = sceAppInstUtilInitialize()) != 0) {
        log_line("launcher: sceAppInstUtilInitialize -> %#x", (unsigned)r);
        return -1;
    }
    r = install(LAUNCHER_TITLE_ID, LAUNCHER_APP_ROOT "/", 0);
    sceAppInstUtilTerminate();
    log_line("launcher: installed %s in the media row -> %#x", LAUNCHER_TITLE_ID, (unsigned)r);
    return r == 0 ? 0 : -1;
}

int apps_remove_launcher(void)
{
    int r = sceAppInstUtilInitialize();

    if (r == 0) {
        r = sceAppInstUtilAppUnInstall(LAUNCHER_TITLE_ID);
        sceAppInstUtilTerminate();
    }
    log_line("launcher: removed %s -> %#x", LAUNCHER_TITLE_ID, (unsigned)r);
    launcher_remove_files(LAUNCHER_APP_ROOT);
    return r == 0 ? 0 : -1;
}
