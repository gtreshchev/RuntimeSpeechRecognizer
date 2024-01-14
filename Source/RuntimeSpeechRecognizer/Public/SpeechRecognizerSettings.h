// Georgy Treshchev 2024.

#pragma once

#include "CoreMinimal.h"
#include "SpeechRecognizerTypes.h"

#include "SpeechRecognizerSettings.generated.h"

/**
 * Settings for the Runtime Speech Recognizer plugin
 */
UCLASS(Config = Game, DefaultConfig)
class RUNTIMESPEECHRECOGNIZER_API USpeechRecognizerSettings : public UObject
{
	GENERATED_BODY()

public:
	USpeechRecognizerSettings();

	/** Model size to use by the speech recognizer, defined once in the project settings and cannot be changed at runtime */
	UPROPERTY(Config, EditAnywhere, Category = "Runtime Speech Recognizer")
	ESpeechRecognizerModelSize ModelSize;

	/** Language model to use by the speech recognizer, defined once in the project settings and cannot be changed at runtime */
	UPROPERTY(Config, EditAnywhere, Category = "Runtime Speech Recognizer")
	ESpeechRecognizerModelLanguage ModelLanguage;

#if WITH_EDITORONLY_DATA
	/** The base URL to download the language model from */
	UPROPERTY(Config, EditAnywhere, Category = "Advanced Runtime Speech Recognizer")
	FString ModelDownloadBaseUrl;
#endif

	/** The custom name to use when downloading the language model. Only used if the language model size is custom
	 * For example, if the custom name is "ggml-medium.en-q5_0.bin", and the base URL is "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/",
	 * the language model will be downloaded from "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.en-q5_0.bin"
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Advanced Runtime Speech Recognizer")
	FString ModelDownloadCustomName;

	/**
	 * Get the name of the language model asset
	 * The format is "[AssetName]"
	 */
	FString GetLanguageModelAssetName() const;

	/**
	 * Get the path to the language model package
	 * The format is "[PackagePath]"
	 */
	FString GetLanguageModelPackagePath() const;

	/**
	 * Get the full path to the language model package
	 * The format is "[PackagePath]/[AssetName]"
	 */
	FString GetLanguageModelFullPackagePath() const;

	/**
	 * Get the full path to the language model asset
	 * The format is "[PackagePath]/[AssetName].[AssetName]"
	 */
	FString GetLanguageModelAssetPath() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif
};
