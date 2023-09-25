#include <QtWidgets/qapplication.h>

#include <common_gui/qt_osg_widget.h>

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	QtOSGWidget osgWnd;
	osgWnd.show();

	app.exec();

	return 0;
}
