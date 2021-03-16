#include <obs-frontend-api.h>

#include <QtCore/QCryptographicHash>
#include <QtCore/QTime>
#include <QtWidgets/QSystemTrayIcon>

#define SECTION_NAME "IRLTKSelfHost"
#define PARAM_CONNECTONLOAD "ConnectOnLoad"
#define PARAM_SESSIONKEY "SessionKey"
#define PARAM_CONNECTURL "ConnectUrl"

#include "plugin-main.h"
#include "Config.h"

Config::Config() :
	ConnectOnLoad(true),
	SessionKey(""),
	ConnectUrl("")
{
	qsrand(QTime::currentTime().msec());

	SetDefaults();
}

void Config::Load()
{
#ifdef DEBUG_MODE
	blog(LOG_INFO, "Loading settings...");
#endif
	config_t* obsConfig = GetConfigStore();

	ConnectOnLoad = config_get_bool(obsConfig, SECTION_NAME, PARAM_CONNECTONLOAD);
	SessionKey = config_get_string(obsConfig, SECTION_NAME, PARAM_SESSIONKEY);
	ConnectUrl = config_get_string(obsConfig, SECTION_NAME, PARAM_CONNECTURL);
#ifdef DEBUG_MODE
    blog(LOG_INFO, "Connect on load: %d", ConnectOnLoad);
	blog(LOG_INFO, "Session Key: %s", SessionKey.toStdString().c_str());
	blog(LOG_INFO, "Websocket connect URL: %s", ConnectUrl.toStdString().c_str());
	blog(LOG_INFO, "Finished loading settings!");
#endif
}

void Config::Save()
{
	config_t* obsConfig = GetConfigStore();

	config_set_bool(obsConfig, SECTION_NAME, PARAM_CONNECTONLOAD,
		ConnectOnLoad);
	config_set_string(obsConfig, SECTION_NAME, PARAM_SESSIONKEY,
		QT_TO_UTF8(SessionKey));
	config_set_string(obsConfig, SECTION_NAME, PARAM_CONNECTURL,
		QT_TO_UTF8(ConnectUrl));

	config_save(obsConfig);

#ifdef DEBUG_MODE
	blog(LOG_INFO, "Settings saved!");
#endif
}

void Config::SetDefaults()
{
	// OBS Config defaults
	config_t* obsConfig = GetConfigStore();
	if (obsConfig) {
		config_set_default_bool(obsConfig, SECTION_NAME,
			PARAM_CONNECTONLOAD, ConnectOnLoad);
		config_set_default_string(obsConfig,
			SECTION_NAME, PARAM_SESSIONKEY, QT_TO_UTF8(SessionKey));
		config_set_default_string(obsConfig,
			SECTION_NAME, PARAM_CONNECTURL, QT_TO_UTF8(ConnectUrl));
	}
}

config_t* Config::GetConfigStore()
{
	return obs_frontend_get_global_config();
}