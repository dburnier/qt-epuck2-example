#include "epuckuserinterface.h"
#include "ui_epuckuserinterface.h"

#include <QDebug>


#define CAPTURE_UI_LED(n)	do { leds[n] = ui->led##n; } while(0)
#define CAPTURE_UI_PROX(n)	do { proxi[n] = ui->proxi##n; } while(0)


EPuckUserInterface::EPuckUserInterface(QWidget *parent) :
        QWidget(parent),
	ui(new Ui::EPuckUserInterface),
	epuck(NULL)
{
	ui->setupUi(this);

	CAPTURE_UI_LED(0);	// will expand to 'leds[0] = ui->led0'
	CAPTURE_UI_LED(1);	// ...
	CAPTURE_UI_LED(2);
	CAPTURE_UI_LED(3);
	CAPTURE_UI_LED(4);
	CAPTURE_UI_LED(5);
	CAPTURE_UI_LED(6);
	CAPTURE_UI_LED(7);

	CAPTURE_UI_PROX(0);
	CAPTURE_UI_PROX(1);
	CAPTURE_UI_PROX(2);
	CAPTURE_UI_PROX(3);
	CAPTURE_UI_PROX(4);
	CAPTURE_UI_PROX(5);
	CAPTURE_UI_PROX(6);
	CAPTURE_UI_PROX(7);

	disableGroups(true);

	// setup refresh timer
	timer.setInterval(100); // 10 Hz
	connect(&timer, SIGNAL(timeout()), this, SLOT(updateProxi()));
}

EPuckUserInterface::~EPuckUserInterface()
{
	delete epuck;
	delete ui;
}

void EPuckUserInterface::disableGroups(bool disable)
{
	ui->motorGroup->setDisabled(disable);
	ui->ledGroup->setDisabled(disable);
	ui->proxiGroup->setDisabled(disable);
}

void EPuckUserInterface::updateMotors(int value)
{
	Q_UNUSED(value);
	epuck->setSpeed(ui->motor0->value(), ui->motor1->value());
}

void EPuckUserInterface::updateLeds(int state)
{
	// find the signal's sender
	for (int i = 0; i < NUM_LEDS; i++)
		if (QObject::sender() == leds[i]) {
			// got it !
			epuck->setLed(i, (int)(state == Qt::Checked));
			return;
		}
	qWarning("oops, no such checkbox...");
}

void EPuckUserInterface::updateProxi()
{
	int result[NUM_PROXI];

    epuck->readProximitySensor(result);

	// update widgets
	for (int i = 0; i < NUM_PROXI; i++)
		proxi[i]->setText(QString::number(result[i]));
}

void EPuckUserInterface::connectClicked()
{
	if (epuck) {
		// disconnect
		timer.stop();
		disableGroups(true);
		delete epuck;
		epuck = NULL;
		ui->connectButton->setText(tr("Connect"));
	} else {
		// connect
		epuck = new EPuckInterface(ui->port->text().toStdString());
		ui->connectButton->setText(tr("Disconnect"));
		disableGroups(false);
		timer.start();
	}
}

void EPuckUserInterface::quitClicked()
{
    this->close();
}
