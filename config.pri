# Try to optimise for code size a bit
QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections -Wl,--gc-sections -flto

CONFIG += \
    c++11 \
    link_pkgconfig
PKGCONFIG += Qt$${QT_MAJOR_VERSION}Contacts

packagesExist(mlite$${QT_MAJOR_VERSION}) {
    DEFINES += HAS_MLITE
    PKGCONFIG += mlite$${QT_MAJOR_VERSION}
}

DEFINES += CONTACTS_DATABASE_PATH=\"\\\"$$[QT_INSTALL_LIBS]/qtcontacts-sqlite-qt$${QT_MAJOR_VERSION}/\\\"\"

DISTFILES += \
    $$PWD/src/qtcontacts-sqlite-qt6-extensions.pc
