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
	void SetConnectionStatusIndicator(bool active=false);

private Q_SLOTS:
	void SaveSettings();
	void DialogButtonClicked(QAbstractButton *button);
	void ConnectButtonClicked();
	void DisconnectButtonClicked();
	void ConnectionStateChanged(QAbstractSocket::SocketState state);
	void AuthenticationFailed(QString failureReason);

private:
	Ui::SettingsDialog* ui;
};
