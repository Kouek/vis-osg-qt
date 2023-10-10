#ifndef DIRECT_VOLUME_RENDER_MAIN_WINDOW_H
#define DIRECT_VOLUME_RENDER_MAIN_WINDOW_H

#include <memory>

#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qslider.h>

#include <ui_main_window.h>

#include <common_gui/tf_widget.h>

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
	class IsoValWidget : public QWidget
	{
	private:
		QLabel* label_Val;
		QLabel* label_MinVal;
		QLabel* label_MaxVal;
		QSlider* slider_Val;

		SciVis::ScalarViser::MarchingCubeCPURenderer* renderer;

	public:
		IsoValWidget(
			uint8_t defIsoVal,
			SciVis::ScalarViser::MarchingCubeCPURenderer* renderer,
			QWidget* parent = nullptr)
			: QWidget(parent), renderer(renderer)
		{
			label_Val = new QLabel(QString::number(static_cast<uint>(defIsoVal)));
			label_MinVal = new QLabel(QString::number(0));
			label_MaxVal = new QLabel(QString::number(255));
			slider_Val = new QSlider(Qt::Horizontal);
			slider_Val->setRange(0, 255);
			slider_Val->setValue(defIsoVal);
			slider_Val->setTracking(false);

			setLayout(new QVBoxLayout);
			layout()->addWidget(label_Val);
			{
				auto hLayout = new QHBoxLayout;
				hLayout->addWidget(label_MinVal);
				hLayout->addWidget(slider_Val);
				hLayout->addWidget(label_MaxVal);
				dynamic_cast<QVBoxLayout*>(layout())->addLayout(hLayout);
			}

			connect(slider_Val, &QSlider::sliderMoved, [&](int val) {
				label_Val->setText(QString::number(val));
				});
			connect(slider_Val, &QSlider::valueChanged, [&](int val) {
				auto bgn = this->renderer->GetVolumes().begin();
				bgn->second.MarchingCube(val / 255.f);
				});
		}
	};

	QLayout* isosurfaceValsLayout;

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

		ui.groupBox_Isosurface->setLayout(new QHBoxLayout);
		isosurfaceValsLayout = ui.groupBox_Isosurface->layout();
	}

	void UpdateFromRenderer()
	{
		if (renderer->GetVolumeNum() == 0) return;

		auto bgn = renderer->GetVolumes().begin();
		auto isoValWdgt = new IsoValWidget(
			static_cast<uint8_t>(255.f * bgn->second.GetIsosurfaceValue()),
			renderer.get());
		isosurfaceValsLayout->addWidget(isoValWdgt);
	}

private:
};

#endif // !DIRECT_VOLUME_RENDER_MAIN_WINDOW_H
