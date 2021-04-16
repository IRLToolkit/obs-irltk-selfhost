#include <inttypes.h>
#include "RequestHandler.h"

const QHash<QString, MethodHandler> RequestHandler::RequestHandlerMap
{
	// General
	{ "Sleep", &RequestHandler::Sleep },
	{ "GetBaseInfo", &RequestHandler::GetBaseInfo },
	{ "GetVideoSettings", &RequestHandler::GetVideoSettings },
#ifdef IRLTK_CLOUD
	{ "SetVideoSettings", &RequestHandler::SetVideoSettings },
#endif
	{ "CacheUpdate", &RequestHandler::CacheUpdate },
	{ "LogDump", &RequestHandler::LogDump },
	{ "GetProfileList", &RequestHandler::GetProfileList },
	{ "SetProfile", &RequestHandler::SetProfile },
	{ "GetSceneCollectionList", &RequestHandler::GetProfileList },
	{ "SetSceneCollection", &RequestHandler::SetProfile },

	// Output
	{ "GetRecordStatus", &RequestHandler::GetRecordStatus },
	{ "StartRecording", &RequestHandler::StartRecording },
	{ "StopRecording", &RequestHandler::StopRecording },
	{ "GetStreamStatus", &RequestHandler::GetStreamStatus },
	{ "StartStreaming", &RequestHandler::StartStreaming },
	{ "StopStreaming", &RequestHandler::StopStreaming },
	{ "GetStreamSettings", &RequestHandler::GetStreamSettings },
	{ "SetStreamSettings", &RequestHandler::SetStreamSettings },

	// Scenes
	{ "SetCurrentScene", &RequestHandler::SetCurrentScene },
};

RequestHandler::RequestHandler()
{
}

RequestResult RequestHandler::ProcessIncomingMessage(QJsonObject parsedMessage)
{
	RequestStatus errorCode = RequestStatus::NoError;
	QString requestType = parsedMessage["requestType"].toString();
    QString requestId;
	if (parsedMessage.contains("requestId"))
		requestId = parsedMessage["requestId"].toString();

	QJsonObject requestData;
	if (parsedMessage.contains("requestData")) {
		if (parsedMessage["requestData"].isObject())
			requestData = parsedMessage["requestData"].toObject();
		else
			errorCode = RequestStatus::InvalidRequestParameterDataType;
	}

	Request request(requestType, requestId, requestData);
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
	result["requestId"] = requestResult.requestId();
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

//============================================ UTILS BELOW ============================================

QString RequestHandler::UtilsGetObsVersion()
{
	uint32_t version = obs_get_version();

	uint8_t major, minor, patch;
	major = (version >> 24) & 0xFF;
	minor = (version >> 16) & 0xFF;
	patch = version & 0xFF;

	QString result = QString("%1.%2.%3")
		.arg(major).arg(minor).arg(patch);

	return result;
}

QJsonObject RequestHandler::UtilsObsDataToQt(obs_data_t *data) // Simple and dirty way to convert obs_data_t to QJsonObject
{
	QJsonObject returnJson;
	if (!data)
		return returnJson;

	const char* jsonText = obs_data_get_json(data);
	return QJsonDocument::fromJson(jsonText).object();
}

obs_data_t *RequestHandler::UtilsQtToObsData(QJsonObject data)
{
	QString resultText = QJsonDocument(data).toJson();
	return obs_data_create_from_json(QT_TO_UTF8(resultText));
}

QString RequestHandler::UtilsGetOutputTimecode(obs_output_t *output)
{
	if (!output || !obs_output_active(output))
		return "00:00:00.000";

	video_t* video = obs_output_video(output);
	uint64_t frameTimeNs = video_output_get_frame_time(video);
	int totalFrames = obs_output_get_total_frames(output);

	uint64_t ms = (((uint64_t)totalFrames) * frameTimeNs) / 1000000ULL;
	uint64_t secs = ms / 1000ULL;
	uint64_t minutes = secs / 60ULL;

	uint64_t hoursPart = minutes / 60ULL;
	uint64_t minutesPart = minutes % 60ULL;
	uint64_t secsPart = secs % 60ULL;
	uint64_t msPart = ms % 1000ULL;

	return QString::asprintf("%02" PRIu64 ":%02" PRIu64 ":%02" PRIu64 ".%03" PRIu64, hoursPart, minutesPart, secsPart, msPart);
}

QString RequestHandler::UtilsGetSourceMediaState(obs_source_t *source)
{
	enum obs_media_state mstate = obs_source_media_get_state(source);
	switch (mstate) {
		case OBS_MEDIA_STATE_NONE:
			return QString("none");
		case OBS_MEDIA_STATE_PLAYING:
			return QString("playing");
		case OBS_MEDIA_STATE_OPENING:
			return QString("opening");
		case OBS_MEDIA_STATE_BUFFERING:
			return QString("buffering");
		case OBS_MEDIA_STATE_PAUSED:
			return QString("paused");
		case OBS_MEDIA_STATE_STOPPED:
			return QString("stopped");
		case OBS_MEDIA_STATE_ENDED:
			return QString("ended");
		case OBS_MEDIA_STATE_ERROR:
			return QString("error");
		default:
			return QString("unknown");
	}
}

QJsonArray RequestHandler::UtilsStringListToQt(char **list)
{
	QJsonArray result;
	if (!list)
		return result;

	size_t index = 0;
	char* value = nullptr;
	do {
		value = list[index];
		if (value) {
			result.append(QString(value));
		}
		index++;
	} while (value != nullptr);

	return result;
}