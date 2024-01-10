#pragma once

#define SB_CHECK_SYS()                                   \
{                                                           \
    ATLAS_SYSTEM* atlasSystem = Atlas_System_GetInstance(); \
    if (!atlasSystem)                                       \
    {                                                       \
        return FMOD_ERR_STUDIO_UNINITIALIZED;               \
    }                                                       \
    FMOD_SYSTEM* fmodSystem = nullptr;                      \
    Atlas_System_GetCoreSystem(&fmodSystem);                \
    if (!fmodSystem)                                        \
    {                                                       \
        return FMOD_ERR_UNINITIALIZED;                      \
    }                                                       \
}

#define SB_CHECK_PTR(ptr)                                   \
if (!ptr)                                                   \
{                                                           \
    return FMOD_ERR_INVALID_PARAM;                          \
}     

#define SB_CHECK_GOBJ(gameObject)                                                               \
{                                                                                               \
    if (!gameObject)                                                                            \
    {                                                                                           \
        ATLAS_SYSTEM* system = Atlas_System_GetInstance();                                      \
        std::optional<ATLAS_GAMEOBJECT*> defaultGameObject = system->GetListenerGameObject();   \
        gameObject = defaultGameObject ? defaultGameObject.value() : nullptr;                   \
        if (!gameObject)                                                                        \
        {                                                                                       \
            return FMOD_ERR_INTERNAL;                                                           \
        }                                                                                       \
    }                                                                                           \
}

#define SB_CHECK(x, err)            \
{                                   \
    if (!x)                         \
    {                               \
        return err;                 \
    }                               \
}