#include "RequestHandler.h"

RequestResult RequestHandler::GetSceneCollectionList(const Request& request)
{
	QJsonObject resultJson;
	resultJson["currentSceneCollectionName"] = obs_frontend_get_current_scene_collection();

	char** sceneCollections = obs_frontend_get_scene_collections();
	QJsonArray collectionList = UtilsStringListToQt(sceneCollections);
	bfree(sceneCollections);
	resultJson["sceneCollections"] = collectionList;

	return RequestResult::BuildSuccess(request, resultJson);
}

RequestResult RequestHandler::SetCurrentSceneCollection(const Request& request)
{
	QString comment;
	RequestStatus checkStatus = request.ValidateString("sceneCollectionName", &comment);
	if (checkStatus != RequestStatus::NoError)
		return RequestResult::BuildFailure(request, checkStatus, comment);

	QString sceneCollectionName = request.RequestData()["sceneCollectionName"].toString();

	char** sceneCollections = obs_frontend_get_scene_collections();
	QJsonArray collectionList = UtilsStringListToQt(sceneCollections);
	bfree(sceneCollections);

	if (!collectionList.contains(sceneCollectionName))
		return RequestResult::BuildFailure(request, RequestStatus::SceneCollectionNotFound);

	const char *sceneCollectionNameConst = QT_TO_UTF8(sceneCollectionName);

	obs_queue_task(OBS_TASK_UI, [](void* param) {
		obs_frontend_set_current_scene_collection(reinterpret_cast<const char*>(param));
	}, (void*)sceneCollectionNameConst, true);

	return RequestResult::BuildSuccess(request);
}

RequestResult RequestHandler::GetProfileList(const Request& request)
{
	QJsonObject resultJson;
	resultJson["currentProfileName"] = obs_frontend_get_current_profile();

	char** profiles = obs_frontend_get_profiles();
	QJsonArray profileList = UtilsStringListToQt(profiles);
	bfree(profiles);
	resultJson["profiles"] = profileList;

	return RequestResult::BuildSuccess(request, resultJson);
}

RequestResult RequestHandler::SetCurrentProfile(const Request& request)
{
	QString comment;
	RequestStatus checkStatus = request.ValidateString("profileName", &comment);
	if (checkStatus != RequestStatus::NoError)
		return RequestResult::BuildFailure(request, checkStatus, comment);

	QString profileName = request.RequestData()["profileName"].toString();

	char** profiles = obs_frontend_get_profiles();
	QJsonArray profileList = UtilsStringListToQt(profiles);
	bfree(profiles);

	if (!profileList.contains(profileName))
		return RequestResult::BuildFailure(request, RequestStatus::ProfileNotFound);

	const char *profileNameConst = QT_TO_UTF8(profileName);

	obs_queue_task(OBS_TASK_UI, [](void* param) {
		obs_frontend_set_current_profile(reinterpret_cast<const char*>(param));
	}, (void*)profileNameConst, true);

	return RequestResult::BuildSuccess(request);
}

RequestResult RequestHandler::GetVideoSettings(const Request& request)
{
	config_t *config = obs_frontend_get_profile_config();
	QJsonObject resultJson;

	resultJson["baseX"] = (int)config_get_uint(config, "Video", "BaseCX");
	resultJson["baseY"] = (int)config_get_uint(config, "Video", "BaseCY");
	resultJson["outputX"] = (int)config_get_uint(config, "Video", "OutputCX");
	resultJson["outputY"] = (int)config_get_uint(config, "Video", "OutputCY");
	resultJson["fpsType"] = (int)config_get_int(config, "Video", "FPSType");
	switch (config_get_int(config, "Video", "FPSType")) {
		case 0:
			resultJson["fps"] = config_get_string(config, "Video", "FPSCommon");
			break;
		case 1:
			resultJson["fps"] = QString::number(config_get_int(config, "Video", "FPSInt"));
			break;
		case 2:
			resultJson["fps"] = QString::number((double)config_get_int(config, "Video", "FPSNum") / (double)config_get_int(config, "Video", "FPSDen"));
			break;
		default:
			resultJson["fps"] = "__INVALID_FPS_MODE__";
			break;
	}

	return RequestResult::BuildSuccess(request, resultJson);
}

#ifdef IRLTK_CLOUD
RequestResult RequestHandler::SetVideoSettings(const Request& request)
{
	if (obs_video_active())
		return RequestResult::BuildFailure(request, RequestStatus::OutputRunning);

	bool hasFpsCommon = false;
	bool hasBaseX = false;
	bool hasBaseY = false;
	bool hasOutputX = false;
	bool hasOutputY = false;

	QString comment;
	RequestStatus checkStatus = RequestStatus::NoError;

	checkStatus = request.ValidateString("fpsCommon", &comment);
	if (checkStatus == RequestStatus::NoError) {
		hasFpsCommon = true;
	} else if (checkStatus != RequestStatus::MissingRequestParameter) {
		return RequestResult::BuildFailure(request, checkStatus, comment);
	}
	checkStatus = request.ValidateDouble("baseX", &comment, 8, 4096);
	if (checkStatus == RequestStatus::NoError) {
		hasBaseX = true;
	} else if (checkStatus != RequestStatus::MissingRequestParameter) {
		return RequestResult::BuildFailure(request, checkStatus, comment);
	}
	checkStatus = request.ValidateDouble("baseY", &comment, 8, 4096);
	if (checkStatus == RequestStatus::NoError) {
		hasBaseY = true;
	} else if (checkStatus != RequestStatus::MissingRequestParameter) {
		return RequestResult::BuildFailure(request, checkStatus, comment);
	}
	checkStatus = request.ValidateDouble("outputX", &comment, 8, 4096);
	if (checkStatus == RequestStatus::NoError) {
		hasOutputX = true;
	} else if (checkStatus != RequestStatus::MissingRequestParameter) {
		return RequestResult::BuildFailure(request, checkStatus, comment);
	}
	checkStatus = request.ValidateDouble("outputY", &comment, 8, 4096);
	if (checkStatus == RequestStatus::NoError) {
		hasOutputY = true;
	} else if (checkStatus != RequestStatus::MissingRequestParameter) {
		return RequestResult::BuildFailure(request, checkStatus, comment);
	}

	bool videoChanged = false;
	config_t *config = obs_frontend_get_profile_config();

	if (hasFpsCommon) {
		config_set_uint(config, "Video", "FPSType", 0);
		config_set_string(config, "Video", "FPSCommon", QT_TO_UTF8(request.RequestData()["fpsCommon"].toString()));
		videoChanged = true;
	}

	if (hasBaseX && hasBaseY) {
		uint32_t bx = request.RequestData()["baseX"].toDouble();
		uint32_t by = request.RequestData()["baseY"].toDouble();
		config_set_uint(config, "Video", "BaseCX", bx);
		config_set_uint(config, "Video", "BaseCY", by);
		videoChanged = true;
	}

	if (hasOutputX && hasOutputY) {
		uint32_t ox = request.RequestData()["outputX"].toDouble();
		uint32_t oy = request.RequestData()["outputY"].toDouble();
		config_set_uint(config, "Video", "OutputCX", ox);
		config_set_uint(config, "Video", "OutputCY", oy);
		videoChanged = true;
	}

	if (videoChanged) {
		config_save_safe(config, "tmp", nullptr);
		obs_frontend_irltk_reset_video();
	}

	return RequestResult::BuildSuccess(request);
}
#endif
