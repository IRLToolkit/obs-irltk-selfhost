#include <obs.hpp>
#include <memory>
#include <QJsonDocument>
#include "plugin-macros.generated.h"

#define DEBUG_MODE

#define QT_TO_UTF8(str) str.toUtf8().constData()

void ___source_dummy_addref(obs_source_t*);
void ___sceneitem_dummy_addref(obs_sceneitem_t*);
void ___data_dummy_addref(obs_data_t*);
void ___data_array_dummy_addref(obs_data_array_t*);
void ___output_dummy_addref(obs_output_t*);

using OBSSourceAutoRelease =
	OBSRef<obs_source_t*, ___source_dummy_addref, obs_source_release>;
using OBSSceneItemAutoRelease =
	OBSRef<obs_sceneitem_t*, ___sceneitem_dummy_addref, obs_sceneitem_release>;
using OBSDataAutoRelease =
	OBSRef<obs_data_t*, ___data_dummy_addref, obs_data_release>;
using OBSDataArrayAutoRelease =
	OBSRef<obs_data_array_t*, ___data_array_dummy_addref, obs_data_array_release>;
using OBSOutputAutoRelease =
	OBSRef<obs_output_t*, ___output_dummy_addref, obs_output_release>;

void ___data_item_dummy_addref(obs_data_item_t*);
void ___data_item_release(obs_data_item_t*);
using OBSDataItemAutoRelease =
	OBSRef<obs_data_item_t*, ___data_item_dummy_addref, ___data_item_release>;

class Config;
typedef std::shared_ptr<Config> ConfigPtr;

class WebsocketManager;
typedef std::shared_ptr<WebsocketManager> WebsocketManagerPtr;

ConfigPtr GetConfig();

WebsocketManagerPtr GetWebsocketManager();