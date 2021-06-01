#include "RequestHandler.h"

RequestResult RequestHandler::GetRecordStatus(const Request& request)
{
	QJsonObject resultJson;

	OBSOutputAutoRelease recordOutput = obs_frontend_get_recording_output();
	resultJson["outputActive"] = obs_output_active(recordOutput);
	resultJson["outputPaused"] = obs_output_paused(recordOutput);
	resultJson["outputTimecode"] = UtilsGetOutputTimecode(recordOutput);
	resultJson["outputDuration"] = (qint64)UtilsGetOutputDuration(recordOutput);

	return RequestResult::BuildSuccess(request, resultJson);
}

RequestResult RequestHandler::StartRecord(const Request& request)
{
	if (obs_frontend_recording_active())
		return RequestResult::BuildFailure(request, RequestStatus::RecordRunning);

	obs_frontend_recording_start();
	return RequestResult::BuildSuccess(request);
}

RequestResult RequestHandler::StopRecord(const Request& request)
{
	if (!obs_frontend_recording_active())
		return RequestResult::BuildFailure(request, RequestStatus::RecordNotRunning);

	obs_frontend_recording_stop();
	return RequestResult::BuildSuccess(request);
}
