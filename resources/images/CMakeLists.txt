SET(IMAGE_DESTINATION_FOLDER ${CMAKE_BINARY_DIR}/images)

configure_file(SplashImage02.png ${IMAGE_DESTINATION_FOLDER}/SplashImage.png COPYONLY)

cmrc_add_resource_library(sound_bakery_icon_images ../../docs/img/sound-bakery-logo.png WHENCE ../../docs/img ALIAS sbk::icon_images NAMESPACE sbk::icon_images)
cmrc_add_resource_library(sound_bakery_images SplashImage02.png ALIAS SoundBakery::Images NAMESPACE sbk::Images)