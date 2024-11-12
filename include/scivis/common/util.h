#ifndef SCIVIS_UTIL_H
#define SCIVIS_UTIL_H

namespace SciVis
{
	inline float Deg2Rad(float deg)
	{
		return deg * osg::PI / 180.f;
	};

	template <typename T>
	struct ReteurnOrError
	{
		bool ok;
		union Result
		{
			std::string errMsg;
			T dat;

			Result(const char* errMsg)
			{
				new (&this->errMsg) std::string(errMsg);
			}
			Result(const T& dat)
			{
				new (&this->dat) T(dat);
			}
			~Result() {}
		} result;

		ReteurnOrError(const char* errMsg) : ok(false), result(errMsg) {}
		ReteurnOrError(const T& dat) : ok(true), result(dat) {}
		ReteurnOrError(const ReteurnOrError& other)
		{
			~ReteurnOrError();
			ok = other.ok;
			if (ok)
				result = other.result;
			else
				result = other.result.errMsg.c_str();
		}
		~ReteurnOrError() {
			if (ok)
				result.dat.~T();
			else
				result.errMsg.~basic_string();
		}
	};
}

#endif // !SCIVIS_UTIL_H
