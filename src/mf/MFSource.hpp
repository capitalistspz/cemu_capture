#ifndef CEMU_CAPTURE_MF_SOURCE_HPP
#define CEMU_CAPTURE_MF_SOURCE_HPP

#include "cemu_capture.hpp"
#include "../SourceCommon.hpp"
#include <memory>
#include <wil/com.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mutex>
#include "Common.hpp"
#include "MFContext.hpp"

namespace cemu_capture
{
	struct Stream
	{
		wil::com_ptr<IMFSourceReader> reader;
		StreamFormat format;
	};
	struct StreamFormatEntry
	{
		StreamFormat format;
		DWORD streamIndex;
		DWORD mediaIndex;
	};
	class MFSource : public SourceCommon, public std::enable_shared_from_this<MFSource>
	{
	  public:
		MFSource(std::shared_ptr<MFContext> ctx, wil::com_ptr<IMFMediaSource> source, SourceInfo sourceInfo);
		~MFSource() override = default;

		std::optional<StreamFormat> StartStreaming(const StreamFormat&) override;
		void StopStreaming() override;

		std::vector<StreamFormat> EnumerateStreamFormats() override;
		void SetOutputFormat(ImageFormat) override;
		bool CanConvert(ImageFormat, ImageFormat) const override;

		void Capture(std::vector<uint8_t>& output) override;

		void SetProperty(StreamIntProperty, int value) override;
		int GetProperty(StreamIntProperty) override;
		SourceInfo GetInfo() const override;

	  private:
		friend class ReaderCallback;
		void UpdateData(IMFSample&);
		void RequestNewData(DWORD streamIndex);

	  private:
		std::shared_ptr<MFContext> m_ctx;
		wil::com_ptr<IMFMediaSource> m_source;
		wil::com_ptr<IMFSourceReaderCallback> m_callbackObj;
		std::vector<StreamFormatEntry> m_enumeratedStreamFormats;
		std::optional<Stream> m_stream;
		ImageFormat m_outputImageFormat{};
		std::mutex m_outputBufferMutex;
		std::vector<std::uint8_t> m_outputBuffer;
		unsigned m_outputStride{};
		SourceInfo m_info;
	};
} // namespace cemu_capture
#endif