/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 * $URL$
 */

#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

#include "modules.h"
#include "thread.h"

static void *module;
static int dlsym_null;
static MOD_CCD_T mod_ccd;
static PESO_T *p_mod_peso;

static void *mod_dlsym(void *p_handle, const char *p_symbol)
{
    void *p_result;

    if ((p_result = dlsym(p_handle, p_symbol)) == NULL)
    {
        dlsym_null = 1;
    }

    return p_result;
}

int mod_ccd_init(char *p_mod_ccd_path, MOD_CCD_T *p_mod_ccd, PESO_T **p_peso)
{
    dlsym_null = 0;

    if ((module = dlopen(p_mod_ccd_path, RTLD_NOW)) == NULL)
    {
        return -1;
    }

    /* Clear any existing error */
    dlerror();

    *p_peso = mod_dlsym(module, "peso");
    p_mod_peso = *p_peso;

    mod_ccd.init = mod_dlsym(module, "ccd_init");
    mod_ccd.uninit = mod_dlsym(module, "ccd_uninit");
    mod_ccd.expose_init = mod_dlsym(module, "ccd_expose_init");
    mod_ccd.expose_start = mod_dlsym(module, "ccd_expose_start");
    mod_ccd.expose = mod_dlsym(module, "ccd_expose");
    mod_ccd.readout = mod_dlsym(module, "ccd_readout");
    mod_ccd.save_raw_image = mod_dlsym(module, "ccd_save_raw_image");
    mod_ccd.save_fits_file = mod_dlsym(module, "ccd_save_fits_file");
    mod_ccd.expose_end = mod_dlsym(module, "ccd_expose_end");
    mod_ccd.expose_uninit = mod_dlsym(module, "ccd_expose_uninit");
    mod_ccd.get_temp = mod_dlsym(module, "ccd_get_temp");
    mod_ccd.set_temp = mod_dlsym(module, "ccd_set_temp");
    mod_ccd.set_readout_speed = mod_dlsym(module, "ccd_set_readout_speed");
    mod_ccd.get_readout_speed = mod_dlsym(module, "ccd_get_readout_speed");
    mod_ccd.get_readout_speeds = mod_dlsym(module, "ccd_get_readout_speeds");
    mod_ccd.set_gain = mod_dlsym(module, "ccd_set_gain");
    mod_ccd.get_gain = mod_dlsym(module, "ccd_get_gain");
    mod_ccd.get_gains = mod_dlsym(module, "ccd_get_gains");

    mod_ccd.peso_set_int = mod_dlsym(module, "peso_set_int");
    mod_ccd.peso_get_int = mod_dlsym(module, "peso_get_int");
    mod_ccd.peso_set_double = mod_dlsym(module, "peso_set_double");
    mod_ccd.peso_set_float = mod_dlsym(module, "peso_set_float");
    mod_ccd.peso_get_float = mod_dlsym(module, "peso_get_float");
    mod_ccd.peso_set_str = mod_dlsym(module, "peso_set_str");
    mod_ccd.peso_set_time = mod_dlsym(module, "peso_set_time");
    mod_ccd.peso_set_imgtype = mod_dlsym(module, "peso_set_imgtype");
    mod_ccd.peso_set_state = mod_dlsym(module, "peso_set_state");
    mod_ccd.peso_get_version = mod_dlsym(module, "peso_get_version");

    if (dlsym_null)
    {
        return -1;
    }

    *p_mod_ccd = mod_ccd;

    return 0;
}

int mod_ccd_uninit(void)
{
    return dlclose(module);
}
