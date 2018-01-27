# Dockerfile for a development environment for rust_qt_binding_generator
FROM ubuntu:17.10

USER root

RUN apt-get update && apt-get dist-upgrade -y

# build dependencies and many extra qml modules
RUN apt-get install -y --no-install-recommends \
  qtcreator rustc cargo ninja-build cmake g++ extra-cmake-modules \
  kdesdk-scripts \
  gdb valgrind \
  qtdeclarative5-dev-tools qtdeclarative5-dev qtquickcontrols2-5-dev \
  libqt5charts5-dev libqt5svg5-dev qml-module-qtcharts \
  qt5-doc qtbase5-doc qtcharts5-doc qtcreator-doc qtquickcontrols2-5-doc \
  qtquickcontrols5-doc qtsvg5-doc qttools5-doc

RUN apt-get install -y --no-install-recommends \
  qtquickcontrols2-5-examples kirigami2-dev  # qml-module-qt-labs-platform

RUN apt-get install -y --no-install-recommends qt5-default

# these are extra
# RUN apt-get install -y --no-install-recommends qbs-doc qml-module-org-kde-activities qml-module-org-kde-analitza qml-module-org-kde-bluezqt qml-module-org-kde-draganddrop qml-module-org-kde-extensionplugin qml-module-org-kde-games-core qml-module-org-kde-kaccounts qml-module-org-kde-kconfig qml-module-org-kde-kcoreaddons qml-module-org-kde-kholidays qml-module-org-kde-kio qml-module-org-kde-kirigami qml-module-org-kde-kirigami2 qml-module-org-kde-kquickcontrols qml-module-org-kde-kquickcontrolsaddons qml-module-org-kde-kwindowsystem qml-module-org-kde-newstuff qml-module-org-kde-people qml-module-org-kde-purpose qml-module-org-kde-runnermodel qml-module-org-kde-solid qml-module-org-kde-telepathy qml-module-qt-labs-calendar qml-module-qt-labs-folderlistmodel qml-module-qt-labs-platform qml-module-qt-labs-settings qml-module-qt-labs-sharedimage qml-module-qt-websockets qml-module-qt3d qml-module-qtaudioengine qml-module-qtbluetooth  qml-module-qtfeedback qml-module-qtgraphicaleffects qml-module-qtgstreamer qml-module-qtlocation qml-module-qtmultimedia qml-module-qtnfc qml-module-qtpositioning qml-module-qtpurchasing qml-module-qtqml-models2 qml-module-qtqml-statemachine qml-module-qtquick-controls qml-module-qtquick-controls-styles-breeze qml-module-qtquick-controls2 qml-module-qtquick-dialogs qml-module-qtquick-extras qml-module-qtquick-layouts qml-module-qtquick-localstorage qml-module-qtquick-particles2 qml-module-qtquick-privatewidgets qml-module-qtquick-scene2d qml-module-qtquick-scene3d qml-module-qtquick-sharedimage qml-module-qtquick-templates2 qml-module-qtquick-virtualkeyboard qml-module-qtquick-window2 qml-module-qtquick-xmllistmodel qml-module-qtquick2 qml-module-qtsensors qml-module-qtsysteminfo qml-module-qttest qml-module-qtwayland-compositor qml-module-qtwebchannel qml-module-qtwebengine qml-module-qtwebkit qml-module-qtwebsockets qml-module-qtwebview qt-at-spi-doc qt3d5-doc qtconnectivity5-doc qtgraphicaleffects5-doc qtgstreamer-doc qtiplot-doc qtlocation5-doc qtmultimedia5-doc qtnetworkauth5-doc qtpim5-doc qtpositioning5-doc qtscript5-doc qtsensors5-doc qtserialport5-doc qtspeech5-doc qtwayland5-doc qtwebchannel-doc qtwebchannel5-doc qtwebengine5-doc qtwebkit5-doc qtwebkit5-examples-doc qtwebsockets5-doc qtwebview5-doc qtx11extras5-doc qtxmlpatterns5-doc

RUN useradd neon

USER neon

WORKDIR /home/neon

CMD ["/bin/bash", "-l"]
