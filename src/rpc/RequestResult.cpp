#include "Request.h"

RequestResult::RequestResult(
	const QString& requestType,
	const QString& requestId,
	RequestStatus status,
	const QString& comment,
	QJsonObject additionalFields
) :
	_requestType(requestType),
	_requestId(requestId),
	_status(status),
	_comment(comment)
{
	if (!additionalFields.empty())
		_additionalFields.swap(additionalFields);
}

const RequestResult RequestResult::BuildSuccess(const Request& request, QJsonObject additionalFields)
{
	RequestResult result(request.RequestType(), request.requestId(), RequestStatus::Success, nullptr, additionalFields);
	return result;
}

const RequestResult RequestResult::BuildFailure(const Request& request, RequestStatus statusCode, const QString& comment)
{
	RequestResult result(request.RequestType(), request.requestId(), statusCode, comment, QJsonObject());
	return result;
}