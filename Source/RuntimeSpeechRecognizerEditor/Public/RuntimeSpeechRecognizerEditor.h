// Georgy Treshchev 2024.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "UObject/StrongObjectPtr.h"
#include "Async/Future.h"
#include "Delegates/IDelegateInstance.h"
#include "Templates/UniquePtr.h"

enum class ESpeechRecognizerModelLanguage : uint8;
enum class ESpeechRecognizerModelSize : uint8;

class FRuntimeSpeechRecognizerEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Set up the language model for speech recognition. 
	 * @return 
	 */
	virtual TFuture<bool> SetupLanguageModel() const;


	/**
	 * Delete the language model from either the local cache or the asset, or both
	 * 
	 * @param bDeleteFromLocalCache Whether to delete from the local cache
	 * @param bDeleteFromAsset Whether to delete from the asset
	 * @return True if the deletion was successful, false otherwise
	 */
	virtual bool DeleteLanguageModels(bool bDeleteFromLocalCache, bool bDeleteFromAsset) const;

	/**
	 * Check if the language model is present in either the local cache or the asset, or both
	 * 
	 * @param bCheckLocalCache Whether to check the local cache. Checks only the currently selected model size and language
	 * @param bCheckAsset Whether to check the asset
	 * @return True if the language model is present, false otherwise
	 */
	virtual bool IsLanguageModelPresent(bool bCheckLocalCache, bool bCheckAsset) const;

protected:
	/**
	 * Get the relative path to the directory containing the language model files relevant in the editor
	 * @warning This function returns a path that is only intended to be used for creating a language model asset in the editor and should not be used directly
	 */
	FString GetEditorLMDirectoryPath() const;

	/**
	 * Get the relative path to the language model file relevant in the editor
	 * The format is "[Path]/[LanguageModelName]"
	 * 
	 * @warning This function returns a path that is only intended to be used for creating a language model asset in the editor and should not be used directly
	 * @param ModelSize The size of the language model
	 * @param ModelLanguage The language of the language model
	 * @return The path to the language model file
	 */
	FString GetEditorLMFilePath(ESpeechRecognizerModelSize ModelSize, ESpeechRecognizerModelLanguage ModelLanguage) const;

	/**
	 * Get the name of the language model file, including the extension
	 * The format is "[LanguageModelName]"
	 *
	 * @warning This function returns a name that is only intended to be used for creating a language model asset in the editor and should not be used directly
	 * @param ModelSize The size of the language model
	 * @param ModelLanguage The language of the language model
	 * @return The name of the language model file
	 */
	FString GetLMFileName(ESpeechRecognizerModelSize ModelSize, ESpeechRecognizerModelLanguage ModelLanguage) const;

private:
	/**
	 * Download a language model for the given size and language
	 * 
	 * @param ModelSize The size of the model to download
	 * @param ModelLanguage The language of the model to download
	 * @return A future that resolves to true if the download was successful, false otherwise
	 */
	virtual TFuture<bool> DownloadLanguageModel(ESpeechRecognizerModelSize ModelSize, ESpeechRecognizerModelLanguage ModelLanguage) const;

	/**
	 * Update packaging settings for the project to include the language model file for cooking
	 * @return true if the packaging settings were updated successfully, false otherwise
	 */
	bool UpdatePackagingSettings() const;

	/**
	 * Delete old language models that are not being used anymore
	 * Removes the language model from the project folder that was relevant in earlier versions of the plugin
	 * @return true if the old language models were deleted successfully, false otherwise
	 */
	bool DeleteOldLanguageModels() const;

	/** Handle to delegate called when module state changes */
	FDelegateHandle ModulesChangedHandle;

	/**
	 * Struct that holds state related to downloading a language model
	 */
	struct
	{
		/** Pointer to the language model downloader */
		mutable TSharedPtr<class FRuntimeChunkDownloader_Recognizer> Downloader;

		/** Pointer to a promise that will resolve when the language model finishes downloading */
		mutable TUniquePtr<TPromise<bool>> Promise;

		/** Pointer to a float value that represents the download progress of the language model */
		mutable TUniquePtr<float> ProgressValue;

		/** Pointer to a progress window that displays the download progress of the language model */
		mutable TUniquePtr<class FSpeechRecognizerProgressDialog> ProgressWindow;
	} LanguageModelDownloadState;
};
