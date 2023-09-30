#ifndef VOL_IO_WIDGET_H
#define VOL_IO_WIDGET_H

#include <cmath>
#include <array>
#include <limits>
#include <vector>

#include <QtWidgets/qfiledialog.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qwidget.h>

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

		ui.groupBox_TF->layout()->addWidget(&tfWdgt);

		dim[0] = dim[1] = dim[2] = 0;

		connect(ui.horizontalSlider_HeatMapZ, &QSlider::valueChanged, [&](int val) {
			ui.label_HeatMapZ->setText(QString::number(val));
			updateHeatMap();
			});

		connect(ui.checkBox_UseValRngForNorm, &QCheckBox::stateChanged, [&](int state) {
			if (state == Qt::Checked) {
				ui.doubleSpinBox_MinValForNorm->setEnabled(true);
				ui.doubleSpinBox_MaxValForNorm->setEnabled(true);
			}
			else {
				ui.doubleSpinBox_MinValForNorm->setEnabled(false);
				ui.doubleSpinBox_MaxValForNorm->setEnabled(false);
			}
			});

		connect(ui.pushButton_ImportRAW, &QPushButton::clicked, this, &VolumeIOWidget::importRAWVolume);
		connect(ui.pushButton_ImportTXT, &QPushButton::clicked, this, &VolumeIOWidget::importTXTVolume);
		connect(ui.pushButton_ImportLabeledTXT, &QPushButton::clicked, this, &VolumeIOWidget::importLabeledTXTVolume);
		connect(ui.pushButton_ImportTF, &QPushButton::clicked, this, &VolumeIOWidget::importTransferFunction);

		connect(ui.comboBox_TFSrc,
			static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &VolumeIOWidget::updateHeatMap);

		connect(&tfWdgt, &TransferFunctionWidget::TransferFunctionChanged, this, &VolumeIOWidget::updateHeatMap);

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
		auto u8Dat = SciVis::Loader::RAWVolume::LoadU8FromFile(filePath.toStdString(), dim, &errMsg);
		if (!errMsg.empty()) {
			ui.label_ImportedRAW->setText(tr(errMsg.c_str()));
			return;
		}

		ui.label_ImportedDim->setText("--, --, --");
		ui.label_ImportedLLHRng->setText("[--, --], [--, --], [--, --]");
		ui.label_ImportedValRng->setText("[--, --]");

		ui.label_ImportedRAW->setText(filePath);
		ui.label_ImportedTXT->clear();
		ui.label_ImportedLabeledTXT->clear();
		ui.label_MinZ->setText(QString::number(0));
		ui.label_MaxZ->setText(QString::number(dim[2] - 1));
		ui.horizontalSlider_HeatMapZ->setRange(0, dim[2] - 1);
		this->dim = dim;
		this->vol = SciVis::Convertor::RAWVolume::U8ToNormalizedFloat(u8Dat);

		updateHeatMap();
	}

	void importTXTVolume()
	{

	}

	void importLabeledTXTVolume()
	{
		auto filePath = QFileDialog::getOpenFileName(
			nullptr, tr("Open TXT File"), "./", tr("Transfer Function (*.txt)"));
		std::string errMsg;
		auto vol = SciVis::Loader::LabeledTXTVolume::LoadFromFile(filePath.toStdString(), &errMsg);
		if (!errMsg.empty()) {
			ui.label_ImportedLabeledTXT->setText(tr(errMsg.c_str()));
			return;
		}
		if (!vol.isDense) {
			ui.label_ImportedLabeledTXT->setText(tr("Sparse Labeled TXT Volume is NOT Supported"));
			return;
		}

		ui.label_ImportedRAW->clear();
		ui.label_ImportedTXT->clear();

		ui.label_ImportedDim->setText(QString("%1, %2, %3").arg(vol.dim[0])
			.arg(vol.dim[1]).arg(vol.dim[2]));
		ui.label_ImportedLLHRng->setText(QString("[%1, %2], [%3, %4], [%5, %6]")
			.arg(static_cast<double>(vol.lonRng[0]), 0, 'f', 2)
			.arg(static_cast<double>(vol.lonRng[1]), 0, 'f', 2)
			.arg(static_cast<double>(vol.latRng[0]), 0, 'f', 2)
			.arg(static_cast<double>(vol.latRng[1]), 0, 'f', 2)
			.arg(static_cast<double>(vol.hRng[0]), 0, 'f', 2)
			.arg(static_cast<double>(vol.hRng[1]), 0, 'f', 2));
		ui.label_ImportedValRng->setText(QString("[%1, %2]")
			.arg(static_cast<double>(vol.valRng[0]), 0, 'f', 2)
			.arg(static_cast<double>(vol.valRng[1]), 0, 'f', 2));
		ui.label_ImportedLabeledTXT->setText(filePath);
		ui.label_MinZ->setText(QString::number(0));
		ui.label_MaxZ->setText(QString::number(vol.dim[2] - 1));
		ui.horizontalSlider_HeatMapZ->setRange(0, vol.dim[2] - 1);

		std::array<float, 2> userSpecifiedValRng = {
			static_cast<float>(ui.doubleSpinBox_MinValForNorm->value()),
			static_cast<float>(ui.doubleSpinBox_MaxValForNorm->value())
		};
		vol.Normalize(ui.checkBox_UseValRngForNorm->isChecked() ?
			&userSpecifiedValRng : nullptr);
		this->vol = vol.dat;
		this->dim = vol.dim;

		updateHeatMap();
	}

	void importTransferFunction()
	{
		auto filePath = QFileDialog::getOpenFileName(
			nullptr, tr("Open TXT File"), "./", tr("Transfer Function (*.txt)"));
		std::string errMsg;
		auto pnts = SciVis::Loader::TransferFunctionPoints::LoadFromFile(filePath.toStdString(), &errMsg);
		if (!errMsg.empty()) {
			ui.label_ImportedTF->setText(tr(errMsg.c_str()));
			return;
		}

		ui.label_ImportedTF->setText(filePath);
		ui.comboBox_TFSrc->setCurrentIndex(static_cast<int>(ComboBoxIndex_TFSrc::FromFileOrCustomed));

		tfWdgt.SetTransferFunctionPointsData(pnts);
		updateHeatMap();
	}

	void exportRAWVolume()
	{
		auto filePath = QFileDialog::getSaveFileName(
			nullptr, tr("Open RAW File"), "./", tr("Binary (*.raw *.bin *.dat)"));
		if (filePath.isEmpty()) return;

		auto u8Dat = SciVis::Convertor::RAWVolume::NormalizedFloatToU8(vol);
		SciVis::Loader::RAWVolume::DumpToFile(filePath.toStdString(), u8Dat);
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
