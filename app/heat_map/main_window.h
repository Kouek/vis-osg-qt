#ifndef DIRECT_VOLUME_RENDER_MAIN_WINDOW_H
#define DIRECT_VOLUME_RENDER_MAIN_WINDOW_H

#include <memory>

#include <QtWidgets/qfiledialog.h>

#include <ui_main_window.h>

#include <common_gui/tf_widget.h>

#include <scivis/io/tf_io.h>
#include <scivis/io/tf_osg_io.h>
#include <scivis/scalar_viser/heat_map_renderer.h>

namespace Ui
{
	class MainWindow;
}

class MainWindow : public QWidget
{
	Q_OBJECT

private:
	QString tfFilePath;
	TransferFunctionWidget tfWdgt;
	Ui::MainWindow ui;

	std::shared_ptr<SciVis::ScalarViser::HeatMap3DRenderer> renderer;

public:
	MainWindow(
		std::shared_ptr<SciVis::ScalarViser::HeatMap3DRenderer> renderer,
		QWidget* parent = nullptr)
		: QWidget(parent), renderer(renderer)
	{
		ui.setupUi(this);

		ui.groupBox_TF->layout()->addWidget(&tfWdgt);

		connect(ui.pushButton_OpenTF, &QPushButton::clicked, this, &MainWindow::openTFFromFile);
		connect(ui.pushButton_SaveTF, &QPushButton::clicked, this, &MainWindow::saveTFToFile);

		connect(&tfWdgt, &TransferFunctionWidget::TransferFunctionChanged,
			this, &MainWindow::updateRendererColorTable);

		connect(ui.horizontalSlider_HeatMapH, &QSlider::valueChanged, [&](int val) {
			ui.label_HeatMapH->setText(QString::number(val));
			updateRendererHeight();
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

		updateRendererColorTable();
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
};

#endif // !DIRECT_VOLUME_RENDER_MAIN_WINDOW_H
