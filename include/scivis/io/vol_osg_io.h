#ifndef SCIVIS_IO_VOL_OSG_IO_H
#define SCIVIS_IO_VOL_OSG_IO_H

#include <array>
#include <vector>

#include <osg/Texture3D>

namespace SciVis
{
	namespace OSGLoader {
		class PointCloud {
		public:
			static std::vector<osg::Vec3f> LoadFromFile(const std::string& filePath, std::string* errMsg = nullptr)
			{
				std::ifstream is(filePath);
				if (!is.is_open()) {
					if (errMsg)
						*errMsg = "Invalid File Path";
					return std::vector<osg::Vec3f>();
				}

				std::string line;
				std::vector<osg::Vec3f> ret;
				int flag = 0;
				while (std::getline(is, line)) {
					/*if (flag == 0) {
						flag++;
						continue;
					}*/
					float x, y, z;
					std::stringstream ss(line);
					ss >> x;
					ss >> y;
					ss >> z;
					osg::Vec3f point(x, y, z);
					ret.push_back(point);
					//flag--;
				}
				return ret;

			}
		};
	}

	namespace OSGConvertor
	{
		class RAWVolume
		{
		public:
			static osg::ref_ptr<osg::Texture3D> NormalizedFloatToTexture(
				const std::vector<float>& dat,
				const std::array<uint32_t, 3>& srcDim,
				const std::array<uint8_t, 3>& logDstDim,
				osg::Texture::FilterMode filterMode = osg::Texture::LINEAR)
			{
				std::array<uint32_t, 3> dstDim = { 1 << logDstDim[0], 1 << logDstDim[1], 1 << logDstDim[2] };
				std::array<float, 3> scaleDst2Src = { static_cast<float>(srcDim[0] - 1) / (dstDim[0] - 1),
										static_cast<float>(srcDim[1] - 1) / (dstDim[1] - 1),
										static_cast<float>(srcDim[2] - 1) / (dstDim[2] - 1) };

				osg::ref_ptr<osg::Image> img = new osg::Image;
				img->allocateImage(dstDim[0], dstDim[1], dstDim[2], GL_RED, GL_FLOAT);
				img->setInternalTextureFormat(GL_RED);
				auto* pxPtr = reinterpret_cast<float*>(img->data());

				for (uint32_t z = 0; z < dstDim[2]; ++z)
					for (uint32_t y = 0; y < dstDim[1]; ++y)
						for (uint32_t x = 0; x < dstDim[0]; ++x) {
							auto srcZ = static_cast<uint32_t>(z * scaleDst2Src[2]);
							auto srcY = static_cast<uint32_t>(y * scaleDst2Src[1]);
							auto srcX = static_cast<uint32_t>(x * scaleDst2Src[0]);
							if (srcX >= srcDim[0])
								srcX = srcDim[0] - 1;
							if (srcY >= srcDim[1])
								srcY = srcDim[1] - 1;
							if (srcZ >= srcDim[2])
								srcZ = srcDim[2] - 1;

							auto XYZ2Offs = [&](int x, int y, int z) {
								return static_cast<size_t>(z) * srcDim[1] * srcDim[0] + y * srcDim[0] + x;
								};

							auto srcX1 = srcX == srcDim[0] - 1 ? srcX : srcX + 1;
							auto srcY1 = srcY == srcDim[1] - 1 ? srcY : srcY + 1;
							auto srcZ1 = srcZ == srcDim[2] - 1 ? srcZ : srcZ + 1;
							if (srcX != 0)
								--srcX;
							if (srcY != 0)
								--srcY;
							if (srcZ != 0)
								--srcZ;

							auto avgV = 0.f;
							uint8_t cnt = 0;
							for (int subZ = srcZ; subZ <= srcZ1; ++subZ)
								for (int subY = srcY; subY <= srcY1; ++subY)
									for (int subX = srcX; subX <= srcX1; ++subX) {
										avgV += dat[XYZ2Offs(subX, subY, subZ)];
										++cnt;
									}
							avgV /= cnt;
							*pxPtr = avgV;

							++pxPtr;
						}

				osg::ref_ptr<osg::Texture3D> tex = new osg::Texture3D;
				tex->setFilter(osg::Texture::MAG_FILTER, filterMode);
				tex->setFilter(osg::Texture::MIN_FILTER, filterMode);
				tex->setWrap(osg::Texture::WRAP_S, osg::Texture::WrapMode::CLAMP_TO_EDGE);
				tex->setWrap(osg::Texture::WRAP_T, osg::Texture::WrapMode::CLAMP_TO_EDGE);
				tex->setWrap(osg::Texture::WRAP_R, osg::Texture::WrapMode::CLAMP_TO_EDGE);
				tex->setInternalFormatMode(osg::Texture::InternalFormatMode::USE_IMAGE_DATA_FORMAT);
				tex->setImage(img);

				return tex;
			}
		};
	}
}

#endif // !SCIVIS_IO_VOL_OSG_IO_H
