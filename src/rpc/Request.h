#pragma once

#include <QJsonObject>
#include "../plugin-main.h"

enum RequestStatus: std::uint16_t {
			Unknown = 0,

			// For internal use to signify a successful parameter check
			NoError = 10,

			Success = 100,

			// The request is denied because the client is not authenticated
			AuthenticationMissing = 200,
			// Connection has already been authenticated (for modules utilizing a request to provide authentication)
			AlreadyAuthenticated = 201,
			// Authentication request was denied (for modules utilizing a request to provide authentication)
			AuthenticationDenied = 202,
			// The `requestType` field is missing from the request data
			RequestTypeMissing = 203,
			// The request type is invalid (does not exist)
			InvalidRequestType = 204,
			// Generic error code (comment is expected to be provided)
			GenericError = 205,

			// A required request parameter is missing
			MissingRequestParameter = 300,

			// Generic invalid request parameter message
			InvalidRequestParameter = 400,
			// A request parameter has the wrong data type
			InvalidRequestParameterDataType = 401,
			// A request parameter (float or int) is out of valid range
			RequestParameterOutOfRange = 402,
			// A request parameter (string or array) is empty and cannot be
			RequestParameterEmpty = 403,

			// An output is running and cannot be in order to perform the request (generic)
			OutputRunning = 500,
			// An output is not running and should be
			OutputNotRunning = 501,
			// Stream is running and cannot be
			StreamRunning = 502,
			// Stream is not running and should be
			StreamNotRunning = 503,
			// Record is running and cannot be
			RecordRunning = 504,
			// Record is not running and should be
			RecordNotRunning = 505,
			// Record is paused and cannot be
			RecordPaused = 506,
			// Replay buffer is running and cannot be
			ReplayBufferRunning = 507,
			// Replay buffer is not running and should be
			ReplayBufferNotRunning = 508,
			// Replay buffer is disabled and cannot be
			ReplayBufferDisabled = 509,
			// Studio mode is active and cannot be
			StudioModeActive = 510,
			// Studio mode is not active and should be
			StudioModeNotActive = 511,

			// The specified source was of the invalid type (Eg. input instead of scene)
			InvalidSourceType = 600,
			// The specified source was not found (generic for input, filter, transition, scene)
			SourceNotFound = 601,
			// The specified source already exists. Applicable to inputs, filters, transitions, scenes
			SourceAlreadyExists = 602,
			// The specified input was not found
			InputNotFound = 603,
			// The specified input had the wrong kind
			InvalidInputKind = 604,
			// The specified filter was not found
			FilterNotFound = 605,
			// The specified transition was not found
			TransitionNotFound = 606,
			// The specified transition does not support setting its position (transition is of fixed type)
			TransitionDurationFixed = 607,
			// The specified scene was not found
			SceneNotFound = 608,
			// The specified scene item was not found
			SceneItemNotFound = 609,
			// The specified scene collection was not found
			SceneCollectionNotFound = 610,
			// The specified profile was not found
			ProfileNotFound = 611,
			// The specified output was not found
			OutputNotFound = 612,
			// The specified encoder was not found
			EncoderNotFound = 613,
			// The specified service was not found
			ServiceNotFound = 614, 

			// Processing the request failed unexpectedly
			RequestProcessingFailed = 700,
			// Starting the Output failed
			OutputStartFailed = 701,
			// Duplicating the scene item failed
			SceneItemDuplicationFailed = 702,
			// Rendering the screenshot failed
			ScreenshotRenderFailed = 703,
			// Encoding the screenshot failed
			ScreenshotEncodeFailed = 704,
			// Saving the screenshot failed
			ScreenshotSaveFailed = 705,
		};

class Request {
	public:
		explicit Request(const QString& requestType, const QString& messageId, QJsonObject requestData);

		const QString& RequestType() const
		{
			return _requestType;
		}

		const QString& MessageId() const
		{
			return _messageId;
		}

		const QJsonObject& RequestData() const
		{
			return _requestData;
		}

		const bool HasRequestData() const
		{
			return (!_requestData.empty());
		}

		const RequestStatus ValidateBasic(const QString keyName, QString *comment = nullptr) const;
		const RequestStatus ValidateDouble(const QString keyName, QString *comment = nullptr, double minValue = -INFINITY, double maxValue = INFINITY) const;
		const RequestStatus ValidateString(const QString keyName, QString *comment = nullptr) const;
		const RequestStatus ValidateBool(const QString keyName, QString *comment = nullptr) const;
		const RequestStatus ValidateObject(const QString keyName, QString *comment = nullptr) const;
		const RequestStatus ValidateArray(const QString keyName, QString *comment = nullptr) const;
	private:
		const QString _requestType;
		const QString _messageId;
		QJsonObject _requestData;
};

class RequestResult {
	public:
		static const RequestResult BuildSuccess(const Request& request, QJsonObject additionalFields = QJsonObject());
		static const RequestResult BuildFailure(const Request& request, RequestStatus statusCode, const QString& comment = nullptr);

		const RequestStatus StatusCode() const
		{
			return _status;
		}

		const QString& Comment() const
		{
			return _comment;
		}

		const QJsonObject& AdditionalFields() const
		{
			return _additionalFields;
		}

		const QString& RequestType() const
		{
			return _requestType;
		}

		const QString& MessageId() const
		{
			return _messageId;
		}

	private:
		explicit RequestResult(
			const QString& requestType,
			const QString& messageId,
			RequestStatus status,
			const QString& comment,
			QJsonObject additionalFields
		);

		const RequestStatus _status;
		QString _comment;
		QJsonObject _additionalFields;
		const QString _requestType;
		const QString _messageId;
};