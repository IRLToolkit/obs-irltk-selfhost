#include "RequestHandler.h"
#include "WebsocketManager.h"

WebsocketManager::WebsocketManager() :
	QObject(nullptr),
	SessionKey(""),
	isAuthenticated(false)
{
	connect(&_socket, &QWebSocket::connected, this, &WebsocketManager::onConnected);
	connect(&_socket, &QWebSocket::disconnected, this, &WebsocketManager::onDisconnected);
	connect(&_socket, QOverload<const QList<QSslError>&>::of(&QWebSocket::sslErrors), this, &WebsocketManager::onSslErrors);
	connect(&_socket, &QWebSocket::textMessageReceived, this, &WebsocketManager::onTextMessageReceived);
	connect(&_socket, &QWebSocket::stateChanged, [=]( QAbstractSocket::SocketState state ) {
		emit connectionStateChanged(state);
	});	
}

WebsocketManager::~WebsocketManager()
{
	_socket.close(QWebSocketProtocol::CloseCodeGoingAway);
}

void WebsocketManager::Connect(QString url)
{
	if (_socket.state() != QAbstractSocket::UnconnectedState)
		return;
#ifdef DEBUG_MODE
	blog(LOG_INFO, "Connecting to websocket server...");
#endif
	_socket.open(QUrl(url));
}

void WebsocketManager::Disconnect()
{
	if (_socket.state() == QAbstractSocket::UnconnectedState)
		return;
#ifdef DEBUG_MODE
	blog(LOG_INFO, "Disconnecting from websocket server...");
#endif
	_socket.close();
}

void WebsocketManager::SendTextMessage(QString message)
{
	//if (!isAuthenticated)
	//	return;
	_socket.sendTextMessage(message);

#ifdef DEBUG_MODE
			blog(LOG_INFO, "Outgoing websocket message:\n%s", QT_TO_UTF8(message));
#endif
}

void WebsocketManager::_Authenticate()
{
	QJsonObject authenticationObject;
	authenticationObject["messageType"] = "authenticate";
	authenticationObject["sessionKey"] = SessionKey;

	_socket.sendTextMessage(QJsonDocument(authenticationObject).toJson());
}

void WebsocketManager::onConnected()
{
	blog(LOG_INFO, "Connected to websocket server.");
	_Authenticate();
}

void WebsocketManager::onDisconnected()
{
	blog(LOG_INFO, "Disconnected from websocket server.");
	if (isAuthenticated)
		isAuthenticated = false;
}

void WebsocketManager::onTextMessageReceived(QString message)
{
#ifdef DEBUG_MODE
	blog(LOG_INFO, "Incoming websocket message:\n%s", QT_TO_UTF8(message));
#endif

	QtConcurrent::run(&_threadPool, [=]() {
		QJsonParseError error;
		QJsonDocument j = QJsonDocument::fromJson(message.toUtf8(), &error);

		if (error.error != QJsonParseError::NoError) {
			blog(LOG_ERROR, "Error parsing incoming websocket message. Error: %s", QT_TO_UTF8(error.errorString()));
			return;
		}

		if (!j.isObject()) {
			blog(LOG_ERROR, "Incoming websocket message is not an object.");
			return;
		}

		QJsonObject parsedMessage = j.object();

		if (!parsedMessage.contains("messageType")) {
			blog(LOG_ERROR, "Incoming websocket message is missing a messageType.");
			return;
		}

		if (parsedMessage["messageType"].toString() == "request") {
			if (!parsedMessage.contains("requestType")) {
				blog(LOG_ERROR, "Incoming message of type `request` is missing the `requestType` field.");
				return;
			}

			RequestHandler handler;
			RequestResult result = handler.ProcessIncomingMessage(parsedMessage);
			QJsonObject resultJson = handler.GetResultJson(result);
			resultJson["messageType"] = "requestResponse";
			QString resultText = QJsonDocument(resultJson).toJson();
			QMetaObject::invokeMethod(this, "SendTextMessage", Qt::QueuedConnection, Q_ARG(QString, resultText));
		} else if (parsedMessage["messageType"].toString() == "requestBatch") {
			if (!parsedMessage.contains("requests")) {
				blog(LOG_ERROR, "Incoming message of type `requestBatch` is missing the `requests` field.");
				return;
			}

			if (!parsedMessage["requests"].isArray()) {
				blog(LOG_ERROR, "Incoming message of type `requestBatch`'s `requests` field is not an array.");
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
			response["messageType"] = "requestBatchResponse";
			if (parsedMessage.contains("messageId"))
				response["messageId"] = parsedMessage["messageId"];
			else
				response["messageId"] = "";
			response["results"] = results;

			QString resultText = QJsonDocument(response).toJson();
			QMetaObject::invokeMethod(this, "SendTextMessage", Qt::QueuedConnection, Q_ARG(QString, resultText));
		} else if (parsedMessage["messageType"].toString() == "authenticationStatus") {
			if (!parsedMessage.contains("authenticationStatus")) {
				blog(LOG_ERROR, "authenticationStatus message does not contain `authenticationStatus` field!");
				return;
			}
			bool authenticationStatus = parsedMessage["authenticationStatus"].toBool();
			if (authenticationStatus) {
				isAuthenticated = true;
				QMetaObject::invokeMethod(this, "connectionAuthenticationSuccess", Qt::QueuedConnection);
			} else {
				isAuthenticated = false;
				QString failureReason = "Unknown reason.";
				if (parsedMessage.contains("comment"))
					failureReason = parsedMessage["comment"].toString();
				QMetaObject::invokeMethod(this, "Disconnect", Qt::QueuedConnection);
				QMetaObject::invokeMethod(this, "connectionAuthenticationFailure", Qt::QueuedConnection, Q_ARG(QString, failureReason));
			}
		} else {
			blog(LOG_ERROR, "Unhandled messageType in websocket message: `%s`.", QT_TO_UTF8(parsedMessage["messageType"].toString()));
		}
	});
}

void WebsocketManager::onSslErrors(const QList<QSslError> &errors)
{
	;
}