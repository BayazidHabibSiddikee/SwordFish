cmake_minimum_required(VERSION 3.20)
project(SwordFish VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Widgets Network
    WebEngineCore WebEngineWidgets WebChannel
)

find_package(CURL REQUIRED)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/web_page.cpp
    src/styles.cpp
    src/adblocker.cpp
    src/folder_picker.cpp
    src/file_picker.cpp
    # tools
    src/tools/pdf_tools.cpp
    src/tools/doc_tools.cpp
    src/tools/office_tools.cpp
    src/tools/archive_tools.cpp
    src/tools/student_tools.cpp
    src/tools/translate.cpp
    src/tools/knowledge_hub.cpp
    src/tools/timer.cpp
    src/tools/bangla.cpp
    src/tools/youtube_transcript.cpp
    # utils
    utils/network_fix.cpp
    utils/tts.cpp
    utils/proxy_tools.cpp
    utils/integrity.cpp
)

set(HEADERS
    src/mainwindow.h
    src/styles.h
    src/adblocker.h
    src/web_page.h
    src/folder_picker.h
    src/file_picker.h
    # tools
    src/tools/pdf_tools.h
    src/tools/doc_tools.h
    src/tools/office_tools.h
    src/tools/archive_tools.h
    src/tools/student_tools.h
    src/tools/translate.h
    src/tools/knowledge_hub.h
    src/tools/timer.h
    src/tools/bangla.h
    src/tools/youtube_transcript.h
    # utils
    utils/network_fix.h
    utils/tts.h
    utils/proxy_tools.h
    utils/integrity.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::WebEngineCore
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    CURL::libcurl
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/tools
    ${CMAKE_SOURCE_DIR}/utils
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY src/ DESTINATION share/swordfish
    FILES_MATCHING
    PATTERN "*.html"
    PATTERN "*.css"
    PATTERN "*.json"
    PATTERN "*.png"
    PATTERN "*.vrm"
)
