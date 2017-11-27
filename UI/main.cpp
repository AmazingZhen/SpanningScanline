#include "ModelDisplayer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	SpanningScanline::ModelDisplayer w;
	w.show();
	return a.exec();
}
