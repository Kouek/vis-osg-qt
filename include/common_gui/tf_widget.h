#ifndef TF_WIDGET_H
#define TF_WIDGET_H

#include <array>
#include <vector>

#include <QtGui/qevent.h>
#include <QtWidgets/qcolordialog.h>
#include <QtWidgets/qgraphicsitem.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qgraphicssceneevent.h>

#include <ui_tf_widget.h>

class TransferFunctionView : public QGraphicsView
{
	Q_OBJECT

public:
	static constexpr qreal AxHeight = 256. / 4.;

public:
	TransferFunctionView(QGraphicsScene* scn, QWidget* parent) : QGraphicsView(scn, parent)
	{}

	void AdjustViewportToFit()
	{
		fitInView(scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
	}

	virtual void resizeEvent(QResizeEvent* ev) override
	{
		AdjustViewportToFit();
		QGraphicsView::resizeEvent(ev);
	}
};

namespace Ui
{
	class TransferFunctionWidget;
}

class TransferFunctionWidget : public QWidget
{
	Q_OBJECT

private:
	class Point : public QGraphicsRectItem
	{
	private:
		uint8_t scalar;
		TransferFunctionWidget* tfWdgt;

	public:
		Point(uint8_t scalar, const QColor& col, TransferFunctionWidget* tfWdgt)
			: QGraphicsRectItem(0., 0., 6.4, 6.4), scalar(scalar), tfWdgt(tfWdgt)
		{
			setPen(QPen(Qt::white, 1.));
			setBrush(QBrush(col));
		}

		void mousePressEvent(QGraphicsSceneMouseEvent* ev)override
		{
			QGraphicsRectItem::mousePressEvent(ev);
			tfWdgt->PointPicked(scalar);
		}

		void mouseReleaseEvent(QGraphicsSceneMouseEvent* ev) override
		{
			QGraphicsRectItem::mouseReleaseEvent(ev);

			auto validPos = [&]() -> QPointF {
				auto p = scenePos();
				if (p.x() < 0.)
					p.setX(0.);
				if (p.x() > 255.)
					p.setX(255.);
				if (p.y() < 0.)
					p.setY(0.);
				if (p.y() > TransferFunctionView::AxHeight)
					p.setY(TransferFunctionView::AxHeight);

				// 不改变两端的x值
				if (scalar == 0)
					p.setX(0.);
				else if (scalar == 255)
					p.setX(255.);

				return p;
			}();
			setPos(validPos);

			auto currScalar = floorf(validPos.x());
			if (currScalar == scalar) {
				tfWdgt->tfPntsDat[scalar][3] = 1. - validPos.y() / TransferFunctionView::AxHeight;
				tfWdgt->updatePointFromData(scalar);
			}
			else {
				tfWdgt->tfPntsDat[currScalar] = tfWdgt->tfPntsDat[scalar];
				tfWdgt->tfPntsDat[scalar][3] = -1.f;
				tfWdgt->tfPntsDat[currScalar][3] = 1. - validPos.y() / TransferFunctionView::AxHeight;

				tfWdgt->updatePointFromData(scalar);
				tfWdgt->updatePointFromData(currScalar);
			}

			tfWdgt->PointPlaced(scalar, currScalar);
			tfWdgt->TransferFunctionChanged(currScalar, tfWdgt->tfPntsDat[currScalar]);
			scalar = currScalar;
		}
	};

	uint8_t prevScalar;
	std::array<float, 4> prevColor;

	std::array<int32_t, 2> tfDatDirtyRng; // 用于确定tfDat需要更新的范围
	std::array<std::array<float, 4>, 256 > tfDat; // 稠密数据，依赖于tfPntsDat，只用于输出

	std::array<std::array<float, 4>, 256> tfPntsDat; // 稀疏关键点，[i][3]<0 表示i处为空
	std::array<Point*, 256> pnts; // 依赖于tfPntsDat，只用于显示和交互

	QGraphicsScene scn;
	TransferFunctionView view;

	Ui::TransferFunctionWidget ui;

public:
	TransferFunctionWidget(QWidget* parent = nullptr)
		: QWidget(parent), view(&scn, this)
	{
		ui.setupUi(this);
		reinterpret_cast<QGridLayout*>(layout())
			->addWidget(&view, 1, 0, 1, 2);

		tfDatDirtyRng[0] = tfDatDirtyRng[1] = -1;

		scn.setBackgroundBrush(QBrush(Qt::black));
		{
			auto x = .1 * 255.;
			auto y = .1 * TransferFunctionView::AxHeight;

			QPen pen(Qt::red, 1.);
			scn.addLine(-x, 0., 255. + x, 0., pen);
			scn.addLine(-x, TransferFunctionView::AxHeight, 255. + x,
				TransferFunctionView::AxHeight, pen);

			pen.setColor(Qt::green);
			scn.addLine(0., -y, 0., TransferFunctionView::AxHeight + y, pen);
			scn.addLine(255., -y, 255., TransferFunctionView::AxHeight + y, pen);
		}

		connect(this, &TransferFunctionWidget::TransferFunctionChanged,
			[&](uint8_t scalar, const std::array<float, 4>& color) {
				if (tfDatDirtyRng[0] == -1)
					tfDatDirtyRng[0] = scalar;
				else if (tfDatDirtyRng[0] > scalar)
					tfDatDirtyRng[0] = scalar;

				if (tfDatDirtyRng[1] == -1)
					tfDatDirtyRng[1] = scalar;
				else if (tfDatDirtyRng[1] < scalar)
					tfDatDirtyRng[1] = scalar;
			});

		connect(this, &TransferFunctionWidget::PointPicked, [&](uint8_t scalar) {
			prevScalar = scalar;
			prevColor = tfPntsDat[scalar];

			ui.label_TFPointMoved->setText(QString("%1 -> -- (--, --, --, --)")
				.arg(static_cast<int>(scalar)));
			});
		connect(this, &TransferFunctionWidget::TransferFunctionChanged,
			[&](uint8_t scalar, const std::array<float, 4>& color) {
				ui.label_TFPointMoved->setText(QString("%1 -> %2 (%3, %4, %5, %6)")
					.arg(static_cast<int>(prevScalar)).arg(static_cast<int>(scalar))
					.arg(static_cast<int>(255.f * color[0]))
					.arg(static_cast<int>(255.f * color[1]))
					.arg(static_cast<int>(255.f * color[2]))
					.arg(static_cast<int>(255.f * color[3])));

				prevScalar = scalar;
				prevColor = color;
			});
		connect(ui.pushButton_ColorTFPoint, &QPushButton::clicked, [&]() {
			QColor prevQCol(
				static_cast<int>(255.f * prevColor[0]),
				static_cast<int>(255.f * prevColor[1]),
				static_cast<int>(255.f * prevColor[2]));
			auto qCol = QColorDialog::getColor(prevQCol);
			if (!qCol.isValid()) return;

			prevColor[0] = static_cast<float>(qCol.red()) / 255.f;
			prevColor[1] = static_cast<float>(qCol.green()) / 255.f;
			prevColor[2] = static_cast<float>(qCol.blue()) / 255.f;
			SetTransferFunctionPointColor(prevScalar, prevColor);
			});
	}

	void SetTransferFunctionPointsData(const std::vector<std::pair<uint8_t, std::array<float, 4>>>& tfPntsDat)
	{
		std::array<float, 4> nullRGBA = { -1.f, -1.f, -1.f, -1.f };
		this->tfPntsDat.fill(nullRGBA);
		this->pnts.fill(nullptr);

		for (uint32_t s = 0; s < 256; ++s)
			if (pnts[s]) {
				scn.removeItem(pnts[s]);
				pnts[s] = nullptr;
			}

		for (auto itr = tfPntsDat.begin(); itr != tfPntsDat.end(); ++itr)
			this->tfPntsDat[itr->first] = itr->second;
		for (uint32_t s = 0; s < 256; ++s)
			updatePointFromData(s);

		// 所有稠密数据均需要更新
		tfDatDirtyRng[0] = 0;
		tfDatDirtyRng[1] = 255;
	}

	void SetTransferFunctionPointColor(uint8_t scalar, const std::array<float, 4>& color)
	{
		tfPntsDat[scalar] = color;
		updatePointFromData(scalar);

		emit TransferFunctionChanged(scalar, color);
	}

	const std::array<float, 4>& GetTransferFunctionPointColor(uint8_t scalar) const
	{
		return tfPntsDat[scalar];
	}

	const std::array<float, 4>& GetTransferFunctionColor(uint8_t scalar)
	{
		updateTransferFunctionData();
		return tfDat[scalar];
	}

signals:
	void PointPicked(uint8_t scalar);
	void PointPlaced(uint8_t fromScalar, uint8_t toScalar);
	void TransferFunctionChanged(uint8_t scalar, const std::array<float, 4>& color);

private:
	void updatePointFromData(uint8_t scalar)
	{
		if (tfPntsDat[scalar][3] < 0.f) {
			auto ptr = pnts[scalar];
			if (ptr) {
				scn.removeItem(ptr);
				pnts[scalar] = nullptr;
			}
			return;
		}

		auto color = QColor(
			static_cast<int>(255.f * tfPntsDat[scalar][0]),
			static_cast<int>(255.f * tfPntsDat[scalar][1]),
			static_cast<int>(255.f * tfPntsDat[scalar][2]));
		if (!pnts[scalar]) {
			auto ptr = new Point(scalar, color, this);
			ptr->setFlag(QGraphicsItem::ItemIsMovable);
			scn.addItem(ptr);
			ptr->setPos(scalar, TransferFunctionView::AxHeight * (1.f - tfPntsDat[scalar][3]));

			pnts[scalar] = ptr;
		}
		else {
			auto ptr = pnts[scalar];
			ptr->setPos(ptr->pos().x(), TransferFunctionView::AxHeight * (1.f - tfPntsDat[scalar][3]));
			ptr->setBrush(QBrush(color));
		}
	}

	void updateTransferFunctionData()
	{
		// 不需要线性插值
		if (tfDatDirtyRng[0] == -1) return;

		uint8_t lft = (tfDatDirtyRng[0] > 0) ? tfDatDirtyRng[0] - 1 : 0;
		while (tfPntsDat[lft][3] < 0.f)
			--lft;
		uint8_t rhtEnd = (tfDatDirtyRng[1] < 255) ? tfDatDirtyRng[1] + 1 : 255;
		while (tfPntsDat[rhtEnd][3] < 0.f)
			++rhtEnd;

		// 使用线性插值，将稀疏的tfPntsDat填充到稠密的tfDat
		uint8_t rht = lft + 1;
		while (true) {
			while (rht < rhtEnd && tfPntsDat[rht][3] < 0.f)
				++rht;
			int32_t curr = lft;
			float div = rht - lft;
			while (curr <= rht) {
				auto t = (curr - lft) / div;
				auto oneMinusT = 1.f - t;
				tfDat[curr][0] = oneMinusT * tfPntsDat[lft][0] + t * tfPntsDat[rht][0];
				tfDat[curr][1] = oneMinusT * tfPntsDat[lft][1] + t * tfPntsDat[rht][1];
				tfDat[curr][2] = oneMinusT * tfPntsDat[lft][2] + t * tfPntsDat[rht][2];
				tfDat[curr][3] = oneMinusT * tfPntsDat[lft][3] + t * tfPntsDat[rht][3];

				++curr;
			}

			lft = rht;
			if (lft == rhtEnd) break;
			++rht;
		}

		tfDatDirtyRng[0] = tfDatDirtyRng[1] = -1;
	}
};

#endif // !TF_WIDGET_H
