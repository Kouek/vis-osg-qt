#ifndef DIRECT_VOLUME_RENDER_MAIN_WINDOW_H
#define DIRECT_VOLUME_RENDER_MAIN_WINDOW_H

#include <memory>

#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qslider.h>

#include <ui_main_window.h>

#include <common_gui/isopleth_widget.h>
#include <common_gui/slider_val_widget.h>

#include <scivis/io/tf_io.h>
#include <scivis/io/tf_osg_io.h>
#include <scivis/scalar_viser/marching_square_renderer.h>

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

	IsoplethWidget isoplethWdgt;

	std::shared_ptr<SciVis::ScalarViser::MarchingSquareCPURenderer> renderer;
	std::shared_ptr<std::vector<float>> heights;

public:
	MainWindow(
		std::shared_ptr<SciVis::ScalarViser::MarchingSquareCPURenderer> renderer,
		QWidget* parent = nullptr)
		: QWidget(parent), isoplethWdgt(500, 500), renderer(renderer)
	{
		ui.setupUi(this);

		ui.groupBox_IsoVal->setLayout(new QVBoxLayout);
		ui.groupBox_IsoVal->layout()->addWidget(&isoValWdgt);

		ui.groupBox_Render->layout()->addWidget(&isoplethWdgt);

		connect(ui.horizontalSlider_IsoplethH, &QSlider::sliderMoved, [&](int val) {
			ui.label_IsoplethH->setText(QString::number(val));
			});
		connect(ui.horizontalSlider_IsoplethH, &QSlider::valueChanged, this, &MainWindow::updateIsoplethWidget);
		connect(&isoValWdgt, &SliderValWidget::ValueChanged, [&](int val) {
			updateRenderer();
			updateIsoplethWidget();
			});
	}

	void UpdateFromRenderer()
	{
		if (renderer->GetVolumeNum() == 0) return;

		auto bgn = renderer->GetVolumes().begin();
		auto hRng = bgn->second.GetHeightFromCenterRange();

		ui.label_MinH->setText(QString::number(static_cast<int>(floorf(hRng[0]))));
		ui.label_MaxH->setText(QString::number(static_cast<int>(floorf(hRng[1]))));
		ui.label_IsoplethH->setText(QString::number(0));

		ui.horizontalSlider_IsoplethH->setRange(
			static_cast<int>(floorf(hRng[0])),
			static_cast<int>(floorf(hRng[1])));
		ui.horizontalSlider_IsoplethH->setValue(static_cast<int>(floorf(hRng[0])));

		isoValWdgt.Set(
			static_cast<int>(255.f * bgn->second.GetIsoplethValue()),
			0, 255);

		updateIsoplethWidget();
	}

	void SetVolumeAndHeights(
		std::shared_ptr<std::vector<float>> volDat, const std::array<uint32_t, 3>& dim,
		std::shared_ptr<std::vector<float>> heights)
	{
		this->heights = heights;
		isoplethWdgt.SetVolume(volDat, dim);
	}

private:
	void updateRenderer()
	{
		auto bgn = renderer->GetVolumes().begin();
		bgn->second.MarchingSquare(isoValWdgt.GetValue() / 255.f, *heights);
	}
	void updateIsoplethWidget()
	{
		uint32_t volZ =
			static_cast<float>(ui.horizontalSlider_IsoplethH->value() - ui.horizontalSlider_IsoplethH->minimum())
			/ (ui.horizontalSlider_IsoplethH->maximum() - ui.horizontalSlider_IsoplethH->minimum())
			* isoplethWdgt.GetDimension()[2];
		volZ = std::min(volZ, isoplethWdgt.GetDimension()[2] - 1);
		isoplethWdgt.Update(isoValWdgt.GetValue() / 255.f, volZ);
	}
};

#endif // !DIRECT_VOLUME_RENDER_MAIN_WINDOW_H
