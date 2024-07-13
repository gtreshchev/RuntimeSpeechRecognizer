// Georgy Treshchev 2024.

#include "SpeechRecognizerSettings.h"
#include "SpeechRecognizerDefines.h"
#include "Misc/Paths.h"
#include "SpeechRecognizerTypes.h"

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
	return TEXT("/RuntimeSpeechRecognizer/LanguageModels");
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
		if (!DoesSupportEnglishOnlyModelLanguage(ModelSize) || ModelSize == ESpeechRecognizerModelSize::Custom)
		{
			ModelLanguage = ESpeechRecognizerModelLanguage::Multilingual;
#if UE_VERSION_OLDER_THAN(5, 0, 0)
			UpdateDefaultConfigFile();
#else
			TryUpdateDefaultConfigFile();
#endif
			SaveConfig();
		}
		else if (!DoesSupportMultilingualModelLanguage(ModelSize))
		{
			ModelLanguage = ESpeechRecognizerModelLanguage::EnglishOnly;
#if UE_VERSION_OLDER_THAN(5, 0, 0)
			UpdateDefaultConfigFile();
#else
			TryUpdateDefaultConfigFile();
#endif
			SaveConfig();
		}

		if (ModelSize != ESpeechRecognizerModelSize::Custom)
		{
			ModelDownloadBaseUrl = GetModelDownloadBaseUrl(ModelSize, ModelLanguage);
#if UE_VERSION_OLDER_THAN(5, 0, 0)
			UpdateDefaultConfigFile();
#else
			TryUpdateDefaultConfigFile();
#endif
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
		return DoesSupportEnglishOnlyModelLanguage(ModelSize) && DoesSupportMultilingualModelLanguage(ModelSize)
		&& ModelSize != ESpeechRecognizerModelSize::Custom;
	}

	if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USpeechRecognizerSettings, ModelDownloadCustomName))
	{
		return ModelSize == ESpeechRecognizerModelSize::Custom;
	}

	return true;
}
#endif

#undef LOCTEXT_NAMESPACE
