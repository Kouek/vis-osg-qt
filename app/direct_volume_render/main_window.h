#ifndef DIRECT_VOLUME_RENDER_MAIN_WINDOW_H
#define DIRECT_VOLUME_RENDER_MAIN_WINDOW_H

#include <memory>

#include <QtWidgets/qfiledialog.h>

#include <ui_main_window.h>

#include <common_gui/tf_widget.h>

#include <scivis/io/tf_io.h>
#include <scivis/io/tf_osg_io.h>
#include <scivis/scalar_viser/direct_volume_renderer.h>

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

	std::shared_ptr<SciVis::ScalarViser::DirectVolumeRenderer> renderer;

public:
	MainWindow(
		std::shared_ptr<SciVis::ScalarViser::DirectVolumeRenderer> renderer,
		QWidget* parent = nullptr)
		: QWidget(parent), renderer(renderer)
	{
		ui.setupUi(this);

		ui.groupBox_TF->layout()->addWidget(&tfWdgt);

		connect(ui.pushButton_OpenTF, &QPushButton::clicked, this, &MainWindow::openTFFromFile);
		connect(ui.pushButton_SaveTF, &QPushButton::clicked, this, &MainWindow::saveTFToFile);

		connect(&tfWdgt, &TransferFunctionWidget::TransferFunctionChanged, this, &MainWindow::updateRenderer);
	}

	osg::ref_ptr<osg::Texture1D> GetTFTexture() const
	{
		auto tfPnts = tfWdgt.GetTransferFunctionPointsData();
		auto tex = SciVis::OSGConvertor::TransferFunctionPoints::ToTexture(tfPnts);
		return tex;
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

		updateRenderer();
	}

	void saveTFToFile()
	{
		auto tfPnts = tfWdgt.GetTransferFunctionPointsData();
		SciVis::Loader::TransferFunctionPoints::DumpToFile(
			tfFilePath.toStdString(), tfPnts);
	}

	void updateRenderer()
	{
		auto tex = GetTFTexture();
		auto& vols = renderer->GetVolumes();
		for (auto itr = vols.begin(); itr != vols.end(); ++itr) {
			itr->second.SetTransferFunction(tex);
		}
	}
};

#endif // !DIRECT_VOLUME_RENDER_MAIN_WINDOW_H
