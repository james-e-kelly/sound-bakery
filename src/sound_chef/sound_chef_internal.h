#ifndef SC_INTERNAL_H
#define SC_INTERNAL_H

#include "sound_chef/sound_chef.h"

/**
 * @def Mallocs an object, sets its memory to 0 and checks for errors and potentially
 * returns.
 *
 * Convinience macro for allocating memory _and_ doing checks on it.
 */
#define SC_CREATE(ptr, t, system)                                       \
    ptr = ma_malloc(sizeof(t), &(system)->engine.allocationCallbacks);  \
    SC_CHECK_MEM(ptr);                                                  \
    memset(ptr, 0, sizeof(t))

#define SC_FREE(ptr, system)                                \
    assert(system != NULL);                                 \
    ma_free(ptr, &(system)->engine.allocationCallbacks);

#ifdef __cplusplus
extern "C"
{
#endif

ma_handle sc_dlopen(ma_log* pLog, const char* filename);
void sc_dlclose(ma_log* pLog, ma_handle handle);
ma_proc sc_dlsym(ma_log* pLog, ma_handle handle, const char* symbol);

SC_CLASS const char* SC_CALL sc_filename_get_ext(const char* filename);

sc_result SC_API sc_clap_load(const char* clapFilePath, sc_clap* clapPlugin);
sc_result SC_API sc_clap_unload(sc_clap* clapPlugin);

sc_result SC_API sc_system_release_clap_plugins(sc_system* system);

#ifdef __cplusplus
}
#endif

#endif  // SC_INTERNAL_H