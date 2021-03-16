#include <obs-module.h>
#include <iostream>
#include <obs-frontend-api.h>
#include <QString>
#include <QAbstractButton>
#include <QMessageBox>
#include <QRegExpValidator>
#include <QIcon>
#include <QPixmap>

#include "../plugin-main.h"
#include "../Config.h"
#include "../WebsocketManager.h"
#include "settings-dialog.h"

SettingsDialog::SettingsDialog(QWidget* parent) :
	QDialog(parent, Qt::Dialog),
	ui(new Ui::SettingsDialog)
{
	ui->setupUi(this);

	connect(ui->buttonBox, &QDialogButtonBox::clicked,
		this, &SettingsDialog::DialogButtonClicked);
	connect(ui->connectButton, &QPushButton::clicked,
		this, &SettingsDialog::ConnectButtonClicked);
	connect(ui->disconnectButton, &QPushButton::clicked,
		this, &SettingsDialog::DisconnectButtonClicked);

	auto websocketManager = GetWebsocketManager();
	QObject::connect(websocketManager.get(), &WebsocketManager::connectionStateChanged, this, &SettingsDialog::ConnectionStateChanged);
	QObject::connect(websocketManager.get(), &WebsocketManager::connectionAuthenticationFailure, this, &SettingsDialog::AuthenticationFailed);
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}

void SettingsDialog::showEvent(QShowEvent* event)
{
	auto conf = GetConfig();

	if (conf->ConnectOnLoad) {
		ui->connectOnLoad->setCheckState(Qt::Checked);
	} else {
		ui->connectOnLoad->setCheckState(Qt::Unchecked);
	}
	ui->sessionKey->setText(conf->SessionKey);
	ui->connectUrl->setText(conf->ConnectUrl);
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
	if (websocketManager)
		websocketManager->SessionKey = ui->sessionKey->text();

	if (ui->connectOnLoad->checkState() == Qt::Checked) {
		conf->ConnectOnLoad = true;
	} else {
		conf->ConnectOnLoad = false;
	}
	conf->SessionKey = ui->sessionKey->text();
	conf->ConnectUrl = ui->connectUrl->text();

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

void SettingsDialog::ConnectButtonClicked()
{
	auto conf = GetConfig();
	auto websocketManager = GetWebsocketManager();
	if (!websocketManager || !conf)
		return;
	websocketManager->Connect(conf->ConnectUrl);
}

void SettingsDialog::DisconnectButtonClicked()
{
	auto websocketManager = GetWebsocketManager();
	if (!websocketManager)
		return;
	websocketManager->Disconnect();
}

void SettingsDialog::ConnectionStateChanged(QAbstractSocket::SocketState state)
{
	auto websocketManager = GetWebsocketManager();
	if (websocketManager->IsConnected()) {
		SetConnectionStatusIndicator(true);
	} else {
		SetConnectionStatusIndicator(false);
	}

	if (state == QAbstractSocket::UnconnectedState) {
		ui->connectButton->setEnabled(true);
		ui->disconnectButton->setEnabled(false);
	} else {
		ui->connectButton->setEnabled(false);
		ui->disconnectButton->setEnabled(true);
	}

#ifdef DEBUG_ENABLED
	blog(LOG_INFO, "Socket state changed. New state: %d", state);
#endif
}

void SettingsDialog::AuthenticationFailed(QString failureReason)
{
	QMessageBox msgBox;
	msgBox.setWindowTitle(obs_module_text("IRLTKSelfHost.Panel.ErrorTitle"));
	msgBox.setText(QString(obs_module_text("IRLTKSelfHost.Panel.AuthenticationFailedMessage")).arg(failureReason));
	msgBox.exec();
}

void SettingsDialog::SetConnectionStatusIndicator(bool active) {
	if (active) {
		ui->connectionStatus->setPixmap(QPixmap(":/logos/checkmark"));
	} else {
		ui->connectionStatus->setPixmap(QPixmap(":/logos/times"));
	}
}