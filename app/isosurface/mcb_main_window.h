#ifndef MARCHING_CUBE_MAIN_WINDOW_H
#define MARCHING_CUBE_MAIN_WINDOW_H

#include <memory>

#ifdef DEPLOY_ON_ZHONGDIAN15
#include <grid/ui/uicomctrl/ccusbasedlg.h>
#else
#include <QtWidgets/qwidget.h>
using CCusBaseDlg = QWidget;
#endif // DEPLOY_ON_ZHONGDIAN15

#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qslider.h>

#include <ui_mcb_main_window.h>

#include <common_gui/slider_val_widget.h>

#include <scivis/io/tf_io.h>
#include <scivis/io/tf_osg_io.h>
#include <scivis/scalar_viser/marching_cube_renderer.h>

namespace Ui
{
	class MCBMainWindow;
}

class MCBMainWindow : public CCusBaseDlg
{
	Q_OBJECT

private:
	SliderValWidget isoValWdgt;

	QString tfFilePath;
	Ui::MCBMainWindow ui;

	std::shared_ptr<SciVis::ScalarViser::MarchingCubeRenderer> renderer;

public:
	MCBMainWindow(
		std::shared_ptr<SciVis::ScalarViser::MarchingCubeRenderer> renderer,
		QWidget* parent = nullptr)
        : CCusBaseDlg(parent), renderer(renderer)
	{
		setWindowFlags(
			Qt::WindowMinimizeButtonHint |
			Qt::WindowCloseButtonHint);

		ui.setupUi(this);

		ui.groupBox_IsoVal->setLayout(new QHBoxLayout);
		ui.groupBox_IsoVal->layout()->addWidget(&isoValWdgt);

		connect(&isoValWdgt, &SliderValWidget::ValueChanged, this, &MCBMainWindow::updateRenderer);
		connect(ui.checkBox_UseSmoothedVolume, &QCheckBox::stateChanged, this, &MCBMainWindow::updateRenderer);
		connect(ui.checkBox_MeshSmoothNone, &QCheckBox::stateChanged, this, [&](bool state) {
			if (!state) return;
			updateRendererMeshSmoothingType(
				SciVis::ScalarViser::MarchingCubeRenderer::MeshSmoothingType::None);
			});
		connect(ui.checkBox_MeshSmoothLaplacian, &QCheckBox::stateChanged, this, [&](bool state) {
			if (!state) return;
			updateRendererMeshSmoothingType(
				SciVis::ScalarViser::MarchingCubeRenderer::MeshSmoothingType::Laplacian);
			});
		connect(ui.checkBox_MeshSmoothCurvature, &QCheckBox::stateChanged, this, [&](bool state) {
			if (!state) return;
			updateRendererMeshSmoothingType(
				SciVis::ScalarViser::MarchingCubeRenderer::MeshSmoothingType::Curvature);
			});

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
			this, &MCBMainWindow::updateRenderer);
		connect(ui.doubleSpinBox_Kd,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MCBMainWindow::updateRenderer);
		connect(ui.doubleSpinBox_Ks,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MCBMainWindow::updateRenderer);
		connect(ui.doubleSpinBox_Shininess,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MCBMainWindow::updateRenderer);
		connect(ui.doubleSpinBox_LightPosLon,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MCBMainWindow::updateRenderer);
		connect(ui.doubleSpinBox_LightPosLat,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MCBMainWindow::updateRenderer);
		connect(ui.doubleSpinBox_LightPosH,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MCBMainWindow::updateRenderer);
	}

	void UpdateFromRenderer()
	{
		if (renderer->GetVolumeNum() == 0) return;

		auto bgn = renderer->GetVolumes().begin();

		isoValWdgt.Set(
			static_cast<int>(255.f * bgn->second.GetIsosurfaceValue()),
			0, 255);

		auto lonRng = bgn->second.GetLongtituteRange();
		auto latRng = bgn->second.GetLatituteRange();
		auto hRng = bgn->second.GetHeightFromCenterRange();
		for (uint8_t i = 0; i < 2; ++i) {
			lonRng[i] = rad2Deg(lonRng[i]);
			latRng[i] = rad2Deg(latRng[i]);
			hRng[i] = rad2Deg(hRng[i]);
		}
		lonRng[0] = std::max(lonRng[0] - 30.f, 0.f);
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

	SciVis::ScalarViser::MarchingCubeRenderer::ShadingParam
		GetShadingParam() const {
		SciVis::ScalarViser::MarchingCubeRenderer::ShadingParam shadingParam;
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

		return shadingParam;
	}

private:
	void updateRenderer()
	{
		if (renderer->GetVolumeNum() == 0) return;

		auto val = isoValWdgt.GetValue();
		auto bgn = this->renderer->GetVolumes().begin();

		auto shadingParam = GetShadingParam();
		renderer->SetShading(shadingParam);

		bgn->second.MarchingCube(val / 255.f, ui.checkBox_UseSmoothedVolume->isChecked());
	}
	void updateRendererMeshSmoothingType(
		SciVis::ScalarViser::MarchingCubeRenderer::MeshSmoothingType type)
	{
		if (renderer->GetVolumeNum() == 0) return;

		auto bgn = renderer->GetVolumes().begin();
		bgn->second.SetMeshSmoothingType(type);
	}

	static float deg2Rad(float deg)
	{
		return deg * osg::PI / 180.f;
	};
	static float rad2Deg(float rad)
	{
		return rad * 180.f / osg::PI;
	}
};

#endif // !MARCHING_CUBE_MAIN_WINDOW_H
