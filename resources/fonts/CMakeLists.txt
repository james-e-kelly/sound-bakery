SET(RAW_FONTS
    Montserrat-Light.ttf
    Montserrat-Regular.ttf
    Montserrat-Black.ttf
    Lucide.ttf
)

SET(FONT_AWESOME_FONTS
    Font-Awesome/webfonts/fa-brands-400.ttf
    Font-Awesome/webfonts/fa-regular-400.ttf
    Font-Awesome/webfonts/fa-solid-900.ttf
)

SET(FONT_AUDIO_FONTS
    fontaudio/font/fontaudio.ttf
)

cmrc_add_resource_library(sound_bakery_fonts ${RAW_FONTS} ${FONT_AWESOME_FONTS} ${FONT_AUDIO_FONTS} ${LUCIDE_FILE} ALIAS SoundBakery::Fonts NAMESPACE sbk::fonts)
cxx_17(sound_bakery_fonts)