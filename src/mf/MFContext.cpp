#include "MFContext.hpp"
#include <combaseapi.h>
#include <comdef.h>

#include <memory>
#include <objbase.h>
#include <strsafe.h>
#include <wil/resource.h>
#include <wil/com.h>
#include <wil/result_macros.h>
#include <mfapi.h>
#include <mfobjects.h>
#include <mfidl.h>

#include "MFSource.hpp"

namespace cemu_capture
{
	std::shared_ptr<Context> Context::Create()
	{
		return std::make_shared<MFContext>();
	}

	MFContext::MFContext()
	{
		THROW_IF_FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
		THROW_IF_FAILED(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET));
	}
	MFContext::~MFContext()
	{
		MFShutdown();
		CoUninitialize();
	}

	std::vector<SourceInfo> MFContext::EnumerateSources()
	{
		wil::com_ptr_t<IMFAttributes> attr;
		THROW_IF_FAILED(MFCreateAttributes(attr.put(), 1));
		THROW_IF_FAILED(attr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));
		wil::unique_cotaskmem_array_ptr<wil::com_ptr<IMFActivate>> sources;
		THROW_IF_FAILED(MFEnumDeviceSources(attr.get(), sources.put(), sources.size_address<uint32_t>()));

		Log(LogLevel::Info, "MFEnumDeviceSources found {} device sources", sources.size());
		std::vector<SourceInfo> infos;
		bool failedToGetSymlink = false;
		for (auto const& p : sources)
		{
			wil::unique_cotaskmem_string symlink;
			auto res = p->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, symlink.put(), nullptr);
			if (FAILED(res))
			{
				failedToGetSymlink = true;
				continue;
			}

			wil::unique_cotaskmem_string name;
			p->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, name.put(), nullptr);

			infos.emplace_back(nowide::narrow(symlink.get()), nowide::narrow(name.get()));
		}
		if (failedToGetSymlink)
			Log(LogLevel::Warning, "Failed to get symlink for some devices, they will be excluded from enumeration");
		return infos;
	}

	std::shared_ptr<Source> MFContext::OpenDevice(const std::string& id)
	{
		wil::com_ptr<IMFAttributes> attr;
		THROW_IF_FAILED(MFCreateAttributes(attr.put(), 1));
		THROW_IF_FAILED(attr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));
		const auto symlink = nowide::widen(id);
		THROW_IF_FAILED(attr->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, symlink.c_str()));
		wil::com_ptr<IMFMediaSource> source;
		THROW_IF_FAILED(MFCreateDeviceSource(attr.get(), source.put()));

		// Even when using MFCreateDeviceSourceActivate, MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME isn't found,
		// despite https://learn.microsoft.com/en-us/windows/win32/medfound/mf-devsource-attribute-friendly-name
		// saying that attribute is set on objects returned by MFCreateDeviceSourceActivate

		return std::make_shared<MFSource>(shared_from_this(), std::move(source), SourceInfo{id, "N/A"});
	}
} // namespace cemu_capture