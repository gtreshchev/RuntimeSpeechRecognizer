// Georgy Treshchev 2024.
// Code below sourced from RuntimeFilesDownloader. Types renamed with "_Recognizer" suffix to prevent conflicts.

#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "Templates/SharedPointer.h"
#include "Misc/EngineVersionComparison.h"
#if UE_VERSION_OLDER_THAN(5, 1, 0)
#include <type_traits>
#endif

/**
* Possible results from a download request
*/
UENUM(BlueprintType, Category = "Speech Recognizer Downloader")
enum class EDownloadToMemoryResult_Recognizer : uint8
{
	Success,
	/** Downloaded successfully, but there was no Content-Length header in the response and thus downloaded by payload */
	SucceededByPayload,
	Cancelled,
	DownloadFailed,
	InvalidURL
};

/**
 * A struct that contains the result of downloading a file
 */
using FRuntimeChunkDownloaderResult_Recognizer = struct{ EDownloadToMemoryResult_Recognizer Result; TArray64<uint8> Data; };

#if UE_VERSION_OLDER_THAN(5, 1, 0)
template <typename InIntType>
struct TIntVector2
{
	using IntType = InIntType;
	static_assert(std::is_integral<IntType>::value, "Only an integer types are supported.");

	union
	{
		struct
		{
			IntType X, Y;
		};
	};

	TIntVector2() = default;

	TIntVector2(IntType InX, IntType InY)
		: X(InX)
		, Y(InY)
	{
	}

	explicit TIntVector2(IntType InValue)
		: X(InValue)
		, Y(InValue)
	{
	}

	bool operator==(const TIntVector2& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}

	bool operator!=(const TIntVector2& Other) const
	{
		return X != Other.X || Y != Other.Y;
	}
};
using FInt64Vector2 = TIntVector2<int64>;
#endif

/**
 * A class that handles downloading language model files from URLs
 * This class is designed to handle large files beyond the limit supported by a TArray<uint8> (i.e. more than 2 GB) by using the HTTP Range header to download the file in chunks
 */
class FRuntimeChunkDownloader_Recognizer : public TSharedFromThis<FRuntimeChunkDownloader_Recognizer>
{
public:
	FRuntimeChunkDownloader_Recognizer();
	virtual ~FRuntimeChunkDownloader_Recognizer();

	using FOnProgress = TFunction<void(int64, int64)>;
	using FOnChunkDownloaded = TFunction<void(TArray64<uint8>&&)>;

	/**
	 * Download a file from the specified URL
	 *
	 * @param URL The URL of the file to download
	 * @param Timeout The timeout value in seconds
	 * @param ContentType The content type of the file
	 * @param MaxChunkSize The maximum size of each chunk to download in bytes
	 * @param OnProgress A function that is called with the progress as BytesReceived and ContentSize
	 * @return A future that resolves to the downloaded data as a TArray64<uint8>
	 */
	virtual TFuture<FRuntimeChunkDownloaderResult_Recognizer> DownloadFile(const FString& URL, float Timeout, const FString& ContentType, int64 MaxChunkSize, const FOnProgress& OnProgress);

	/**
	 * Download a file by dividing it into chunks and downloading each chunk separately
	 *
	 * @param URL The URL of the file to download
	 * @param Timeout The timeout value in seconds
	 * @param ContentType The content type of the file
	 * @param MaxChunkSize The maximum size of each chunk to download in bytes
	 * @param ChunkRange The range of chunks to download
	 * @param OnProgress A function that is called with the progress as BytesReceived and ContentSize
	 * @param OnChunkDownloaded A function that is called when each chunk is downloaded
	 * @return A future that resolves to true if all chunks are downloaded successfully, false otherwise
	 */
	virtual TFuture<EDownloadToMemoryResult_Recognizer> DownloadFilePerChunk(const FString& URL, float Timeout, const FString& ContentType, int64 MaxChunkSize, FInt64Vector2 ChunkRange, const FOnProgress& OnProgress, const FOnChunkDownloaded& OnChunkDownloaded);

	/**
	 * Download a single chunk of a file
	 *
	 * @param URL The URL of the file to download
	 * @param Timeout The timeout value in seconds
	 * @param ContentType The content type of the file
	 * @param ContentSize The size of the file in bytes
	 * @param ChunkRange The range of the chunk to download
	 * @param OnProgress A function that is called with the progress as BytesReceived and ContentSize
	 * @return A future that resolves to the downloaded data as a TArray64<uint8>
	 */
	virtual TFuture<FRuntimeChunkDownloaderResult_Recognizer> DownloadFileByChunk(const FString& URL, float Timeout, const FString& ContentType, int64 ContentSize, FInt64Vector2 ChunkRange, const FOnProgress& OnProgress);

	/**
	 * Download a file using payload-based approach. This approach is used when the server does not return the Content-Length header
	 *
	 * @param URL The URL of the file to download
	 * @param Timeout The timeout value in seconds
	 * @param ContentType The content type of the file
	 * @param OnProgress A function that is called with the progress as BytesReceived and ContentSize
	 * @return A future that resolves to the downloaded data as a TArray64<uint8>
	 * @note This approach cannot be used to download files that are larger than 2 GB
	 */
	virtual TFuture<FRuntimeChunkDownloaderResult_Recognizer> DownloadFileByPayload(const FString& URL, float Timeout, const FString& ContentType, const FOnProgress& OnProgress);
	
	/**
	 * Get the content size of the file to be downloaded
	 *
	 * @param URL The URL of the file to be downloaded
	 * @param Timeout The timeout value in seconds
	 * @return A future that resolves to the content length of the file to be downloaded in bytes
	 */
	TFuture<int64> GetContentSize(const FString& URL, float Timeout);

	/**
	 * Cancel the download
	 */
	virtual void CancelDownload();

protected:
	/** A weak pointer to the HTTP request being used for the download */
#if UE_VERSION_NEWER_THAN(4, 26, 0)
	TWeakPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequestPtr;
#else
	TWeakPtr<IHttpRequest> HttpRequestPtr;
#endif

	/** A flag indicating whether the download has been canceled */
	bool bCanceled;
};
