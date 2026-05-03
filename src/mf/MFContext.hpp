#ifndef CEMU_CAPTURE_MF_CONTEXT_HPP
#define CEMU_CAPTURE_MF_CONTEXT_HPP
#include <cemu_capture.hpp>
#include <memory>
#include "../ContextCommon.hpp"

namespace cemu_capture
{
	class MFContext : public ContextCommon, public std::enable_shared_from_this<MFContext>
	{
	  public:
		MFContext();
		~MFContext() override;

		std::vector<SourceInfo> EnumerateSources() override;
		std::shared_ptr<Source> OpenDevice(const std::string& id) override;
	};
} // namespace cemu_capture
#endif