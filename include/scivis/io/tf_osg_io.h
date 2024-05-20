#ifndef SCIVIS_IO_TF_OSG_IO_H
#define SCIVIS_IO_TF_OSG_IO_H

#include <array>
#include <vector>

#include <osg/Texture1D>
#include <osg/Texture2D>

namespace SciVis
{
	namespace OSGConvertor
	{
		class TransferFunctionPoints
		{
		public:
			static osg::ref_ptr<osg::Texture1D> ToTexture(
				const std::vector<std::pair<uint8_t,
				std::array<float, 4>>>& tfPnts)
			{
				osg::ref_ptr<osg::Image> img = new osg::Image;
				img->allocateImage(256, 1, 1, GL_RGBA, GL_FLOAT);
				img->setInternalTextureFormat(GL_RGBA);

				auto pntItr = tfPnts.begin();
				auto lftPntItr = pntItr;
				auto lft2Rht = 1.f;
				auto* pxPtr = reinterpret_cast<osg::Vec4*>(img->data());

				for (int i = 0; i < 256; ++i) {
					auto assign = [&](float t) {
						pxPtr[i].r() = (1.0 - t) * lftPntItr->second[0] + t * pntItr->second[0];
						pxPtr[i].g() = (1.0 - t) * lftPntItr->second[1] + t * pntItr->second[1];
						pxPtr[i].b() = (1.0 - t) * lftPntItr->second[2] + t * pntItr->second[2];
						pxPtr[i].a() = (1.0 - t) * lftPntItr->second[3] + t * pntItr->second[3];
						};

					if (pntItr == tfPnts.end())
						assign(1.f);
					else if (i == static_cast<int>(pntItr->first)) {
						assign(1.f);
						lftPntItr = pntItr;
						++pntItr;
						if (pntItr != tfPnts.end())
							lft2Rht = pntItr->first - lftPntItr->first;
						else
							lft2Rht = 1.f;
					}
					else
						assign((i - lftPntItr->first) / lft2Rht);
				}

				osg::ref_ptr<osg::Texture1D> tex = new osg::Texture1D;
				tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::FilterMode::LINEAR);
				tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::FilterMode::LINEAR);
				tex->setWrap(osg::Texture::WRAP_S, osg::Texture::WrapMode::CLAMP_TO_EDGE);
				tex->setInternalFormatMode(osg::Texture::InternalFormatMode::USE_IMAGE_DATA_FORMAT);
				tex->setImage(img);

				return tex;
			}

			static osg::ref_ptr<osg::Texture2D> ToPreIntgratedTexture(
				const std::vector<std::pair<uint8_t,
				std::array<float, 4>>>& tfPnts)
			{
				osg::ref_ptr<osg::Image> img = new osg::Image;
				img->allocateImage(256, 256, 1, GL_RGBA, GL_FLOAT);
				img->setInternalTextureFormat(GL_RGBA);

				auto pntItr = tfPnts.begin();
				auto lftPntItr = pntItr;
				auto lft2Rht = 1.f;
				std::vector<std::array<float, 4>> tfIntDat(256);
				std::vector<std::array<float, 4>> tfDat(256);
				for (int i = 0; i < 256; ++i) {
					auto assign = [&](float t) {
						tfDat[i][0] = (1.0 - t) * lftPntItr->second[0] + t * pntItr->second[0];
						tfDat[i][1] = (1.0 - t) * lftPntItr->second[1] + t * pntItr->second[1];
						tfDat[i][2] = (1.0 - t) * lftPntItr->second[2] + t * pntItr->second[2];
						tfDat[i][3] = (1.0 - t) * lftPntItr->second[3] + t * pntItr->second[3];
						};

					if (pntItr == tfPnts.end())
						assign(1.f);
					else if (i == static_cast<int>(pntItr->first)) {
						assign(1.f);
						lftPntItr = pntItr;
						++pntItr;
						if (pntItr != tfPnts.end())
							lft2Rht = pntItr->first - lftPntItr->first;
						else
							lft2Rht = 1.f;
					}
					else
						assign((i - lftPntItr->first) / lft2Rht);
				}
				tfIntDat[0][0] = tfDat[0][0];
				tfIntDat[0][1] = tfDat[0][1];
				tfIntDat[0][2] = tfDat[0][2];
				tfIntDat[0][3] = tfDat[0][3];
				for (int i = 1; i < 256; ++i) {
					auto a = .5f * (tfDat[i - 1][3] + tfDat[i][3]);
					auto r = .5f * (tfDat[i - 1][0] + tfDat[i][0]) * a;
					auto g = .5f * (tfDat[i - 1][1] + tfDat[i][1]) * a;
					auto b = .5f * (tfDat[i - 1][2] + tfDat[i][2]) * a;

					tfIntDat[i][0] = tfIntDat[i - 1][0] + r;
					tfIntDat[i][1] = tfIntDat[i - 1][1] + g;
					tfIntDat[i][2] = tfIntDat[i - 1][2] + b;
					tfIntDat[i][3] = tfIntDat[i - 1][3] + a;
				}

				auto tfPreIntDatPtr = reinterpret_cast<std::array<float, 4> *>(img->data());
				for (int sf = 0; sf < 256; ++sf)
					for (int sb = 0; sb < 256; ++sb) {
						auto sMin = sf;
						auto sMax = sb;
						if (sf > sb)
							std::swap(sMin, sMax);

						if (sMin == sMax) {
							auto a = tfDat[sMin][3];
							(*tfPreIntDatPtr)[0] = tfDat[sMin][0] * a;
							(*tfPreIntDatPtr)[1] = tfDat[sMin][1] * a;
							(*tfPreIntDatPtr)[2] = tfDat[sMin][2] * a;
							(*tfPreIntDatPtr)[3] = 1.f - std::exp(-a);
						}
						else {
							auto factor = 1.f / (sMax - sMin);
							(*tfPreIntDatPtr)[0] = (tfIntDat[sMax][0] - tfIntDat[sMin][0]) * factor;
							(*tfPreIntDatPtr)[1] = (tfIntDat[sMax][1] - tfIntDat[sMin][1]) * factor;
							(*tfPreIntDatPtr)[2] = (tfIntDat[sMax][2] - tfIntDat[sMin][2]) * factor;
							(*tfPreIntDatPtr)[3] =
								1.f - std::exp((tfIntDat[sMin][3] - tfIntDat[sMax][3]) * factor);
						}

						++tfPreIntDatPtr;
					}

				osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
				tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::FilterMode::LINEAR);
				tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::FilterMode::LINEAR);
				tex->setWrap(osg::Texture::WRAP_S, osg::Texture::WrapMode::CLAMP_TO_EDGE);
				tex->setInternalFormatMode(osg::Texture::InternalFormatMode::USE_IMAGE_DATA_FORMAT);
				tex->setImage(img);

				return tex;
			}
		};
	}
}

#endif // !SCIVIS_IO_TF_OSG_IO_H
