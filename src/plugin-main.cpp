#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs-data.h>

#include <QtCore/QTimer>
#include <QtWidgets/QAction>
#include <QtWidgets/QMainWindow>
#include <QSslSocket>

#include "Config.h"
#include "WebsocketManager.h"
#include "RequestHandler.h"
#include "forms/settings-dialog.h"

#include "plugin-main.h"

void ___source_dummy_addref(obs_source_t*) {}
void ___sceneitem_dummy_addref(obs_sceneitem_t*) {}
void ___data_dummy_addref(obs_data_t*) {}
void ___data_array_dummy_addref(obs_data_array_t*) {}
void ___output_dummy_addref(obs_output_t*) {}

void ___data_item_dummy_addref(obs_data_item_t*) {}
void ___data_item_release(obs_data_item_t* dataItem) {
	obs_data_item_release(&dataItem);
}

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

ConfigPtr _config;

WebsocketManagerPtr _websocketManager;

bool obs_module_load(void)
{
	_config = ConfigPtr(new Config());
	_config->Load();

	_websocketManager = WebsocketManagerPtr(new WebsocketManager());
	_websocketManager->SessionKey = _config->SessionKey;

	obs_frontend_push_ui_translation(obs_module_get_string);
	QMainWindow* mainWindow = (QMainWindow*)obs_frontend_get_main_window();
	SettingsDialog* settingsDialog = new SettingsDialog(mainWindow);
	obs_frontend_pop_ui_translation();
	
	const char* menuActionText = obs_module_text("IRLTKSelfHost.Panel.DialogTitle");
	QAction* menuAction = (QAction*)obs_frontend_add_tools_menu_qaction(menuActionText);
	QObject::connect(menuAction, &QAction::triggered, [settingsDialog] {
		// The settings dialog belongs to the main window. Should be ok
		// to pass the pointer to this QAction belonging to the main window
		settingsDialog->ToggleShowHide();
	});

#ifdef DEBUG_MODE
	if (QSslSocket::supportsSsl()) {
		blog(LOG_INFO, "SSL is supported.");
		blog(LOG_INFO, "QT SSL version: '%s'", QSslSocket::sslLibraryBuildVersionString().toStdString().c_str());
	} else {
		blog(LOG_ERROR, "SSL is not supported! Plugin load halted.");
		return false;
	}
#endif
	blog(LOG_INFO, "Plugin loaded successfully (version %s)", PLUGIN_VERSION);

	if (_config->ConnectOnLoad) {
#ifdef DEBUG_MODE
		blog(LOG_INFO, "Connect on load enabled. Connecting to websocket server now.");
#endif
		QMetaObject::invokeMethod(_websocketManager.get(), "Connect", Q_ARG(QString, _config->ConnectUrl));
	}

	return true;
}

void obs_module_unload()
{
	_websocketManager->GetThreadPool()->waitForDone();
	QMetaObject::invokeMethod(_websocketManager.get(), "Disconnect");
	_websocketManager.reset();
	_config.reset();
	blog(LOG_INFO, "Finished unloading.");
}

ConfigPtr GetConfig() {
	return _config;
}

WebsocketManagerPtr GetWebsocketManager() {
	return _websocketManager;
}