#ifndef HEAT_MAP_MAIN_WINDOW_H
#define HEAT_MAP_MAIN_WINDOW_H

#include <memory>

#ifdef DEPLOY_ON_ZHONGDIAN15
#include <grid/ui/uicomctrl/ccusbasedlg.h>
#else
#include <QtWidgets/qwidget.h>
using CCusBaseDlg = QWidget;
#endif // DEPLOY_ON_ZHONGDIAN15

#include <QtWidgets/qfiledialog.h>

#include <ui_hmp_main_window.h>

#include <common_gui/tf_widget.h>
#include <common_gui/heat_map_widget.h>

#include <scivis/io/tf_io.h>
#include <scivis/io/tf_osg_io.h>
#include <scivis/scalar_viser/heat_map_renderer.h>

namespace Ui
{
	class HMPMainWindow;
}

class HMPMainWindow : public CCusBaseDlg
{
	Q_OBJECT

private:
	QString tfFilePath;
	TransferFunctionWidget tfWdgt;
	HeatMapWidget heatMapWdgt;
	Ui::HMPMainWindow ui;

	std::shared_ptr<SciVis::ScalarViser::HeatMap3DRenderer> renderer;

public:
	HMPMainWindow(
		std::shared_ptr<SciVis::ScalarViser::HeatMap3DRenderer> renderer,
		QWidget* parent = nullptr)
        : CCusBaseDlg(parent), renderer(renderer), heatMapWdgt(500, 500)
	{
		setWindowFlags(
			Qt::WindowMinimizeButtonHint |
			Qt::WindowCloseButtonHint);

		ui.setupUi(this);

		ui.groupBox_TF->layout()->addWidget(&tfWdgt);
		ui.groupBox_TF->layout()->addWidget(&heatMapWdgt);

		connect(ui.pushButton_OpenTF, &QPushButton::clicked, this, &HMPMainWindow::openTFFromFile);
		connect(ui.pushButton_SaveTF, &QPushButton::clicked, this, &HMPMainWindow::saveTFToFile);

		connect(&tfWdgt, &TransferFunctionWidget::TransferFunctionChanged,
			[&]() {
				updateRendererColorTable();
				updateHeatMapWidget();
			});

		connect(ui.horizontalSlider_HeatMapH, &QSlider::valueChanged, [&](int val) {
			ui.label_HeatMapH->setText(QString::number(val));
			updateRendererHeight();
			updateHeatMapWidget();
			});
	}

	osg::ref_ptr<osg::Texture1D> GetTFTexture() const
	{
		auto tfPnts = tfWdgt.GetTransferFunctionPointsData();
		auto tex = SciVis::OSGConvertor::TransferFunctionPoints::ToTexture(tfPnts);
		return tex;
	}

	void UpdateFromRenderer()
	{
		if (renderer->GetVolumeNum() == 0) return;

		auto bgn = renderer->GetVolumes().begin();
		auto hRng = bgn->second.GetHeightFromCenterRange();
		auto h = bgn->second.GetHeightFromCenter();

		ui.label_MinH->setText(QString::number(static_cast<int>(floorf(hRng[0]))));
		ui.label_MaxH->setText(QString::number(static_cast<int>(floorf(hRng[1]))));
		ui.label_HeatMapH->setText(QString::number(static_cast<int>(floorf(h))));

		ui.horizontalSlider_HeatMapH->setRange(
			static_cast<int>(floorf(hRng[0])),
			static_cast<int>(floorf(hRng[1])));
		ui.horizontalSlider_HeatMapH->setValue(static_cast<int>(floorf(h)));
	}

	void SetVolume(std::shared_ptr<std::vector<float>> volDat, const std::array<uint32_t, 3>& dim)
	{
		heatMapWdgt.SetVolume(volDat, dim);
	}

	void SetTFTemplate(TransferFunctionWidget::TFTemplate tmplt)
	{
		tfWdgt.SetTFTemplate(tmplt);
	}

private:
	void openTFFromFile()
	{
		auto filePath = QFileDialog::getOpenFileName(
			this, tr("Open TXT File"), "./", tr("Transfer Function (*.txt)"));
		if (filePath.isEmpty()) return;

		std::string errMsg;
		auto pnts = SciVis::Loader::TransferFunctionPoints::LoadFromFile(filePath.toStdString(), &errMsg);
		if (!errMsg.empty()) {
			ui.label_OpenedTF->setText(tr(errMsg.c_str()));
			return;
		}

		ui.label_OpenedTF->setText(filePath);
		tfFilePath = filePath;

		tfWdgt.SetTransferFunctionPointsData(pnts);
	}

	void saveTFToFile()
	{
		auto tfPnts = tfWdgt.GetTransferFunctionPointsData();
		SciVis::Loader::TransferFunctionPoints::DumpToFile(
			tfFilePath.toStdString(), tfPnts);
	}

	void updateRendererHeight() {
		auto& vols = renderer->GetVolumes();
		for (auto itr = vols.begin(); itr != vols.end(); ++itr)
			itr->second.SetHeightFromCenter(ui.horizontalSlider_HeatMapH->value());
	}

	void updateRendererColorTable()
	{
		auto tex = GetTFTexture();
		auto& vols = renderer->GetVolumes();
		for (auto itr = vols.begin(); itr != vols.end(); ++itr)
			itr->second.SetColorTable(tex);
	}

	void updateHeatMapWidget()
	{
		uint32_t volZ =
			static_cast<float>(ui.horizontalSlider_HeatMapH->value() - ui.horizontalSlider_HeatMapH->minimum())
			/ (ui.horizontalSlider_HeatMapH->maximum() - ui.horizontalSlider_HeatMapH->minimum())
			* heatMapWdgt.GetDimension()[2];
		volZ = std::min(volZ, heatMapWdgt.GetDimension()[2] - 1);
		heatMapWdgt.Update(tfWdgt.GetTransferFunctionColors(), volZ);
	}
};

#endif // !HEAT_MAP_MAIN_WINDOW_H
