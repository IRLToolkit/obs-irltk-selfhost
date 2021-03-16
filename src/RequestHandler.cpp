#include <QJsonObject>
#include <QtCore/QThread>
#include "RequestHandler.h"

const QHash<QString, MethodHandler> RequestHandler::RequestHandlerMap
{
	{ "GetBaseInfo", &RequestHandler::GetBaseInfo },
	{ "Sleep", &RequestHandler::Sleep }
};

RequestHandler::RequestHandler()
{
}

RequestResult RequestHandler::ProcessIncomingMessage(QJsonObject parsedMessage)
{
	RequestStatus errorCode = RequestStatus::NoError;
	QString requestType = parsedMessage["requestType"].toString();
    QString messageId;
	if (parsedMessage.contains("messageId"))
		messageId = parsedMessage["messageId"].toString();

	QJsonObject requestData;
	if (parsedMessage.contains("requestData")) {
		if (parsedMessage["requestData"].isObject())
			requestData = parsedMessage["requestData"].toObject();
		else
			errorCode = RequestStatus::InvalidRequestParameterDataType;
	}

	Request request(requestType, messageId, requestData);
	if (errorCode != RequestStatus::NoError)
		return RequestResult::BuildFailure(request, errorCode);

	MethodHandler handler = RequestHandlerMap[requestType];
	if (!handler)
		return RequestResult::BuildFailure(request, RequestStatus::InvalidRequestType);

	return std::bind(handler, this, std::placeholders::_1)(request);
}

QJsonObject RequestHandler::GetResultJson(const RequestResult requestResult)
{
	QJsonObject result;

	result["requestType"] = requestResult.RequestType();
	result["messageId"] = requestResult.MessageId();
	result["statusCode"] = requestResult.StatusCode();
	if (requestResult.StatusCode() == RequestStatus::Success) {
		result["status"] = true;
		QJsonObject additionalFields = requestResult.AdditionalFields();
		if (!additionalFields.empty())
			result["responseData"] = additionalFields;
	} else {
		result["status"] = false;
		QString comment = requestResult.Comment();
		if (!comment.isEmpty() && !comment.isNull())
			result["comment"] = comment;
	}

	return result;
}

RequestResult RequestHandler::GetBaseInfo(const Request& request)
{
	QJsonObject j;
	j["penis"] = "balls";
	return RequestResult::BuildSuccess(request, j);
}

RequestResult RequestHandler::Sleep(const Request& request)
{
	QString comment;
	RequestStatus millisCheck = request.ValidateDouble("sleepMillis", &comment, 0, 15000);
	if (millisCheck != RequestStatus::NoError)
		return RequestResult::BuildFailure(request, millisCheck, comment);

	long long sleepMillis = request.RequestData()["sleepMillis"].toDouble();
	QThread::msleep(sleepMillis);

	return RequestResult::BuildSuccess(request);
}