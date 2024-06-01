// Georgy Treshchev 2024.

#include "SpeechRecognizer.h"
#include "SpeechRecognizerDefines.h"
#include "SpeechRecognizerThread.h"

USpeechRecognizer::USpeechRecognizer()
{
	Thread = MakeShared<FSpeechRecognizerThread>();
	ensure(Thread.IsValid());

	Thread->OnRecognitionFinished.AddWeakLambda(this, [this]()
	{
		OnRecognitionFinished.Broadcast();
		OnRecognitionFinishedNative.Broadcast();
	});

	Thread->OnRecognitionError.AddWeakLambda(this, [this](const FString& ShortErrorMessage, const FString& LongErrorMessage)
	{
		OnRecognitionError.Broadcast(ShortErrorMessage, LongErrorMessage);
		OnRecognitionErrorNative.Broadcast(ShortErrorMessage, LongErrorMessage);
	});

	Thread->OnRecognizedTextSegment.AddWeakLambda(this, [this](const FString& RecognizedWords)
	{
		OnRecognizedTextSegment.Broadcast(RecognizedWords);
		OnRecognizedTextSegmentNative.Broadcast(RecognizedWords);
	});

	Thread->OnRecognitionProgress.AddWeakLambda(this, [this](int32 Progress)
	{
		OnRecognitionProgress.Broadcast(Progress);
		OnRecognitionProgressNative.Broadcast(Progress);
	});

	Thread->OnRecognitionStopped.AddWeakLambda(this, [this]()
	{
		OnRecognitionStopped.Broadcast();
		OnRecognitionStoppedNative.Broadcast();
	});
}

USpeechRecognizer* USpeechRecognizer::CreateSpeechRecognizer()
{
	return NewObject<USpeechRecognizer>();
}

void USpeechRecognizer::StartSpeechRecognition(const FOnSpeechRecognitionStartedDynamic& OnStarted)
{
	StartSpeechRecognition(FOnSpeechRecognitionStartedStatic::CreateWeakLambda(this, [OnStarted](bool bSucceeded) { OnStarted.ExecuteIfBound(bSucceeded); }));
}

void USpeechRecognizer::StartSpeechRecognition(const FOnSpeechRecognitionStartedStatic& OnStarted)
{
	if (!Thread->GetIsStopped())
	{
		UE_LOG(LogRuntimeSpeechRecognizer, Error, TEXT("Failed to start speech recognition: Speech recognition is already running"));
		OnStarted.ExecuteIfBound(false);
		return;
	}
	if (Thread->GetIsStopping())
	{
		UE_LOG(LogRuntimeSpeechRecognizer, Error, TEXT("Failed to start speech recognition: Speech recognition is stopping"));
		OnStarted.ExecuteIfBound(false);
		return;
	}
	Thread->StartThread().Next([OnStarted](bool bSucceeded) { OnStarted.ExecuteIfBound(bSucceeded); });
}

void USpeechRecognizer::StopSpeechRecognition()
{
	Thread->StopThread();
}

void USpeechRecognizer::ProcessAudioData(TArray<float> PCMData, float SampleRate, int32 NumOfChannels, bool bLast)
{
	ProcessAudioData(Audio::FAlignedFloatBuffer(MoveTemp(PCMData)), SampleRate, NumOfChannels, bLast);
}

void USpeechRecognizer::ProcessAudioData(Audio::FAlignedFloatBuffer PCMData, float SampleRate, int32 NumOfChannels, bool bLast)
{
	Thread->ProcessPCMData(MoveTemp(PCMData), SampleRate, NumOfChannels, bLast);
}

void USpeechRecognizer::ForceProcessPendingAudioData()
{
	Thread->ForceProcessPendingAudioData();
}

void USpeechRecognizer::ClearAudioData(bool bClearPendingAudioData, bool bClearAudioQueue)
{
	Thread->ClearAudioData(bClearPendingAudioData, bClearAudioQueue);
}

bool USpeechRecognizer::GetIsStopped() const
{
	return Thread->GetIsStopped();
}

bool USpeechRecognizer::GetIsStopping() const
{
	return Thread->GetIsStopping();
}

bool USpeechRecognizer::GetIsFinished() const
{
	return Thread->GetIsFinished();
}

bool USpeechRecognizer::SetRecognitionParameters(const FSpeechRecognitionParameters& Parameters)
{
	return Thread->SetRecognitionParameters(Parameters);
}

FSpeechRecognitionParameters USpeechRecognizer::GetNonStreamingDefaults()
{
	return FSpeechRecognizerThread::GetNonStreamingDefaults();
}

FSpeechRecognitionParameters USpeechRecognizer::GetStreamingDefaults()
{
	return FSpeechRecognizerThread::GetStreamingDefaults();
}

FSpeechRecognitionParameters USpeechRecognizer::GetRecognitionParameters() const
{
	return Thread->GetRecognitionParameters();
}

bool USpeechRecognizer::SetNonStreamingDefaults()
{
	return Thread->SetNonStreamingDefaults();
}

bool USpeechRecognizer::SetStreamingDefaults()
{
	return Thread->SetStreamingDefaults();
}

bool USpeechRecognizer::SetNumOfThreads(int32 Value)
{
	return Thread->SetNumOfThreads(Value);
}

bool USpeechRecognizer::SetLanguage(ESpeechRecognizerLanguage Language)
{
	return Thread->SetLanguage(Language);
}

bool USpeechRecognizer::SetTranslateToEnglish(bool bTranslate)
{
	return Thread->SetTranslateToEnglish(bTranslate);
}

bool USpeechRecognizer::SetStepSize(int32 Value)
{
	return Thread->SetStepSize(Value);
}

bool USpeechRecognizer::SetNoContext(bool bNoContext)
{
	return Thread->SetNoContext(bNoContext);
}

bool USpeechRecognizer::SetSingleSegment(bool bSingleSegment)
{
	return Thread->SetSingleSegment(bSingleSegment);
}

bool USpeechRecognizer::SetMaxTokens(int32 Value)
{
	return Thread->SetMaxTokens(Value);
}

bool USpeechRecognizer::SetSpeedUp(bool bSpeedUp)
{
	return Thread->SetSpeedUp(bSpeedUp);
}

bool USpeechRecognizer::SetAudioContextSize(int32 Value)
{
	return Thread->SetAudioContextSize(Value);
}

bool USpeechRecognizer::SetTemperatureToIncrease(float Value)
{
	return Thread->SetTemperatureToIncrease(Value);
}

bool USpeechRecognizer::SetEntropyThreshold(float Value)
{
	return Thread->SetEntropyThreshold(Value);
}

bool USpeechRecognizer::SetSuppressBlank(bool Value)
{
	return Thread->SetSuppressBlank(Value);
}

bool USpeechRecognizer::SetSuppressNonSpeechTokens(bool Value)
{
	return Thread->SetSuppressNonSpeechTokens(Value);
}

bool USpeechRecognizer::SetBeamSize(int32 Value)
{
	return Thread->SetBeamSize(Value);
}

bool USpeechRecognizer::SetInitialPrompt(const FString& Value)
{
	return Thread->SetInitialPrompt(Value);
}
