#ifndef SC_INTERNAL_H
#define SC_INTERNAL_H

#include "sound_chef/sound_chef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    ma_handle sc_dlopen(ma_log* pLog, const char* filename);
    void sc_dlclose(ma_log* pLog, ma_handle handle);
    ma_proc sc_dlsym(ma_log* pLog, ma_handle handle, const char* symbol);

    SC_CLASS const char* SC_CALL sc_filename_get_ext(const char* filename);

    sbk_status SC_API sc_clap_load(const char* clapFilePath, sc_clap* clapPlugin);
    sbk_status SC_API sc_clap_unload(sc_clap* clapPlugin);

    sbk_status SC_API sc_system_release_clap_plugins(sc_system* system);

#ifdef __cplusplus
}
#endif

#endif  // SC_INTERNAL_H