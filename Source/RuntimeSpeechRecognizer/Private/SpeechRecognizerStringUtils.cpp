// Georgy Treshchev 2024.

#include "SpeechRecognizerStringUtils.h"

#include "SpeechRecognizerDefines.h"
#include "Math/UnrealMathUtility.h"
#include "Algo/LevenshteinDistance.h"

float USpeechRecognizerStringUtils::ComputeLevenshteinSimilarity(const FString& BaseString, const FString& CandidateString)
{
	// Convert strings to lowercase and trim whitespace
	const FString BaseStringTrimmed = BaseString.TrimStartAndEnd().ToLower();
	const FString CandidateStringTrimmed = CandidateString.TrimStartAndEnd().ToLower();

	// If candidate string is empty, return perfect match
	if (CandidateStringTrimmed.IsEmpty())
	{
		return 1.0f;
	}

	// Compute similarity using Levenshtein distance
	const float WorstCase = FMath::Max3(CandidateStringTrimmed.Len(), BaseStringTrimmed.Len(), 1);
	const float Similarity = 1.0f - (Algo::LevenshteinDistance(BaseStringTrimmed, CandidateStringTrimmed) / WorstCase);

	UE_LOG(LogRuntimeSpeechRecognizer, Verbose, TEXT("Levenshtein similarity between '%s' and '%s' is %f"), *BaseStringTrimmed, *CandidateStringTrimmed, Similarity);
	return Similarity;
}
