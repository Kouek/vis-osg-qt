#include <QtWidgets/qapplication.h>

#include <common_gui/vol_io_widget.h>

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	VolumeIOWidget wnd;
	wnd.show();

	app.exec();

	return 0;
}
