// Georgy Treshchev 2023.

#include "RuntimeSpeechRecognizerEditor.h"
#include "Editor.h"
#include "AssetToolsModule.h"
#include "FileHelpers.h"
#include "HAL/PlatformFileManager.h"
#include "Interfaces/IPluginManager.h"
#include "ISettingsModule.h"
#include "Settings/ProjectPackagingSettings.h"
#include "SpeechRecognizerEditorDefines.h"
#include "SpeechRecognizerModel.h"
#include "SpeechRecognizerModelFactory.h"
#include "SpeechRecognizerSettings.h"
#include "SpeechRecognizerDownloader.h"
#include "SpeechRecognizerSettingsCustomization.h"
#include "SpeechRecognizerProgressWindow.h"
#include "Misc/FileHelper.h"

/** The language model file extension */
static const FString LanguageModelExtension = TEXT("bin");

#define LOCTEXT_NAMESPACE "FRuntimeSpeechRecognizerEditorModule"

void RegisterSettings()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		USpeechRecognizerSettings::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FSpeechRecognizerSettingsCustomization::MakeInstance)
	);

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
		SetupLanguageModel();
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

TFuture<bool> FRuntimeSpeechRecognizerEditorModule::SetupLanguageModel() const
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	UProjectPackagingSettings* PackagingSettings = Cast<UProjectPackagingSettings>(UProjectPackagingSettings::StaticClass()->GetDefaultObject());
	if (!PackagingSettings)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("PackagingSettingsMissing", "The RuntimeSpeechRecognizer cannot function correctly because the packaging settings are missing"));
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot get the project packaging settings"));
		return MakeFulfilledPromise<bool>(false).GetFuture();
	}

	const USpeechRecognizerSettings* SpeechRecognizerSettings = GetDefault<USpeechRecognizerSettings>();
	if (!SpeechRecognizerSettings)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SpeechRecognizerSettingsMissing", "The RuntimeSpeechRecognizer cannot function correctly because the recognizer settings are missing"));
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot get the speech recognizer settings"));
		return MakeFulfilledPromise<bool>(false).GetFuture();
	}

	DeleteOldLanguageModels();

	const ESpeechRecognizerModelSize ModelSize = SpeechRecognizerSettings->ModelSize;
	const ESpeechRecognizerModelLanguage ModelLanguage = SpeechRecognizerSettings->ModelLanguage;

	const FString EditorLMDirectoryPath = GetEditorLMDirectoryPath();
	const FString EditorLMFilePath = GetEditorLMFilePath(ModelSize, ModelLanguage);

	const FString EditorLMFilePathFull = FPaths::ConvertRelativePathToFull(EditorLMFilePath);

	TFuture<bool> DownloadFuture;

	// Making sure the language model file exists
	if (!PlatformFile.FileExists(*EditorLMFilePathFull))
	{
		DownloadFuture = DownloadLanguageModel(ModelSize, ModelLanguage).Next([EditorLMFilePathFull](bool bDownloadSucceeded) mutable
		{
			if (!bDownloadSucceeded)
			{
				return false;
			}

			if (!FPaths::FileExists(EditorLMFilePathFull))
			{
				FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("LanguageModelDownloadFailed", "The RuntimeSpeechRecognizer cannot function correctly because the language model file could not be downloaded"));
				UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot download language model file: %s"), *EditorLMFilePathFull);
				return false;
			}

			return true;
		});
	}
	else
	{
		DownloadFuture = MakeFulfilledPromise<bool>(true).GetFuture();
	}

	FString AssetName = SpeechRecognizerSettings->GetLanguageModelAssetName();
	FString PackagePath = SpeechRecognizerSettings->GetLanguageModelPackagePath();
	FString AssetPath = SpeechRecognizerSettings->GetLanguageModelAssetPath();

	return DownloadFuture.Next([this, EditorLMFilePathFull, AssetName = MoveTemp(AssetName), PackagePath = MoveTemp(PackagePath), AssetPath = MoveTemp(AssetPath)](bool bDownloadSucceeded) mutable
	{
		if (!bDownloadSucceeded)
		{
			return false;
		}

		if (!DeleteLanguageModels(false, true))
		{
			return false;
		}

		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
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

		UpdatePackagingSettings();
		return true;
	});
}

bool FRuntimeSpeechRecognizerEditorModule::DeleteLanguageModels(bool bDeleteFromLocalCache, bool bDeleteFromAsset) const
{
	if (bDeleteFromLocalCache)
	{
		const FString EditorLMDirectoryPath = FPaths::ConvertRelativePathToFull(GetEditorLMDirectoryPath());
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		TArray<FString> Files;
		PlatformFile.FindFilesRecursively(Files, *EditorLMDirectoryPath, *LanguageModelExtension);

		for (const FString& File : Files)
		{
			PlatformFile.DeleteFile(*File);
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Log, TEXT("Deleted local cache for Runtime Speech Recognizer: %s"), *File);
		}

		UE_LOG(LogEditorRuntimeSpeechRecognizer, Log, TEXT("Deleted local cache for Runtime Speech Recognizer. Do not forget to download the language model again! %s"), *EditorLMDirectoryPath);
	}

	if (bDeleteFromAsset)
	{
		const USpeechRecognizerSettings* SpeechRecognizerSettings = GetDefault<USpeechRecognizerSettings>();
		if (!SpeechRecognizerSettings)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SpeechRecognizerSettingsMissing", "The RuntimeSpeechRecognizer cannot function correctly because the recognizer settings are missing"));
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot get the speech recognizer settings"));
			return false;
		}

		const FString AssetPath = FPaths::ChangeExtension(SpeechRecognizerSettings->GetLanguageModelAssetPath(), TEXT(""));

		// Delete the language model asset if it already exists
		// Commented out to delete the asset more reliably
		/*if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
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
		}*/

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		const FString AbsoluteAssetPath = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(*AssetPath, TEXT(".uasset")));
		if (PlatformFile.FileExists(*AbsoluteAssetPath))
		{
			PlatformFile.DeleteFile(*AbsoluteAssetPath);
		}
	}

	return true;
}

bool FRuntimeSpeechRecognizerEditorModule::IsLanguageModelPresent(bool bCheckLocalCache, bool bCheckAsset) const
{
	const USpeechRecognizerSettings* SpeechRecognizerSettings = GetDefault<USpeechRecognizerSettings>();
	if (!SpeechRecognizerSettings)
	{
		return false;
	}

	if (bCheckLocalCache)
	{
		const ESpeechRecognizerModelSize ModelSize = SpeechRecognizerSettings->ModelSize;
		const ESpeechRecognizerModelLanguage ModelLanguage = SpeechRecognizerSettings->ModelLanguage;

		const FString EditorLMFilePath = GetEditorLMFilePath(ModelSize, ModelLanguage);
		const FString EditorLMFilePathFull = FPaths::ConvertRelativePathToFull(EditorLMFilePath);

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.FileExists(*EditorLMFilePathFull))
		{
			return false;
		}
	}

	if (bCheckAsset)
	{
		const FString AssetPath = FPaths::ChangeExtension(SpeechRecognizerSettings->GetLanguageModelAssetPath(), TEXT(""));
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		const FString AbsoluteAssetPath = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(*AssetPath, TEXT(".uasset")));
		if (!PlatformFile.FileExists(*AbsoluteAssetPath))
		{
			return false;
		}
	}

	return true;
}

FString FRuntimeSpeechRecognizerEditorModule::GetEditorLMDirectoryPath() const
{
	const FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("RuntimeSpeechRecognizer"))->GetContentDir();
	return ContentDir;
}

FString FRuntimeSpeechRecognizerEditorModule::GetEditorLMFilePath(ESpeechRecognizerModelSize ModelSize, ESpeechRecognizerModelLanguage ModelLanguage) const
{
	return FPaths::Combine(GetEditorLMDirectoryPath(), GetLMFileName(ModelSize, ModelLanguage));
}

FString FRuntimeSpeechRecognizerEditorModule::GetLMFileName(ESpeechRecognizerModelSize ModelSize, ESpeechRecognizerModelLanguage ModelLanguage) const
{
	const FString ModelSizeString = ModelSizeToStringForFileName(ModelSize);
	return FString::Printf(TEXT("ggml-%s%s.%s"), *ModelSizeString, ModelLanguage == ESpeechRecognizerModelLanguage::EnglishOnly ? TEXT(".en") : TEXT(""), *LanguageModelExtension);
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

TFuture<bool> FRuntimeSpeechRecognizerEditorModule::DownloadLanguageModel(ESpeechRecognizerModelSize ModelSize, ESpeechRecognizerModelLanguage ModelLanguage) const
{
	if (LanguageModelDownloadState.Downloader.IsValid())
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot download language model because another download is already in progress"));
		return MakeFulfilledPromise<bool>(false).GetFuture();
	}

	const bool bDownload = EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(LOCTEXT("DownloadLanguageModel", "RuntimeSpeechRecognizer will download a language model of size '{0}' and language '{1}'. This may take a while. Do you want to continue?"),
		UEnum::GetDisplayValueAsText(ModelSize), UEnum::GetDisplayValueAsText(ModelLanguage)));

	if (!bDownload)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Log, TEXT("User cancelled download of language model"));
		return MakeFulfilledPromise<bool>(false).GetFuture();
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	const FString EditorLMDirectoryPath = GetEditorLMDirectoryPath();
	const FString EditorLMFilePath = GetEditorLMFilePath(ModelSize, ModelLanguage);

	const FString EditorLMFilePathFull = FPaths::ConvertRelativePathToFull(EditorLMFilePath);

	if (PlatformFile.FileExists(*EditorLMFilePathFull))
	{
		return MakeFulfilledPromise<bool>(true).GetFuture();
	}

	const FString LMFileName = GetLMFileName(ModelSize, ModelLanguage);

	const USpeechRecognizerSettings* SpeechRecognizerSettings = GetDefault<USpeechRecognizerSettings>();
	if (!SpeechRecognizerSettings)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SpeechRecognizerSettingsMissing", "The RuntimeSpeechRecognizer cannot function correctly because the recognizer settings are missing"));
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot get the speech recognizer settings"));
		return MakeFulfilledPromise<bool>(false).GetFuture();
	}

	const FString URL = FPaths::Combine(SpeechRecognizerSettings->ModelDownloadBaseUrl, LMFileName);

	LanguageModelDownloadState.Downloader = MakeShared<FLanguageModelDownloader>();
	LanguageModelDownloadState.Promise = MakeUnique<TPromise<bool>>();
	LanguageModelDownloadState.ProgressValue = MakeUnique<float>(0.0f);
	LanguageModelDownloadState.ProgressWindow = MakeUnique<FSpeechRecognizerProgressDialog>(LOCTEXT("DownloadLM", "Downloading Language Model"), FText::Format(LOCTEXT("DownloadLMDescription", "Downloading Language Model: {0}"), FText::FromString(LMFileName)),
		FOnSpeechRecognizerGetPercentage::CreateLambda([this]()
		{
			return *LanguageModelDownloadState.ProgressValue;
		}), FOnSpeechRecognizerCancelClicked::CreateLambda([this]() mutable
		{
			LanguageModelDownloadState.Downloader->CancelDownload();
		}));

	LanguageModelDownloadState.Downloader->DownloadFile(URL, 15, FString(), TNumericLimits<TArray<uint8>::SizeType>::Max(), [this](int64 BytesReceived, int64 ContentSize)
	{
		const float ProgressRatio = static_cast<float>(BytesReceived) / ContentSize;
		*LanguageModelDownloadState.ProgressValue = ProgressRatio;
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Log, TEXT("Downloading language model file: %f"), ProgressRatio);
	}).Next([this, EditorLMFilePathFull](TArray64<uint8>&& DownloadedData) mutable
	{
		if (DownloadedData.Num() <= 0)
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LanguageModelDownloadFailed", "The RuntimeSpeechRecognizer cannot function correctly because the language model file could not be downloaded. The file should be located at: {0}"), FText::FromString(EditorLMFilePathFull)));
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot download language model file: %s"), *EditorLMFilePathFull);
			LanguageModelDownloadState.Promise->SetValue(false);
			return;
		}

		if (!FFileHelper::SaveArrayToFile(DownloadedData, *EditorLMFilePathFull))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("LanguageModelSaveFailed", "The RuntimeSpeechRecognizer cannot function correctly because the language model file could not be saved. The file should be located at: {0}"), FText::FromString(EditorLMFilePathFull)));
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot save language model file: %s"), *EditorLMFilePathFull);
			LanguageModelDownloadState.Promise->SetValue(false);
			return;
		}

		LanguageModelDownloadState.Promise->SetValue(true);
	});

	// Clean up the downloader object and promise object after the download has finished
	TFuture<bool> Future = LanguageModelDownloadState.Promise->GetFuture();
	return Future.Next([this](bool bSuccess) mutable
	{
		LanguageModelDownloadState.Downloader.Reset();
		LanguageModelDownloadState.Promise.Reset();
		LanguageModelDownloadState.ProgressValue.Reset();
		LanguageModelDownloadState.ProgressWindow.Reset();
		return bSuccess;
	});
}

bool FRuntimeSpeechRecognizerEditorModule::UpdatePackagingSettings() const
{
	const USpeechRecognizerSettings* SpeechRecognizerSettings = GetDefault<USpeechRecognizerSettings>();
	if (!SpeechRecognizerSettings)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot get the speech recognizer settings"));
		return false;
	}

	UProjectPackagingSettings* PackagingSettings = GetMutableDefault<UProjectPackagingSettings>();
	if (!PackagingSettings)
	{
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot get the packaging settings"));
		return false;
	}

	const FString LanguageModelPath = SpeechRecognizerSettings->GetLanguageModelPackagePath();

	if (!PackagingSettings->DirectoriesToAlwaysCook.ContainsByPredicate([&LanguageModelPath](const FDirectoryPath& DirPath)
	{
		return FPaths::IsSamePath(DirPath.Path, LanguageModelPath);
	}))
	{
		FDirectoryPath NewDirPath;
		NewDirPath.Path = LanguageModelPath;
		PackagingSettings->DirectoriesToAlwaysCook.Add(NewDirPath);
	}

	return true;
}

bool FRuntimeSpeechRecognizerEditorModule::DeleteOldLanguageModels() const
{
	const USpeechRecognizerSettings* SpeechRecognizerSettings = GetDefault<USpeechRecognizerSettings>();
	if (!SpeechRecognizerSettings)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SpeechRecognizerSettingsMissing", "The RuntimeSpeechRecognizer cannot function correctly because the recognizer settings are missing"));
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot get the speech recognizer settings"));
		return false;
	}

	const FString OldAssetPath = TEXT("/Game/RuntimeSpeechRecognizer/LanguageModel");

	// Delete old language model asset
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString AbsoluteOldAssetPath = FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(*OldAssetPath, TEXT(".uasset")));
	if (PlatformFile.FileExists(*AbsoluteOldAssetPath))
	{
		PlatformFile.DeleteFile(*AbsoluteOldAssetPath);
	}

	// Remove the old package path from the list of directories to always cook
	{
		const FString OldPackagePath = TEXT("/Game/RuntimeSpeechRecognizer");
		UProjectPackagingSettings* PackagingSettings = Cast<UProjectPackagingSettings>(UProjectPackagingSettings::StaticClass()->GetDefaultObject());
		if (!PackagingSettings)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("PackagingSettingsMissing", "The RuntimeSpeechRecognizer cannot function correctly because the packaging settings are missing"));
			UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Cannot get the packaging settings"));
			return false;
		}

		PackagingSettings->DirectoriesToAlwaysCook.RemoveAll([&OldPackagePath](const FDirectoryPath& DirPath)
		{
			return FPaths::IsSamePath(DirPath.Path, OldPackagePath);
		});
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRuntimeSpeechRecognizerEditorModule, RuntimeSpeechRecognizerEditor)

DEFINE_LOG_CATEGORY(LogEditorRuntimeSpeechRecognizer);
