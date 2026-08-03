cmake_minimum_required(VERSION 3.20)

# ── Project ───────────────────────────────────────────────────────────────────
project(SwordFish
    VERSION      2.0.0
    DESCRIPTION  "Privacy-first power-user web browser"
    HOMEPAGE_URL "https://github.com/BayazidHabibSiddikee/SwordFish"
    LANGUAGES    CXX
)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Optimise release builds
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# ── Qt6 ───────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Widgets Network
    WebEngineCore WebEngineWidgets WebChannel
)
find_package(CURL REQUIRED)

# ── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/web_page.cpp
    src/styles.cpp
    src/adblocker.cpp
    src/folder_picker.cpp
    src/file_picker.cpp
    src/pip_window.cpp
    src/password_manager.cpp
    src/extension_system.cpp
    src/sync_manager.cpp
    src/media_bar.cpp
    src/reading_mode.cpp
    src/resources.qrc
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
    src/pip_window.h
    src/password_manager.h
    src/extension_system.h
    src/sync_manager.h
    src/media_bar.h
    src/reading_mode.h
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
    utils/network_fix.h
    utils/tts.h
    utils/proxy_tools.h
    utils/integrity.h
)

# ── Executable ────────────────────────────────────────────────────────────────
if(WIN32)
    # Windows: embed icon + version info via RC file, no console window
    set(WIN_RESOURCE "${CMAKE_SOURCE_DIR}/packaging/windows/swordfish.rc")
    if(EXISTS "${WIN_RESOURCE}")
        list(APPEND SOURCES "${WIN_RESOURCE}")
    endif()
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

# ── Compile definitions ───────────────────────────────────────────────────────
target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
    APP_NAME="${PROJECT_NAME}"
    QT_DEPRECATED_WARNINGS
)

# ── Link ──────────────────────────────────────────────────────────────────────
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

# ── Install rules ─────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT Runtime
)

# Web assets
install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/swordfish
    COMPONENT Runtime
    FILES_MATCHING
        PATTERN "*.html"
        PATTERN "*.css"
        PATTERN "*.json"
        PATTERN "*.png"
        PATTERN "*.vrm"
        PATTERN "__pycache__" EXCLUDE
)

# Desktop file
install(FILES packaging/linux/swordfish.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    COMPONENT Runtime
)

# Icon — multiple sizes
install(FILES icon.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
    RENAME swordfish.png
    COMPONENT Runtime
)
install(FILES icon.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
    RENAME swordfish.png
    COMPONENT Runtime
)

# Man page (if present)
if(EXISTS "${CMAKE_SOURCE_DIR}/packaging/linux/swordfish.1")
    install(FILES packaging/linux/swordfish.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
        COMPONENT Runtime
    )
endif()

# ── CPack — shared settings ───────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME              "swordfish")
set(CPACK_PACKAGE_VENDOR            "SwordFish Project")
set(CPACK_PACKAGE_CONTACT           "BayazidHabibSiddikee")
set(CPACK_PACKAGE_DESCRIPTION_SHORT "Privacy-first power-user web browser")
set(CPACK_PACKAGE_DESCRIPTION
    "SwordFish is a fast, privacy-first web browser built with Qt6 WebEngine.\n"
    "Features: AdGuard DNS-over-HTTPS, built-in ad blocker, dark mode,\n"
    "file tools (PDF/Office/Archive), YouTube transcript, and more.")
set(CPACK_PACKAGE_VERSION_MAJOR     ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR     ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH     ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION           ${PROJECT_VERSION})
set(CPACK_PACKAGE_ICON              "${CMAKE_SOURCE_DIR}/icon.png")
set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "SwordFish")
set(CPACK_STRIP_FILES               TRUE)
set(CPACK_PACKAGE_EXECUTABLES       "${PROJECT_NAME};SwordFish Browser")

# ── CPack — DEB (Debian / Ubuntu) ────────────────────────────────────────────
set(CPACK_DEBIAN_PACKAGE_SECTION       "web")
set(CPACK_DEBIAN_PACKAGE_PRIORITY      "optional")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE  "amd64")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER    "BayazidHabibSiddikee")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE      "https://github.com/BayazidHabibSiddikee/SwordFish")
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt6core6 (>= 6.4), libqt6gui6 (>= 6.4), libqt6widgets6 (>= 6.4),"
    " libqt6webenginecore6 (>= 6.4), libqt6webenginewidgets6 (>= 6.4),"
    " libqt6webchannel6 (>= 6.4), libcurl4 (>= 7.68), libc6 (>= 2.17)")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS
    "qpdf, libreoffice-core, poppler-utils, p7zip-full, qrencode, espeak-ng")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
    "Privacy-first power-user web browser\n"
    " SwordFish is a fast, privacy-first web browser built with Qt6 WebEngine.\n"
    " Features AdGuard DNS-over-HTTPS, ad blocker, dark mode, file conversion\n"
    " tools, YouTube transcript, and much more.")
set(CPACK_DEB_COMPONENT_INSTALL OFF)

# ── CPack — RPM (Fedora / openSUSE / Arch) ────────────────────────────────────
set(CPACK_RPM_PACKAGE_GROUP         "Applications/Internet")
set(CPACK_RPM_PACKAGE_LICENSE       "GPL-3.0")
set(CPACK_RPM_PACKAGE_URL           "https://github.com/BayazidHabibSiddikee/SwordFish")
set(CPACK_RPM_PACKAGE_REQUIRES
    "qt6-qtbase >= 6.4, qt6-qtwebengine >= 6.4, libcurl >= 7.68")
set(CPACK_RPM_PACKAGE_RECOMMENDS    "qpdf, libreoffice-core, poppler-utils, p7zip")
set(CPACK_RPM_PACKAGE_SUMMARY       "Privacy-first power-user web browser")
set(CPACK_RPM_PACKAGE_DESCRIPTION
    "SwordFish is a fast, privacy-first web browser built with Qt6 WebEngine.\n"
    "Features AdGuard DNS-over-HTTPS, ad blocker, dark mode, file conversion\n"
    "tools, YouTube transcript, and much more.")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    "/usr" "/usr/bin" "/usr/share" "/usr/share/applications"
    "/usr/share/icons" "/usr/share/icons/hicolor"
    "/usr/share/icons/hicolor/256x256"
    "/usr/share/icons/hicolor/256x256/apps"
    "/usr/share/pixmaps")

# ── CPack — NSIS (Windows installer) ─────────────────────────────────────────
set(CPACK_NSIS_PACKAGE_NAME         "SwordFish Browser")
set(CPACK_NSIS_DISPLAY_NAME         "SwordFish Browser ${PROJECT_VERSION}")
set(CPACK_NSIS_HELP_LINK            "https://github.com/BayazidHabibSiddikee/SwordFish")
set(CPACK_NSIS_URL_INFO_ABOUT       "https://github.com/BayazidHabibSiddikee/SwordFish")
set(CPACK_NSIS_CONTACT              "BayazidHabibSiddikee")
set(CPACK_NSIS_MODIFY_PATH          ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MUI_ICON             "${CMAKE_SOURCE_DIR}/packaging/windows/swordfish.ico")
set(CPACK_NSIS_MUI_UNIICON          "${CMAKE_SOURCE_DIR}/packaging/windows/swordfish.ico")
set(CPACK_NSIS_INSTALLED_ICON_NAME  "bin\\\\SwordFish.exe")
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\SwordFish.lnk' '$INSTDIR\\\\bin\\\\SwordFish.exe'")
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\SwordFish.lnk'")

# ── Windows: install Qt DLLs and runtime into bin/ for NSIS packaging ─────────
if(WIN32)
    # Qt DLLs and MSVC runtime from the build output directory
    install(DIRECTORY "${CMAKE_BINARY_DIR}/Release/"
        DESTINATION bin
        COMPONENT Runtime
        FILES_MATCHING
            PATTERN "*.dll"
            PATTERN "*.exe"
            PATTERN "*.dat"
            PATTERN "*.pak"
            PATTERN "*.bin"
    )
    # Qt plugin subdirectories (platforms, imageformats, etc.)
    foreach(_dir generic iconengines imageformats networkinformation platforms
                 position qmltooling styles tls resources translations)
        if(EXISTS "${CMAKE_BINARY_DIR}/Release/${_dir}")
            install(DIRECTORY "${CMAKE_BINARY_DIR}/Release/${_dir}"
                DESTINATION bin
                COMPONENT Runtime
            )
        endif()
    endforeach()
    # QML directory
    if(EXISTS "${CMAKE_BINARY_DIR}/Release/qml")
        install(DIRECTORY "${CMAKE_BINARY_DIR}/Release/qml"
            DESTINATION bin
            COMPONENT Runtime
        )
    endif()
    # vcpkg runtime DLLs (curl, zlib)
    if(EXISTS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
        install(DIRECTORY "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/"
            DESTINATION bin
            COMPONENT Runtime
            FILES_MATCHING PATTERN "*.dll"
        )
    endif()
endif()

include(CPack)
