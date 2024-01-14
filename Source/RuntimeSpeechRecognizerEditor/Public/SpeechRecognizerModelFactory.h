// Georgy Treshchev 2024.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "SpeechRecognizerModelFactory.generated.h"

/**
 * Language model factory for creating language model assets. This factory is only used in the editor when creating a language model asset from a file
 */
UCLASS(HideCategories = Object)
class RUNTIMESPEECHRECOGNIZEREDITOR_API USpeechRecognizerModelFactory : public UFactory
{
	GENERATED_BODY()

public:
	USpeechRecognizerModelFactory();

	/** The path to the language model file to load */
	FString LanguageModelPath;

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ End UFactory Interface
};
