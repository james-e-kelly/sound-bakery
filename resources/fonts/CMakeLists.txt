SET(FONT_DESTINATION_FOLDER ${CMAKE_BINARY_DIR}/fonts)

SET(RAW_FONTS
    Montserrat-Light.ttf
)

SET(FONT_AWESOME_FONTS
    fa-brands-400.ttf
    fa-regular-400.ttf
    fa-solid-900.ttf
)

SET(FONT_AUDIO_FONTS
    fontaudio.ttf
)

FOREACH(FONT ${RAW_FONTS})
    configure_file(${FONT} ${FONT_DESTINATION_FOLDER}/${FONT} COPYONLY)
ENDFOREACH()

FOREACH(FONT ${FONT_AWESOME_FONTS})
    configure_file(Font-Awesome/webfonts/${FONT} ${FONT_DESTINATION_FOLDER}/${FONT} COPYONLY)
ENDFOREACH()

FOREACH(FONT ${FONT_AUDIO_FONTS})
    configure_file(fontaudio/font/${FONT} ${FONT_DESTINATION_FOLDER}/${FONT} COPYONLY)
ENDFOREACH()