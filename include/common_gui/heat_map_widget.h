#ifndef HEAT_MAP_WIDGET_H
#define HEAT_MAP_WIDGET_H

#include <QtWidgets/qwidget.h>

#include <ui_heat_map_widget.h>

namespace Ui
{
	class HeatMapWidget;
}

class HeatMapWidget : public QWidget
{
	Q_OBJECT
private:
	Ui::HeatMapWidget ui;

public:
	HeatMapWidget(QWidget* parent = nullptr) :QWidget(parent)
	{
		ui.setupUi(this);
	}
};

#endif // !HEAT_MAP_WIDGET_H
