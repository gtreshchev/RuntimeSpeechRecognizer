// Georgy Treshchev 2023.

#include "RuntimeSpeechRecognizerEditor.h"

#include "Editor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "FileHelpers.h"
#include "HAL/PlatformFileManager.h"
#include "Interfaces/IPluginManager.h"
#include "ISettingsModule.h"
#include "Misc/MessageDialog.h"
#include "ObjectTools.h"
#include "Settings/ProjectPackagingSettings.h"
#include "SpeechRecognizerEditorDefines.h"
#include "SpeechRecognizerModel.h"
#include "SpeechRecognizerModelFactory.h"
#include "SpeechRecognizerSettings.h"
#include "EditorAssetLibrary.h"
#include "IContentBrowserSingleton.h"

#define LOCTEXT_NAMESPACE "FRuntimeSpeechRecognizerEditorModule"

void RegisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "RuntimeSpeechRecognizer",
		                                 LOCTEXT("SpeechRecognizerSettingsName", "Runtime Speech Recognizer"),
		                                 LOCTEXT("SpeechRecognizerSettingsDescription", "Settings for Runtime Speech Recognizer"),
		                                 GetMutableDefault<USpeechRecognizerSettings>()
			);
	}
}

void UnregisterSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "RuntimeSpeechRecognizer");
	}
}

void FRuntimeSpeechRecognizerEditorModule::StartupModule()
{
	RegisterSettings();

	if (!IsRunningDedicatedServer() && !IsRunningCommandlet())
	{
		SetupStaging();
	}
}

void FRuntimeSpeechRecognizerEditorModule::ShutdownModule()
{
	if (!UObjectInitialized())
	{
		return;
	}

	UnregisterSettings();
}

bool FRuntimeSpeechRecognizerEditorModule::SetupStaging() const
{
	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	UProjectPackagingSettings* PackagingSettings = Cast<UProjectPackagingSettings>(UProjectPackagingSettings::StaticClass()->GetDefaultObject());
	if (!PackagingSettings)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("PackagingSettingsMissing", "The RuntimeSpeechRecognizer cannot function correctly because the packaging settings are missing"));
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot get the project packaging settings"));
		return false;
	}

	const USpeechRecognizerSettings* SpeechRecognizerSettings = GetDefault<USpeechRecognizerSettings>();
	if (!SpeechRecognizerSettings)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SpeechRecognizerSettingsMissing", "The RuntimeSpeechRecognizer cannot function correctly because the recognizer settings are missing"));
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot get the speech recognizer settings"));
		return false;
	}

	const ESpeechRecognizerModelSize ModelSize = SpeechRecognizerSettings->ModelSize;
	const ESpeechRecognizerModelLanguage ModelLanguage = SpeechRecognizerSettings->ModelLanguage;

	const FString EditorLMDirectoryPath = GetEditorLMDirectoryPath(ModelSize, ModelLanguage);
	const FString EditorLMFilePath = GetEditorLMFilePath(ModelSize, ModelLanguage);

	const FString EditorLMFilePathFull = FPaths::ConvertRelativePathToFull(EditorLMFilePath);

	// Making sure the language model file exists
	if (!PlatformFile.FileExists(*EditorLMFilePathFull))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LanguageModelMissing", "The RuntimeSpeechRecognizer cannot function correctly because the language model file is missing. The file should be located at: {0}"), FText::FromString(EditorLMFilePathFull)));
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot find language model file: %s"), *EditorLMFilePathFull);
		return false;
	}

	const FString AssetName = SpeechRecognizerSettings->GetLanguageModelAssetName();
	const FString PackagePath = SpeechRecognizerSettings->GetLanguageModelPackagePath();
	const FString AssetPath = SpeechRecognizerSettings->GetLanguageModelAssetPath();

	// Delete the language model asset if it already exists
	if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
	{
		const bool bDeleteSucceeded = UEditorAssetLibrary::DeleteAsset(AssetPath);
		if (!bDeleteSucceeded)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("LanguageModelDeletionFailed", "The RuntimeSpeechRecognizer cannot function correctly because the language model asset could not be deleted to be replaced"));
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot delete language model asset"));
			return false;
		}

		// Collect garbage to completely remove the asset from memory
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}

	USpeechRecognizerModelFactory* Factory = NewObject<USpeechRecognizerModelFactory>();
	Factory->LanguageModelPath = EditorLMFilePathFull;

	UObject* LanguageModelAsset = AssetTools.CreateAsset(AssetName, PackagePath, USpeechRecognizerModel::StaticClass(), Factory);
	if (!LanguageModelAsset)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("LanguageModelCreationFailed", "The RuntimeSpeechRecognizer cannot function correctly because the language model asset could not be created"));
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot create language model asset"));
		return false;
	}

	UPackage* LanguageModelPackage = LanguageModelAsset->GetOutermost();
	if (!LanguageModelPackage)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("LanguageModelCreationFailed", "The RuntimeSpeechRecognizer cannot function correctly because the language model package could not be retrieved from the asset"));
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot get language model package"));
		return false;
	}

	FEditorFileUtils::EPromptReturnCode Result = [LanguageModelPackage]()
	{
		TArray<UPackage*> PackagesToSave;
		PackagesToSave.Add(LanguageModelPackage);

		constexpr bool bCheckDirty = true;
		constexpr bool bPromptToSave = false;
		return FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave);
	}();

	if (Result != FEditorFileUtils::PR_Success)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LanguageModelSaveFailed", "The RuntimeSpeechRecognizer cannot function correctly because the language model asset could not be saved. The error code is: {0}"), FText::AsNumber(Result)));
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot save language model asset, error code: %d"), Result);
		return false;
	}

	return true;
}

FString FRuntimeSpeechRecognizerEditorModule::GetEditorLMDirectoryPath(ESpeechRecognizerModelSize ModelSize, ESpeechRecognizerModelLanguage ModelLanguage) const
{
	const FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("RuntimeSpeechRecognizer"))->GetContentDir();
	return ContentDir;
}

FString FRuntimeSpeechRecognizerEditorModule::GetEditorLMFilePath(ESpeechRecognizerModelSize ModelSize, ESpeechRecognizerModelLanguage ModelLanguage) const
{
	const FString ModelSizeString = ModelSizeToStringForFileName(ModelSize);
	return FPaths::Combine(GetEditorLMDirectoryPath(ModelSize, ModelLanguage), FString::Printf(TEXT("ggml-%s%s.bin"), *ModelSizeString, ModelLanguage == ESpeechRecognizerModelLanguage::EnglishOnly ? TEXT(".en") : TEXT("")));
}

FString FRuntimeSpeechRecognizerEditorModule::ModelSizeToStringForFileName(ESpeechRecognizerModelSize ModelSize) const
{
	switch (ModelSize)
	{
	case ESpeechRecognizerModelSize::Tiny:
		return TEXT("tiny");
	case ESpeechRecognizerModelSize::Base:
		return TEXT("base");
	case ESpeechRecognizerModelSize::Small:
		return TEXT("small");
	case ESpeechRecognizerModelSize::Medium:
		return TEXT("medium");
	case ESpeechRecognizerModelSize::Large:
		return TEXT("large");
	default:
		return TEXT("invalid");
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRuntimeSpeechRecognizerEditorModule, RuntimeSpeechRecognizerEditor)

DEFINE_LOG_CATEGORY(LogEditorRuntimeSpeechRecognizer);
