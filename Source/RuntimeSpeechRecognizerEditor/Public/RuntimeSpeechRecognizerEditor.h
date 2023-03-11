// Georgy Treshchev 2023.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

enum class ESpeechRecognizerModelLanguage : uint8;
enum class ESpeechRecognizerModelSize : uint8;

class FRuntimeSpeechRecognizerEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    virtual bool SetupStaging() const;

protected:
    /**
     * Get the relative path to the directory containing the language model files relevant in the editor
     * @warning This function returns a path that is only intended to be used for creating a language model asset in the editor and should not be used directly
     */
    FString GetEditorLMDirectoryPath(ESpeechRecognizerModelSize ModelSize, ESpeechRecognizerModelLanguage ModelLanguage) const;

    /**
     * Get the relative path to the language model file relevant in the editor
     * @warning This function returns a path that is only intended to be used for creating a language model asset in the editor and should not be used directly
     */
    FString GetEditorLMFilePath(ESpeechRecognizerModelSize ModelSize, ESpeechRecognizerModelLanguage ModelLanguage) const;

	/**
	 * Convert the language model size enum to a string that can be used to build a file name to the editor-specific language model
	 */
	FString ModelSizeToStringForFileName(ESpeechRecognizerModelSize ModelSize) const;
};
