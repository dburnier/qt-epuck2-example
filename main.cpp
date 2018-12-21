#include "epuckuserinterface.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	EPuckUserInterface w;
	w.show();
	
	return a.exec();
}
