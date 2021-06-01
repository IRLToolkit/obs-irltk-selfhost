#include "RequestHandler.h"
#include "WebsocketManager.h"

WebsocketManager::WebsocketManager() :
	QObject(nullptr),
	SessionKey(""),
	isIdentified(false)
{
	qRegisterMetaType<QAbstractSocket::SocketState>();

	connect(&_socket, &QWebSocket::connected, this, &WebsocketManager::onConnected);
	connect(&_socket, &QWebSocket::disconnected, this, &WebsocketManager::onDisconnected);
	connect(&_socket, QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors), this, &WebsocketManager::onSslErrors);
	connect(&_socket, &QWebSocket::textMessageReceived, this, &WebsocketManager::onTextMessageReceived);
	connect(&_socket, &QWebSocket::stateChanged, [=]( QAbstractSocket::SocketState state ) {
		emit connectionStateChanged(state);
	});

	_socket.moveToThread(&_workerThread);
	this->moveToThread(&_workerThread); // This is required for some fuckshit reason
	_workerThread.start();
}

WebsocketManager::~WebsocketManager()
{
	QMetaObject::invokeMethod(this, "Disconnect", Qt::BlockingQueuedConnection);
	
	_workerThread.quit();
	_workerThread.wait();
}

void WebsocketManager::Connect(QString url)
{
	if (_socket.state() != QAbstractSocket::UnconnectedState)
		return;
#ifdef DEBUG_MODE
	blog(LOG_INFO, "[WebsocketManager::Connect] Connecting to websocket server...");
#endif
	_socket.open(QUrl(url));
}

void WebsocketManager::Disconnect()
{
	if (_socket.state() == QAbstractSocket::UnconnectedState)
		return;
#ifdef DEBUG_MODE
	blog(LOG_INFO, "[WebsocketManager::Disconnect] Disconnecting from websocket server...");
#endif
	_socket.close();
}

void WebsocketManager::SendTextMessage(QString message)
{
	_socket.sendTextMessage(message);

#ifdef DEBUG_MODE
			blog(LOG_INFO, "[WebsocketManager::SendTextMessage] Outgoing websocket message:\n%s\n", QT_TO_UTF8(message));
#endif
}

void WebsocketManager::_SendIdentify()
{
	QJsonObject identificationObject;
	identificationObject["messageType"] = "Identify";
	identificationObject["sessionKey"] = SessionKey;
	identificationObject["rpcVersion"] = PLUGIN_VERSION;

	QString messageText(QJsonDocument(identificationObject).toJson());
	QMetaObject::invokeMethod(this, "SendTextMessage", Q_ARG(QString, messageText));
}

void WebsocketManager::onConnected()
{
	blog(LOG_INFO, "[WebsocketManager::onConnected] Connected to websocket server. Waiting for `Hello`.");
}

void WebsocketManager::onDisconnected()
{
#ifdef DEBUG_MODE
	blog(LOG_INFO, "[WebsocketManager::onDisconnected] Raw close reason: `%s` | Raw close code: %d", QT_TO_UTF8(_socket.closeReason()), _socket.closeCode());
	blog(LOG_INFO, "[WebsocketManager::onDisconnected] Socket error string: `%s`", QT_TO_UTF8(_socket.errorString()));
#endif
	blog(LOG_INFO, "[WebsocketManager::onDisconnected] Disconnected from websocket server.");
	isIdentified = false;
}

void WebsocketManager::onTextMessageReceived(QString message)
{
#ifdef DEBUG_MODE
	blog(LOG_INFO, "[WebsocketManager::onTextMessageReceived] Incoming websocket message:\n%s\n", QT_TO_UTF8(message));
#endif

	QtConcurrent::run(&_threadPool, [=]() {
		QJsonParseError error;
		QJsonDocument j = QJsonDocument::fromJson(message.toUtf8(), &error);

		if (error.error != QJsonParseError::NoError) {
			blog(LOG_ERROR, "[WebsocketManager::onTextMessageReceived] Error parsing incoming websocket message. Error: %s", QT_TO_UTF8(error.errorString()));
			return;
		}

		if (!j.isObject()) {
			blog(LOG_ERROR, "[WebsocketManager::onTextMessageReceived] Incoming websocket message is not an object.");
			return;
		}

		QJsonObject parsedMessage = j.object();

		if (!parsedMessage.contains("messageType")) {
			blog(LOG_ERROR, "[WebsocketManager::onTextMessageReceived] Incoming websocket message is missing a messageType.");
			return;
		}

		if (parsedMessage["messageType"].toString() == "Request") {
			if (!isIdentified)
				return;

			if (!parsedMessage.contains("requestType")) {
				blog(LOG_ERROR, "[WebsocketManager::onTextMessageReceived] Incoming message of type `Request` is missing the `requestType` field.");
				return;
			}

			RequestHandler handler;
			RequestResult result = handler.ProcessIncomingMessage(parsedMessage);
			QJsonObject resultJson = handler.GetResultJson(result);
			resultJson["messageType"] = "RequestResponse";
			QString resultText = QJsonDocument(resultJson).toJson();
			QMetaObject::invokeMethod(this, "SendTextMessage", Q_ARG(QString, resultText));
		} else if (parsedMessage["messageType"].toString() == "RequestBatch") {
			if (!isIdentified)
				return;

			if (!parsedMessage.contains("requests")) {
				blog(LOG_ERROR, "[WebsocketManager::onTextMessageReceived] Incoming message of type `RequestBatch` is missing the `requests` field.");
				return;
			}

			if (!parsedMessage["requests"].isArray()) {
				blog(LOG_ERROR, "[WebsocketManager::onTextMessageReceived] Incoming message of type `RequestBatch`'s `requests` field is not an array.");
				return;
			}

			RequestHandler handler;
			QJsonArray results;
			for (auto j : parsedMessage["requests"].toArray()) {
				QJsonObject element = j.toObject();
				if (!element.contains("requestType"))
					continue;
				RequestResult result = handler.ProcessIncomingMessage(element);
				QJsonValue resultJson = QJsonValue(handler.GetResultJson(result));
				results.append(resultJson);
			}

			QJsonObject response;
			response["messageType"] = "RequestBatchResponse";
			if (parsedMessage.contains("requestId"))
				response["requestId"] = parsedMessage["requestId"];
			else
				response["requestId"] = "";
			response["results"] = results;

			QString resultText = QJsonDocument(response).toJson();
			QMetaObject::invokeMethod(this, "SendTextMessage", Q_ARG(QString, resultText));
		} else if (parsedMessage["messageType"].toString() == "Hello") {
#ifdef DEBUG_MODE
			blog(LOG_INFO, "[WebsocketManager::onTextMessageReceived] `Hello` received! Sending `Identify`");
#endif
			_SendIdentify();
		} else if (parsedMessage["messageType"].toString() == "Identified") {
#ifdef DEBUG_MODE
			blog(LOG_INFO, "[WebsocketManager::onTextMessageReceived] Received `Identified`!");
#endif
			isIdentified = true;
			QMetaObject::invokeMethod(this, "connectionIdentificationSuccess");
		} else {
			blog(LOG_ERROR, "[WebsocketManager::onTextMessageReceived] Unhandled messageType in websocket message: `%s`.", QT_TO_UTF8(parsedMessage["messageType"].toString()));
		}
	});
}

void WebsocketManager::onSslErrors(const QList<QSslError> &errors)
{
	;
}