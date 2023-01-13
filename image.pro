TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

Release:DESTDIR = bin/release
Release:OBJECTS_DIR = bin/release/.obj
Release:MOC_DIR = bin/release/.moc
Release:RCC_DIR = bin/release/.rcc
Release:UI_DIR = bin/release/.ui

Debug:DESTDIR = bin/debug
Debug:OBJECTS_DIR = bin/debug/.obj
Debug:MOC_DIR = bin/debug/.moc
Debug:RCC_DIR = bin/debug/.rcc
Debug:UI_DIR = bin/debug/.ui
DESTDIR = ./bin

HOME = $$system(echo $HOME)
INCLUDE_PATH_SFML = $${HOME}/userspace/shared/include
LIB_PATH_SFML = $${HOME}/userspace/shared/lib64
LIB_NAME_SFML_GRAPHICS = libsfml-graphics.so
LIB_NAME_SFML_WINDOW = libsfml-window.so
LIB_NAME_SFML_SYSTEM = libsfml-system.so

HEADERS += \
    image_publisher.h \
    image_subscriber.h \
    frameserver.h \
    frameclient.h \
    framebase.h \
    image_text_subscriber.h

SOURCES += \
    main.cpp \
    image_publisher.cpp \
    image_subscriber.cpp \
    frameserver.cpp \
    frameclient.cpp \
    framebase.cpp \
    image_text_subscriber.cpp

INCLUDEPATH += \
  $${INCLUDE_PATH_SFML}

LIBS += \
  $${LIB_PATH_SFML}/$${LIB_NAME_SFML_GRAPHICS} \
  $${LIB_PATH_SFML}/$${LIB_NAME_SFML_WINDOW} \
  $${LIB_PATH_SFML}/$${LIB_NAME_SFML_SYSTEM}
