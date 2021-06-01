#include <obs-module.h>
#include <iostream>
#include <obs-frontend-api.h>
#include <QString>
#include <QAbstractButton>
#include <QMessageBox>
#include <QRegExpValidator>
#include <QIcon>
#include <QPixmap>
#include <QTimer>

#include "../plugin-main.h"
#include "../Config.h"
#include "../WebsocketManager.h"
#include "settings-dialog.h"

SettingsDialog::SettingsDialog(QWidget* parent) :
	QDialog(parent, Qt::Dialog),
	ui(new Ui::SettingsDialog),
	websocketManuallyDisconnected(false)
{
	ui->setupUi(this);

	connect(ui->buttonBox, &QDialogButtonBox::clicked,
		this, &SettingsDialog::DialogButtonClicked);
	connect(ui->connectDisconnect, &QPushButton::clicked,
		this, &SettingsDialog::ConnectDisconnectButtonClicked);

	reconnectTimer = new QTimer(this);
	reconnectTimer->setSingleShot(true);
	connect(reconnectTimer, &QTimer::timeout, this, &SettingsDialog::onReconnectTimerTimeout);

	auto websocketManager = GetWebsocketManager();
	QObject::connect(websocketManager.get(), &WebsocketManager::connectionStateChanged, this, &SettingsDialog::onConnectionStateChanged);
	QObject::connect(websocketManager.get(), &WebsocketManager::connectionIdentificationSuccess, [=]() {SetConnectionStatusIndicator(true);});
}

SettingsDialog::~SettingsDialog()
{
	if (reconnectTimer->isActive())
		reconnectTimer->stop();
	delete reconnectTimer;
	delete ui;
}

void SettingsDialog::showEvent(QShowEvent* event)
{
	auto conf = GetConfig();

	if (conf->ConnectOnLoad)
		ui->connectOnLoad->setCheckState(Qt::Checked);
	else
		ui->connectOnLoad->setCheckState(Qt::Unchecked);
	ui->sessionKey->setText(conf->SessionKey);
	ui->connectUrl->setText(conf->ConnectUrl);
	if (conf->AutoReconnect)
		ui->autoReconnect->setCheckState(Qt::Checked);
	else
		ui->autoReconnect->setCheckState(Qt::Unchecked);
}

void SettingsDialog::ToggleShowHide()
{
	if (!isVisible())
		setVisible(true);
	else
		setVisible(false);
}

void SettingsDialog::SaveSettings()
{
	auto conf = GetConfig();
	auto websocketManager = GetWebsocketManager();

	if (ui->autoReconnect->checkState() == Qt::Unchecked && reconnectTimer->isActive()) {
		StopReconnectTimer();
		UpdateConnectUi();
	}

	websocketManager->SessionKey = ui->sessionKey->text();

	if (ui->connectOnLoad->checkState() == Qt::Checked)
		conf->ConnectOnLoad = true;
	else
		conf->ConnectOnLoad = false;
	conf->SessionKey = ui->sessionKey->text();
	conf->ConnectUrl = ui->connectUrl->text();
	if (ui->autoReconnect->checkState() == Qt::Checked)
		conf->AutoReconnect = true;
	else
		conf->AutoReconnect = false;

	conf->Save();
}

void SettingsDialog::DialogButtonClicked(QAbstractButton *button)
{
	QDialogButtonBox::ButtonRole signalButton = ui->buttonBox->buttonRole(button);
	if (signalButton == QDialogButtonBox::ApplyRole || signalButton == QDialogButtonBox::AcceptRole) {
		QRegExpValidator keyValidator = QRegExpValidator(QRegExp("[A-Za-z0-9]{64}"));
		int pos = 0;
		QString sessionKeyText = ui->sessionKey->text();
		if ((!(keyValidator.validate(sessionKeyText, pos) == QValidator::Acceptable)) && (!sessionKeyText.isEmpty())) {
			QMessageBox msgBox;
			msgBox.setWindowTitle(obs_module_text("IRLTKSelfHost.Panel.ErrorTitle"));
			msgBox.setText(obs_module_text("IRLTKSelfHost.Panel.InvalidSessionKeyFormat"));
			msgBox.exec();
			return;
		}
		SettingsDialog::SaveSettings();
		if (signalButton == QDialogButtonBox::AcceptRole) {
			QDialog::accept();
			return;
		}
	}
}

void SettingsDialog::ConnectDisconnectButtonClicked()
{
	auto conf = GetConfig();
	auto websocketManager = GetWebsocketManager();
	if (!websocketManager || !conf)
		return;

	if (reconnectTimer->isActive()) {
		StopReconnectTimer();
		UpdateConnectUi();
	} else if (websocketManager->IsConnected()) {
		websocketManuallyDisconnected = true;
		QMetaObject::invokeMethod(websocketManager.get(), "Disconnect");
	} else {
		QMetaObject::invokeMethod(websocketManager.get(), "Connect", Q_ARG(QString, conf->ConnectUrl));
	}
}

void SettingsDialog::onConnectionStateChanged(QAbstractSocket::SocketState state)
{
#ifdef DEBUG_MODE
	blog(LOG_INFO, "[SettingsDialog::onConnectionStateChanged] Socket state changed. New state: %d", state);
#endif
	auto config = GetConfig();
	auto websocketManager = GetWebsocketManager();
	uint16_t closeCode = websocketManager->GetCloseCode();

	UpdateConnectUi();
	if (state == QAbstractSocket::UnconnectedState) {
		if (websocketManager->GetCloseError() != QAbstractSocket::UnknownSocketError) {
			if (isVisible() && !reconnectTimer->isActive()) {
				QMessageBox msgBox;
				msgBox.setWindowTitle(obs_module_text("IRLTKSelfHost.Panel.ErrorTitle"));
				msgBox.setText(QString(obs_module_text("IRLTKSelfHost.Panel.ConnectionClosedProtocolMessage")).arg(websocketManager->GetCloseErrorString()));
				msgBox.exec();
			}
		} else if (closeCode == WebsocketManager::CloseCode::AuthenticationFailed) {
			blog(LOG_INFO, "[SettingsDialog::onConnectionStateChanged] Identification failed/expired with the following reason: %s", QT_TO_UTF8(websocketManager->GetCloseReason()));
			if (isVisible() && !reconnectTimer->isActive()) {
				QMessageBox msgBox;
				msgBox.setWindowTitle(obs_module_text("IRLTKSelfHost.Panel.ErrorTitle"));
				msgBox.setText(QString(obs_module_text("IRLTKSelfHost.Panel.IdentificationFailedMessage")).arg(websocketManager->GetCloseReason()));
				msgBox.exec();
			}
		} else if (closeCode == WebsocketManager::CloseCode::SessionAlreadyExists) {
			blog(LOG_INFO, "[SettingsDialog::onConnectionStateChanged] Identification failed because there is already an OBS client connected to the server with the configured session key.");
			if (isVisible() && !reconnectTimer->isActive()) {
				QMessageBox msgBox;
				msgBox.setWindowTitle(obs_module_text("IRLTKSelfHost.Panel.ErrorTitle"));
				msgBox.setText(QString(obs_module_text("IRLTKSelfHost.Panel.IdentificationFailedMessage")).arg(websocketManager->GetCloseReason()));
				msgBox.exec();
			}
		} else if (closeCode >= 4000) {
			if (isVisible() && !reconnectTimer->isActive()) {
				QMessageBox msgBox;
				msgBox.setWindowTitle(obs_module_text("IRLTKSelfHost.Panel.ErrorTitle"));
				msgBox.setText(QString(obs_module_text("IRLTKSelfHost.Panel.IdentificationFailedMessage")).arg(websocketManager->GetCloseReason()));
				msgBox.exec();
			}
		}

		if (config->AutoReconnect && 
			closeCode != WebsocketManager::CloseCode::NotIdentified &&
			closeCode != WebsocketManager::CloseCode::AuthenticationFailed &&
			closeCode != WebsocketManager::CloseCode::InvalidIdentifyParameter &&
			closeCode != WebsocketManager::CloseCode::UnsupportedProtocolVersion &&
			closeCode != WebsocketManager::CloseCode::SessionAlreadyExists
		){
			StartReconnectTimer();
		}
	} else if (state == QAbstractSocket::ConnectedState) {
		StopReconnectTimer();
	}
}

void SettingsDialog::onReconnectTimerTimeout()
{
	if (reconnectAttempts > 99) {
#ifdef DEBUG_MODE
		blog(LOG_INFO, "[SettingsDialog::onReconnectTimerTimeout] Stopping reconnect timer because reconnect attempts reached 100.");
#endif
		UpdateConnectUi();
		return;
	}
	auto config = GetConfig();
	auto websocketManager = GetWebsocketManager();
	if (websocketManager->IsConnected())
		return;

	reconnectTimerCurrentCountdown--;
	ui->connectDisconnect->setText(QString("Retrying in %1...").arg(reconnectTimerCurrentCountdown + 1));
	ui->connectDisconnect->setEnabled(true);
	if (reconnectTimerCurrentCountdown == -1) {
        if (reconnectTimerTotal <= 30) {
            reconnectTimerTotal *= 2;
        }
		reconnectTimerCurrentCountdown = reconnectTimerTotal;
		reconnectAttempts++;
		QMetaObject::invokeMethod(websocketManager.get(), "Connect", Q_ARG(QString, config->ConnectUrl));
	}
	reconnectTimer->start(1000);
}

void SettingsDialog::StartReconnectTimer()
{
	if (reconnectTimer->isActive()) {
		ui->connectDisconnect->setText(QString("Retrying in %1...").arg(reconnectTimerCurrentCountdown + 1));
		ui->connectDisconnect->setEnabled(true);
		return;
	}

	if (websocketManuallyDisconnected) {
		websocketManuallyDisconnected = false;
		return;
	}

	reconnectTimerTotal = 1;
    reconnectTimerCurrentCountdown = 1;
	reconnectAttempts = 0;
	reconnectTimer->start(100);
}

void SettingsDialog::StopReconnectTimer()
{
	auto websocketManager = GetWebsocketManager();

	if (reconnectTimer->isActive())
		reconnectTimer->stop();

	reconnectTimerTotal = 0;
    reconnectTimerCurrentCountdown = 0;
	reconnectAttempts = 0;
}

void SettingsDialog::UpdateConnectUi()
{
	auto websocketManager = GetWebsocketManager();
	auto state = websocketManager->GetSocketState();

	switch (state) {
		case QAbstractSocket::UnconnectedState:
			ui->connectDisconnect->setEnabled(true);
			ui->connectDisconnect->setText("Connect");
			SetConnectionStatusIndicator(false);
			break;
		case QAbstractSocket::ConnectedState:
			ui->connectDisconnect->setEnabled(true);
			ui->connectDisconnect->setText("Disconnect");
			break;
		default:
			ui->connectDisconnect->setEnabled(false);
			ui->connectDisconnect->setText("Connecting...");
			break;
	}
}

void SettingsDialog::SetConnectionStatusIndicator(bool active) {
	if (active) {
		ui->connectionStatus->setPixmap(QPixmap(":/logos/checkmark"));
	} else {
		ui->connectionStatus->setPixmap(QPixmap(":/logos/times"));
	}
}