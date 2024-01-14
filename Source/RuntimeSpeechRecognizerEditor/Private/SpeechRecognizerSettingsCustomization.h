// Georgy Treshchev 2024.

#pragma once

#include "CoreMinimal.h"
#include "DetailCategoryBuilder.h"
#include "Input/Reply.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "RuntimeSpeechRecognizerEditor.h"

#define LOCTEXT_NAMESPACE "SpeechRecognizerSettings"

/**
 * Customization for the SpeechRecognizerSettings class. Adds buttons to setup and delete language models.
 */
class FSpeechRecognizerSettingsCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FSpeechRecognizerSettingsCustomization);
	}

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override
	{
		IDetailCategoryBuilder& RuntimeSpeechRecognizerCategory = DetailLayout.EditCategory(TEXT("Runtime Speech Recognizer"));
		RuntimeSpeechRecognizerCategory.AddCustomRow(LOCTEXT("SpeechRecognizerSettingsActions", "Speech Recognizer Settings Actions"), false)
		                               .WholeRowWidget
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  .Padding(FMargin(0, 5, 5, 5))
			  .AutoWidth()
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnClicked(this, &FSpeechRecognizerSettingsCustomization::OnSetupLanguageModelClicked)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SetupLanguageModel", "Setup Language Model"))
					.ToolTipText(LOCTEXT("SetupLanguageModel_Tooltip", "Stages the language model to a separate asset. Asks to automatically download the language model from the server if it is not present."))
				]
			]
			+ SHorizontalBox::Slot()
			  .Padding(FMargin(0, 5, 5, 5))
			  .AutoWidth()
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnClicked(this, &FSpeechRecognizerSettingsCustomization::OnDeleteLanguageModelsClicked)
				.IsEnabled(this, &FSpeechRecognizerSettingsCustomization::IsDeleteLanguageModelsEnabled)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DeleteLanguageModels", "Clear Language Models"))
					.ToolTipText(LOCTEXT("DeleteLanguageModels_Tooltip", "Deletes the language models from the local cache. After that, the language models need to be downloaded again (see Setup Language Model)."))
				]
			]
		];
	}
	// End of IDetailCustomization interface

private:
	FReply OnSetupLanguageModelClicked()
	{
		{
			FNotificationInfo Info(LOCTEXT("SpeechRecognizerSettingUpProgress", "Setting up language model..."));
			Info.FadeInDuration = 0.0f;
			FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Pending);
		}

		const FRuntimeSpeechRecognizerEditorModule& RecognizerEditorModule = FModuleManager::LoadModuleChecked<FRuntimeSpeechRecognizerEditorModule>("RuntimeSpeechRecognizerEditor");
		RecognizerEditorModule.SetupLanguageModel().Next([this](bool bSuccess)
		{
			if (bSuccess)
			{
				FNotificationInfo Info(LOCTEXT("SpeechRecognizerSettingUpSucceded", "Setting up language model succeeded"));
				FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Success);
			}
			else
			{
				FNotificationInfo Info(LOCTEXT("SpeechRecognizerSettingUpFailed", "Setting up language model failed"));
				Info.ExpireDuration = 5.0f;
				FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
			}
		});

		return FReply::Handled();
	}

	FReply OnDeleteLanguageModelsClicked()
	{
		{
			FNotificationInfo Info(LOCTEXT("DeletingLanguageModelsProgress", "Deleting language models..."));
			Info.FadeInDuration = 0.0f;
			FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Pending);
		}

		FRuntimeSpeechRecognizerEditorModule& RecognizerEditorModule = FModuleManager::LoadModuleChecked<FRuntimeSpeechRecognizerEditorModule>("RuntimeSpeechRecognizerEditor");
		if (RecognizerEditorModule.DeleteLanguageModels(true, true))
		{
			FNotificationInfo Info(LOCTEXT("DeletingLanguageModelsSucceded", "Deleting language models succeeded. Do not forget to setup the language model again."));
			FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Success);
		}
		else
		{
			FNotificationInfo Info(LOCTEXT("DeletingLanguageModelsFailed", "Deleting language models failed"));
			FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
		}

		return FReply::Handled();
	}

	/**
	 * Checks if the language model is present and if it is not, checks if it can be deleted
	 * @return True if the language model can be deleted, false otherwise
	 */
	bool IsDeleteLanguageModelsEnabled() const
	{
		FRuntimeSpeechRecognizerEditorModule& RecognizerEditorModule = FModuleManager::LoadModuleChecked<FRuntimeSpeechRecognizerEditorModule>("RuntimeSpeechRecognizerEditor");
		return RecognizerEditorModule.IsLanguageModelPresent(true, false);
	}
};

#undef LOCTEXT_NAMESPACE
