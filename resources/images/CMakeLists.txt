SET(IMAGE_DESTINATION_FOLDER ${CMAKE_BINARY_DIR}/images)

configure_file(SplashImage02.png ${IMAGE_DESTINATION_FOLDER}/SplashImage.png COPYONLY)

cmrc_add_resource_library(sound_bakery_images SplashImage02.png ALIAS SoundBakery::Images NAMESPACE sb::Images)