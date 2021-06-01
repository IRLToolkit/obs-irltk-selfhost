#pragma once

#include <obs.hpp>
#include <obs-frontend-api.h>
#include <obs-data.h>
#include <util/config-file.h>

#include <QJsonObject>
#include <QJsonArray>
#include <QtCore/QString>
#include <QtCore/QHash>

#include "rpc/Request.h"
#include "plugin-main.h"

class RequestHandler;
typedef RequestResult(RequestHandler::*MethodHandler)(const Request&);

class RequestHandler {
	public:
		RequestHandler();
		RequestResult ProcessIncomingMessage(QJsonObject parsedMessage);
		static QJsonObject GetResultJson(const RequestResult requestResult);
	private:
		static const QHash<QString, MethodHandler> RequestHandlerMap;
		QString UtilsGetObsVersion();
		QJsonObject UtilsObsDataToQt(obs_data_t *data);
		obs_data_t *UtilsQtToObsData(QJsonObject data);
		QString UtilsGetOutputTimecode(obs_output_t *output);
		uint64_t UtilsGetOutputDuration(obs_output_t *output);
		QString UtilsGetSourceMediaState(obs_source_t *source);
		QJsonArray UtilsStringListToQt(char **list);

		// General
		RequestResult GetVersion(const Request&);
		RequestResult Sleep(const Request&);
		RequestResult CacheUpdate(const Request&);
		RequestResult LogDump(const Request&);

		// Config
		RequestResult GetProfileList(const Request&);
		RequestResult SetCurrentProfile(const Request&);
		RequestResult GetSceneCollectionList(const Request&);
		RequestResult SetCurrentSceneCollection(const Request&);
		RequestResult GetVideoSettings(const Request&);
#ifdef IRLTK_CLOUD // Pending newer OBS version
		RequestResult SetVideoSettings(const Request&);
#endif

		// Scenes
		RequestResult SetCurrentProgramScene(const Request&);

		// Stream
		RequestResult GetStreamStatus(const Request&);
		RequestResult StartStream(const Request&);
		RequestResult StopStream(const Request&);
		RequestResult GetStreamServiceSettings(const Request&);
		RequestResult SetStreamServiceSettings(const Request&);

		// Record
		RequestResult GetRecordStatus(const Request&);
		RequestResult StartRecord(const Request&);
		RequestResult StopRecord(const Request&);
};