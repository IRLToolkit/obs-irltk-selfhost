#pragma once

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QSslError>
#include <QList>
#include <QString>
#include <QUrl>
#include <QtCore/QMutex>
#include <QtCore/QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <QThread>
#include "plugin-main.h"

class WebsocketManager : public QObject {
	Q_OBJECT

	public:
		enum CloseCode: std::uint16_t {
			UnknownReason = 4000,

			// The server was unable to decode the incoming websocket message
			MessageDecodeError = 4001,
			// The specified `messageType` was invalid
			UnknownMessageType = 4002,
			// The client sent a websocket message without first sending `Identify` message
			NotIdentified = 4003,
			// The client sent an `Identify` message while already identified
			AlreadyIdentified = 4004,
			// The authentication attempt (via `Identify`) failed
			AuthenticationFailed = 4005,
			// There was an invalid parameter the client's `Identify` message
			InvalidIdentifyParameter = 4006,
			// A `Request` or `RequestBatch` was missing its `requestId`
			RequestMissingRequestId = 4007,
			// The websocket session has been invalidated by the obs-websocket server.
			SessionInvalidated = 4008,
			// The server detected the usage of an old version of the obs-websocket protocol.
			UnsupportedProtocolVersion = 4009,
			// There is already a session connected with that sessionKey
			SessionAlreadyExists = 4010,
		};


		explicit WebsocketManager();
		~WebsocketManager();

		QString SessionKey;

		QThreadPool* GetThreadPool() {
			return &_threadPool;
		}

		QAbstractSocket::SocketState GetSocketState() {
			return _socket.state();
		}

		QWebSocketProtocol::CloseCode GetCloseCode() {
			return _socket.closeCode();
		}

		QString GetCloseReason() {
			return _socket.closeReason();
		}

		QAbstractSocket::SocketError GetCloseError() {
			return _socket.error();
		}

		QString GetCloseErrorString() {
			return _socket.errorString();
		}

		bool IsConnected() {
			return _socket.isValid();
		}

		bool IsIdentified() {
			return isIdentified;
		}

	public Q_SLOTS:
		void Connect(QString url);
		void Disconnect();
		void SendTextMessage(QString message);

	signals:
		void connectionStateChanged(QAbstractSocket::SocketState state);
		void connectionIdentificationSuccess();

	private Q_SLOTS:
		void onConnected();
		void onDisconnected();
		void onTextMessageReceived(QString message);
		void onSslErrors(const QList<QSslError> &errors);
		void _SendIdentify();

	private:
		QThread _workerThread;
		QWebSocket _socket;
		QThreadPool _threadPool;
		bool isIdentified;
};