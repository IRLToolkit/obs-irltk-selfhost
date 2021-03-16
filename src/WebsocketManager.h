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
#include "plugin-main.h"

class WebsocketManager : public QObject {
	Q_OBJECT

	public:
		explicit WebsocketManager();
		~WebsocketManager();

		QString SessionKey;

		void Connect(QString url);
		void Disconnect();

		QThreadPool* GetThreadPool() {
			return &_threadPool;
		}

		bool IsConnected() {
			return _socket.isValid();
		}
		bool IsAuthenticated() {
			return isAuthenticated;
		}
	public Q_SLOTS:
			void SendTextMessage(QString message);

	signals:
		void connectionStateChanged(QAbstractSocket::SocketState state);
		void connectionAuthenticationSuccess();
		void connectionAuthenticationFailure(QString failureReason);

	private Q_SLOTS:
		void onConnected();
		void onDisconnected();
		void onTextMessageReceived(QString message);
		void onSslErrors(const QList<QSslError> &errors);
	private:
		QWebSocket _socket;
		QThreadPool _threadPool;
		bool isAuthenticated;

		void _Authenticate();
};