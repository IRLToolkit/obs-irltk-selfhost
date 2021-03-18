#pragma once

#include <QtWidgets/QDialog>

#include "ui_settings-dialog.h"

class SettingsDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit SettingsDialog(QWidget* parent = 0);
		~SettingsDialog();
		void showEvent(QShowEvent* event);
		void ToggleShowHide();

	private Q_SLOTS:
		void SaveSettings();
		void DialogButtonClicked(QAbstractButton *button);
		void ConnectDisconnectButtonClicked();
		void onConnectionStateChanged(QAbstractSocket::SocketState state);
		void onAuthenticationFailed(QString failureReason);
		void onReconnectTimerTimeout();

	private:
		Ui::SettingsDialog* ui;
		QTimer *reconnectTimer;

		bool websocketManuallyDisconnected;
		int reconnectTimerTotal;
		int reconnectTimerCurrentCountdown;
		int reconnectAttempts;

		void StartReconnectTimer();
		void StopReconnectTimer();

		void UpdateConnectUi();
		void SetConnectionStatusIndicator(bool active = false);
};
