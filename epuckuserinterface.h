#ifndef EPUCKUSERINTERFACE_H
#define EPUCKUSERINTERFACE_H

#include <QWidget>
#include <QCheckBox>
#include <QLabel>
#include <QTimer>

#include "epuckinterface.h"


#define NUM_LEDS	8
#define NUM_PROXI	8


namespace Ui {
class EPuckUserInterface;
}

class EPuckUserInterface : public QWidget
{
	Q_OBJECT
	
public:
	explicit EPuckUserInterface(QWidget *parent = 0);
	~EPuckUserInterface();

protected:
	void disableGroups(bool disable);

	QCheckBox* leds[NUM_LEDS];
	QLabel* proxi[NUM_PROXI];

	QTimer timer;
	EPuckInterface* epuck;
	
protected slots:
	void updateMotors(int value);
	void updateLeds(int state);
	void updateProxi();

	void connectClicked();
    void quitClicked();

private:
	Ui::EPuckUserInterface *ui;
};

#endif // EPUCKUSERINTERFACE_H
