#include <util/platform.h>

#include <QThread>
#include <QtGui/QImageWriter>

#include "RequestHandler.h"

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

RequestResult RequestHandler::GetBaseInfo(const Request& request)
{
	QJsonObject resultJson;

	resultJson["pluginVersion"] = PLUGIN_VERSION;
	resultJson["obsVersion"] = UtilsGetObsVersion();

	QJsonArray requestHandlers;
	QList<QString> requestHandlerNames = RequestHandlerMap.keys();
	for (auto name : requestHandlerNames) {
		requestHandlers.append(name);
	}
	resultJson["availableRequests"] = requestHandlers;

	QJsonArray supportedImageFormats;
	QList<QByteArray> imageWriterFormats = QImageWriter::supportedImageFormats();
	for (auto imageFormat : imageWriterFormats) {
		supportedImageFormats.append(QJsonValue(QString(imageFormat)));
	}
	resultJson["supportedImageFormats"] = supportedImageFormats;

	return RequestResult::BuildSuccess(request, resultJson);
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

RequestResult RequestHandler::CacheUpdate(const Request& request)
{
	QString comment;
	RequestStatus checkStatus = request.ValidateArray("ingestSources", &comment);
	if (checkStatus != RequestStatus::NoError)
		return RequestResult::BuildFailure(request, checkStatus, comment);

	QJsonObject resultJson;

	OBSSourceAutoRelease currentScene = obs_frontend_get_current_scene();
	resultJson["currentScene"] = obs_source_get_name(currentScene);

	obs_frontend_source_list sceneList = {};
	obs_frontend_get_scenes(&sceneList);
	QJsonArray scenes;
	for (size_t i = 0; i < sceneList.sources.num; i++) {
		obs_source_t* scene = sceneList.sources.array[i];
		scenes.append(QJsonValue(obs_source_get_name(scene)));
	}
	obs_frontend_source_list_free(&sceneList);
	resultJson["sceneList"] = scenes;

	QJsonArray ingests;
	for (auto ingest : request.RequestData()["ingestSources"].toArray()) {
		QJsonObject ingestObject;
		QString ingestSourceName = ingest.toString();
		if (ingestSourceName.isEmpty() || ingestSourceName.isNull())
			continue;
		ingestObject["sourceName"] = ingestSourceName;
		OBSSourceAutoRelease ingestSource = obs_get_source_by_name(ingestSourceName.toUtf8());
		if (!ingestSource) {
			ingestObject["sourceOk"] = false;
			ingests.append(ingestObject);
			continue;
		}
		ingestObject["sourceOk"] = true;
		double volume = obs_mul_to_db(obs_source_get_volume(ingestSource));
		if (volume == -INFINITY) {
			volume = -100.0;
		}
		ingestObject["volume"] = volume;
		ingestObject["muted"] = obs_source_muted(ingestSource);
		QString sourceKind = obs_source_get_id(ingestSource);
		ingestObject["sourceKind"] = sourceKind;

		if (sourceKind == "vlc_source") {
			OBSDataAutoRelease statsData = obs_data_create();
#ifdef IRLTK_CLOUD
			obs_source_media_irltk_get_stats(ingestSource, statsData);
			ingestObject["statsData"] = UtilsObsDataToQt(statsData);
#endif
			ingestObject["mediaState"] = UtilsGetSourceMediaState(ingestSource);
		}
		ingests.append(ingestObject);
	}
	resultJson["ingestSources"] = ingests;

	if (obs_frontend_streaming_active()) {
		resultJson["isStreaming"] = true;
		OBSOutputAutoRelease streamingOutput = obs_frontend_get_streaming_output();
		resultJson["streamTimecode"] = UtilsGetOutputTimecode(streamingOutput);
	} else {
		resultJson["isStreaming"] = false;
		resultJson["streamTimecode"] = "00:00:00.000";
	}

	QJsonObject streamSettings;
	OBSService service = obs_frontend_get_streaming_service();
	streamSettings["serviceType"] = obs_service_get_type(service);
	OBSDataAutoRelease serviceSettings = obs_service_get_settings(service);
	streamSettings["settings"] = UtilsObsDataToQt(serviceSettings);

	return RequestResult::BuildSuccess(request, resultJson);
}

RequestResult RequestHandler::LogDump(const Request& request)
{
	blog(LOG_INFO, "--------------------LOG DUMP BEGIN--------------------");

	OBSSourceAutoRelease currentScene = obs_frontend_get_current_scene();
    blog(LOG_INFO, "Current Scene: %s", obs_source_get_name(currentScene));

	if (obs_frontend_streaming_active()) {
        blog(LOG_INFO, "Streaming: active");
		OBSOutputAutoRelease streamingOutput = obs_frontend_get_streaming_output();
		blog(LOG_INFO, "Current stream timecode: %s", QT_TO_UTF8(UtilsGetOutputTimecode(streamingOutput)));		
	} else {
		blog(LOG_INFO, "Streaming: inactive");
	}

	video_t* mainVideo = obs_get_video();
	blog(LOG_INFO, "Total output frames: %d", video_output_get_total_frames(mainVideo));
	blog(LOG_INFO, "Total output frames skipped: %d", video_output_get_skipped_frames(mainVideo));

	blog(LOG_INFO, "Total render frames: %d", obs_get_total_frames());
	blog(LOG_INFO, "Total render frames skipped: %d", obs_get_lagged_frames());
	blog(LOG_INFO, "Current FPS: %f", obs_get_active_fps());
	blog(LOG_INFO, "Average frame time: %f", (double)obs_get_average_frame_time_ns() / 1000000.0);
	blog(LOG_INFO, "Memory Usage: %f", (double)os_get_proc_resident_size() / (1024.0 * 1024.0));

    blog(LOG_INFO, "--------------------LOG DUMP END--------------------");

	return RequestResult::BuildSuccess(request);
}

RequestResult RequestHandler::GetProfileList(const Request& request)
{
	QJsonObject resultJson;
	resultJson["currentProfile"] = obs_frontend_get_current_profile();

	char** profiles = obs_frontend_get_profiles();
	QJsonArray profileList = UtilsStringListToQt(profiles);
	bfree(profiles);
	resultJson["profiles"] = profileList;

	return RequestResult::BuildSuccess(request, resultJson);
}

RequestResult RequestHandler::SetProfile(const Request& request)
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

RequestResult RequestHandler::GetSceneCollectionList(const Request& request)
{
	QJsonObject resultJson;
	resultJson["currentSceneCollection"] = obs_frontend_get_current_scene_collection();

	char** sceneCollections = obs_frontend_get_scene_collections();
	QJsonArray collectionList = UtilsStringListToQt(sceneCollections);
	bfree(sceneCollections);
	resultJson["sceneCollections"] = collectionList;

	return RequestResult::BuildSuccess(request, resultJson);
}

RequestResult RequestHandler::SetSceneCollection(const Request& request)
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