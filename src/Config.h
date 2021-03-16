#pragma once

#include <obs-frontend-api.h>
#include <util/config-file.h>
#include <QtCore/QString>
#include <QtCore/QSharedPointer>

#include "plugin-main.h"

class Config {
	public:
		Config();
		void Load();
		void Save();
		void SetDefaults();
		config_t* GetConfigStore();

		bool ConnectOnLoad;
		QString SessionKey;
		QString ConnectUrl;

	private:
		;
};
