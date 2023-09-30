// Georgy Treshchev 2023.

#include "SpeechRecognizerSettings.h"
#include "SpeechRecognizerDefines.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "RuntimeSpeechRecognizerEditor/Public/RuntimeSpeechRecognizerEditor.h"
#include "Misc/ScopedSlowTask.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Editor.h"
#endif

#define LOCTEXT_NAMESPACE "SpeechRecognizerSettings"

USpeechRecognizerSettings::USpeechRecognizerSettings()
	: ModelSize(ESpeechRecognizerModelSize::Tiny)
  , ModelLanguage(ESpeechRecognizerModelLanguage::EnglishOnly)
#if WITH_EDITORONLY_DATA
  , ModelDownloadBaseUrl(TEXT("https://huggingface.co/ggerganov/whisper.cpp/resolve/main/"))
#endif
{
}

FString USpeechRecognizerSettings::GetLanguageModelAssetName() const
{
	return TEXT("LanguageModel");
}

FString USpeechRecognizerSettings::GetLanguageModelPackagePath() const
{
	return TEXT("/RuntimeSpeechRecognizer");
}

FString USpeechRecognizerSettings::GetLanguageModelFullPackagePath() const
{
	return FPaths::Combine(GetLanguageModelPackagePath(), GetLanguageModelAssetName());
}

FString USpeechRecognizerSettings::GetLanguageModelAssetPath() const
{
	const FString AssetName = GetLanguageModelAssetName();
	return FString::Printf(TEXT("%s.%s"), *FPaths::Combine(*GetLanguageModelPackagePath(), *AssetName), *AssetName);
}

#if WITH_EDITOR
void USpeechRecognizerSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(USpeechRecognizerSettings, ModelSize))
	{
		if (ModelSize == ESpeechRecognizerModelSize::Large)
		{
			ModelLanguage = ESpeechRecognizerModelLanguage::Multilingual;
			SaveConfig();
		}
	}
}

bool USpeechRecognizerSettings::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty) || !InProperty)
	{
		return false;
	}

	const bool bIsInPIE = (GEditor != nullptr && GEditor->PlayWorld != nullptr) || GIsPlayInEditorWorld;

	// Don't allow editing speech recognizer settings while in PIE
	if (bIsInPIE)
	{
		return false;
	}

	if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USpeechRecognizerSettings, ModelLanguage))
	{
		// Can't edit the language if the model size is large since it has only a single option available (multilingual one)
		return ModelSize != ESpeechRecognizerModelSize::Large;
	}

	return true;
}
#endif

#undef LOCTEXT_NAMESPACE
