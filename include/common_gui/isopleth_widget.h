#ifndef ISOPLETH_WIDGET_H
#define ISOPLETH_WIDGET_H

#include <QtGui/qevent.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qgraphicsview.h>

#include <osg/Vec2>

#include <memory>

#include <array>
#include <vector>

namespace Ui
{
	class HeatMapWidget;
}

class IsoplethWidget : public QWidget
{
	Q_OBJECT
private:
	class View : public QGraphicsView
	{
	private:
		int mapW, mapH;

		IsoplethWidget* isplWdgt;
		QGraphicsScene scn;

	public:
		View(IsoplethWidget* isplWdgt, int mapW, int mapH)
			: isplWdgt(isplWdgt), mapW(mapW), mapH(mapH)
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
	};

	bool currUseSmoothedVol;
	std::array<uint32_t, 3> dim = { 0,0,0 };
	uint32_t dimYxX;
	uint32_t currZ;
	float currIsoVal;
	osg::Vec2 voxSz;
	std::array<float, 2> scaleImgToVol;

	std::shared_ptr<std::vector<float>> volDat;
	std::shared_ptr<std::vector<float>> volDatSmoothed;

	QGraphicsScene isoplethScn;
	QImage isopleth;
	View isoplethView;

public:
	IsoplethWidget(int w, int h, QWidget* parent = nullptr)
		: QWidget(parent),
		isopleth(w, h, QImage::Format_RGBA8888),
		isoplethView(this, w, h)
	{
		currUseSmoothedVol = false;
		currIsoVal = -1.f;

		setLayout(new QVBoxLayout);
		layout()->addWidget(&isoplethView);
	}

	const std::array<uint32_t, 3>& GetDimension() const
	{
		return dim;
	}

	void SetVolume(
		std::shared_ptr<std::vector<float>> volDat,
		std::shared_ptr<std::vector<float>> volDatSmoothed,
		const std::array<uint32_t, 3>& dim)
	{
		this->volDat = volDat;
		this->volDatSmoothed = volDatSmoothed;
		this->dim = dim;

		dimYxX = static_cast<size_t>(dim[1]) * dim[0];
		currZ = dim[2];

		voxSz = osg::Vec2(1000.f / dim[0], 1000.f / dim[1]);

		scaleImgToVol[0] = float(dim[0]) / isopleth.width();
		scaleImgToVol[1] = float(dim[1]) / isopleth.height();
	}

	void Update(float isoVal, uint32_t volZ, bool useSmoothedVol = false)
	{
		if (currUseSmoothedVol != useSmoothedVol || currIsoVal != isoVal || currZ != volZ) {
			currUseSmoothedVol = useSmoothedVol;
			currIsoVal = isoVal;
			currZ = volZ;
			marchingSquare();
		}

		isopleth.fill(Qt::transparent);
		QPainter painter(&isopleth);
		painter.setRenderHint(QPainter::Antialiasing);
		isoplethScn.render(&painter);

		isoplethView.UpdateFrom(isopleth);
	}

private:
	void marchingSquare() {
		isoplethScn.clear();

		auto volDimYxX = static_cast<size_t>(dim[1]) * dim[0];
		auto addLineSeg = [&](const std::array<uint32_t, 2>& startPos, const std::array<float, 4>& scalars,
			const std::array<float, 4>& omegas, uint8_t mask) {
				std::array<QPointF, 2> pnts;
				uint8_t pntCnt = 0;

				for (uint8_t i = 0; i < 4; ++i) {
					if (((mask >> i) & 0b1) == 0)
						continue;

					pnts[pntCnt].setX(startPos[0] + (i == 0 || i == 2 ?
						omegas[i] : i == 1 ? 1.f : 0.f));
					pnts[pntCnt].setY(startPos[1] + (i == 1 || i == 3 ?
						omegas[i] : i == 2 ? 1.f : 0.f));
					++pntCnt;
				}

				isoplethScn.addLine(QLineF(pnts[0], pnts[1]));
			};

		std::array<uint32_t, 2> pos;
		for (pos[1] = 0; pos[1] < dim[1] - 1; ++pos[1])
			for (pos[0] = 0; pos[0] < dim[0] - 1; ++pos[0]) {
				// Voxels in CCW order form a grid
				// +------------+
				// |  3 <--- 2  |
				// |  |     /|\ |
				// | \|/     |  |
				// |  0 ---> 1  |
				// +------------+
				uint8_t cornerState = 0;
				auto surfStart =
					(currUseSmoothedVol ? volDatSmoothed->data() : volDat->data())
					+ currZ * volDimYxX;
				std::array<float, 4> scalars = {
					surfStart[pos[1] * dim[0] + pos[0]],
					surfStart[pos[1] * dim[0] + pos[0] + 1],
					surfStart[(pos[1] + 1) * dim[0] + pos[0] + 1],
					surfStart[(pos[1] + 1) * dim[0] + pos[0]]
				};
				for (uint8_t i = 0; i < 4; ++i)
					if (scalars[i] >= currIsoVal)
						cornerState |= 1 << i;

				std::array<float, 4>  omegas = {
					scalars[0] / (scalars[1] + scalars[0]),
					scalars[1] / (scalars[2] + scalars[1]),
					scalars[3] / (scalars[3] + scalars[2]),
					scalars[0] / (scalars[0] + scalars[3])
				};

				switch (cornerState) {
				case 0b0001:
				case 0b1110:
					addLineSeg(pos, scalars, omegas, 0b1001);
					break;
				case 0b0010:
				case 0b1101:
					addLineSeg(pos, scalars, omegas, 0b0011);
					break;
				case 0b0011:
				case 0b1100:
					addLineSeg(pos, scalars, omegas, 0b1010);
					break;
				case 0b0100:
				case 0b1011:
					addLineSeg(pos, scalars, omegas, 0b0110);
					break;
				case 0b0101:
					addLineSeg(pos, scalars, omegas, 0b0011);
					addLineSeg(pos, scalars, omegas, 0b1100);
					break;
				case 0b1010:
					addLineSeg(pos, scalars, omegas, 0b0110);
					addLineSeg(pos, scalars, omegas, 0b1001);
					break;
				case 0b0110:
				case 0b1001:
					addLineSeg(pos, scalars, omegas, 0b0101);
					break;
				case 0b0111:
				case 0b1000:
					addLineSeg(pos, scalars, omegas, 0b1100);
					break;
				}
			}
	}
};

#endif // !ISOPLETH_WIDGET_H
