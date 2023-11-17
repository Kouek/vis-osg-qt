#ifndef HEAT_MAP_WIDGET_H
#define HEAT_MAP_WIDGET_H

#include <QtGui/qevent.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qgraphicsview.h>

#include <memory>
#include <cmath>

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
	class View : public QGraphicsView
	{
	private:
		int mapW, mapH;

		HeatMapWidget* hmpWdgt;
		QGraphicsScene scn;

	public:
		View(HeatMapWidget* hmpWdgt, int mapW, int mapH)
			: hmpWdgt(hmpWdgt), mapW(mapW), mapH(mapH)
		{
			setScene(&scn);
			setMouseTracking(true);
		}

		void UpdateFrom(const QImage& image)
		{
			auto pixmap = QPixmap::fromImage(image);
			scn.clear();
			scn.addPixmap(pixmap);

			AdjustViewportToFit();
		}

		void AdjustViewportToFit()
		{
			fitInView(scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
			update();
		}

	protected:
		virtual void resizeEvent(QResizeEvent* ev) override
		{
			AdjustViewportToFit();
			QGraphicsView::resizeEvent(ev);
		}

		virtual void mouseMoveEvent(QMouseEvent* ev) override
		{
			auto scnPos = mapToScene(ev->pos());
			if (scnPos.x() < 0. || scnPos.x() >= mapW - 1. ||
				scnPos.y() < 0. || scnPos.y() >= mapH - 1.) {
				hmpWdgt->ui.label_HeatMapPoint->setText(QString("__"));
				return;
			}

			auto scalar = hmpWdgt->SampleVolume({
				static_cast<int>(floorf(scnPos.x())),
				static_cast<int>(floorf(scnPos.y())) });
			hmpWdgt->ui.label_HeatMapPoint->setText(
				QString("%1")
				.arg(static_cast<uint8_t>(scalar * 255.f)));
		}
	};

	std::array<uint32_t, 3> dim;
	uint32_t dimYxX;
	uint32_t currZ;
	std::array<float, 2> scaleImgToVol;
	std::shared_ptr<std::vector<float>> volDat;

	Ui::HeatMapWidget ui;

	QImage heatMap;
	View heatMapView;

public:
	HeatMapWidget(int w, int h, QWidget* parent = nullptr)
		: QWidget(parent),
		heatMap(w, h, QImage::Format_RGB32),
		heatMapView(this, w, h)
	{
		ui.setupUi(this);

		layout()->addWidget(&heatMapView);
	}

	const std::array<uint32_t, 3>& GetDimension() const
	{
		return dim;
	}

	void SetVolume(std::shared_ptr<std::vector<float>> volDat, const std::array<uint32_t, 3>& dim)
	{
		this->volDat = volDat;
		this->dim = dim;

		dimYxX = static_cast<size_t>(dim[1]) * dim[0];
		currZ = dim[2];

		scaleImgToVol[0] = float(dim[0]) / heatMap.width();
		scaleImgToVol[1] = float(dim[1]) / heatMap.height();
	}

	float SampleVolume(const std::array<int, 2>& heatMapPos)
	{
		if (currZ == dim[2]) return 0.f;

		auto volY = static_cast<uint32_t>(floorf(heatMapPos[1] * scaleImgToVol[1]));
		volY = dim[1] - 1 - volY;
		auto volX = static_cast<uint32_t>(floorf(heatMapPos[0] * scaleImgToVol[0]));

		auto volIdx = currZ * dimYxX + volY * dim[0] + volX;
		return (*volDat)[volIdx];
	}

	void Update(const std::array<std::array<float, 4>, 256>& tfDat, uint32_t volZ)
	{
		currZ = volZ;

		for (int y = 0; y < heatMap.height(); ++y) {
			auto pxPtr = reinterpret_cast<QRgb*>(heatMap.scanLine(y));
			for (int x = 0; x < heatMap.width(); ++x, ++pxPtr) {
				auto scalar = SampleVolume({ x, y });
				auto& color = tfDat[scalar * 255.f];
				*pxPtr = qRgb(color[0] * 255.f, color[1] * 255.f, color[2] * 255.f);
			}
		}

		heatMapView.UpdateFrom(heatMap);
	}
};

#endif // !HEAT_MAP_WIDGET_H
