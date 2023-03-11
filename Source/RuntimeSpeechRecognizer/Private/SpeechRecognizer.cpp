// Georgy Treshchev 2023.

#include "SpeechRecognizer.h"
#include "SpeechRecognizerDefines.h"
#include "SpeechRecognizerThread.h"

USpeechRecognizer::USpeechRecognizer()
{
	Thread = MakeShared<FSpeechRecognizerThread>();
	ensure(Thread.IsValid());
	
	Thread->OnRecognitionFinished.BindWeakLambda(this, [this]()
	{
		OnRecognitionFinished.Broadcast();
	});

	Thread->OnRecognitionError.BindWeakLambda(this, [this](const FString& ShortErrorMessage, const FString& LongErrorMessage)
	{
		OnRecognitionError.Broadcast(ShortErrorMessage, LongErrorMessage);
	});

	Thread->OnRecognizedTextSegment.BindWeakLambda(this, [this](const FString& RecognizedWords)
	{
		OnRecognizedTextSegment.Broadcast(RecognizedWords);
	});
}

USpeechRecognizer* USpeechRecognizer::CreateSpeechRecognizer()
{
	return NewObject<USpeechRecognizer>();
}

bool USpeechRecognizer::StartSpeechRecognition()
{
	return Thread->StartThread();
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

bool USpeechRecognizer::GetIsStopped() const
{
	return Thread->GetIsStopped();
}

bool USpeechRecognizer::GetIsFinished() const
{
	return Thread->GetIsFinished();
}

bool USpeechRecognizer::SetRecognitionParameters(const FSpeechRecognitionParameters& Parameters)
{
	return Thread->SetRecognitionParameters(Parameters);
}

bool USpeechRecognizer::SetStreamingDefaults()
{
	return Thread->SetStreamingDefaults();
}

bool USpeechRecognizer::SetNonStreamingDefaults()
{
	return Thread->SetNonStreamingDefaults();
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
