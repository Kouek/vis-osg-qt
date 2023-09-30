#include <QtWidgets/qapplication.h>

#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>

#include <common/osg.h>

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	auto* viewer = new osgViewer::Viewer;
	viewer->setUpViewInWindow(200, 50, 800, 600);
	auto* manipulator = new osgGA::TrackballManipulator;
	viewer->setCameraManipulator(manipulator);

	osg::ref_ptr<osg::Group> grp = new osg::Group;
	grp->addChild(createEarth());

	viewer->setSceneData(grp);
	while (!viewer->done()) {
		app.processEvents();
		viewer->frame();
	}

	return 0;
}
