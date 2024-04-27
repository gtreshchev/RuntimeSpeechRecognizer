// Georgy Treshchev 2024.

#include "SpeechRecognizerDownloader.h"

#include "HttpModule.h"
#include "SpeechRecognizerEditorDefines.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

FRuntimeChunkDownloader_Recognizer::FRuntimeChunkDownloader_Recognizer()
	: bCanceled(false)
{}

FRuntimeChunkDownloader_Recognizer::~FRuntimeChunkDownloader_Recognizer()
{
	UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("FRuntimeChunkDownloader_Recognizer destroyed"));
}

TFuture<FRuntimeChunkDownloaderResult_Recognizer> FRuntimeChunkDownloader_Recognizer::DownloadFile(const FString& URL, float Timeout, const FString& ContentType, int64 MaxChunkSize, const FOnProgress& OnProgress)
{
	if (bCanceled)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file download from %s"), *URL);
		return MakeFulfilledPromise<FRuntimeChunkDownloaderResult_Recognizer>(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::Cancelled, TArray64<uint8>()}).GetFuture();
	}

	TSharedPtr<TPromise<FRuntimeChunkDownloaderResult_Recognizer>> PromisePtr = MakeShared<TPromise<FRuntimeChunkDownloaderResult_Recognizer>>();
	TWeakPtr<FRuntimeChunkDownloader_Recognizer> WeakThisPtr = AsShared();
	GetContentSize(URL, Timeout).Next([WeakThisPtr, PromisePtr, URL, Timeout, ContentType, MaxChunkSize, OnProgress](int64 ContentSize) mutable
	{
		TSharedPtr<FRuntimeChunkDownloader_Recognizer> SharedThis = WeakThisPtr.Pin();
		if (!SharedThis.IsValid())
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file chunk from %s: downloader has been destroyed"), *URL);
			PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()});
			return;
		}

		if (SharedThis->bCanceled)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file download from %s"), *URL);
			PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::Cancelled, TArray64<uint8>()});
			return;
		}

		auto DownloadByPayload = [SharedThis, WeakThisPtr, PromisePtr, URL, Timeout, ContentType, OnProgress]()
		{
			SharedThis->DownloadFileByPayload(URL, Timeout, ContentType, OnProgress).Next([WeakThisPtr, PromisePtr, URL, Timeout, ContentType, OnProgress](FRuntimeChunkDownloaderResult_Recognizer Result) mutable
			{
				TSharedPtr<FRuntimeChunkDownloader_Recognizer> SharedThis = WeakThisPtr.Pin();
				if (!SharedThis.IsValid())
					{
					UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file chunk from %s: downloader has been destroyed"), *URL);
					PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()});
					return;
				}

				if (SharedThis->bCanceled)
				{
					UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file chunk download from %s"), *URL);
					PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::Cancelled, TArray64<uint8>()});
					return;
				}

				PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{Result.Result, MoveTemp(Result.Data)});
			});
		};
		
		if (ContentSize <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Unable to get content size for %s. Trying to download the file by payload"), *URL);
			DownloadByPayload();
			return;
		}

		if (MaxChunkSize <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: MaxChunkSize is <= 0. Trying to download the file by payload"), *URL);
			DownloadByPayload();
			return;
		}

		TSharedPtr<TArray64<uint8>> OverallDownloadedDataPtr = MakeShared<TArray64<uint8>>();
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Pre-allocating %lld bytes for file download from %s"), ContentSize, *URL);
			OverallDownloadedDataPtr->SetNumUninitialized(ContentSize);
		}

		FInt64Vector2 ChunkRange;
		{
			ChunkRange.X = 0;
			ChunkRange.Y = FMath::Min(MaxChunkSize, ContentSize) - 1;
		}

		TSharedPtr<int64> ChunkOffsetPtr = MakeShared<int64>(ChunkRange.X);
		TSharedPtr<bool> bChunkDownloadedFilledPtr = MakeShared<bool>(false);

		auto OnChunkDownloadedFilled = [bChunkDownloadedFilledPtr]()
		{
			if (bChunkDownloadedFilledPtr.IsValid())
			{
				*bChunkDownloadedFilledPtr = true;
			}
		};

		auto OnChunkDownloaded = [WeakThisPtr, PromisePtr, URL, ContentSize, Timeout, ContentType, OnProgress, DownloadByPayload, OverallDownloadedDataPtr, bChunkDownloadedFilledPtr, ChunkOffsetPtr, OnChunkDownloadedFilled](TArray64<uint8>&& ResultData) mutable
		{
			TSharedPtr<FRuntimeChunkDownloader_Recognizer> SharedThis = WeakThisPtr.Pin();
			if (!SharedThis.IsValid())
			{
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file chunk from %s: downloader has been destroyed"), *URL);
				PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()});
				OnChunkDownloadedFilled();
				return;
			}

			if (ResultData.Num() <= 0)
			{
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file chunk from %s: result data is empty"), *URL);
				PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()});
				OnChunkDownloadedFilled();
				return;
			}

			if (SharedThis->bCanceled)
			{
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file chunk download from %s"), *URL);
				PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::Cancelled, TArray64<uint8>()});
				OnChunkDownloadedFilled();
				return;
			}

			// Calculate the currently size of the downloaded content in the result buffer
			const int64 CurrentlyDownloadedSize = *ChunkOffsetPtr + ResultData.Num();

			// Check if some values are out of range
			{
				if (*ChunkOffsetPtr < 0 || *ChunkOffsetPtr >= OverallDownloadedDataPtr->Num())
				{
					UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: data offset is out of range (%lld, expected [0, %lld]). Trying to download the file by payload"), *URL, *ChunkOffsetPtr, OverallDownloadedDataPtr->Num());
					DownloadByPayload();
					OnChunkDownloadedFilled();
					return;
				}

				if (CurrentlyDownloadedSize > OverallDownloadedDataPtr->Num())
				{
					UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: overall downloaded size is out of range (%lld, expected [0, %lld]). Trying to download the file by payload"), *URL, CurrentlyDownloadedSize, OverallDownloadedDataPtr->Num());
					DownloadByPayload();
					OnChunkDownloadedFilled();
					return;
				}
			}

			// Append the downloaded chunk to the result data
			FMemory::Memcpy(OverallDownloadedDataPtr->GetData() + *ChunkOffsetPtr, ResultData.GetData(), ResultData.Num());

			// If the download is complete, return the result data
			if (*ChunkOffsetPtr + ResultData.Num() >= ContentSize)
			{
				PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::Success, MoveTemp(*OverallDownloadedDataPtr.Get())});
				OnChunkDownloadedFilled();
				return;
			}

			// Increase the offset by the size of the downloaded chunk
			*ChunkOffsetPtr += ResultData.Num();
		};

		SharedThis->DownloadFilePerChunk(URL, Timeout, ContentType, MaxChunkSize, ChunkRange, OnProgress, OnChunkDownloaded).Next([PromisePtr, bChunkDownloadedFilledPtr, URL, OverallDownloadedDataPtr, OnChunkDownloadedFilled, DownloadByPayload](EDownloadToMemoryResult_Recognizer Result) mutable
		{
			// Only return data if no chunk was downloaded
			if (bChunkDownloadedFilledPtr.IsValid() && (*bChunkDownloadedFilledPtr.Get() == false))
			{
				if (Result != EDownloadToMemoryResult_Recognizer::Success && Result != EDownloadToMemoryResult_Recognizer::SucceededByPayload)
				{
					UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: download failed. Trying to download the file by payload"), *URL);
					DownloadByPayload();
					OnChunkDownloadedFilled();
					return;
				}
				OverallDownloadedDataPtr->Shrink();
				PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{Result, MoveTemp(*OverallDownloadedDataPtr.Get())});
			}
		});
	});
	return PromisePtr->GetFuture();
}

TFuture<EDownloadToMemoryResult_Recognizer> FRuntimeChunkDownloader_Recognizer::DownloadFilePerChunk(const FString& URL, float Timeout, const FString& ContentType, int64 MaxChunkSize, FInt64Vector2 ChunkRange, const FOnProgress& OnProgress, const FOnChunkDownloaded& OnChunkDownloaded)
{
	if (bCanceled)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file chunk download from %s"), *URL);
		return MakeFulfilledPromise<EDownloadToMemoryResult_Recognizer>(EDownloadToMemoryResult_Recognizer::Cancelled).GetFuture();
	}

	TSharedPtr<TPromise<EDownloadToMemoryResult_Recognizer>> PromisePtr = MakeShared<TPromise<EDownloadToMemoryResult_Recognizer>>();
	TWeakPtr<FRuntimeChunkDownloader_Recognizer> WeakThisPtr = AsShared();
	GetContentSize(URL, Timeout).Next([WeakThisPtr, PromisePtr, URL, Timeout, ContentType, MaxChunkSize, OnProgress, OnChunkDownloaded, ChunkRange](int64 ContentSize) mutable
	{
		TSharedPtr<FRuntimeChunkDownloader_Recognizer> SharedThis = WeakThisPtr.Pin();
		if (!SharedThis.IsValid())
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file chunk from %s: downloader has been destroyed"), *URL);
			PromisePtr->SetValue(EDownloadToMemoryResult_Recognizer::DownloadFailed);
			return;
		}
		
		if (SharedThis->bCanceled)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file chunk download from %s"), *URL);
			PromisePtr->SetValue(EDownloadToMemoryResult_Recognizer::Cancelled);
			return;
		}

		if (ContentSize <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Unable to get content size for %s. Trying to download the file by payload"), *URL);
			SharedThis->DownloadFileByPayload(URL, Timeout, ContentType, OnProgress).Next([WeakThisPtr, PromisePtr, URL, Timeout, ContentType, OnChunkDownloaded, OnProgress](FRuntimeChunkDownloaderResult_Recognizer Result) mutable
			{
				TSharedPtr<FRuntimeChunkDownloader_Recognizer> SharedThis = WeakThisPtr.Pin();
				if (!SharedThis.IsValid())
				{
					UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file chunk from %s: downloader has been destroyed"), *URL);
					PromisePtr->SetValue(EDownloadToMemoryResult_Recognizer::DownloadFailed);
					return;
				}

				if (SharedThis->bCanceled)
				{
					UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file chunk download from %s"), *URL);
					PromisePtr->SetValue(EDownloadToMemoryResult_Recognizer::Cancelled);
					return;
				}

				if (Result.Result != EDownloadToMemoryResult_Recognizer::Success && Result.Result != EDownloadToMemoryResult_Recognizer::SucceededByPayload)
				{
					UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: %s"), *URL, *UEnum::GetValueAsString(Result.Result));
					PromisePtr->SetValue(Result.Result);
					return;
				}

				if (Result.Data.Num() <= 0)
				{
					UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: downloaded content is empty"), *URL);
					PromisePtr->SetValue(EDownloadToMemoryResult_Recognizer::DownloadFailed);
					return;
				}

				PromisePtr->SetValue(Result.Result);
				OnChunkDownloaded(MoveTemp(Result.Data));
			});
			return;
		}

		if (MaxChunkSize <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: max chunk size is <= 0"), *URL);
			PromisePtr->SetValue(EDownloadToMemoryResult_Recognizer::DownloadFailed);
			return;
		}

		// If the chunk range is not specified, determine the range based on the max chunk size and the content size
		if (ChunkRange.X == 0 && ChunkRange.Y == 0)
		{
			ChunkRange.Y = FMath::Min(MaxChunkSize, ContentSize) - 1;
		}

		if (ChunkRange.Y > ContentSize)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: chunk range is out of range (%lld, expected [0, %lld])"), *URL, ChunkRange.Y, ContentSize);
			PromisePtr->SetValue(EDownloadToMemoryResult_Recognizer::DownloadFailed);
			return;
		}

		auto OnProgressInternal = [WeakThisPtr, PromisePtr, URL, Timeout, ContentType, MaxChunkSize, OnChunkDownloaded, OnProgress, ChunkRange](int64 BytesReceived, int64 ContentSize) mutable
		{
			TSharedPtr<FRuntimeChunkDownloader_Recognizer> SharedThis = WeakThisPtr.Pin();
			if (SharedThis.IsValid())
			{
				const float Progress = ContentSize <= 0 ? 0.0f : static_cast<float>(BytesReceived + ChunkRange.X) / ContentSize;
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Log, TEXT("Downloaded %lld bytes of file chunk from %s. Range: {%lld; %lld}, Overall: %lld, Progress: %f"), BytesReceived, *URL, ChunkRange.X, ChunkRange.Y, ContentSize, Progress);
				OnProgress(BytesReceived + ChunkRange.X, ContentSize);
			}
		};

		SharedThis->DownloadFileByChunk(URL, Timeout, ContentType, ContentSize, ChunkRange, OnProgressInternal).Next([WeakThisPtr, PromisePtr, URL, Timeout, ContentType, ContentSize, MaxChunkSize, OnChunkDownloaded, OnProgress, ChunkRange](FRuntimeChunkDownloaderResult_Recognizer&& Result)
		{
			TSharedPtr<FRuntimeChunkDownloader_Recognizer> SharedThis = WeakThisPtr.Pin();
			if (!SharedThis.IsValid())
			{
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file chunk from %s: downloader has been destroyed"), *URL);
				PromisePtr->SetValue(EDownloadToMemoryResult_Recognizer::DownloadFailed);
				return;
			}

			if (SharedThis->bCanceled)
			{
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file chunk download from %s"), *URL);
				PromisePtr->SetValue(EDownloadToMemoryResult_Recognizer::Cancelled);
				return;
			}

			if (Result.Result != EDownloadToMemoryResult_Recognizer::Success && Result.Result != EDownloadToMemoryResult_Recognizer::SucceededByPayload)
			{
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: %s"), *URL, *UEnum::GetValueAsString(Result.Result));
				PromisePtr->SetValue(Result.Result);
				return;
			}

			OnChunkDownloaded(MoveTemp(Result.Data));

			// Check if the download is complete
			if (ContentSize > ChunkRange.Y + 1)
			{
				const int64 ChunkStart = ChunkRange.Y + 1;
				const int64 ChunkEnd = FMath::Min(ChunkStart + MaxChunkSize, ContentSize) - 1;

				SharedThis->DownloadFilePerChunk(URL, Timeout, ContentType, MaxChunkSize, FInt64Vector2(ChunkStart, ChunkEnd), OnProgress, OnChunkDownloaded).Next([WeakThisPtr, PromisePtr](EDownloadToMemoryResult_Recognizer Result)
				{
					PromisePtr->SetValue(Result);
				});
			}
			else
			{
				PromisePtr->SetValue(EDownloadToMemoryResult_Recognizer::Success);
			}
		});
	});

	return PromisePtr->GetFuture();
}

TFuture<FRuntimeChunkDownloaderResult_Recognizer> FRuntimeChunkDownloader_Recognizer::DownloadFileByChunk(const FString& URL, float Timeout, const FString& ContentType, int64 ContentSize, FInt64Vector2 ChunkRange, const FOnProgress& OnProgress)
{
	if (bCanceled)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file download from %s"), *URL);
		return MakeFulfilledPromise<FRuntimeChunkDownloaderResult_Recognizer>(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::Cancelled, TArray64<uint8>()}).GetFuture();
	}

	if (ChunkRange.X < 0 || ChunkRange.Y <= 0 || ChunkRange.X > ChunkRange.Y)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: chunk range (%lld; %lld) is invalid"), *URL, ChunkRange.X, ChunkRange.Y);
		return MakeFulfilledPromise<FRuntimeChunkDownloaderResult_Recognizer>(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()}).GetFuture();
	}

	if (ChunkRange.Y - ChunkRange.X + 1 > ContentSize)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: chunk range (%lld; %lld) is out of range (%lld)"), *URL, ChunkRange.X, ChunkRange.Y, ContentSize);
		return MakeFulfilledPromise<FRuntimeChunkDownloaderResult_Recognizer>(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()}).GetFuture();
	}

	TWeakPtr<FRuntimeChunkDownloader_Recognizer> WeakThisPtr = AsShared();

#if UE_VERSION_NEWER_THAN(4, 26, 0)
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequestRef = FHttpModule::Get().CreateRequest();
#else
	const TSharedRef<IHttpRequest> HttpRequestRef = FHttpModule::Get().CreateRequest();
#endif

	HttpRequestRef->SetVerb("GET");
	HttpRequestRef->SetURL(URL);

#if UE_VERSION_NEWER_THAN(4, 26, 0)
	HttpRequestRef->SetTimeout(Timeout);
#else
	UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("The Timeout feature is only supported in engine version 4.26 or later. Please update your engine to use this feature"));
#endif

	if (!ContentType.IsEmpty())
	{
		HttpRequestRef->SetHeader(TEXT("Content-Type"), ContentType);
	}

	const FString RangeHeaderValue = FString::Format(TEXT("bytes={0}-{1}"), {ChunkRange.X, ChunkRange.Y});
	HttpRequestRef->SetHeader(TEXT("Range"), RangeHeaderValue);

	HttpRequestRef->
#if UE_VERSION_OLDER_THAN(5, 4, 0)
		OnRequestProgress().BindLambda([WeakThisPtr, ContentSize, ChunkRange, OnProgress](FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
#else
		OnRequestProgress64().BindLambda([WeakThisPtr, ContentSize, ChunkRange, OnProgress](FHttpRequestPtr Request, uint64 BytesSent, uint64 BytesReceived)
#endif
	{
		TSharedPtr<FRuntimeChunkDownloader_Recognizer> SharedThis = WeakThisPtr.Pin();
		if (SharedThis.IsValid())
		{
			const float Progress = ContentSize <= 0 ? 0.0f : static_cast<float>(BytesReceived) / ContentSize;
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Log, TEXT("Downloaded %lld bytes of file chunk from %s. Range: {%lld; %lld}, Overall: %lld, Progress: %f"), static_cast<int64>(BytesReceived), *Request->GetURL(), ChunkRange.X, ChunkRange.Y, ContentSize, Progress);
			OnProgress(BytesReceived, ContentSize);
		}
	});

	TSharedPtr<TPromise<FRuntimeChunkDownloaderResult_Recognizer>> PromisePtr = MakeShared<TPromise<FRuntimeChunkDownloaderResult_Recognizer>>();
	HttpRequestRef->OnProcessRequestComplete().BindLambda([WeakThisPtr, PromisePtr, URL, ChunkRange](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess) mutable
	{
		TSharedPtr<FRuntimeChunkDownloader_Recognizer> SharedThis = WeakThisPtr.Pin();
		if (!SharedThis.IsValid())
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file chunk from %s: downloader has been destroyed"), *URL);
			PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()});
			return;
		}

		if (SharedThis->bCanceled)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file chunk download from %s"), *URL);
			PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::Cancelled, TArray64<uint8>()});
			return;
		}

		if (!bSuccess || !Response.IsValid())
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: request failed"), *Request->GetURL());
			PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()});
			return;
		}

		if (Response->GetContentLength() <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: content length is 0"), *Request->GetURL());
			PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()});
			return;
		}

		const int64 ContentLength = FCString::Atoi64(*Response->GetHeader("Content-Length"));

		if (ContentLength != ChunkRange.Y - ChunkRange.X + 1)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: content length (%lld) does not match the expected length (%lld)"), *Request->GetURL(), ContentLength, ChunkRange.Y - ChunkRange.X + 1);
			PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()});
			return;
		}

		UE_LOG(LogEditorRuntimeSpeechRecognizer, Log, TEXT("Successfully downloaded file chunk from %s. Range: {%lld; %lld}, Overall: %lld"), *Request->GetURL(), ChunkRange.X, ChunkRange.Y, ContentLength);
		PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::Success, TArray64<uint8>(Response->GetContent())});
	});

	if (!HttpRequestRef->ProcessRequest())
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: request failed"), *URL);
		return MakeFulfilledPromise<FRuntimeChunkDownloaderResult_Recognizer>(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()}).GetFuture();
	}

	HttpRequestPtr = HttpRequestRef;
	return PromisePtr->GetFuture();
}

TFuture<FRuntimeChunkDownloaderResult_Recognizer> FRuntimeChunkDownloader_Recognizer::DownloadFileByPayload(const FString& URL, float Timeout, const FString& ContentType, const FOnProgress& OnProgress)
{
	if (bCanceled)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file download from %s"), *URL);
		return MakeFulfilledPromise<FRuntimeChunkDownloaderResult_Recognizer>(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::Cancelled, TArray64<uint8>()}).GetFuture();
	}

	TWeakPtr<FRuntimeChunkDownloader_Recognizer> WeakThisPtr = AsShared();

#if UE_VERSION_NEWER_THAN(4, 26, 0)
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequestRef = FHttpModule::Get().CreateRequest();
#else
	const TSharedRef<IHttpRequest> HttpRequestRef = FHttpModule::Get().CreateRequest();
#endif

	HttpRequestRef->SetVerb("GET");
	HttpRequestRef->SetURL(URL);

#if UE_VERSION_NEWER_THAN(4, 26, 0)
	HttpRequestRef->SetTimeout(Timeout);
#else
	UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("The Timeout feature is only supported in engine version 4.26 or later. Please update your engine to use this feature"));
#endif

	HttpRequestRef->
#if UE_VERSION_OLDER_THAN(5, 4, 0)
		OnRequestProgress().BindLambda([WeakThisPtr, OnProgress](FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
#else
		OnRequestProgress64().BindLambda([WeakThisPtr, OnProgress](FHttpRequestPtr Request, uint64 BytesSent, uint64 BytesReceived)
#endif
	{
		TSharedPtr<FRuntimeChunkDownloader_Recognizer> SharedThis = WeakThisPtr.Pin();
		if (SharedThis.IsValid())
		{
			const int64 ContentLength = Request->GetContentLength();
			const float Progress = ContentLength <= 0 ? 0.0f : static_cast<float>(BytesReceived) / ContentLength;
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Log, TEXT("Downloaded %lld bytes of file chunk from %s by payload. Overall: %lld, Progress: %f"), static_cast<int64>(BytesReceived), *Request->GetURL(), static_cast<int64>(Request->GetContentLength()), Progress);
			OnProgress(BytesReceived, ContentLength);
		}
	});

	TSharedPtr<TPromise<FRuntimeChunkDownloaderResult_Recognizer>> PromisePtr = MakeShared<TPromise<FRuntimeChunkDownloaderResult_Recognizer>>();
	HttpRequestRef->OnProcessRequestComplete().BindLambda([WeakThisPtr, PromisePtr, URL](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess) mutable
	{
		TSharedPtr<FRuntimeChunkDownloader_Recognizer> SharedThis = WeakThisPtr.Pin();
		if (!SharedThis.IsValid())
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file from %s by payload: downloader has been destroyed"), *URL);
			PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()});
			return;
		}

		if (SharedThis->bCanceled)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file download from %s by payload"), *URL);
			PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::Cancelled, TArray64<uint8>()});
			return;
		}

		if (!bSuccess || !Response.IsValid())
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file from %s by payload: request failed"), *Request->GetURL());
			PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()});
			return;
		}

		if (Response->GetContentLength() <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file from %s by payload: content length is 0"), *Request->GetURL());
			PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()});
			return;
		}

		UE_LOG(LogEditorRuntimeSpeechRecognizer, Log, TEXT("Successfully downloaded file from %s by payload. Overall: %lld"), *Request->GetURL(), static_cast<int64>(Response->GetContentLength()));
		return PromisePtr->SetValue(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::SucceededByPayload, TArray64<uint8>(Response->GetContent())});
	});

	if (!HttpRequestRef->ProcessRequest())
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file from %s by payload: request failed"), *URL);
		return MakeFulfilledPromise<FRuntimeChunkDownloaderResult_Recognizer>(FRuntimeChunkDownloaderResult_Recognizer{EDownloadToMemoryResult_Recognizer::DownloadFailed, TArray64<uint8>()}).GetFuture();
	}

	HttpRequestPtr = HttpRequestRef;
	return PromisePtr->GetFuture();
}

TFuture<int64> FRuntimeChunkDownloader_Recognizer::GetContentSize(const FString& URL, float Timeout)
{
	TSharedPtr<TPromise<int64>> PromisePtr = MakeShared<TPromise<int64>>();

#if UE_VERSION_NEWER_THAN(4, 26, 0)
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequestRef = FHttpModule::Get().CreateRequest();
#else
	const TSharedRef<IHttpRequest> HttpRequestRef = FHttpModule::Get().CreateRequest();
#endif

	HttpRequestRef->SetVerb("HEAD");
	HttpRequestRef->SetURL(URL);

#if UE_VERSION_NEWER_THAN(4, 26, 0)
	HttpRequestRef->SetTimeout(Timeout);
#else
	UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("The Timeout feature is only supported in engine version 4.26 or later. Please update your engine to use this feature"));
#endif

	HttpRequestRef->OnProcessRequestComplete().BindLambda([PromisePtr, URL](const FHttpRequestPtr& Request, const FHttpResponsePtr& Response, const bool bSucceeded)
	{
		if (!bSucceeded || !Response.IsValid())
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to get size of file from %s: request failed"), *URL);
			PromisePtr->SetValue(0);
			return;
		}

		const int64 ContentLength = FCString::Atoi64(*Response->GetHeader("Content-Length"));
		if (ContentLength <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to get size of file from %s: content length is %lld, expected > 0"), *URL, ContentLength);
			PromisePtr->SetValue(0);
			return;
		}

		UE_LOG(LogEditorRuntimeSpeechRecognizer, Log, TEXT("Got size of file from %s: %lld"), *URL, ContentLength);
		PromisePtr->SetValue(ContentLength);
	});

	if (!HttpRequestRef->ProcessRequest())
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to get size of file from %s: request failed"), *URL);
		return MakeFulfilledPromise<int64>(0).GetFuture();
	}

	HttpRequestPtr = HttpRequestRef;
	return PromisePtr->GetFuture();
}

void FRuntimeChunkDownloader_Recognizer::CancelDownload()
{
	bCanceled = true;
	if (HttpRequestPtr.IsValid())
	{
#if UE_VERSION_NEWER_THAN(4, 26, 0)
		const TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = HttpRequestPtr.Pin();
#else
		const TSharedPtr<IHttpRequest> HttpRequest = HttpRequestPtr.Pin();
#endif

		HttpRequest->CancelRequest();
	}
	UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Download canceled"));
}
