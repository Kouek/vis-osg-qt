#ifndef DIRECT_VOLUME_RENDER_MAIN_WINDOW_H
#define DIRECT_VOLUME_RENDER_MAIN_WINDOW_H

#include <memory>

#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qslider.h>

#include <ui_main_window.h>

#include <common_gui/slider_val_widget.h>

#include <scivis/io/tf_io.h>
#include <scivis/io/tf_osg_io.h>
#include <scivis/scalar_viser/marching_cube_renderer.h>

namespace Ui
{
	class MainWindow;
}

class MainWindow : public QWidget
{
	Q_OBJECT

private:
	SliderValWidget isoValWdgt;

	QString tfFilePath;
	Ui::MainWindow ui;

	std::shared_ptr<SciVis::ScalarViser::MarchingCubeCPURenderer> renderer;

public:
	MainWindow(
		std::shared_ptr<SciVis::ScalarViser::MarchingCubeCPURenderer> renderer,
		QWidget* parent = nullptr)
		: QWidget(parent), renderer(renderer)
	{
		ui.setupUi(this);

		ui.groupBox_IsoVal->setLayout(new QHBoxLayout);
		ui.groupBox_IsoVal->layout()->addWidget(&isoValWdgt);

		connect(&isoValWdgt, &SliderValWidget::ValueChanged, [&](int val) {
			auto bgn = this->renderer->GetVolumes().begin();
			bgn->second.MarchingCube(val / 255.f);
			});
	}

	void UpdateFromRenderer()
	{
		if (renderer->GetVolumeNum() == 0) return;

		auto bgn = renderer->GetVolumes().begin();
		isoValWdgt.Set(
			static_cast<int>(255.f * bgn->second.GetIsosurfaceValue()),
			0, 255);
	}
};

#endif // !DIRECT_VOLUME_RENDER_MAIN_WINDOW_H
