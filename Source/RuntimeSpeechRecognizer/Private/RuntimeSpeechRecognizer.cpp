// Georgy Treshchev 2023.

#include "RuntimeSpeechRecognizer.h"
#include "SpeechRecognizerDefines.h"

#define LOCTEXT_NAMESPACE "FRuntimeSpeechRecognizerModule"

void FRuntimeSpeechRecognizerModule::StartupModule()
{
}

void FRuntimeSpeechRecognizerModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRuntimeSpeechRecognizerModule, RuntimeSpeechRecognizer)

DEFINE_LOG_CATEGORY(LogRuntimeSpeechRecognizer);
