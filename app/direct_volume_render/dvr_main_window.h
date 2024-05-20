#ifndef DIRECT_VOLUME_RENDER_MAIN_WINDOW_H
#define DIRECT_VOLUME_RENDER_MAIN_WINDOW_H

#include <memory>

#ifdef DEPLOY_ON_ZHONGDIAN15
#include <grid/ui/uicomctrl/ccusbasedlg.h>
#else
#include <QtWidgets/qwidget.h>
using CCusBaseDlg = QWidget;
#endif // DEPLOY_ON_ZHONGDIAN15

#include <QtWidgets/qfiledialog.h>

#include <ui_dvr_main_window.h>

#include <common_gui/tf_widget.h>

#include <scivis/io/tf_io.h>
#include <scivis/io/tf_osg_io.h>
#include <scivis/scalar_viser/direct_volume_renderer.h>

namespace Ui
{
	class DVRMainWindow;
}

class DVRMainWindow : public CCusBaseDlg
{
	Q_OBJECT

private:
	QString tfFilePath;
	TransferFunctionWidget tfWdgt;
	Ui::DVRMainWindow ui;

	std::shared_ptr<SciVis::ScalarViser::DirectVolumeRenderer> renderer;

public:
	DVRMainWindow(
		std::shared_ptr<SciVis::ScalarViser::DirectVolumeRenderer> renderer,
		QWidget* parent = nullptr)
		: CCusBaseDlg(parent), renderer(renderer)
	{
		setWindowFlags(
			Qt::WindowMinimizeButtonHint |
			Qt::WindowCloseButtonHint);

		ui.setupUi(this);

		{
			auto dt = renderer->GetDeltaT();
			ui.doubleSpinBox_DeltaT->setRange(dt * .1, dt * 10.);
			ui.doubleSpinBox_DeltaT->setSingleStep(dt * .1);
			ui.doubleSpinBox_DeltaT->setValue(dt);
		}
		ui.spinBox_MaxStepCnt->setValue(renderer->GetMaxStepCount());

		ui.groupBox_TF->layout()->addWidget(&tfWdgt);

		connect(ui.pushButton_OpenTF, &QPushButton::clicked, this, &DVRMainWindow::openTFFromFile);
		connect(ui.pushButton_SaveTF, &QPushButton::clicked, this, &DVRMainWindow::saveTFToFile);

		connect(ui.checkBox_UsePreIntTF, &QCheckBox::stateChanged, this, &DVRMainWindow::updateRendererTF);
		connect(&tfWdgt, &TransferFunctionWidget::TransferFunctionChanged, this, &DVRMainWindow::updateRendererTF);

		connect(ui.doubleSpinBox_DeltaT,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &DVRMainWindow::updateRenderer);
		connect(ui.spinBox_MaxStepCnt, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, &DVRMainWindow::updateRenderer);

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

				this->renderer->DisableSlicing();
			}
			});
		connect(ui.doubleSpinBox_SliceCntrX,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &DVRMainWindow::updateSlice);
		connect(ui.doubleSpinBox_SliceCntrY,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &DVRMainWindow::updateSlice);
		connect(ui.doubleSpinBox_SliceCntrZ,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &DVRMainWindow::updateSlice);
		connect(ui.doubleSpinBox_SliceDirX,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &DVRMainWindow::updateSlice);
		connect(ui.doubleSpinBox_SliceDirY,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &DVRMainWindow::updateSlice);
		connect(ui.doubleSpinBox_SliceDirZ,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &DVRMainWindow::updateSlice);

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
			this, &DVRMainWindow::updateRenderer);
		connect(ui.doubleSpinBox_Kd,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &DVRMainWindow::updateRenderer);
		connect(ui.doubleSpinBox_Ks,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &DVRMainWindow::updateRenderer);
		connect(ui.doubleSpinBox_Shininess,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &DVRMainWindow::updateRenderer);
		connect(ui.doubleSpinBox_LightPosLon,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &DVRMainWindow::updateRenderer);
		connect(ui.doubleSpinBox_LightPosLat,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &DVRMainWindow::updateRenderer);
		connect(ui.doubleSpinBox_LightPosH,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &DVRMainWindow::updateRenderer);
	}

	osg::ref_ptr<osg::Texture1D> GetTFTexture() const
	{
		auto tfPnts = tfWdgt.GetTransferFunctionPointsData();
		auto tex = SciVis::OSGConvertor::TransferFunctionPoints::ToTexture(tfPnts);
		return tex;
	}
	osg::ref_ptr<osg::Texture2D> GetPreIntegratedTFTexture() const
	{
		auto tfPnts = tfWdgt.GetTransferFunctionPointsData();
		auto tex = SciVis::OSGConvertor::TransferFunctionPoints::ToPreIntgratedTexture(tfPnts);
		return tex;
	}

	struct SliceParam
	{
		osg::Vec3 cntr;
		osg::Vec3 dir;
	};
	SliceParam GetSliceParam()
	{
		SliceParam param;

		param.dir.x() = ui.doubleSpinBox_SliceDirX->value();
		param.dir.y() = ui.doubleSpinBox_SliceDirY->value();
		param.dir.z() = ui.doubleSpinBox_SliceDirZ->value();
		param.dir.normalize();
		ui.doubleSpinBox_SliceDirX->setValue(param.dir.x());
		ui.doubleSpinBox_SliceDirY->setValue(param.dir.y());
		ui.doubleSpinBox_SliceDirZ->setValue(param.dir.z());

		param.cntr.x() = ui.doubleSpinBox_SliceCntrX->value();
		param.cntr.y() = ui.doubleSpinBox_SliceCntrY->value();
		param.cntr.z() = ui.doubleSpinBox_SliceCntrZ->value();

		return param;
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
		lonRng[0] = std::max(lonRng[0] - 30.f, -180.f);
		lonRng[1] = std::min(lonRng[1] + 30.f, 180.f);
		latRng[0] = std::max(latRng[0] - 30.f, -90.f);
		latRng[1] = std::min(latRng[1] + 30.f, +90.f);
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

	void LoadTFFromFile(const std::string& path)
	{
		std::string errMsg;
		auto pnts = SciVis::Loader::TransferFunctionPoints::LoadFromFile(path, &errMsg);
		if (!errMsg.empty()) {
			ui.label_OpenedTF->setText(tr(errMsg.c_str()));
			return;
		}

		auto filePath = QString::fromStdString(path);
		ui.label_OpenedTF->setText(filePath);
		tfFilePath = filePath;

		tfWdgt.SetTransferFunctionPointsData(pnts);
	}

private:
	void openTFFromFile()
	{
		auto filePath = QFileDialog::getOpenFileName(
			this, tr("Open TXT File"), "./", tr("Transfer Function (*.txt)"));
		if (filePath.isEmpty()) return;

		LoadTFFromFile(filePath.toStdString());
	}

	void saveTFToFile()
	{
		auto tfPnts = tfWdgt.GetTransferFunctionPointsData();
		SciVis::Loader::TransferFunctionPoints::DumpToFile(
			tfFilePath.toStdString(), tfPnts);
	}

	void updateSlice()
	{
		auto slice = GetSliceParam();

		renderer->SetSlicing(slice.cntr, slice.dir);

		updateRenderer();
	}

	void updateRendererTF()
	{
		if (ui.checkBox_UsePreIntTF->isChecked())
		{
			auto tex = GetPreIntegratedTFTexture();
			auto& vols = renderer->GetVolumes();
			for (auto itr = vols.begin(); itr != vols.end(); ++itr)
				itr->second.SetPreIntegratedTransferFunction(tex);
		}
		else
		{
			auto tex = GetTFTexture();
			auto& vols = renderer->GetVolumes();
			for (auto itr = vols.begin(); itr != vols.end(); ++itr)
				itr->second.SetTransferFunction(tex);
		}

		updateRenderer();
	}

	void updateRenderer()
	{
		renderer->SetDeltaT(ui.doubleSpinBox_DeltaT->value());
		renderer->SetMaxStepCount(ui.spinBox_MaxStepCnt->value());
		renderer->SetUsePreIntegratedTF(ui.checkBox_UsePreIntTF->isChecked());

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
