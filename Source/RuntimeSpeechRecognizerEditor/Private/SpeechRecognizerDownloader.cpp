// Georgy Treshchev 2024.

#include "SpeechRecognizerDownloader.h"

#include "HttpModule.h"
#include "SpeechRecognizerEditorDefines.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

FLanguageModelDownloader::FLanguageModelDownloader()
	: bCanceled(false)
{
}

TFuture<TArray64<uint8>> FLanguageModelDownloader::DownloadFile(const FString& URL, float Timeout, const FString& ContentType, int64 MaxChunkSize, const TFunction<void(int64, int64)>& OnProgress)
{
	if (bCanceled)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file download from %s"), *URL);
		return MakeFulfilledPromise<TArray64<uint8>>(TArray64<uint8>()).GetFuture();
	}

	TSharedPtr<TPromise<TArray64<uint8>>> PromisePtr = MakeShared<TPromise<TArray64<uint8>>>();
	TWeakPtr<FLanguageModelDownloader> WeakThisPtr = AsShared();
	GetContentSize(URL, Timeout).Next([WeakThisPtr, PromisePtr, URL, Timeout, ContentType, MaxChunkSize, OnProgress](int64 ContentSize) mutable
	{
		TSharedPtr<FLanguageModelDownloader> SharedThis = WeakThisPtr.Pin();
		if (!SharedThis.IsValid())
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file chunk from %s: downloader has been destroyed"), *URL);
			PromisePtr->SetValue(TArray64<uint8>());
			return;
		}

		if (ContentSize <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file from %s: ContentSize is <= 0"), *URL);
			PromisePtr->SetValue(TArray64<uint8>());
			return;
		}

		if (MaxChunkSize <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file from %s: MaxChunkSize is <= 0"), *URL);
			PromisePtr->SetValue(TArray64<uint8>());
			return;
		}

		TArray64<uint8> OverallDownloadedData;
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Pre-allocating %lld bytes for file download from %s"), ContentSize, *URL);
			OverallDownloadedData.SetNumUninitialized(ContentSize);
		}

		FInt64Vector2 ChunkRange;
		{
			ChunkRange.X = 0;
			ChunkRange.Y = FMath::Min(MaxChunkSize, ContentSize) - 1;
		}

		TSharedPtr<int64> ChunkOffsetPtr = MakeShared<int64>(ChunkRange.X);
		TSharedPtr<bool> bChunkDownloadedPtr = MakeShared<bool>(false);

		auto OnChunkDownloaded = [WeakThisPtr, PromisePtr, URL, ContentSize, OverallDownloadedData = MoveTemp(OverallDownloadedData), bChunkDownloadedPtr, ChunkOffsetPtr](TArray64<uint8>&& ResultData) mutable
		{
			if (bChunkDownloadedPtr.IsValid())
			{
				*bChunkDownloadedPtr = true;
			}

			TSharedPtr<FLanguageModelDownloader> SharedThis = WeakThisPtr.Pin();
			if (!SharedThis.IsValid())
			{
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file chunk from %s: downloader has been destroyed"), *URL);
				PromisePtr->SetValue(TArray64<uint8>());
				return;
			}

			if (SharedThis->bCanceled)
			{
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file chunk download from %s"), *URL);
				PromisePtr->SetValue(TArray64<uint8>());
				return;
			}

			// Calculate the currently size of the downloaded content in the result buffer
			const int64 CurrentlyDownloadedSize = *ChunkOffsetPtr + ResultData.Num();

			// Check if some values are out of range
			{
				if (*ChunkOffsetPtr < 0 || *ChunkOffsetPtr >= OverallDownloadedData.Num())
				{
					UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: data offset is out of range (%lld, expected [0, %lld])"), *URL, *ChunkOffsetPtr, OverallDownloadedData.Num());
					PromisePtr->SetValue(TArray64<uint8>());
					return;
				}

				if (CurrentlyDownloadedSize > OverallDownloadedData.Num())
				{
					UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: overall downloaded size is out of range (%lld, expected [0, %lld])"), *URL, CurrentlyDownloadedSize, OverallDownloadedData.Num());
					PromisePtr->SetValue(TArray64<uint8>());
					return;
				}
			}

			// Append the downloaded chunk to the result data
			FMemory::Memcpy(OverallDownloadedData.GetData() + *ChunkOffsetPtr, ResultData.GetData(), ResultData.Num());

			// If the download is complete, return the result data
			if (*ChunkOffsetPtr + ResultData.Num() >= ContentSize)
			{
				PromisePtr->SetValue(MoveTemp(OverallDownloadedData));
				return;
			}

			// Increase the offset by the size of the downloaded chunk
			*ChunkOffsetPtr += ResultData.Num();
		};

		SharedThis->DownloadFilePerChunk(URL, Timeout, ContentType, MaxChunkSize, ChunkRange, OnProgress, OnChunkDownloaded).Next([PromisePtr, bChunkDownloadedPtr, URL](bool bSucceeded)
		{
			if (!bSucceeded)
			{
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file by chunk from %s"), *URL);

				// Only return an empty data if no chunk was downloaded
				if (bChunkDownloadedPtr.IsValid() && (*bChunkDownloadedPtr.Get() == false))
				{
					PromisePtr->SetValue(TArray64<uint8>());
				}
			}
		});
	});
	return PromisePtr->GetFuture();
}

TFuture<bool> FLanguageModelDownloader::DownloadFilePerChunk(const FString& URL, float Timeout, const FString& ContentType, int64 MaxChunkSize, FInt64Vector2 ChunkRange, const TFunction<void(int64, int64)>& OnProgress, const TFunction<void(TArray64<uint8>&&)>& OnChunkDownloaded)
{
	if (bCanceled)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file chunk download from %s"), *URL);
		return MakeFulfilledPromise<bool>(false).GetFuture();
	}

	TSharedPtr<TPromise<bool>> PromisePtr = MakeShared<TPromise<bool>>();
	TWeakPtr<FLanguageModelDownloader> WeakThisPtr = AsShared();
	GetContentSize(URL, Timeout).Next([WeakThisPtr, PromisePtr, URL, Timeout, ContentType, MaxChunkSize, OnProgress, OnChunkDownloaded, ChunkRange](int64 ContentSize) mutable
	{
		TSharedPtr<FLanguageModelDownloader> SharedThis = WeakThisPtr.Pin();
		if (!SharedThis.IsValid())
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file chunk from %s: downloader has been destroyed"), *URL);
			PromisePtr->SetValue(false);
			return;
		}
		
		if (SharedThis->bCanceled)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file chunk download from %s"), *URL);
			PromisePtr->SetValue(false);
			return;
		}

		if (ContentSize <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: content size is <= 0"), *URL);
			PromisePtr->SetValue(false);
			return;
		}

		if (MaxChunkSize <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: max chunk size is <= 0"), *URL);
			PromisePtr->SetValue(false);
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
			PromisePtr->SetValue(false);
			return;
		}

		auto OnProgressInternal = [WeakThisPtr, PromisePtr, URL, Timeout, ContentType, MaxChunkSize, OnChunkDownloaded, OnProgress, ChunkRange](int64 BytesReceived, int64 ContentSize) mutable
		{
			TSharedPtr<FLanguageModelDownloader> SharedThis = WeakThisPtr.Pin();
			if (SharedThis.IsValid())
			{
				const float Progress = static_cast<float>(BytesReceived + ChunkRange.X) / ContentSize;
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Verbose, TEXT("Downloaded %lld bytes of file chunk from %s. Range: {%lld; %lld}, Overall: %lld, Progress: %f"), BytesReceived, *URL, ChunkRange.X, ChunkRange.Y, ContentSize, Progress);
				OnProgress(BytesReceived + ChunkRange.X, ContentSize);
			}
		};

		SharedThis->DownloadFileByChunk(URL, Timeout, ContentType, ContentSize, ChunkRange, OnProgressInternal).Next([WeakThisPtr, PromisePtr, URL, Timeout, ContentType, ContentSize, MaxChunkSize, OnChunkDownloaded, OnProgress, ChunkRange](TArray64<uint8>&& ResultData)
		{
			TSharedPtr<FLanguageModelDownloader> SharedThis = WeakThisPtr.Pin();
			if (!SharedThis.IsValid())
			{
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file chunk from %s: downloader has been destroyed"), *URL);
				PromisePtr->SetValue(false);
				return;
			}

			if (SharedThis->bCanceled)
			{
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file chunk download from %s"), *URL);
				PromisePtr->SetValue(false);
				return;
			}

			OnChunkDownloaded(MoveTemp(ResultData));

			// Check if the download is complete
			if (ContentSize > ChunkRange.Y + 1)
			{
				const int64 ChunkStart = ChunkRange.Y + 1;
				const int64 ChunkEnd = FMath::Min(ChunkStart + MaxChunkSize, ContentSize) - 1;

				SharedThis->DownloadFilePerChunk(URL, Timeout, ContentType, MaxChunkSize, FInt64Vector2(ChunkStart, ChunkEnd), OnProgress, OnChunkDownloaded).Next([WeakThisPtr, PromisePtr](bool bSuccess)
				{
					PromisePtr->SetValue(bSuccess);
				});
			}
			else
			{
				PromisePtr->SetValue(true);
			}
		});
	});

	return PromisePtr->GetFuture();
}

TFuture<TArray64<uint8>> FLanguageModelDownloader::DownloadFileByChunk(const FString& URL, float Timeout, const FString& ContentType, int64 ContentSize, FInt64Vector2 ChunkRange, const TFunction<void(int64, int64)>& OnProgress)
{
	if (bCanceled)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file download from %s"), *URL);
		return MakeFulfilledPromise<TArray64<uint8>>(TArray64<uint8>()).GetFuture();
	}

	if (ChunkRange.X < 0 || ChunkRange.Y <= 0 || ChunkRange.X > ChunkRange.Y)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: chunk range (%lld; %lld) is invalid"), *URL, ChunkRange.X, ChunkRange.Y);
		return MakeFulfilledPromise<TArray64<uint8>>(TArray64<uint8>()).GetFuture();
	}

	if (ChunkRange.Y - ChunkRange.X + 1 > ContentSize)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: chunk range (%lld; %lld) is out of range (%lld)"), *URL, ChunkRange.X, ChunkRange.Y, ContentSize);
		return MakeFulfilledPromise<TArray64<uint8>>(TArray64<uint8>()).GetFuture();
	}

	TWeakPtr<FLanguageModelDownloader> WeakThisPtr = AsShared();

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

	HttpRequestRef->OnRequestProgress().BindLambda([WeakThisPtr, ContentSize, ChunkRange, OnProgress](FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
	{
		TSharedPtr<FLanguageModelDownloader> SharedThis = WeakThisPtr.Pin();
		if (SharedThis.IsValid())
		{
			const float Progress = static_cast<float>(BytesReceived) / ContentSize;
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Verbose, TEXT("Downloaded %d bytes of file chunk from %s. Range: {%lld; %lld}, Overall: %lld, Progress: %f"), BytesReceived, *Request->GetURL(), ChunkRange.X, ChunkRange.Y, ContentSize, Progress);
			OnProgress(BytesReceived, ContentSize);
		}
	});

	TSharedPtr<TPromise<TArray64<uint8>>> PromisePtr = MakeShared<TPromise<TArray64<uint8>>>();
	HttpRequestRef->OnProcessRequestComplete().BindLambda([WeakThisPtr, PromisePtr, URL, Timeout, ContentType, ContentSize, ChunkRange, OnProgress](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess) mutable
	{
		TSharedPtr<FLanguageModelDownloader> SharedThis = WeakThisPtr.Pin();
		if (!SharedThis.IsValid())
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Failed to download file chunk from %s: downloader has been destroyed"), *URL);
			PromisePtr->SetValue(TArray64<uint8>());
			return;
		}

		if (SharedThis->bCanceled)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled file chunk download from %s"), *URL);
			PromisePtr->SetValue(TArray64<uint8>());
			return;
		}

		if (!bSuccess || !Response.IsValid())
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: request failed"), *Request->GetURL());
			PromisePtr->SetValue(TArray64<uint8>());
			return;
		}

		if (Response->GetContentLength() <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: content length is 0"), *Request->GetURL());
			PromisePtr->SetValue(TArray64<uint8>());
			return;
		}

		const int64 ContentLength = FCString::Atoi64(*Response->GetHeader("Content-Length"));

		if (ContentLength != ChunkRange.Y - ChunkRange.X + 1)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: content length (%lld) does not match the expected length (%lld)"), *Request->GetURL(), ContentLength, ChunkRange.Y - ChunkRange.X + 1);
			PromisePtr->SetValue(TArray64<uint8>());
			return;
		}
		
		PromisePtr->SetValue(TArray64<uint8>(Response->GetContent()));
	});

	if (!HttpRequestRef->ProcessRequest())
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download file chunk from %s: request failed"), *URL);
		return MakeFulfilledPromise<TArray64<uint8>>(TArray64<uint8>()).GetFuture();
	}

	HttpRequestPtr = HttpRequestRef;
	return PromisePtr->GetFuture();
}

TFuture<int64> FLanguageModelDownloader::GetContentSize(const FString& URL, float Timeout)
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

		if (Response->GetResponseCode() == EHttpResponseCodes::NotFound)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to get size of file from %s: file not found"), *URL);
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

void FLanguageModelDownloader::CancelDownload()
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
