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
		}

	protected:
		virtual void resizeEvent(QResizeEvent* ev) override
		{
			AdjustViewportToFit();
			QGraphicsView::resizeEvent(ev);
		}
	};

	std::array<uint32_t, 3> dim;
	uint32_t dimYxX;
	uint32_t currZ;
	float isoVal;
	osg::Vec2 voxSz;
	std::array<float, 2> scaleImgToVol;
	std::shared_ptr<std::vector<float>> volDat;

	QGraphicsScene isoplethScn;
	QImage isopleth;
	View isoplethView;

public:
	IsoplethWidget(int w, int h, QWidget* parent = nullptr)
		: QWidget(parent),
		isopleth(w, h, QImage::Format_RGBA8888),
		isoplethView(this, w, h)
	{
		isoVal = -1.f;

		setLayout(new QVBoxLayout);
		layout()->addWidget(&isoplethView);
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

		voxSz = osg::Vec2(1000.f / dim[0], 1000.f / dim[1]);

		scaleImgToVol[0] = float(dim[0]) / isopleth.width();
		scaleImgToVol[1] = float(dim[1]) / isopleth.height();
	}

	void Update(float isoVal, uint32_t volZ)
	{
		if (this->isoVal != isoVal || currZ != volZ) {
			this->isoVal = isoVal;
			currZ = volZ;
			marchingSquare(isoVal, volZ);
		}

		isopleth.fill(Qt::transparent);
		QPainter painter(&isopleth);
		painter.setRenderHint(QPainter::Antialiasing);
		isoplethScn.render(&painter);

		isoplethView.UpdateFrom(isopleth);
	}

private:
	void marchingSquare(float isoVal, uint32_t volZ) {
		isoplethScn.clear();

		auto volDimYxX = static_cast<size_t>(dim[1]) * dim[0];
		auto vertInterp = [&](const osg::Vec2& p0, const osg::Vec2& p1,
			float t) -> osg::Vec2 {
				auto oneMinusT = 1.f - t;
				return osg::Vec2(
					oneMinusT * p0.x() + t * p1.x(),
					oneMinusT * p0.y() + t * p1.y());
		};
		auto addTriangle = [&](
			const osg::Vec2& v0, const osg::Vec2& v1, const osg::Vec2& v2) {
				QPolygonF poly;
				poly.append(QPointF(v0.x(), 1.f - v0.y()));
				poly.append(QPointF(v1.x(), 1.f - v1.y()));
				poly.append(QPointF(v2.x(), 1.f - v2.y()));
				poly.append(poly.front());

				isoplethScn.addPolygon(poly);
		};
		auto addQuad = [&](
			const osg::Vec2& v0, const osg::Vec2& v1,
			const osg::Vec2& v2, const osg::Vec2& v3) {
				QPolygonF poly;
				poly.append(QPointF(v0.x(), 1.f - v0.y()));
				poly.append(QPointF(v1.x(), 1.f - v1.y()));
				poly.append(QPointF(v2.x(), 1.f - v2.y()));
				poly.append(QPointF(v3.x(), 1.f - v3.y()));
				poly.append(poly.front());

				isoplethScn.addPolygon(poly);
		};
		auto addPentagon = [&](
			const osg::Vec2& v0, const osg::Vec2& v1,
			const osg::Vec2& v2, const osg::Vec2& v3,
			const osg::Vec2& v4) {
				QPolygonF poly;
				poly.append(QPointF(v0.x(), 1.f - v0.y()));
				poly.append(QPointF(v1.x(), 1.f - v1.y()));
				poly.append(QPointF(v2.x(), 1.f - v2.y()));
				poly.append(QPointF(v3.x(), 1.f - v3.y()));
				poly.append(QPointF(v4.x(), 1.f - v4.y()));
				poly.append(poly.front());

				isoplethScn.addPolygon(poly);
		};
		auto mcsqrVox = [&](uint32_t x, uint32_t y) {
			/* об╠Й
			*  ^
			* [2]--3--[3]
			*  |      |
			*  2      1
			*  |      |
			* [0]--0--[1] >
			*/
			auto surfStart = volDat->data() + volZ * volDimYxX;
			std::array<float, 4> val4 = {
				surfStart[y * dim[0] + x],
				surfStart[y * dim[0] + x + 1],
				surfStart[(y + 1) * dim[0] + x],
				surfStart[(y + 1) * dim[0] + x + 1]
			};
			std::array<float, 4> t4 = {
				(isoVal - val4[0]) / (val4[1] - val4[0]),
				(isoVal - val4[1]) / (val4[3] - val4[1]),
				(isoVal - val4[0]) / (val4[2] - val4[0]),
				(isoVal - val4[2]) / (val4[3] - val4[2])
			};
			std::array<osg::Vec2, 4> pos4;
			pos4[0] = osg::Vec2(x * voxSz.x(), y * voxSz.y());
			pos4[1] = pos4[0] + osg::Vec2(voxSz.x(), 0.f);
			pos4[2] = pos4[0] + osg::Vec2(0.f, voxSz.y());
			pos4[3] = pos4[0] + osg::Vec2(voxSz.x(), voxSz.y());
			std::array<osg::Vec2, 4> vert4 = {
				vertInterp(pos4[0], pos4[1], t4[0]),
				vertInterp(pos4[1], pos4[3], t4[1]),
				vertInterp(pos4[0], pos4[2], t4[2]),
				vertInterp(pos4[2], pos4[3], t4[3])
			};

			uint8_t flag = 0;
			if (val4[0] >= isoVal) flag |= 0b0001;
			if (val4[1] >= isoVal) flag |= 0b0010;
			if (val4[2] >= isoVal) flag |= 0b0100;
			if (val4[3] >= isoVal) flag |= 0b1000;
			switch (flag) {
			case 0: break;
			case 1: addTriangle(vert4[0], vert4[2], pos4[0]); break;
			case 2: addTriangle(vert4[1], vert4[0], pos4[1]); break;
			case 4: addTriangle(vert4[2], vert4[3], pos4[2]); break;
			case 8: addTriangle(vert4[3], vert4[1], pos4[3]); break;
			case 3: addQuad(pos4[1], vert4[1], vert4[2], pos4[0]); break;
			case 5: addQuad(vert4[0], vert4[3], pos4[2], pos4[0]); break;
			case 10: addQuad(pos4[1], pos4[3], vert4[3], vert4[0]); break;
			case 12: addQuad(vert4[1], pos4[3], pos4[2], vert4[2]); break;
			case  7: addPentagon(pos4[1], vert4[1], vert4[3], pos4[2], pos4[0]); break;
			case 11: addPentagon(pos4[3], vert4[3], vert4[2], pos4[0], pos4[1]); break;
			case 13: addPentagon(pos4[0], vert4[0], vert4[1], pos4[3], pos4[2]); break;
			case 14: addPentagon(pos4[2], vert4[2], vert4[0], pos4[1], pos4[3]); break;
			case 6:
				addTriangle(vert4[1], vert4[0], pos4[1]);
				addTriangle(vert4[2], vert4[3], pos4[2]);
				break;
			case 9:
				addTriangle(vert4[0], vert4[2], pos4[0]);
				addTriangle(vert4[3], vert4[1], pos4[3]);
				break;
			}
		};

		for (uint32_t y = 0; y < dim[1] - 1; ++y)
			for (uint32_t x = 0; x < dim[0] - 1; ++x)
				mcsqrVox(x, y);
	}
};

#endif // !ISOPLETH_WIDGET_H
