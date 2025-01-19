find_package(Git)
if(Git_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD OUTPUT_VARIABLE SOUND_BAKERY_COMMIT_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    set(SOUND_BAKERY_COMMIT_ID "Local")
endif()

message(STATUS "Git Commit is ${SOUND_BAKERY_COMMIT_ID}")