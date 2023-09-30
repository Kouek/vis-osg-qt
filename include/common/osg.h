#ifndef COMMON_OSG_H
#define COMMON_OSG_H

#include <osg/PositionAttitudeTransform>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>

#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgViewer/Viewer>

inline osg::Node* createEarth() {
    auto* hints = new osg::TessellationHints;
    hints->setDetailRatio(5.0f);

    auto* sd = new osg::ShapeDrawable(
        new osg::Sphere(osg::Vec3(0.0, 0.0, 0.0), osg::WGS_84_RADIUS_POLAR), hints);
    sd->setUseVertexBufferObjects(true);

    auto* geode = new osg::Geode;
    geode->addDrawable(sd);

    auto filename = osgDB::findDataFile("land_shallow_topo_2048.jpg");
    geode->getOrCreateStateSet()->setTextureAttributeAndModes(
        0, new osg::Texture2D(osgDB::readImageFile(filename)));

    return geode;
}

#endif // !COMMON_OSG_H
