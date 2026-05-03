#ifndef CEMU_CAPTURE_MF_PRECOMPILED
#define CEMU_CAPTURE_MF_PRECOMPILED
#ifndef CEMU_CAPTURE_NOWIDE_STANDLONE
#include <boost/nowide/convert.hpp>
namespace nowide = boost::nowide;
#else
#include <nowide/convert.hpp>
#endif
#include <winerror.h>

#if defined(_MSC_VER)
#define DEBUG_BREAK __debugbreak()
#else
#include <csignal>
#define DEBUG_BREAK raise(SIGTRAP)
#endif

namespace cemu_capture
{

	inline void assert_hres_eval(HRESULT hres)
	{
#ifndef NDEBUG
		if (FAILED(hres))
		{
			DEBUG_BREAK;
		}
#endif
	}
} // namespace cemu_capture
#endif