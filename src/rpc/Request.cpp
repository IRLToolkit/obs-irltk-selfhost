#include "Request.h"

Request::Request(const QString& requestType, const QString& messageId, QJsonObject requestData) :
	_requestType(requestType),
	_messageId(messageId)
{
	if (!requestData.empty())
		_requestData.swap(requestData);
}

const RequestStatus Request::ValidateBasic(const QString keyName, QString *comment) const
{
	if (!HasRequestData()) {
		if (comment)
			*comment = "Parameter: requestData";
		return RequestStatus::MissingRequestParameter;
	}

	if (!_requestData.contains(keyName)) {
		if (comment)
			*comment = QString("Parameter: %1").arg(keyName);
		return RequestStatus::MissingRequestParameter;
	}

	return RequestStatus::NoError;
}

const RequestStatus Request::ValidateDouble(const QString keyName, QString *comment, double minValue, double maxValue) const
{
	RequestStatus basicValidation = ValidateBasic(keyName, comment);
	if (basicValidation != RequestStatus::NoError)
		return basicValidation;

	if (!_requestData[keyName].isDouble()) {
		if (comment)
			*comment = QString("Parameter: %1\nRequired: double").arg(keyName);
		return RequestStatus::InvalidRequestParameterDataType;
	}

	double value = _requestData[keyName].toDouble();
	if (value < minValue) {
		if (comment)
			*comment = QString("Parameter: %1\nMinimum: %2").arg(keyName).arg(minValue);
		return RequestStatus::RequestParameterOutOfRange;
	} else if (value > maxValue) {
		if (comment)
			*comment = QString("Parameter: %1\nMaximum: %2").arg(keyName).arg(maxValue);
		return RequestStatus::RequestParameterOutOfRange;
	}

	return RequestStatus::NoError;
}

const RequestStatus Request::ValidateString(const QString keyName, QString *comment) const
{
	RequestStatus basicValidation = ValidateBasic(keyName, comment);
	if (basicValidation != RequestStatus::NoError)
		return basicValidation;

	if (!_requestData[keyName].isString()) {
		if (comment)
			*comment = QString("Parameter: %1\nRequired: string").arg(keyName);
		return RequestStatus::InvalidRequestParameterDataType;
	}

	return RequestStatus::NoError;
}

const RequestStatus Request::ValidateBool(const QString keyName, QString *comment) const
{
	RequestStatus basicValidation = ValidateBasic(keyName, comment);
	if (basicValidation != RequestStatus::NoError)
		return basicValidation;

	if (!_requestData[keyName].isBool()) {
		if (comment)
			*comment = QString("Parameter: %1\nRequired: bool").arg(keyName);
		return RequestStatus::InvalidRequestParameterDataType;
	}

	return RequestStatus::NoError;
}

const RequestStatus Request::ValidateObject(const QString keyName, QString *comment) const
{
	RequestStatus basicValidation = ValidateBasic(keyName, comment);
	if (basicValidation != RequestStatus::NoError)
		return basicValidation;

	if (!_requestData[keyName].isObject()) {
		if (comment)
			*comment = QString("Parameter: %1\nRequired: object").arg(keyName);
		return RequestStatus::InvalidRequestParameterDataType;
	}

	return RequestStatus::NoError;
}