#-----------------------------------------------
# environment management
#-----------------------------------------------

unix {
#  QMAKE_CXX = /usr/bin/colorgcc
  QMAKE_CXX = /usr/bin/clang++
  QMAKE_CXXFLAGS += -ferror-limit=1 
  QMAKE_CXXFLAGS += -Wno-invalid-source-encoding
}

# release only on win32
win32 {
  CONFIG += release
}

#-----------------------------------------------
# output file management
#-----------------------------------------------

# start at pwd
TARGET_PREFIX = $$PWD

# separate unix and windows builds
unix {
  TARGET_PREFIX = $$TARGET_PREFIX/unix
} 
win32 {
  TARGET_PREFIX = $$TARGET_PREFIX/win32
}

debug {
  DEFINES += DEBUG
}

CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

#debug_and_release:build_pass {
  # separate debug from release
  CONFIG(debug, debug|release) {
    TARGET_PREFIX = $$TARGET_PREFIX/debug
  } else {
    TARGET_PREFIX = $$TARGET_PREFIX/release
  }
#}

OUT_PREFIX = $$TARGET_PREFIX/$$TARGET

# define all significant output directories
DESTDIR     = $$OUT_PREFIX/bin
MOC_DIR     = $$OUT_PREFIX/moc
OBJECTS_DIR = $$OUT_PREFIX/obj
RCC_DIR     = $$OUT_PREFIX/rcc
UI_DIR      = $$OUT_PREFIX/ui

