#ifndef VOL_IO_WIDGET_H
#define VOL_IO_WIDGET_H

#include <cmath>
#include <array>
#include <limits>
#include <vector>

#include <QtWidgets/qfiledialog.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qcolordialog.h>

#include <ui_vol_io_widget.h>

#include <common_gui/tf_widget.h>

#include <scivis/io/vol_io.h>
#include <scivis/io/tf_io.h>

namespace Ui
{
	class VolumeIOWidget;
}

class VolumeIOWidget : public QWidget
{
	Q_OBJECT

private:
	std::array<uint32_t, 3> dim;
	std::array<float, 2> scalarRng;
	std::vector<float> vol;

	uint8_t prevScalar;
	std::array<float, 4> prevColor;

	QImage heatMap;
	TransferFunctionWidget tfWdgt;
	Ui::VolumeIOWidget ui;

	enum class ComboBoxIndex_TFSrc : int
	{
		NoSRC,
		FromFileOrCustomed,
		BlueToRed
	};

public:
	VolumeIOWidget(QWidget* parent = nullptr) : QWidget(parent), heatMap(100, 80, QImage::Format_RGB32)
	{
		ui.setupUi(this);

		reinterpret_cast<QVBoxLayout*>(ui.groupBox_TF->layout())->insertWidget(2, &tfWdgt);

		dim[0] = dim[1] = dim[2] = 0;

		connect(ui.horizontalSlider_HeatMapZ, &QSlider::valueChanged, [&](int val) {
			ui.label_HeatMapZ->setText(QString::number(val));
			updateHeatMap();
			});

		connect(ui.pushButton_ImportRAW, &QPushButton::clicked, this, &VolumeIOWidget::importRAWVolume);
		connect(ui.pushButton_ImportTXT, &QPushButton::clicked, this, &VolumeIOWidget::importTXTVolume);
		connect(ui.pushButton_ImportLabeledTXT, &QPushButton::clicked, this, &VolumeIOWidget::importLabeledTXTVolume);
		connect(ui.pushButton_ImportTF, &QPushButton::clicked, this, &VolumeIOWidget::importTransferFunction);

		connect(ui.comboBox_TFSrc,
			static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &VolumeIOWidget::updateHeatMap);
		connect(&tfWdgt, &TransferFunctionWidget::PointPicked, [&](uint8_t scalar) {
			ui.label_TFPointPicked->setText(QString("%1 -> --").arg(static_cast<int>(scalar)));
			});
		connect(&tfWdgt, &TransferFunctionWidget::PointPlaced, [&](uint8_t from, uint8_t to) {
			ui.label_TFPointPicked->setText(QString("%1 -> %2").arg(static_cast<int>(from)).arg(static_cast<int>(to)));
			});
		connect(&tfWdgt, &TransferFunctionWidget::TransferFunctionChanged,
			[&](uint8_t scalar, const std::array<float, 4>& color) {
				prevScalar = scalar;
				prevColor = color;

				updateHeatMap();
			});
		connect(ui.pushButton_TFPointColored, &QPushButton::clicked, [&]() {
			QColor prevQCol(
				static_cast<int>(255.f * prevColor[0]),
				static_cast<int>(255.f * prevColor[1]),
				static_cast<int>(255.f * prevColor[2]));
			auto qCol = QColorDialog::getColor(prevQCol);
			if (!qCol.isValid()) return;

			prevColor[0] = static_cast<float>(qCol.red()) / 255.f;
			prevColor[1] = static_cast<float>(qCol.green()) / 255.f;
			prevColor[2] = static_cast<float>(qCol.blue()) / 255.f;
			tfWdgt.SetTransferFunctionPointColor(prevScalar, prevColor);

			update();
			});

		connect(ui.pushButton_ExportVolume, &QPushButton::clicked, this, &VolumeIOWidget::exportRAWVolume);
		connect(ui.pushButton_ExportTF, &QPushButton::clicked, this, &VolumeIOWidget::exportTransferFunction);
	}

	virtual void resizeEvent(QResizeEvent* ev) override
	{
		updateHeatMap();
		QWidget::resizeEvent(ev);
	}

private:
	void updateHeatMap()
	{
		auto newWid = std::min(width(), height());
		newWid >>= 1;
		heatMap = heatMap.scaledToWidth(newWid);

		if (dim[0] == 0 || dim[1] == 0 || dim[2] == 0) {
			heatMap.fill(Qt::gray);
			ui.label_HeatMap->setPixmap(QPixmap::fromImage(heatMap));
			return;
		}

		std::array<float, 2> scaleImgToVol = {
			float(dim[0]) / heatMap.width(),
			float(dim[1]) / heatMap.height()
		};
		auto volZ = static_cast<size_t>(ui.horizontalSlider_HeatMapZ->value());
		auto dimYxX = static_cast<size_t>(dim[1]) * dim[0];
		for (int y = 0; y < heatMap.height(); ++y) {
			auto pxPtr = reinterpret_cast<QRgb*>(heatMap.scanLine(y));
			for (int x = 0; x < heatMap.width(); ++x, ++pxPtr) {
				auto volY = static_cast<uint32_t>(floorf(y * scaleImgToVol[1]));
				volY = dim[1] - 1 - volY;
				auto volX = static_cast<uint32_t>(floorf(x * scaleImgToVol[0]));

				auto volIdx = volZ * dimYxX + volY * dim[0] + volX;
				auto scalar = vol[volIdx];

				if (ui.comboBox_TFSrc->currentIndex() == static_cast<int>(ComboBoxIndex_TFSrc::NoSRC))
					*pxPtr = qRgb(scalar * 255.f, scalar * 255.f, scalar * 255.f);
				else {
					auto& color = tfWdgt.GetTransferFunctionColor(scalar * 255.f);
					*pxPtr = qRgb(color[0] * 255.f, color[1] * 255.f, color[2] * 255.f);
				}
			}
		}

		ui.label_HeatMap->setPixmap(QPixmap::fromImage(heatMap));
	}


	void importRAWVolume()
	{
		clearVolumeData();

		auto filePath = QFileDialog::getOpenFileName(
			nullptr, tr("Open RAW File"), "./", tr("Binary (*.raw *.bin *.dat)"));
		if (filePath.isEmpty()) return;
		std::string errMsg;
		auto dim = readDimensionFromUI();
		auto u8Dat = SciVis::Loader::RAWVolume::LoadFromFile(filePath.toStdString(), dim, &errMsg);
		if (!errMsg.empty()) {
			ui.label_ImportRAW->setText(tr(errMsg.c_str()));
			return;
		}

		ui.label_ImportRAW->setText(filePath);
		this->dim = dim;
		this->vol = SciVis::Convertor::RAWVolume::U8ToFloat(u8Dat);

		updateHeatMap();
	}

	void importTXTVolume()
	{

	}

	void importLabeledTXTVolume()
	{
	}

	void importTransferFunction()
	{
		auto filePath = QFileDialog::getOpenFileName(
			nullptr, tr("Open TXT File"), "./", tr("Transfer Function (*.txt)"));
		std::string errMsg;
		auto pnts = SciVis::Loader::TransferFunctionPoints::LoadFromFile(filePath.toStdString(), &errMsg);
		if (!errMsg.empty()) {
			ui.label_ImportTF->setText(tr(errMsg.c_str()));
			return;
		}

		ui.label_ImportTF->setText(filePath);
		ui.comboBox_TFSrc->setCurrentIndex(static_cast<int>(ComboBoxIndex_TFSrc::FromFileOrCustomed));

		tfWdgt.SetTransferFunctionPointsData(pnts);
		updateHeatMap();
	}

	void exportRAWVolume()
	{

	}

	void exportTransferFunction()
	{
		auto filePath = QFileDialog::getSaveFileName(
			nullptr, tr("Open TXT File"), "./", tr("Transfer Function (*.txt)"));
		if (filePath.isEmpty()) return;

		std::vector<std::pair<uint8_t, std::array<float, 4>>> tfPntsDat;
		for (uint32_t s = 0; s <= 255; ++s) {
			auto color = tfWdgt.GetTransferFunctionPointColor(s);
			if (color[3] >= 0.f)
				tfPntsDat.emplace_back(std::make_pair(static_cast<uint8_t>(s), color));
		}

		SciVis::Loader::TransferFunctionPoints::DumpToFile(filePath.toStdString(), tfPntsDat);
	}

	void clearVolumeData()
	{
		scalarRng[0] = std::numeric_limits<float>::min();
		scalarRng[1] = std::numeric_limits<float>::max();
		vol.clear();
	}

	std::array<uint32_t, 3> readDimensionFromUI()
	{
		std::array<uint32_t, 3> dim;
		dim[0] = ui.spinBox_DimX->value();
		dim[1] = ui.spinBox_DimY->value();
		dim[2] = ui.spinBox_DimZ->value();
		return dim;
	}
};

#endif // !VOL_IO_WIDGET_H
