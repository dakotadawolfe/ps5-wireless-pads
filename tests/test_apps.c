/* Exercise the 13.60 missing-export path without the console libraries. */
#include "../src/ps5_apps.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

const unsigned char icon_png_data[] = { 0 };
const unsigned int icon_png_data_len = 1;
static int available, writes, installs, initializes;

void log_line(const char *fmt, ...) { (void)fmt; }
int launcher_write(const char *root, const unsigned char *icon, size_t size)
{ (void)root; (void)icon; (void)size; writes++; return 1; }
int launcher_remove_files(const char *root) { (void)root; return 0; }
int sceAppInstUtilInitialize(void) { initializes++; return 0; }
int sceAppInstUtilTerminate(void) { return 0; }
int sceAppInstUtilAppUnInstall(const char *title) { (void)title; return 0; }
static int install(const char *title, const char *root, void *reserved)
{ (void)title; (void)root; (void)reserved; installs++; return 0; }
void *dlsym(void *handle, const char *name)
{
    (void)handle;
    assert(strcmp(name, "sceAppInstUtilAppInstallTitleDir") == 0);
    return available ? (void *)install : NULL;
}

int main(void)
{
    assert(apps_install_launcher() == -1);
    assert(writes == 0 && installs == 0 && initializes == 0);
    available = 1;
    assert(apps_install_launcher() == 0);
    assert(writes == 1 && installs == 1 && initializes == 1);
    puts("optional launcher export checks passed");
    return 0;
}
