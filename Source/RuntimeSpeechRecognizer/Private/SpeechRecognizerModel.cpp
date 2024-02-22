// Georgy Treshchev 2024.

#include "SpeechRecognizerModel.h"
#include "SpeechRecognizerDefines.h"

void USpeechRecognizerModel::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// BULKDATA_Force_NOT_InlinePayload sometimes leads to crashes on Quest 3 and possibly other platforms, so it's removed
	// BULKDATA_Size64Bit is automatically set by the engine when the data is larger than 2GB, so it's not necessary to set it manually
	// LanguageModelBulkData.SetBulkDataFlags(BULKDATA_Force_NOT_InlinePayload | BULKDATA_Size64Bit);

	LanguageModelBulkData.Serialize(Ar, this, INDEX_NONE, false);
	UE_LOG(LogRuntimeSpeechRecognizer, Log, TEXT("Serializing language model data with the size of %lld bytes"), LanguageModelBulkData.GetBulkDataSize());
}