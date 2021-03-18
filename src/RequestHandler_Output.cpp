#include "RequestHandler.h"

RequestResult RequestHandler::GetRecordStatus(const Request& request)
{
	QJsonObject resultJson;

	resultJson["isRecording"] = obs_frontend_recording_active();
	resultJson["isRecordingPaused"] = obs_frontend_recording_paused();

	OBSOutputAutoRelease recordingOutput = obs_frontend_get_recording_output();
	resultJson["recordTimecode"] = UtilsGetOutputTimecode(recordingOutput);

	return RequestResult::BuildSuccess(request, resultJson);
}

RequestResult RequestHandler::StartRecording(const Request& request)
{
	if (obs_frontend_recording_active())
		return RequestResult::BuildFailure(request, RequestStatus::RecordRunning);

	obs_frontend_recording_start();
	return RequestResult::BuildSuccess(request);
}

RequestResult RequestHandler::StopRecording(const Request& request)
{
	if (!obs_frontend_recording_active())
		return RequestResult::BuildFailure(request, RequestStatus::RecordNotRunning);

	obs_frontend_recording_stop();
	return RequestResult::BuildSuccess(request);
}

RequestResult RequestHandler::GetStreamStatus(const Request& request)
{
	QJsonObject resultJson;

	resultJson["isStreaming"] = obs_frontend_streaming_active();

	OBSOutputAutoRelease streamingOutput = obs_frontend_get_streaming_output();
	resultJson["streamTimecode"] = UtilsGetOutputTimecode(streamingOutput);

	return RequestResult::BuildSuccess(request, resultJson);
}

RequestResult RequestHandler::StartStreaming(const Request& request)
{
	if (obs_frontend_streaming_active())
		return RequestResult::BuildFailure(request, RequestStatus::StreamRunning);

	obs_frontend_streaming_start();
	return RequestResult::BuildSuccess(request);
}

RequestResult RequestHandler::StopStreaming(const Request& request)
{
	if (!obs_frontend_streaming_active())
		return RequestResult::BuildFailure(request, RequestStatus::StreamNotRunning);

	obs_frontend_streaming_stop();
	return RequestResult::BuildSuccess(request);
}

RequestResult RequestHandler::GetStreamSettings(const Request& request)
{
	QJsonObject resultJson;

	OBSService service = obs_frontend_get_streaming_service();
	resultJson["serviceType"] = obs_service_get_type(service);
	OBSDataAutoRelease settings = obs_service_get_settings(service);
	resultJson["settings"] = UtilsObsDataToQt(settings);

	return RequestResult::BuildSuccess(request, resultJson);
}

RequestResult RequestHandler::SetStreamSettings(const Request& request)
{
	if (obs_frontend_streaming_active())
		return RequestResult::BuildFailure(request, RequestStatus::StreamRunning);

	QString comment;
	RequestStatus checkStatus = RequestStatus::NoError;

	checkStatus = request.ValidateString("serviceType", &comment);
	if (checkStatus != RequestStatus::NoError)
		return RequestResult::BuildFailure(request, checkStatus, comment);

	checkStatus = request.ValidateObject("settings", &comment);
	if (checkStatus != RequestStatus::NoError)
		return RequestResult::BuildFailure(request, checkStatus, comment);

	OBSService service = obs_frontend_get_streaming_service();

	QString serviceType = obs_service_get_type(service);
	QString requestedServiceType = request.RequestData()["serviceType"].toString();

	OBSDataAutoRelease requestedSettings = UtilsQtToObsData(request.RequestData()["settings"].toObject());

	if (serviceType == requestedServiceType) {
		OBSDataAutoRelease existingSettings = obs_service_get_settings(service);
		OBSDataAutoRelease newSettings = obs_data_create();
	
		obs_data_apply(newSettings, existingSettings);
		obs_data_apply(newSettings, requestedSettings);

		obs_service_update(service, newSettings);
	} else {
		OBSDataAutoRelease currentHotkeys = obs_hotkeys_save_service(service);
		service = obs_service_create(requestedServiceType.toUtf8(), "irltk_custom_service", requestedSettings, currentHotkeys);
		obs_frontend_set_streaming_service(service);
	}

	obs_frontend_save_streaming_service();

	return RequestResult::BuildSuccess(request);
}