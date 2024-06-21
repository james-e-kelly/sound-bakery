SET(FONT_DESTINATION_FOLDER ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})

SET(RAW_FONTS
    Montserrat-Light.ttf
)

SET(FONT_AWESOME_FONTS
    Font-Awesome/webfonts/fa-brands-400.ttf
    Font-Awesome/webfonts/fa-regular-400.ttf
    Font-Awesome/webfonts/fa-solid-900.ttf
)

SET(FONT_AUDIO_FONTS
    fontaudio/font/fontaudio.ttf
)

cmrc_add_resource_library(sound_bakery_fonts ${RAW_FONTS} ${FONT_AWESOME_FONTS} ${FONT_AUDIO_FONTS} ALIAS SoundBakery::Fonts NAMESPACE sbk::Fonts)