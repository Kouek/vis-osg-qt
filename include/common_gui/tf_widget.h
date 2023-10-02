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

namespace Ui
{
	class TransferFunctionWidget;
}

class TransferFunctionWidget : public QWidget
{
	Q_OBJECT

private:
	class View : public QGraphicsView
	{
	public:
		static constexpr qreal AxHeight = 256. / 4.;

	private:
		TransferFunctionWidget* tfWdgt;
		bool isCTRLPressed;

	public:
		View(QGraphicsScene* scn, TransferFunctionWidget* tfWdgt)
			: QGraphicsView(scn, tfWdgt),
			tfWdgt(tfWdgt),
			isCTRLPressed(false)
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


		virtual void keyPressEvent(QKeyEvent* ev) override
		{
			if (ev->key() == Qt::Key_Control)
				isCTRLPressed = true;
			QWidget::keyPressEvent(ev);
		}

		virtual void keyReleaseEvent(QKeyEvent* ev) override
		{
			if (ev->key() == Qt::Key_Control)
				isCTRLPressed = false;
			QWidget::keyReleaseEvent(ev);
		}

		virtual void mousePressEvent(QMouseEvent* ev) override
		{
			if (isCTRLPressed) {
				// 添加关键点
				auto scenePos = mapToScene(ev->pos());
				auto scalar = static_cast<uint8_t>(scenePos.x());
				if (scalar == 0 || scalar == 255) return;

				auto& color = tfWdgt->tfPntsDat[scalar];
				color[0] = color[1] = color[2] = color[3] = 1.f;
				tfWdgt->updatePointFromData(scalar);

				tfWdgt->TransferFunctionChanged(scalar, tfWdgt->tfPntsDat[scalar]);
			}
			else
				QGraphicsView::mousePressEvent(ev);
		}

		virtual void mouseReleaseEvent(QMouseEvent* ev) override
		{
			if (!isCTRLPressed)
				QGraphicsView::mouseReleaseEvent(ev);
		}
	};

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
				if (p.y() > View::AxHeight)
					p.setY(View::AxHeight);

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
				tfWdgt->tfPntsDat[scalar][3] = 1. - validPos.y() / View::AxHeight;
				tfWdgt->updatePointFromData(scalar);
			}
			else {
				tfWdgt->tfPntsDat[currScalar] = tfWdgt->tfPntsDat[scalar];
				tfWdgt->tfPntsDat[scalar][3] = -1.f;
				tfWdgt->tfPntsDat[currScalar][3] = 1. - validPos.y() / View::AxHeight;

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
	View view;

	Ui::TransferFunctionWidget ui;

public:
	TransferFunctionWidget(QWidget* parent = nullptr)
		: QWidget(parent), view(&scn, this)
	{
		ui.setupUi(this);
		reinterpret_cast<QGridLayout*>(layout())
			->addWidget(&view, 1, 0, 1, 2);

		{
			std::array<float, 4> nullRGBA = { -1.f, -1.f, -1.f, -1.f };
			this->tfPntsDat.fill(nullRGBA);
			this->pnts.fill(nullptr);
			// 设置首尾端点
			tfPntsDat[0] = std::array<float, 4>{0.f, 0.f, 0.f, 0.f};
			tfPntsDat[255] = std::array<float, 4>{1.f, 1.f, 1.f, 1.f};
			// 所有稠密数据均需要更新
			tfDatDirtyRng[0] = 0;
			tfDatDirtyRng[1] = 255;
			// 设置无交互点
			prevColor = std::array<float, 4>{-1.f, -1.f, -1.f, -1.f};

			updatePointFromData(0);
			updatePointFromData(255);
		}

		scn.setBackgroundBrush(QBrush(Qt::black));
		{
			auto x = .1 * 255.;
			auto y = .1 * View::AxHeight;

			QPen pen(Qt::red, 1.);
			scn.addLine(-x, 0., 255. + x, 0., pen);
			scn.addLine(-x, View::AxHeight, 255. + x, View::AxHeight, pen);

			pen.setColor(Qt::green);
			scn.addLine(0., -y, 0., View::AxHeight + y, pen);
			scn.addLine(255., -y, 255., View::AxHeight + y, pen);
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
		connect(ui.pushButton_DeleteTFPoint, &QPushButton::clicked, [&]() {
			if (prevColor[3] < 0.f) return; // 无交互点
			if (prevScalar == 0 || prevScalar == 255) return; // 不能删除首尾端点

			tfPntsDat[prevScalar][3] = -1.f;
			updatePointFromData(prevScalar);

			prevColor[3] = -1.f; // 清除交互点
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

		for (auto itr = tfPntsDat.begin(); itr != tfPntsDat.end(); ++itr)
			this->tfPntsDat[itr->first] = itr->second;
		for (uint32_t s = 0; s < 256; ++s)
			updatePointFromData(s);

		prevColor = std::array<float, 4>{-1.f, -1.f, -1.f, -1.f};

		tfDatDirtyRng[0] = 0;
		tfDatDirtyRng[1] = 255;
	}

	void SetTransferFunctionPointColor(uint8_t scalar, const std::array<float, 4>& color)
	{
		tfPntsDat[scalar] = color;
		updatePointFromData(scalar);

		emit TransferFunctionChanged(scalar, color);
	}

	std::vector<std::pair<uint8_t, std::array<float, 4>>> GetTransferFunctionPointsData() const
	{
		std::vector<std::pair<uint8_t, std::array<float, 4>>> tfPntsDat;
		for (uint32_t s = 0; s <= 255; ++s) {
			auto color = GetTransferFunctionPointColor(s);
			if (color[3] >= 0.f)
				tfPntsDat.emplace_back(std::make_pair(static_cast<uint8_t>(s), color));
		}
		return tfPntsDat;
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
			ptr->setPos(scalar, View::AxHeight * (1.f - tfPntsDat[scalar][3]));

			pnts[scalar] = ptr;
		}
		else {
			auto ptr = pnts[scalar];
			ptr->setPos(ptr->pos().x(), View::AxHeight * (1.f - tfPntsDat[scalar][3]));
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
