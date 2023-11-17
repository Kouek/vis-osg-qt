#ifndef SCIVIS_ZHONGDIAN15_H
#define SCIVIS_ZHONGDIAN15_H

#include <string>

#ifdef DEPLOY_ON_ZHONGDIAN15
#include <grid/common/base/AppEnv.h>
#endif // DEPLOY_ON_ZHONGDIAN15

namespace SciVis
{
	static inline std::string GetDataPathPrefix()
	{
#ifdef DEPLOY_ON_ZHONGDIAN15
		using namespace grid::common;

		auto p = AppEnv::instance().getPath(AppEnv::DATA_DIR);
		return std::string(p.c_str()) + "/";
#else
		return ""; // 使用宏绝对路径前缀，见CMakeLists.txt
#endif
	}
}

#endif // !SCIVIS_ZHONGDIAN15_H
