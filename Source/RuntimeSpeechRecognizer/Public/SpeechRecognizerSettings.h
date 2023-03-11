// Georgy Treshchev 2023.

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
	 * Get the full path to the language model asset
	 * The format is "[PackagePath]/[AssetName].[AssetName]"
	 */
	FString GetLanguageModelAssetPath() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif
};
