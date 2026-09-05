#include <QDebug>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QTimer>
#include <QUrl>
#include <QQuickView>

#include <iostream>


#include "ISingularityAudio.h"
#include "../SingularityController.h"

#include PLUGIN_CLASS_HEADER

#if defined(__linux__)
#include "PipeWire.h"
using PlatformAudio = PipeWire<PLUGIN_CLASS>;
#elif defined(__APPLE__)
#include "coreAudio.h"
using PlatformAudio = CoreAudio<PLUGIN_CLASS>;
#elif defined(_WIN32)
#include "ASIO.h"
#include "WASAPI.h"
using PlatformAudio = WASAPI<PLUGIN_CLASS>;
#endif


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    auto audio = std::make_unique<PlatformAudio>();
    setOnParameterChanged([&](int id, double value) {
        audio->pushParameterChange(id, value);
    });

    auto controller = std::make_unique<SingularityController>(getParameterContainer(), "", &audio->audioDataQueue());
    controller->setLogger([](const std::string& msg) {
        std::cout << msg << std::endl;
    });

    QQuickView view;
    controller->attachToView(view);
    view.show();
    view.requestActivate();

    return QGuiApplication::exec();
}
