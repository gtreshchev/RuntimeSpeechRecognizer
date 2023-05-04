// Fill out your copyright notice in the Description page of Project Settings.


#include "SpeechRecognizerDownloader.h"

#include "HttpModule.h"
#include "SpeechRecognizerEditorDefines.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

ULanguageModelDownloader::ULanguageModelDownloader()
	: bCanceled(false)
{
}

TFuture<TArray64<uint8>> ULanguageModelDownloader::DownloadFile(const FString& URL, const TFunction<void(float)>& OnProgress)
{
	constexpr float Timeout = 5.0f;
	TSharedPtr<TPromise<TArray64<uint8>>> PromisePtr = MakeShared<TPromise<TArray64<uint8>>>();
	GetLanguageModelSize(URL, Timeout).Next([this, PromisePtr, URL, OnProgress](int64 ContentLength)
	{
		if (ContentLength <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download language model: content length is 0"));
			PromisePtr->SetValue(TArray64<uint8>());
			return;
		}

		DownloadFileByChunk(URL, ContentLength, TNumericLimits<TArray<uint8>::SizeType>::Max(), OnProgress).Next([PromisePtr](TArray64<uint8>&& ResultData)
		{
			PromisePtr->SetValue(MoveTemp(ResultData));
		});
	});
	return PromisePtr->GetFuture();
}

TFuture<TArray64<uint8>> ULanguageModelDownloader::DownloadFileByChunk(const FString& URL, int64 LanguageModelSize, int64 MaxChunkSize, const TFunction<void(float)>& OnProgress, FInt64Vector2 InternalContentRange, TArray64<uint8>&& InternalResultData)
{
	// Check if download has been canceled before starting the download
	if (bCanceled)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Canceled download of language model from %s"), *URL);
		return MakeFulfilledPromise<TArray64<uint8>>(TArray64<uint8>()).GetFuture();
	}

	if (MaxChunkSize <= 0)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download language model chunk from %s: MaxChunkSize is 0"), *URL);
		return MakeFulfilledPromise<TArray64<uint8>>(TArray64<uint8>()).GetFuture();
	}

	// If the InternalResultData was not provided, initialize it to the size of the language model
	if (InternalResultData.Num() <= 0)
	{
		InternalResultData.SetNumUninitialized(LanguageModelSize);
	}

	// If the InternalContentRange was not provided, set it to the first chunk of size MaxChunkSize
	if (InternalContentRange.X == 0 && InternalContentRange.Y == 0)
	{
		InternalContentRange.Y = FMath::Min(LanguageModelSize, MaxChunkSize) - 1;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequestRef = FHttpModule::Get().CreateRequest();
	HttpRequestRef->SetVerb("GET");
	HttpRequestRef->SetURL(URL);

	const FString RangeHeaderValue = FString::Format(TEXT("bytes={0}-{1}"), {InternalContentRange.X, InternalContentRange.Y});
	HttpRequestRef->SetHeader(TEXT("Range"), RangeHeaderValue);

	HttpRequestRef->OnRequestProgress().BindWeakLambda(this, [LanguageModelSize, InternalContentRange, OnProgress](FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
	{
		const float Progress = static_cast<float>(InternalContentRange.X + BytesReceived) / LanguageModelSize;
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Verbose, TEXT("Downloaded %d bytes of language model chunk from %s. Range: {%lld; %lld}, Overall: %lld, Progress: %f"), BytesReceived, *Request->GetURL(), InternalContentRange.X, InternalContentRange.Y, LanguageModelSize, Progress);
		OnProgress(Progress);
	});

	TSharedPtr<TPromise<TArray64<uint8>>> PromisePtr = MakeShared<TPromise<TArray64<uint8>>>();
	HttpRequestRef->OnProcessRequestComplete().BindWeakLambda(this, [this, PromisePtr, URL, LanguageModelSize, MaxChunkSize, InternalContentRange, InternalResultData = MoveTemp(InternalResultData), OnProgress](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess) mutable
	{
		if (!bSuccess || !Response.IsValid())
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download language model chunk from %s: request failed"), *Request->GetURL());
			PromisePtr->SetValue(TArray64<uint8>());
			return;
		}

		if (Response->GetContentLength() <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download language model chunk from %s: content length is 0"), *Request->GetURL());
			PromisePtr->SetValue(TArray64<uint8>());
			return;
		}

		const int64 DataOffset = InternalContentRange.X;
		const TArray<uint8>& ResponseContent = Response->GetContent();

		// Calculate the overall size of the downloaded content in the result buffer
		const int64 OverallDownloadedSize = DataOffset + ResponseContent.Num();

		// Check if some values are out of range
		{
			if (DataOffset < 0 || DataOffset >= InternalResultData.Num())
			{
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download language model chunk from %s: data offset is out of range"), *Request->GetURL());
				PromisePtr->SetValue(TArray64<uint8>());
				return;
			}

			if (OverallDownloadedSize > InternalResultData.Num())
			{
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to download language model chunk from %s: overall downloaded size is out of range"), *Request->GetURL());
				PromisePtr->SetValue(TArray64<uint8>());
				return;
			}
		}

		FMemory::Memcpy(InternalResultData.GetData() + DataOffset, ResponseContent.GetData(), ResponseContent.Num());

		// Check if there's still more content to download
		if (OverallDownloadedSize < LanguageModelSize)
		{
			// Calculate how much more data needs to be downloaded in the next chunk
			const int64 BytesRemaining = LanguageModelSize - OverallDownloadedSize;
			const int64 BytesToDownload = FMath::Min(BytesRemaining, MaxChunkSize);

			// Calculate the range of data to download in the next chunk
			const FInt64Vector2 NewContentRange = FInt64Vector2(OverallDownloadedSize, OverallDownloadedSize + BytesToDownload - 1);

			// Initiate the next download chunk
			DownloadFileByChunk(URL, LanguageModelSize, MaxChunkSize, OnProgress, NewContentRange, MoveTemp(InternalResultData)).Next([PromisePtr](TArray64<uint8>&& ResultData)
			{
				PromisePtr->SetValue(MoveTemp(ResultData));
			});
		}
		else
		{
			// If there is no more content to download, then the download is complete
			PromisePtr->SetValue(MoveTemp(InternalResultData));
		}
	});

	HttpRequestRef->ProcessRequest();
	HttpRequestPtr = HttpRequestRef;
	return PromisePtr->GetFuture();
}

void ULanguageModelDownloader::CancelDownload()
{
	bCanceled = true;
	if (HttpRequestPtr.IsValid())
	{
		TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = HttpRequestPtr.Pin();
		HttpRequest->CancelRequest();
	}
	UE_LOG(LogEditorRuntimeSpeechRecognizer, Warning, TEXT("Language model download canceled"));
}

TFuture<int64> ULanguageModelDownloader::GetLanguageModelSize(const FString& URL, float Timeout)
{
	TSharedPtr<TPromise<int64>> PromisePtr = MakeShared<TPromise<int64>>();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequestRef = FHttpModule::Get().CreateRequest();

	HttpRequestRef->SetVerb("HEAD");
	HttpRequestRef->SetURL(URL);

	HttpRequestRef->SetTimeout(Timeout);

	HttpRequestRef->OnProcessRequestComplete().BindWeakLambda(this, [PromisePtr](const FHttpRequestPtr& Request, const FHttpResponsePtr& Response, const bool bSucceeded)
	{
		const int64 ContentLength = FCString::Atoi64(Response.IsValid() ? *Response->GetHeader("Content-Length") : TEXT("0"));
		if (ContentLength <= 0)
		{
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to get size of language model: content length is 0"));
			PromisePtr->SetValue(0);
			return;
		}
		PromisePtr->SetValue(ContentLength);
	});

	if (!HttpRequestRef->ProcessRequest())
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to get size of language model: request failed"));
		return MakeFulfilledPromise<int64>(0).GetFuture();
	}

	HttpRequestPtr = HttpRequestRef;
	return PromisePtr->GetFuture();
}
