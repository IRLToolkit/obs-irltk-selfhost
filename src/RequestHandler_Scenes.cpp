#include "RequestHandler.h"

RequestResult RequestHandler::SetCurrentScene(const Request& request)
{
	QString comment;
	RequestStatus checkStatus = request.ValidateString("sceneName", &comment);
	if (checkStatus != RequestStatus::NoError)
		return RequestResult::BuildFailure(request, checkStatus, comment);

	QString sceneName = request.RequestData()["sceneName"].toString();

	OBSSourceAutoRelease sceneSource = obs_get_source_by_name(QT_TO_UTF8(sceneName));

	if (!sceneSource)
		return RequestResult::BuildFailure(request, RequestStatus::SceneNotFound);

	obs_frontend_set_current_scene(sceneSource);

	return RequestResult::BuildSuccess(request);
}