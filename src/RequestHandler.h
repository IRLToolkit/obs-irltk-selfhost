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
		QString UtilsGetSourceMediaState(obs_source_t *source);
		QJsonArray UtilsStringListToQt(char **list);

		RequestResult Sleep(const Request&);
		RequestResult GetBaseInfo(const Request&);
		RequestResult GetVideoSettings(const Request&);
#ifdef IRLTK_CLOUD // Pending newer OBS version
		RequestResult SetVideoSettings(const Request&);
#endif
		RequestResult CacheUpdate(const Request&);
		RequestResult LogDump(const Request&);
		RequestResult GetProfileList(const Request&);
		RequestResult SetProfile(const Request&);
		RequestResult GetSceneCollectionList(const Request&);
		RequestResult SetSceneCollection(const Request&);
		RequestResult GetRecordStatus(const Request&);
		RequestResult StartRecording(const Request&);
		RequestResult StopRecording(const Request&);
		RequestResult GetStreamStatus(const Request&);
		RequestResult StartStreaming(const Request&);
		RequestResult StopStreaming(const Request&);
		RequestResult GetStreamSettings(const Request&);
		RequestResult SetStreamSettings(const Request&);
};