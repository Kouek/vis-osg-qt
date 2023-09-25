#ifndef QT_OSG_WIDGET_H
#define QT_OSG_WIDGET_H

#include <QtGui/qevent.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qopenglwidget.h>

#include <osg/ShapeDrawable>
#include <osg/Texture2D>

#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgGA/GUIEventAdapter>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>

class QtOSGWidget : public QOpenGLWidget, osgViewer::Viewer
{
	Q_OBJECT

private:
	osg::ref_ptr<osg::Group> root;
	osgViewer::GraphicsWindow* osgWnd;

public:
	QtOSGWidget(QWidget* parent = nullptr)
	{
		resize(800, 600);

		initOSG();
		setMouseTracking(true);
		setFocusPolicy(Qt::FocusPolicy::StrongFocus);
	}

	virtual void keyPressEvent(QKeyEvent* ev) override
	{

		osgWnd->getEventQueue()->keyPress(ev->key());
		QOpenGLWidget::keyPressEvent(ev);
		update();
	}

	virtual void keyReleaseEvent(QKeyEvent* ev) override
	{
		osgWnd->getEventQueue()->keyRelease(ev->key());
		QOpenGLWidget::keyReleaseEvent(ev);
		update();
	}

	virtual void mousePressEvent(QMouseEvent* ev) override
	{
		osgWnd->getEventQueue()->mouseButtonPress(ev->pos().x(), ev->pos().y(), qMouseEventToOSGButton(ev));
		QOpenGLWidget::mousePressEvent(ev);
		update();
	}

	virtual void mouseDoubleClickEvent(QMouseEvent* ev) override
	{
		osgWnd->getEventQueue()->mouseDoubleButtonPress(ev->pos().x(), ev->pos().y(), qMouseEventToOSGButton(ev));
		QOpenGLWidget::mouseDoubleClickEvent(ev);
		update();
	}

	virtual void mouseMoveEvent(QMouseEvent* ev) override
	{
		osgWnd->getEventQueue()->mouseMotion(ev->pos().x(), ev->pos().y());
		QOpenGLWidget::mouseMoveEvent(ev);
		update();
	}

	virtual void mouseReleaseEvent(QMouseEvent* ev) override
	{
		osgWnd->getEventQueue()->mouseButtonRelease(ev->pos().x(), ev->pos().y(), qMouseEventToOSGButton(ev));
		QOpenGLWidget::mouseReleaseEvent(ev);
		update();
	}

	virtual void wheelEvent(QWheelEvent* ev) override
	{
		osgWnd->getEventQueue()->mouseScroll(
			ev->orientation() == Qt::Vertical
			? (ev->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN)
			: (ev->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_LEFT : osgGA::GUIEventAdapter::SCROLL_RIGHT));
		QOpenGLWidget::wheelEvent(ev);
		update();
	}

	virtual void resizeEvent(QResizeEvent* ev) override
	{
		const auto& sz = ev->size();
		osgWnd->resized(x(), y(), sz.width(), sz.height());
		osgWnd->getEventQueue()->windowResize(x(), y(), sz.width(), sz.height());
		QOpenGLWidget::resizeEvent(ev);
	}

	virtual void paintEvent(QPaintEvent* ev) override
	{
		QOpenGLWidget::paintEvent(ev);
		update(); // ³ÖÐøË¢ÐÂOpenGL
	}

	osg::Group* GetRoot()
	{
		return root;
	}

protected:
	virtual void paintGL() override
	{
		if (isVisibleTo(QApplication::activeWindow()))
			frame();
	}

private:
	void initOSG()
	{
		root = new osg::Group;
		root->setName("Root");
		osgWnd = new osgViewer::GraphicsWindowEmbedded(0, 0, width(), height());

		auto camera = new osg::Camera;
		camera->setGraphicsContext(osgWnd);
		camera->setViewport(new osg::Viewport(0, 0, width(), height()));
		camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		camera->setProjectionMatrixAsPerspective(60., double(width()) / height(), 1., 5 * osg::WGS_84_RADIUS_EQUATOR);
		camera->setClearColor(osg::Vec4(.2f, .2f, .2f, 1.f));
		setCamera(camera);

		osg::ref_ptr<osgGA::TrackballManipulator> manipulator = new osgGA::TrackballManipulator;
		setCameraManipulator(manipulator);

		auto earth = []() {
			auto* hints = new osg::TessellationHints;
			hints->setDetailRatio(5.0f);

			auto* sd =
				new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0.0, 0.0, 0.0), osg::WGS_84_RADIUS_POLAR), hints);
			sd->setUseVertexBufferObjects(true);

			auto* geode = new osg::Geode;
			geode->addDrawable(sd);

			auto filename = osgDB::findDataFile("land_shallow_topo_2048.jpg");
			geode->getOrCreateStateSet()->setTextureAttributeAndModes(
				0, new osg::Texture2D(osgDB::readImageFile(filename)));

			return geode;
		}();
		root->addChild(earth);
		auto states = root->getOrCreateStateSet();
		states->setMode(GL_LIGHTING, osg::StateAttribute::ON);
		states->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

		setSceneData(root);
	}

	void setKeyboardModifiers(QKeyEvent* ev)
	{
		int modKey = ev->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
		if (modKey == 0)
			return;

		int mask = 0;
		if ((modKey & Qt::ShiftModifier) != 0)
			mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
		if ((modKey & Qt::ControlModifier) != 0)
			mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
		if ((modKey & Qt::AltModifier) != 0)
			mask |= osgGA::GUIEventAdapter::MODKEY_ALT;

		osgWnd->getEventQueue()->getCurrentEventState()->setModKeyMask(mask);
		update();
	}

	unsigned int qMouseEventToOSGButton(QMouseEvent* ev)
	{
		unsigned int btn = 0;
		switch (ev->button())
		{
		case Qt::LeftButton:
			btn = 1;
			break;
		case Qt::MidButton:
			btn = 2;
			break;
		case Qt::RightButton:
			btn = 3;
			break;
		default:
			btn = 0;
			break;
		}
		return btn;
	}
};

#endif // !QT_OSG_WIDGET_H
