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

		{
			auto dt = renderer->GetDeltaT();
			ui.doubleSpinBox_DeltaT->setRange(dt * .1, dt * 10.);
			ui.doubleSpinBox_DeltaT->setSingleStep(dt * .1);
			ui.doubleSpinBox_DeltaT->setValue(dt);
		}
		ui.spinBox_MaxStepCnt->setValue(renderer->GetMaxStepCount());

		connect(ui.pushButton_OpenTF, &QPushButton::clicked, this, &MainWindow::openTFFromFile);
		connect(ui.pushButton_SaveTF, &QPushButton::clicked, this, &MainWindow::saveTFToFile);

		connect(&tfWdgt, &TransferFunctionWidget::TransferFunctionChanged, this, &MainWindow::updateRenderer);

		connect(ui.doubleSpinBox_DeltaT,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateRenderer);
		connect(ui.spinBox_MaxStepCnt, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, &MainWindow::updateRenderer);

		connect(ui.checkBox_UseSlice, &QCheckBox::stateChanged, [&](int state) {
			if (state == Qt::Checked) {
				ui.doubleSpinBox_SliceCntrX->setEnabled(true);
				ui.doubleSpinBox_SliceCntrY->setEnabled(true);
				ui.doubleSpinBox_SliceCntrZ->setEnabled(true);
				ui.doubleSpinBox_SliceDirX->setEnabled(true);
				ui.doubleSpinBox_SliceDirY->setEnabled(true);
				ui.doubleSpinBox_SliceDirZ->setEnabled(true);

				updateSlice();
			}
			else {
				ui.doubleSpinBox_SliceCntrX->setEnabled(false);
				ui.doubleSpinBox_SliceCntrY->setEnabled(false);
				ui.doubleSpinBox_SliceCntrZ->setEnabled(false);
				ui.doubleSpinBox_SliceDirX->setEnabled(false);
				ui.doubleSpinBox_SliceDirY->setEnabled(false);
				ui.doubleSpinBox_SliceDirZ->setEnabled(false);

				renderer->DisableSlicing();
			}
			});
		connect(ui.doubleSpinBox_SliceCntrX,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateSlice);
		connect(ui.doubleSpinBox_SliceCntrY,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateSlice);
		connect(ui.doubleSpinBox_SliceCntrZ,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateSlice);
		connect(ui.doubleSpinBox_SliceDirX,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateSlice);
		connect(ui.doubleSpinBox_SliceDirY,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateSlice);
		connect(ui.doubleSpinBox_SliceDirZ,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateSlice);
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

	void updateSlice()
	{
		osg::Vec3 dir(
			ui.doubleSpinBox_SliceDirX->value(),
			ui.doubleSpinBox_SliceDirY->value(),
			ui.doubleSpinBox_SliceDirZ->value());
		dir.normalize();
		ui.doubleSpinBox_SliceDirX->setValue(dir.x());
		ui.doubleSpinBox_SliceDirY->setValue(dir.y());
		ui.doubleSpinBox_SliceDirZ->setValue(dir.z());

		osg::Vec3 cntr(
			ui.doubleSpinBox_SliceCntrX->value(),
			ui.doubleSpinBox_SliceCntrY->value(),
			ui.doubleSpinBox_SliceCntrZ->value());

		renderer->SetSlicing(cntr, dir);

		updateRenderer();
	}

	void updateRenderer()
	{
		auto tex = GetTFTexture();
		auto& vols = renderer->GetVolumes();
		for (auto itr = vols.begin(); itr != vols.end(); ++itr) {
			itr->second.SetTransferFunction(tex);
		}

		renderer->SetDeltaT(ui.doubleSpinBox_DeltaT->value());
		renderer->SetMaxStepCount(ui.spinBox_MaxStepCnt->value());
	}
};

#endif // !DIRECT_VOLUME_RENDER_MAIN_WINDOW_H
