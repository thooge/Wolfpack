PROJECT = uoclient
TARGET = uoclient
TEMPLATE = app
OBJECTS_DIR = obj

QT += core gui network xml opengl
CONFIG += qt thread exceptions flat
CONFIG += debug

win32:CONFIG += windows
unix:CONFIG += x11 
unix:INCLUDEPATH += /usr/include/GL /usr/include/X11 /usr/include
win32:INCLUDEPATH += 3rdparty/openal/include C:/Python27/include/
# FIXME: 64bit windows
win32:LIBPATH+= 3rdparty/openal/libs/Win32/ C:/Python27/libs/

# No idea what it does
#DEFINES += QT_CLEAN_NAMESPACE QT_COMPAT_WARNINGS

# Take care of dual cores
win32:QMAKE_CXXFLAGS_RELEASE += -MP

RC_FILE = uoclient.rc
CONFIG(release, debug|release):OBJECTS_DIR = obj/release
CONFIG(debug, debug|release):OBJECTS_DIR = obj/debug
MOC_DIR = obj

win32:DEFINES -= UNICODE
#win32:LIBS += advapi32.lib shell32.lib openal32.lib
win32:LIBS += -ladvapi32 -lshell32 -lopenal32

unix:LIBS += -lGL -lGLU -lpython2.7 -lopenal

DEPENDPATH += src
win32:INCLUDEPATH += include;libs/include
unix:INCLUDEPATH += include /usr/include/python2.7

# MAIN INCLUDES
HEADERS += \
	include/config.h \
	include/enums.h \
	include/exceptions.h \
	include/log.h \
	include/mainwindow.h \
	include/mersennetwister.h \
	include/sound.h \
	include/surface.h \
	include/texture.h \
	include/client.h \
	include/vector.h \
	include/md5.h \
	include/profile.h \
	include/macros.h \
	include/skills.h \
	include/scripts.h
	
HEADERS += \
	include/dialogs/login.h \
	include/dialogs/movecenter.h \
	include/dialogs/cachestatistics.h
	
HEADERS += \
	include/game/dynamicentity.h \
	include/game/dynamicitem.h \
	include/game/entity.h \
	include/game/groundtile.h \
	include/game/mobile.h \
	include/game/multi.h \
	include/game/statictile.h \
	include/game/world.h \
	include/game/tooltips.h \
	include/game/targetrequest.h

# GUI INCLUDES
HEADERS += \
	include/gui/asciilabel.h \
	include/gui/checkertrans.h \
	include/gui/genericgump.h \
	include/gui/bordergump.h \
	include/gui/container.h \
	include/gui/contextmenu.h \
	include/gui/control.h \
	include/gui/cursor.h \
	include/gui/gui.h \
	include/gui/gumpimage.h \
	include/gui/imagebutton.h \
	include/gui/itemimage.h \
	include/gui/label.h \
	include/gui/scrollbar.h \
	include/gui/textfield.h \
	include/gui/tiledgumpimage.h \
	include/gui/window.h \
	include/gui/tooltip.h \
	include/gui/worldview.h \
	include/gui/containergump.h \
	include/gui/textbutton.h \
	include/gui/stripeimage.h \
	include/gui/combobox.h \
	include/gui/radiobutton.h \
	include/gui/statuswindow.h \
	include/gui/checkbox.h \
	include/gui/colorpicker.h \
	include/gui/paperdoll.h

# MUL INCLUDES
HEADERS += \
	include/muls/animations.h \
	include/muls/multis.h \
	include/muls/art.h \
	include/muls/asciifonts.h \
	include/muls/gumpart.h \
	include/muls/hues.h \
	include/muls/maps.h \
	include/muls/textures.h \
	include/muls/tiledata.h \
	include/muls/unicodefonts.h \
	include/muls/verdata.h \
	include/muls/localization.h \
	include/muls/speech.h \
	include/muls/sounds.h

HEADERS += \
	include/network/encryption.h \
	include/network/uopacket.h \
	include/network/uosocket.h \
	include/network/outgoingpacket.h \
	include/network/outgoingpackets.h \
	include/network/twofish2.h \
	include/network/network.h

# Python includes
HEADERS += \
	include/python/utilities.h \
	include/python/clientmodule.h \
	include/python/genericwrapper.h \
	include/python/loginterface.h \
	include/python/universalslot.h \
	include/python/argparser.h

win32:HEADERS += \
	include/windows/gmtoolwnd.h

# MAIN src

SOURCES += \
	src/config.cpp \
	src/log.cpp \
	src/mainwindow.cpp \
	src/sound.cpp \
	src/surface.cpp \
	src/texture.cpp \
	src/client.cpp \
	src/utilities.cpp \
	src/vector.cpp \
	src/main.cpp \
	src/md5.cpp \
	src/profile.cpp \
	src/macros.cpp \
	src/skills.cpp \
	src/scripts.cpp

SOURCES += \
	src/dialogs/login.cpp \
	src/dialogs/movecenter.cpp \
	src/dialogs/cachestatistics.cpp

SOURCES += \
	src/gui/checkertrans.cpp \
	src/gui/genericgump.cpp \
	src/gui/asciilabel.cpp \
	src/gui/statuswindow.cpp \
	src/gui/bordergump.cpp \
	src/gui/container.cpp \
	src/gui/contextmenu.cpp \
	src/gui/control.cpp \
	src/gui/cursor.cpp \
	src/gui/gui.cpp \
	src/gui/gumpimage.cpp \
	src/gui/imagebutton.cpp \
	src/gui/itemimage.cpp \
	src/gui/label.cpp \
	src/gui/scrollbar.cpp \
	src/gui/textfield.cpp \
	src/gui/tiledgumpimage.cpp \
	src/gui/tooltip.cpp \
	src/gui/window.cpp \
	src/gui/worldview.cpp \
	src/gui/containergump.cpp \
	src/gui/textbutton.cpp \
	src/gui/stripeimage.cpp \
	src/gui/combobox.cpp \
	src/gui/radiobutton.cpp \
	src/gui/checkbox.cpp \
	src/gui/colorpicker.cpp \
	src/gui/paperdoll.cpp

SOURCES += \
	src/network/encryption.cpp \
	src/network/loginpackets.cpp \
	src/network/uopacket.cpp \
	src/network/uosocket.cpp \
	src/network/decompress.cpp \
	src/network/outgoingpacket.cpp \
	src/network/outgoingpackets.cpp \
	src/network/gamepackets.cpp \
	src/network/twofish2.cpp \
	src/network/network.cpp

SOURCES += \
	src/game/dynamicentity.cpp \
	src/game/dynamicitem.cpp \
	src/game/entity.cpp \
	src/game/groundtile.cpp \
	src/game/mobile.cpp \
	src/game/multi.cpp \
	src/game/statictile.cpp \
	src/game/world.cpp \
	src/game/tooltips.cpp \
	src/game/targetrequest.cpp
	
# Python includes
SOURCES += \
	src/python/clientmodule.cpp \
	src/python/genericwrapper.cpp \
	src/python/loginterface.cpp \
	src/python/universalslot.cpp \
	src/python/argparser.cpp

# MUL srcS
SOURCES += \
	src/muls/animations.cpp \
	src/muls/multis.cpp \
	src/muls/art.cpp \
	src/muls/asciifonts.cpp \
	src/muls/gumpart.cpp \
	src/muls/hues.cpp \
	src/muls/localization.cpp \
	src/muls/maps.cpp \
	src/muls/textures.cpp \
	src/muls/tiledata.cpp \
	src/muls/unicodefonts.cpp \
	src/muls/verdata.cpp \
	src/muls/speech.cpp \
	src/muls/sounds.cpp

win32:SOURCES += \
	src/windows/gmtoolwnd.cpp

FORMS =

#TRANSLATIONS = \
#	languages/wolfpack_pt_br.ts \
#	languages/wolfpack_it.ts \
#	languages/wolfpack_nl.ts \
#	languages/wolfpack_es.ts \
#	languages/wolfpack_de.ts \
#	languages/wolfpack_fr.ts \
#	languages/wolfpack_ge.ts


win32:SOURCES += \
	src/config_win.cpp \
	src/utilities_win.cpp
	
unix:SOURCES += \
	src/config_unix.cpp \
	src/utilities_unix.cpp

#
#DISTFILES += \
#	../release/AUTHORS.txt \
#	../release/LICENSE.GPL

