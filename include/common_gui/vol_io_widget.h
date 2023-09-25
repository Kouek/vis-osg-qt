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

#include <scivis/io/vol_io.h>

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
	QImage heatMap;

	Ui::VolumeIOWidget ui;

public:
	VolumeIOWidget(QWidget* parent = nullptr) : QWidget(parent), heatMap(100, 100, QImage::Format_RGB32)
	{
		ui.setupUi(this);

		dim[0] = dim[1] = dim[2] = 0;

		connect(ui.horizontalSlider_HeatMapZ, &QSlider::valueChanged, this, &VolumeIOWidget::UpdateHeatMap);
		connect(ui.horizontalSlider_HeatMapZ, &QSlider::valueChanged, [&](int val) {
			ui.label_HeatMapZ->setText(QString::number(val));
			});

		connect(ui.pushButton_ImportRAW, &QPushButton::clicked, this, &VolumeIOWidget::LoadRAWVolume);
		connect(ui.pushButton_ImportTXT, &QPushButton::clicked, this, &VolumeIOWidget::LoadTXTVolume);
		connect(ui.pushButton_ImportLabeledTXT, &QPushButton::clicked, this, &VolumeIOWidget::LoadLabeledTXTVolume);
		connect(ui.pushButton_ImportTF, &QPushButton::clicked, this, &VolumeIOWidget::LoadTransferFunction);
	}

	void UpdateHeatMap()
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
				*pxPtr = qRgb(scalar * 255.f, scalar * 255.f, scalar * 255.f);
			}
		}

		ui.label_HeatMap->setPixmap(QPixmap::fromImage(heatMap));
	}


	void LoadRAWVolume()
	{
		clearVolumeData();

		auto filePath = QFileDialog::getOpenFileName(nullptr, tr("Open RAW File"), "./", tr("Binary (*.raw *.bin *.dat)"));
		if (filePath.isEmpty()) return;
		std::string errMsg;
		auto dim = readDimensionFromUI();
		auto u8Dat = SciVis::Loader::RAWVolume::LoadFromFile(filePath.toStdString(), dim, &errMsg);
		if (!errMsg.empty()) {
			ui.label_ImportRAW->setText(tr(errMsg.c_str()));
			return;
		}

		this->dim = dim;
		this->vol = SciVis::Convertor::RAWVolume::U8ToFloat(u8Dat);

		UpdateHeatMap();
	}

	void LoadTXTVolume()
	{

	}

	void LoadLabeledTXTVolume()
	{
	}

	void LoadTransferFunction()
	{

	}

	virtual void resizeEvent(QResizeEvent* ev) override
	{
		UpdateHeatMap();
		QWidget::resizeEvent(ev);
	}

private:
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
