#ifndef HEAT_MAP_WIDGET_H
#define HEAT_MAP_WIDGET_H

#include <QtWidgets/qwidget.h>

#include <memory>

#include <array>
#include <vector>

#include <ui_heat_map_widget.h>

namespace Ui
{
	class HeatMapWidget;
}

class HeatMapWidget : public QWidget
{
	Q_OBJECT
private:
	std::array<uint32_t, 3> dim;
	std::shared_ptr<std::vector<float>> volDat;

	Ui::HeatMapWidget ui;

	QImage heatMap;

public:
	HeatMapWidget(QWidget* parent = nullptr)
		: QWidget(parent), heatMap(100, 100, QImage::Format_RGB32)
	{
		ui.setupUi(this);
	}

	const std::array<uint32_t, 3>& GetDimension() const
	{
		return dim;
	}

	void SetVolume(std::shared_ptr<std::vector<float>> volDat, const std::array<uint32_t, 3>& dim)
	{
		this->volDat = volDat;
		this->dim = dim;
	}

	void Update(const std::array<std::array<float, 4>, 256>& tfDat, uint32_t volZ)
	{
		std::array<float, 2> scaleImgToVol = {
			float(dim[0]) / heatMap.width(),
			float(dim[1]) / heatMap.height()
		};
		auto dimYxX = static_cast<size_t>(dim[1]) * dim[0];
		for (int y = 0; y < heatMap.height(); ++y) {
			auto pxPtr = reinterpret_cast<QRgb*>(heatMap.scanLine(y));
			for (int x = 0; x < heatMap.width(); ++x, ++pxPtr) {
				auto volY = static_cast<uint32_t>(floorf(y * scaleImgToVol[1]));
				volY = dim[1] - 1 - volY;
				auto volX = static_cast<uint32_t>(floorf(x * scaleImgToVol[0]));

				auto volIdx = volZ * dimYxX + volY * dim[0] + volX;
				auto scalar = (*volDat)[volIdx];

				auto& color = tfDat[scalar * 255.f];
				*pxPtr = qRgb(color[0] * 255.f, color[1] * 255.f, color[2] * 255.f);
			}
		}

		ui.label_HeatMap->setPixmap(QPixmap::fromImage(heatMap));
	}
};

#endif // !HEAT_MAP_WIDGET_H
