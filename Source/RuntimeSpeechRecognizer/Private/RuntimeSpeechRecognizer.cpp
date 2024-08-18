// Georgy Treshchev 2024.

#include "RuntimeSpeechRecognizer.h"
#include "SpeechRecognizerDefines.h"

#ifdef GGML_USE_BLAS
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#endif

#define LOCTEXT_NAMESPACE "FRuntimeSpeechRecognizerModule"

void FRuntimeSpeechRecognizerModule::StartupModule()
{
#ifdef GGML_USE_BLAS
// TODO: Add support for other platforms
#if PLATFORM_WINDOWS
	const FString BasePluginDir = IPluginManager::Get().FindPlugin("RuntimeSpeechRecognizer")->GetBaseDir();
	const FString OpenBLASBinPath = FPaths::Combine(*BasePluginDir, TEXT("Source"), TEXT("ThirdParty"), TEXT("OpenBLAS"), TEXT("bin"));
	const FString OpenBLASDLLPath = FPaths::Combine(*OpenBLASBinPath, TEXT("libopenblas.dll"));

	FPlatformProcess::PushDllDirectory(*FPaths::GetPath(OpenBLASBinPath));
	OpenBLASLibHandle = FPlatformProcess::GetDllHandle(*OpenBLASDLLPath);
	FPlatformProcess::PopDllDirectory(*FPaths::GetPath(OpenBLASBinPath));
	
	if (OpenBLASLibHandle)
	{
		UE_LOG(LogRuntimeSpeechRecognizer, Log, TEXT("Successfully loaded OpenBLAS library: %s"), *OpenBLASDLLPath);
	}
	else
	{
		UE_LOG(LogRuntimeSpeechRecognizer, Fatal, TEXT("Failed to load OpenBLAS library: %s"), *OpenBLASDLLPath);
	}
#else
#error "OpenBLAS is only supported on Windows"
#endif
#endif
}

void FRuntimeSpeechRecognizerModule::ShutdownModule()
{
#ifdef GGML_USE_BLAS
	if (OpenBLASLibHandle)
	{
		FPlatformProcess::FreeDllHandle(OpenBLASLibHandle);
		OpenBLASLibHandle = nullptr;
	}
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRuntimeSpeechRecognizerModule, RuntimeSpeechRecognizer)

DEFINE_LOG_CATEGORY(LogRuntimeSpeechRecognizer);
