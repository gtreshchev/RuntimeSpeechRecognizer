// Georgy Treshchev 2024.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SpeechRecognizerStringUtils.generated.h"

/**
 * Utility class for string-related operations used by the speech recognizer.
 */
UCLASS()
class RUNTIMESPEECHRECOGNIZER_API USpeechRecognizerStringUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Computes the Levenshtein similarity between two strings
	 * This method is useful for recognizing spoken commands
	 * 
	 * @param BaseString The base string to compare against
	 * @param CandidateString The candidate string to compare
	 * @return The similarity between 0 and 1, where 1 is a perfect match
	 */
	UFUNCTION(BlueprintCallable, Category = "SpeechRecognizer|String")
	static float ComputeLevenshteinSimilarity(const FString& BaseString, const FString& CandidateString);
};
