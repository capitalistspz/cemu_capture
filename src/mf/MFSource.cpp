#include "MFSource.hpp"
#include <cassert>
#include <cguid.h>
#include <cstdlib>
#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfreadwrite.h>
#include <shlwapi.h>
#include <ranges>
#include <stdexcept>
#include <wil/com.h>
#include <wil/result_macros.h>
#include "Conversion.hpp"
#include "Common.hpp"

namespace cemu_capture
{
	inline ImageFormat FromMFSubType(const GUID& guid)
	{
		if (guid == MFVideoFormat_MJPG)
			return ImageFormat::MJPG;
		else if (guid == MFVideoFormat_NV12)
			return ImageFormat::NV12;
		else if (guid == MFVideoFormat_YUY2)
			return ImageFormat::YUYV;
		else if (guid == MFVideoFormat_RGB24)
			return ImageFormat::RGB24;
		else if (guid == MFVideoFormat_ARGB32)
			return ImageFormat::ARGB32;
		else if (guid == MFVideoFormat_NV21)
			return ImageFormat::NV21;
		else if (guid == MFVideoFormat_UYVY)
			return ImageFormat::UYVY;
		return ImageFormat::Unspecified;
	}

	inline GUID ToMFSubType(ImageFormat format)
	{
		switch (format)
		{
		case ImageFormat::NV12:
			return MFVideoFormat_NV12;
		case ImageFormat::NV21:
			return MFVideoFormat_NV21;
		case ImageFormat::YUYV:
			return MFVideoFormat_YUY2;
		case ImageFormat::UYVY:
			return MFVideoFormat_UYVY;
		case ImageFormat::MJPG:
			return MFVideoFormat_MJPG;
		case ImageFormat::RGB24:
			return MFVideoFormat_RGB24;
		case ImageFormat::ARGB32:
			return MFVideoFormat_ARGB32;
		default:
			return GUID_NULL;
		}
	}
	// Mostly copied from https://learn.microsoft.com/en-us/windows/win32/medfound/using-the-source-reader-in-asynchronous-mode
	class ReaderCallback : public IMFSourceReaderCallback
	{
	  public:
		explicit ReaderCallback(MFSource& source)
			: m_source(source), m_refCount(1)
		{
			InitializeCriticalSection(&m_critsec);
		}

		// IUnknown methods
		STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override
		{
			static const QITAB qit[] =
				{
					QITABENT(ReaderCallback, IMFSourceReaderCallback),
					{},
				};
			return QISearch(this, qit, iid, ppv);
		}
		STDMETHODIMP_(ULONG)
		AddRef() override
		{
			return InterlockedIncrement(&m_refCount);
		}
		STDMETHODIMP_(ULONG)
		Release() override
		{
			ULONG count = InterlockedDecrement(&m_refCount);
			if (count == 0)
				delete this;
			return count;
		}

		// IMFSourceReaderCallback methods
		STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD streamIndex,
								  DWORD streamFlags, LONGLONG timestamp, IMFSample* sample) override
		{
			EnterCriticalSection(&m_critsec);
			if (sample != nullptr && SUCCEEDED(hrStatus))
			{
				m_source.UpdateData(*sample);
			}
			m_source.RequestNewData(streamIndex);
			LeaveCriticalSection(&m_critsec);
			return S_OK;
		}

		STDMETHODIMP OnEvent(DWORD, IMFMediaEvent*) override
		{
			return S_OK;
		}

		STDMETHODIMP OnFlush(DWORD) override
		{
			return S_OK;
		}

	  private:
		// WIN32 prefers Release().
		virtual ~ReaderCallback()
		{
			DeleteCriticalSection(&m_critsec);
		};

	  private:
		MFSource& m_source;
		long m_refCount;
		CRITICAL_SECTION m_critsec{};
	};

	MFSource::MFSource(std::shared_ptr<MFContext> ctx, wil::com_ptr<IMFMediaSource> source, SourceInfo sourceInfo)
		: m_ctx(std::move(ctx)), m_source(std::move(source)), m_info(std::move(sourceInfo)), m_callbackObj(new ReaderCallback(*this))
	{
	}

	std::optional<StreamFormat> MFSource::StartStreaming(const StreamFormat& streamFormat)
	{
		if (m_stream)
			return std::nullopt;
		const auto isMatchingFormat = [&streamFormat](const StreamFormatEntry& entry) {
			const auto& entryFormat = entry.format;
			return (entryFormat.format == streamFormat.format || streamFormat.format == ImageFormat::Unspecified) &&
				   entryFormat.dimensions.width == streamFormat.dimensions.width &&
				   entryFormat.dimensions.height == streamFormat.dimensions.height;
		};
		auto matchingFormats = m_enumeratedStreamFormats | std::views::filter(isMatchingFormat);

		const StreamFormatEntry* selectedStreamFormat = nullptr;
		double closestFramerate = 0.0;
		for (const auto& enumeratedFormat : matchingFormats)
		{
			const auto diff = std::abs(enumeratedFormat.format.framerate - streamFormat.framerate);
			// This should be a good enough heuristic for a framerate being way out of range
			if (diff >= streamFormat.framerate)
				continue;
			if (diff > std::abs(streamFormat.framerate - closestFramerate))
				continue;
			closestFramerate = enumeratedFormat.format.framerate;
			selectedStreamFormat = &enumeratedFormat;
		}
		if (!selectedStreamFormat)
		{
			return std::nullopt;
		}
		wil::com_ptr<IMFPresentationDescriptor> descriptor;
		THROW_IF_FAILED(m_source->CreatePresentationDescriptor(descriptor.put()));
		BOOL selected = false;
		wil::com_ptr<IMFStreamDescriptor> streamDescriptor;
		THROW_IF_FAILED(descriptor->GetStreamDescriptorByIndex(selectedStreamFormat->streamIndex, &selected, streamDescriptor.put()));
		wil::com_ptr<IMFMediaTypeHandler> mediaTypeHandler;
		THROW_IF_FAILED(streamDescriptor->GetMediaTypeHandler(mediaTypeHandler.put()));
		wil::com_ptr<IMFMediaType> mediaType;
		THROW_IF_FAILED(mediaTypeHandler->GetMediaTypeByIndex(selectedStreamFormat->mediaIndex, mediaType.put()));
		THROW_IF_FAILED(mediaTypeHandler->SetCurrentMediaType(mediaType.get()));
		THROW_IF_FAILED(descriptor->SelectStream(selectedStreamFormat->streamIndex));

		wil::com_ptr<IMFAttributes> readerAttributes;
		THROW_IF_FAILED(MFCreateAttributes(readerAttributes.put(), 3));

		// THE MOST important thing for performance, capture speed on my system cratered without converters being disabled
		THROW_IF_FAILED(readerAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE));
		THROW_IF_FAILED(readerAttributes->SetUINT32(MF_LOW_LATENCY, TRUE));

		THROW_IF_FAILED(readerAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, m_callbackObj.get()));
		wil::com_ptr<IMFSourceReader> sourceReader;
		THROW_IF_FAILED(MFCreateSourceReaderFromMediaSource(m_source.get(), readerAttributes.get(), sourceReader.put()));
		sourceReader->ReadSample(selectedStreamFormat->streamIndex, 0, nullptr, nullptr, nullptr, nullptr);
		m_stream.emplace(std::move(sourceReader), selectedStreamFormat->format);

		m_ctx->Log(LogLevel::Info, "Started streaming {}x{}@{}fps", selectedStreamFormat->format.dimensions.width, selectedStreamFormat->format.dimensions.height, selectedStreamFormat->format.framerate);
		return selectedStreamFormat->format;
	}

	void MFSource::StopStreaming()
	{
		m_stream.reset();
	}

	std::vector<StreamFormat> MFSource::EnumerateStreamFormats()
	{
		wil::com_ptr<IMFPresentationDescriptor> descriptor;
		THROW_IF_FAILED(m_source->CreatePresentationDescriptor(descriptor.put()));

		DWORD descriptorCount = 0;
		THROW_IF_FAILED(descriptor->GetStreamDescriptorCount(&descriptorCount));

		std::vector<StreamFormatEntry> entries;
		for (auto streamIndex : std::views::iota(0u, descriptorCount))
		{
			wil::com_ptr<IMFStreamDescriptor> streamDescriptor;
			BOOL isSelected = false;
			auto res = descriptor->GetStreamDescriptorByIndex(streamIndex, &isSelected, streamDescriptor.put());
			if (FAILED(res))
			{
				m_ctx->Log(LogLevel::Warning, "Failed to get stream {} descriptor", streamIndex);
				continue;
			}
			wil::com_ptr<IMFMediaTypeHandler> mediaTypeHandler;
			res = streamDescriptor->GetMediaTypeHandler(mediaTypeHandler.put());
			if (FAILED(res))
			{
				m_ctx->Log(LogLevel::Warning, "Failed to get media type handler for stream {}", streamIndex);
				continue;
			}
			DWORD mediaTypeCount = 0;
			res = mediaTypeHandler->GetMediaTypeCount(&mediaTypeCount);
			if (FAILED(res))
			{
				m_ctx->Log(LogLevel::Warning, "Failed to get media type count for stream {}", streamIndex);
				continue;
			}

			for (auto mediaTypeIndex : std::views::iota(0u, mediaTypeCount))
			{
				wil::com_ptr<IMFMediaType> mediaType;
				res = mediaTypeHandler->GetMediaTypeByIndex(mediaTypeIndex, mediaType.put());
				if (FAILED(res))
				{
					m_ctx->Log(LogLevel::Warning, "Failed to get media type {} on stream {}", mediaTypeIndex, streamIndex);
					continue;
				}
				GUID subType{};
				res = mediaType->GetGUID(MF_MT_SUBTYPE, &subType);
				if (FAILED(res))
				{
					m_ctx->Log(LogLevel::Warning, "Failed to get media type {} subtype on stream {}", mediaTypeIndex, streamIndex);
					continue;
				}
				const auto imageFormat = FromMFSubType(subType);
				if (imageFormat == ImageFormat::Unspecified)
				{
					m_ctx->Log(LogLevel::Warning, "Skipped media type {} image format on stream {}", mediaTypeIndex, streamIndex);
					continue;
				}
				UINT32 width = 0;
				UINT32 height = 0;
				res = MFGetAttributeSize(mediaType.get(), MF_MT_FRAME_SIZE, &width, &height);
				if (FAILED(res))
				{
					m_ctx->Log(LogLevel::Warning, "Failed to get media type {} frame size on stream {}", mediaTypeIndex, streamIndex);
					continue;
				}
				UINT32 numerator = 0;
				UINT32 denominator = 0;
				res = MFGetAttributeRatio(mediaType.get(), MF_MT_FRAME_RATE_RANGE_MAX, &numerator, &denominator);
				if (FAILED(res))
				{
					m_ctx->Log(LogLevel::Warning, "Failed to get media type {} framerate on stream {}", mediaTypeIndex, streamIndex);
					continue;
				}
				assert(denominator != 0);
				auto streamFormat = StreamFormat{Dimensions{width, height}, static_cast<double>(numerator) / static_cast<double>(denominator), imageFormat};
				entries.emplace_back(streamFormat, streamIndex, mediaTypeIndex);
			}
		}
		m_enumeratedStreamFormats = std::move(entries);
		auto entryView = m_enumeratedStreamFormats | std::views::transform([](const StreamFormatEntry& entry) { return entry.format; });
		return std::vector(entryView.begin(), entryView.end());
	}

	void MFSource::SetOutputFormat(ImageFormat format)
	{
		m_outputImageFormat = format;
	}

	bool MFSource::CanConvert(ImageFormat in, ImageFormat out) const
	{
		return conversion::CanConvert(in, out);
	}

	void MFSource::Capture(std::vector<uint8_t>& output)
	{
		std::scoped_lock lock(m_outputBufferMutex);
		output.assign(m_outputBuffer.cbegin(), m_outputBuffer.cend());
	}

	void MFSource::UpdateData(IMFSample& sample)
	{
		if (!m_stream)
			return;
		const auto format = m_stream->format;
		std::scoped_lock lock(m_outputBufferMutex);
		const auto outputStride = m_outputStride ? m_outputStride : format.dimensions.width;
		const auto outputImageFormat = m_outputImageFormat != ImageFormat::Unspecified ? m_outputImageFormat : format.format;
		UINT32 frameCorrupted;
		// If it failed, then who knows?
		if (FAILED(sample.GetUINT32(MFSampleExtension_FrameCorruption, &frameCorrupted)))
			frameCorrupted = false;
		CaptureErrorType errorType = CaptureErrorType::None;
		if (frameCorrupted)
			errorType = CaptureErrorType::DataCorrupted;
		const auto errorPolicy = GetFrameErrorPolicy();
		DWORD bufferCount = 0;
		if (frameCorrupted && errorPolicy == CaptureErrorPolicy::IgnoreFrame)
		{
			// Do nothing
		}
		else if (SUCCEEDED(sample.GetBufferCount(&bufferCount)) && bufferCount == 1)
		{
			wil::com_ptr_nothrow<IMFMediaBuffer> buffer;
			assert_hres_eval(sample.GetBufferByIndex(0, buffer.put()));
			conversion::Convert(*buffer, format.format, outputImageFormat, outputStride, format.dimensions, m_outputBuffer);
		}
		else
		{
			assert(false);
		}
		InvokeCaptureCallback(errorType, m_outputBuffer);
	}
	void MFSource::RequestNewData(DWORD streamIndex)
	{
		m_stream->reader->ReadSample(streamIndex, 0, nullptr, nullptr, nullptr, nullptr);
	}

	void MFSource::SetProperty(StreamIntProperty prop, int value)
	{
		if (prop == StreamIntProperty::OutputStride)
		{
			if (value < 0)
				throw std::out_of_range("Stride cannot be negative");
			m_outputStride = value;
		}
		else
		{
			throw std::invalid_argument("Unsupported property");
		}
	}
	int MFSource::GetProperty(StreamIntProperty prop)
	{
		if (prop == StreamIntProperty::OutputStride)
			return static_cast<int>(m_outputStride);
		throw std::invalid_argument("Unsupported property");
	}

	SourceInfo MFSource::GetInfo() const
	{
		return m_info;
	}

} // namespace cemu_capture