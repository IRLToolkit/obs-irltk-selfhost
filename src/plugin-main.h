#include <obs.hpp>
#include <memory>
#include <QJsonDocument>
#include "plugin-macros.generated.h"

#define DEBUG_MODE

#define QT_TO_UTF8(str) str.toUtf8().constData()

class Config;
typedef std::shared_ptr<Config> ConfigPtr;

class WebsocketManager;
typedef std::shared_ptr<WebsocketManager> WebsocketManagerPtr;

ConfigPtr GetConfig();

WebsocketManagerPtr GetWebsocketManager();