#ifndef SLIDER_VAL_WIDGET_H
#define SLIDER_VAL_WIDGET_H

#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qslider.h>

class SliderValWidget : public QWidget
{
	Q_OBJECT

private:
	QLabel* label_Val;
	QLabel* label_MinVal;
	QLabel* label_MaxVal;
	QSlider* slider_Val;

public:
	SliderValWidget(QWidget* parent = nullptr) : QWidget(parent)
	{
		init();
	}
	
	SliderValWidget(int defVal, int minVal, int maxVal, QWidget* parent = nullptr)
		: QWidget(parent)
	{
		init();
		Set(defVal, minVal, maxVal);
	}

	void Set(int defVal, int minVal, int maxVal)
	{
		label_Val->setText(QString::number(defVal));
		label_MinVal->setText(QString::number(minVal));
		label_MaxVal->setText(QString::number(maxVal));
		slider_Val->setRange(minVal, maxVal);
		slider_Val->setValue(defVal);
	}

	int GetValue() const
	{
		return slider_Val->value();
	}

signals:
	void ValueChanged(int val);

private:
	void init() {
		label_Val = new QLabel;
		label_MinVal = new QLabel;
		label_MaxVal = new QLabel;
		slider_Val = new QSlider(Qt::Horizontal);

		label_Val->setAlignment(Qt::AlignHCenter);
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
		connect(slider_Val, &QSlider::valueChanged, this, &SliderValWidget::ValueChanged);
	}
};

#endif // !SLIDER_VAL_WIDGET_H
