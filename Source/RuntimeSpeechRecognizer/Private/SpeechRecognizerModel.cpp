// Georgy Treshchev 2023.

#include "SpeechRecognizerModel.h"
#include "SpeechRecognizerDefines.h"

void USpeechRecognizerModel::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// Set the bulk data flags to force loading the data on-demand and to use 64-bit size
	LanguageModelBulkData.SetBulkDataFlags(BULKDATA_Force_NOT_InlinePayload | BULKDATA_Size64Bit);

	LanguageModelBulkData.Serialize(Ar, this, INDEX_NONE, false);
	UE_LOG(LogRuntimeSpeechRecognizer, Log, TEXT("Serializing language model data with the size of %lld bytes"), LanguageModelBulkData.GetBulkDataSize());
}
