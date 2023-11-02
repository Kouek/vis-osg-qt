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

		connect(ui.checkBox_UseShading, &QCheckBox::stateChanged, [&](int state) {
			if (state == Qt::Checked) {
				ui.doubleSpinBox_Ka->setEnabled(true);
				ui.doubleSpinBox_Kd->setEnabled(true);
				ui.doubleSpinBox_Ks->setEnabled(true);
				ui.doubleSpinBox_Shininess->setEnabled(true);
				ui.doubleSpinBox_LightPosLon->setEnabled(true);
				ui.doubleSpinBox_LightPosLat->setEnabled(true);
				ui.doubleSpinBox_LightPosH->setEnabled(true);

				updateRenderer();
			}
			else {
				ui.doubleSpinBox_Ka->setEnabled(false);
				ui.doubleSpinBox_Kd->setEnabled(false);
				ui.doubleSpinBox_Ks->setEnabled(false);
				ui.doubleSpinBox_Shininess->setEnabled(false);
				ui.doubleSpinBox_LightPosLon->setEnabled(false);
				ui.doubleSpinBox_LightPosLat->setEnabled(false);
				ui.doubleSpinBox_LightPosH->setEnabled(false);

				updateRenderer();
			}
			});
		connect(ui.doubleSpinBox_Ka,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateRenderer);
		connect(ui.doubleSpinBox_Kd,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateRenderer);
		connect(ui.doubleSpinBox_Ks,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateRenderer);
		connect(ui.doubleSpinBox_Shininess,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateRenderer);
		connect(ui.doubleSpinBox_LightPosLon,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateRenderer);
		connect(ui.doubleSpinBox_LightPosLat,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateRenderer);
		connect(ui.doubleSpinBox_LightPosH,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MainWindow::updateRenderer);
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
		auto lonRng = bgn->second.GetLongtituteRange();
		auto latRng = bgn->second.GetLatituteRange();
		auto hRng = bgn->second.GetHeightFromCenterRange();
		for (uint8_t i = 0; i < 2; ++i) {
			lonRng[i] = rad2Deg(lonRng[i]);
			latRng[i] = rad2Deg(latRng[i]);
			hRng[i] = rad2Deg(hRng[i]);
		}
		lonRng[0] = std::max(lonRng[0] - 10.f, 0.f);
		lonRng[1] = std::min(lonRng[1] + 10.f, 180.f);
		latRng[0] = std::max(latRng[0] - 10.f, -90.f);
		latRng[1] = std::min(latRng[1] + 10.f, +90.f);
		hRng[1] *= 2.f;

		ui.doubleSpinBox_LightPosLon->setMinimum(lonRng[0]);
		ui.doubleSpinBox_LightPosLon->setMaximum(lonRng[1]);
		ui.doubleSpinBox_LightPosLon->setValue(.5f * (lonRng[0] + lonRng[1]));
		ui.doubleSpinBox_LightPosLon->setSingleStep(10.f);
		
		ui.doubleSpinBox_LightPosLat->setMinimum(latRng[0]);
		ui.doubleSpinBox_LightPosLat->setMaximum(latRng[1]);
		ui.doubleSpinBox_LightPosLat->setValue(.5f * (latRng[0] + latRng[1]));
		ui.doubleSpinBox_LightPosLat->setSingleStep(10.f);
		
		ui.doubleSpinBox_LightPosH->setMinimum(hRng[0]);
		ui.doubleSpinBox_LightPosH->setMaximum(hRng[1]);
		ui.doubleSpinBox_LightPosH->setValue(.5f * (hRng[0] + hRng[1]));
		ui.doubleSpinBox_LightPosH->setSingleStep(.1f * (hRng[1] - hRng[0]));
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

		SciVis::ScalarViser::DirectVolumeRenderer::ShadingParam shadingParam;
		shadingParam.useShading = ui.checkBox_UseShading->isChecked();
		shadingParam.ka = ui.doubleSpinBox_Ka->value();
		shadingParam.kd = ui.doubleSpinBox_Kd->value();
		shadingParam.ks = ui.doubleSpinBox_Ks->value();
		shadingParam.shininess = ui.doubleSpinBox_Shininess->value();
		{
			auto lon = deg2Rad(ui.doubleSpinBox_LightPosLon->value());
			auto lat = deg2Rad(ui.doubleSpinBox_LightPosLat->value());
			auto h = ui.doubleSpinBox_LightPosH->value();

			shadingParam.lightPos.z() = h * sinf(lat);
			h = h * cosf(lat);
			shadingParam.lightPos.y() = h * sinf(lon);
			shadingParam.lightPos.x() = h * cosf(lon);
		}
		renderer->SetShading(shadingParam);
	}

	float deg2Rad(float deg)
	{
		return deg * osg::PI / 180.f;
	};
	float rad2Deg(float rad)
	{
		return rad * 180.f / osg::PI;
	}
};

#endif // !DIRECT_VOLUME_RENDER_MAIN_WINDOW_H
