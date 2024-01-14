// Georgy Treshchev 2024.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Serialization/BulkData.h"
#include "SpeechRecognizerModel.generated.h"

/**
 * Intended to be presented as an asset in the editor containing language model data
 */
UCLASS()
class RUNTIMESPEECHRECOGNIZER_API USpeechRecognizerModel : public UObject
{
	GENERATED_BODY()

public:
	/** Language model data in ggml format */
	FByteBulkData LanguageModelBulkData;

	//~ Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface
};
