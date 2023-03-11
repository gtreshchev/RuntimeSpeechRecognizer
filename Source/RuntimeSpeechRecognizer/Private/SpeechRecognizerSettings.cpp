// Georgy Treshchev 2023.

#include "SpeechRecognizerSettings.h"
#include "SpeechRecognizerDefines.h"

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
{
}

FString USpeechRecognizerSettings::GetLanguageModelAssetName() const
{
	return TEXT("LanguageModel");
}

FString USpeechRecognizerSettings::GetLanguageModelPackagePath() const
{
	return TEXT("/Game/RuntimeSpeechRecognizer/");
}

FString USpeechRecognizerSettings::GetLanguageModelAssetPath() const
{
	const FString AssetName = GetLanguageModelAssetName();
	return FString::Printf(TEXT("%s%s.%s"), *GetLanguageModelPackagePath(), *AssetName, *AssetName);
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

	// Copying language model to project content directory (staging)
	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(USpeechRecognizerSettings, ModelSize) ||
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(USpeechRecognizerSettings, ModelLanguage))
	{
		// TODO: Make this window appear instantly (now it is displayed only after processing, as it uses one thread)
		{
			FNotificationInfo Info(LOCTEXT("SettingSpeechRecognizerPackagingProgress", "Copying language model to project content directory..."));
			Info.FadeInDuration = 0.0f;
			FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Pending);
		}

		const FRuntimeSpeechRecognizerEditorModule& MyEditorModule = FModuleManager::LoadModuleChecked<FRuntimeSpeechRecognizerEditorModule>("RuntimeSpeechRecognizerEditor");
		if (!MyEditorModule.SetupStaging())
		{
			FNotificationInfo Info(LOCTEXT("SettingSpeechRecognizerPackagingFailed", "Copying language model to project content directory failed"));
			Info.ExpireDuration = 5.0f;
			FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
		}

		{
			FNotificationInfo Info(LOCTEXT("SettingSpeechRecognizerPackagingSuccess", "Copying language model to project content directory succeeded"));
			FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Success);
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
