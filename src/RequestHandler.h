#pragma once

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

		RequestResult GetBaseInfo(const Request&);
		RequestResult Sleep(const Request&);
};