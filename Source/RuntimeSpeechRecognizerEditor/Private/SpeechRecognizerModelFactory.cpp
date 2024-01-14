// Georgy Treshchev 2024.

#include "SpeechRecognizerModelFactory.h"
#include "SpeechRecognizerEditorDefines.h"
#include "SpeechRecognizerModel.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "SpeechRecognizerModelFactory"

USpeechRecognizerModelFactory::USpeechRecognizerModelFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USpeechRecognizerModel::StaticClass();
}

UObject* USpeechRecognizerModelFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class->IsChildOf(USpeechRecognizerModel::StaticClass()));
	
	USpeechRecognizerModel* LanguageModel = NewObject<USpeechRecognizerModel>(InParent, Class, Name, Flags);
	if (!LanguageModel)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("LanguageModelCreateFailed", "The RuntimeSpeechRecognizer cannot function correctly because the language model asset could not be created"));
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to create language model asset"));
		return nullptr;
	}

	TArray64<uint8> ModelData;
	if (!FFileHelper::LoadFileToArray(ModelData, *LanguageModelPath))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("LanguageModelLoadFailed", "The RuntimeSpeechRecognizer cannot function correctly because the language model file could not be loaded"));
		UE_LOG(LogEditorRuntimeSpeechRecognizer, Error, TEXT("Failed to load language model file from %s"), *LanguageModelPath);
		return nullptr;
	}

	// Copy the language model data into the bulk data
	{
		LanguageModel->LanguageModelBulkData.Lock(LOCK_READ_WRITE);
		void* DataPtr = LanguageModel->LanguageModelBulkData.Realloc(ModelData.Num());
		FMemory::Memcpy(DataPtr, ModelData.GetData(), ModelData.Num());
		LanguageModel->LanguageModelBulkData.Unlock();
	}

	UE_LOG(LogEditorRuntimeSpeechRecognizer, Log, TEXT("Loaded language model file from '%s' to asset '%s' with size %lld bytes"), *LanguageModelPath, *LanguageModel->GetPathName(), LanguageModel->LanguageModelBulkData.GetBulkDataSize());
	return LanguageModel;
}

#undef LOCTEXT_NAMESPACE