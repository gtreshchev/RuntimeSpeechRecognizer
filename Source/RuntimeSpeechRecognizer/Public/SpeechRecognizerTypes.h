// Georgy Treshchev 2024.

#pragma once

#include "Engine/EngineBaseTypes.h"
#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(4, 26, 0)
#include "DSP/BufferVectorOperations.h"
#endif

#include "SpeechRecognizerTypes.generated.h"

#if UE_VERSION_OLDER_THAN(4, 26, 0)
namespace Audio
{
	using FAlignedFloatBuffer = Audio::AlignedFloatBuffer;
}
#endif

/**
 * The size of the language model to use by the speech recognizer
 * The larger the model, the more accurate the recognition, but the slower the recognition
 * Intended to be defined once in the project settings and cannot be changed at runtime
 */
UENUM(BlueprintType, Category = "Runtime Speech Recognizer")
enum class ESpeechRecognizerModelSize : uint8
{
	Tiny,
	Tiny_Q5_1 UMETA(DisplayName = "Tiny Quantized (Q5_1)", ToolTip = "Tiny model with quantization to 5 bits and 1 decimal point"),
	Tiny_Q8_0 UMETA(DisplayName = "Tiny Quantized (Q8_0)", ToolTip = "Tiny model with quantization to 8 bits and 0 decimal points"),
	Base,
	Base_Q5_1 UMETA(DisplayName = "Base Quantized (Q5_1)", ToolTip = "Base model with quantization to 5 bits and 1 decimal point"),
	Small,
	Small_Q5_1 UMETA(DisplayName = "Small Quantized (Q5_1)", ToolTip = "Small model with quantization to 5 bits and 1 decimal point"),
	Distil_Small UMETA(DisplayName = "Distil Small", ToolTip = "Distilled version of the Small model"),
	Medium,
	Medium_Q5_0 UMETA(DisplayName = "Medium Quantized (Q5_0)", ToolTip = "Medium model with quantization to 5 bits and 0 decimal points"),
	Distil_Medium UMETA(DisplayName = "Distil Medium", ToolTip = "Distilled version of the Medium model"),
	Large_V1,
	Large_V2,
	Large_V2_Q5_0 UMETA(DisplayName = "Large V2 Quantized (Q5_0)", ToolTip = "Large V2 model with quantization to 5 bits and 0 decimal points"),
	Distil_Large_V2 UMETA(DisplayName = "Distil Large V2", ToolTip = "Distilled version of the Large V2 model"),
	Large_V3,
	Large_V3_Q5_0 UMETA(DisplayName = "Large V3 Quantized (Q5_0)", ToolTip = "Large V3 model with quantization to 5 bits and 0 decimal points"),
	Distil_Large_V3 UMETA(DisplayName = "Distil Large V3", ToolTip = "Distilled version of the Large V3 model. Please prefer this over the Large V3 model, as it could be 5x faster"),
	Large_V3_Turbo UMETA(DisplayName = "Large V3 Turbo", ToolTip = "Large V3 model optimized for faster inference with minimal accuracy trade-off"),
	Large_V3_Turbo_Q5_0 UMETA(DisplayName = "Large V3 Turbo Quantized (Q5_0)", ToolTip = "Quantized version of Large V3 Turbo for enhanced speed and reduced model size"),
	Custom UMETA(ToolTip = "Custom model size. The model size will be determined by the language model file name (e.g. 'ggml-medium.en-q5_0.bin'")
};

/**
 * Check if the language model size supports English-only model language
 */
RUNTIMESPEECHRECOGNIZER_API inline bool DoesSupportEnglishOnlyModelLanguage(ESpeechRecognizerModelSize ModelSize)
{
	if (ModelSize == ESpeechRecognizerModelSize::Large_V1 ||
		ModelSize == ESpeechRecognizerModelSize::Large_V2 || ModelSize == ESpeechRecognizerModelSize::Large_V2_Q5_0 ||
		ModelSize == ESpeechRecognizerModelSize::Large_V3 || ModelSize == ESpeechRecognizerModelSize::Large_V3_Q5_0 ||
		ModelSize == ESpeechRecognizerModelSize::Large_V3_Turbo || ModelSize == ESpeechRecognizerModelSize::Large_V3_Turbo_Q5_0)
	{
		return false;
	}
	return true;
}

/**
 * Check if the language model size supports multilingual model language
 */
RUNTIMESPEECHRECOGNIZER_API inline bool DoesSupportMultilingualModelLanguage(ESpeechRecognizerModelSize ModelSize)
{
	if (ModelSize == ESpeechRecognizerModelSize::Tiny_Q8_0 || ModelSize == ESpeechRecognizerModelSize::Distil_Small ||
		ModelSize == ESpeechRecognizerModelSize::Distil_Medium ||
		ModelSize == ESpeechRecognizerModelSize::Distil_Large_V2 ||
		ModelSize == ESpeechRecognizerModelSize::Distil_Large_V3)
	{
		return false;
	}
	return true;
}

/**
 * The language model for the speech recognizer. Defines the vocabulary of words the recognizer will understand
 * The English-only model contains a larger vocabulary of English-only words, while the multilingual model contains a smaller vocabulary of words in multiple language
 * The model should be defined once in the project settings and cannot be changed at runtime
 */
UENUM(BlueprintType, Category = "Runtime Speech Recognizer")
enum class ESpeechRecognizerModelLanguage : uint8
{
	EnglishOnly,
	Multilingual
};

/**
 * The language to use for the speech recognizer
 * If the model is EnglishOnly, only English language is supported
 * Otherwise, any of the supported languages, including "auto" to automatically detect the language can be used
 */
UENUM(BlueprintType, Category = "Runtime Speech Recognizer")
enum class ESpeechRecognizerLanguage : uint8
{
	Auto,
	En UMETA(DisplayName = "English"),
	Zh UMETA(DisplayName = "Chinese"),
	De UMETA(DisplayName = "German"),
	Es UMETA(DisplayName = "Spanish"),
	Ru UMETA(DisplayName = "Russian"),
	Ko UMETA(DisplayName = "Korean"),
	Fr UMETA(DisplayName = "French"),
	Ja UMETA(DisplayName = "Japanese"),
	Pt UMETA(DisplayName = "Portuguese"),
	Tr UMETA(DisplayName = "Turkish"),
	Pl UMETA(DisplayName = "Polish"),
	Ca UMETA(DisplayName = "Catalan"),
	Nl UMETA(DisplayName = "Dutch"),
	Ar UMETA(DisplayName = "Arabic"),
	Sv UMETA(DisplayName = "Swedish"),
	It UMETA(DisplayName = "Italian"),
	Id UMETA(DisplayName = "Indonesian"),
	Hi UMETA(DisplayName = "Hindi"),
	Fi UMETA(DisplayName = "Finnish"),
	Vi UMETA(DisplayName = "Vietnamese"),
	He UMETA(DisplayName = "Hebrew"),
	Uk UMETA(DisplayName = "Ukrainian"),
	El UMETA(DisplayName = "Greek"),
	Ms UMETA(DisplayName = "Malay"),
	Cs UMETA(DisplayName = "Czech"),
	Ro UMETA(DisplayName = "Romanian"),
	Da UMETA(DisplayName = "Danish"),
	Hu UMETA(DisplayName = "Hungarian"),
	Ta UMETA(DisplayName = "Tamil"),
	No UMETA(DisplayName = "Norwegian"),
	Th UMETA(DisplayName = "Thai"),
	Ur UMETA(DisplayName = "Urdu"),
	Hr UMETA(DisplayName = "Croatian"),
	Bg UMETA(DisplayName = "Bulgarian"),
	Lt UMETA(DisplayName = "Lithuanian"),
	La UMETA(DisplayName = "Latin"),
	Mi UMETA(DisplayName = "Maori"),
	Ml UMETA(DisplayName = "Malayalam"),
	Cy UMETA(DisplayName = "Welsh"),
	Sk UMETA(DisplayName = "Slovak"),
	Te UMETA(DisplayName = "Telugu"),
	Fa UMETA(DisplayName = "Persian"),
	Lv UMETA(DisplayName = "Latvian"),
	Bn UMETA(DisplayName = "Bengali"),
	Sr UMETA(DisplayName = "Serbian"),
	Az UMETA(DisplayName = "Azerbaijani"),
	Sl UMETA(DisplayName = "Slovenian"),
	Kn UMETA(DisplayName = "Kannada"),
	Et UMETA(DisplayName = "Estonian"),
	Mk UMETA(DisplayName = "Macedonian"),
	Br UMETA(DisplayName = "Breton"),
	Eu UMETA(DisplayName = "Basque"),
	Is UMETA(DisplayName = "Icelandic"),
	Hy UMETA(DisplayName = "Armenian"),
	Ne UMETA(DisplayName = "Nepali"),
	Mn UMETA(DisplayName = "Mongolian"),
	Bs UMETA(DisplayName = "Bosnian"),
	Kk UMETA(DisplayName = "Kazakh"),
	Sq UMETA(DisplayName = "Albanian"),
	Sw UMETA(DisplayName = "Swahili"),
	Gl UMETA(DisplayName = "Galician"),
	Mr UMETA(DisplayName = "Marathi"),
	Pa UMETA(DisplayName = "Punjabi"),
	Si UMETA(DisplayName = "Sinhala"),
	Km UMETA(DisplayName = "Khmer"),
	Sn UMETA(DisplayName = "Shona"),
	Yo UMETA(DisplayName = "Yoruba"),
	So UMETA(DisplayName = "Somali"),
	Af UMETA(DisplayName = "Afrikaans"),
	Oc UMETA(DisplayName = "Occitan"),
	Ka UMETA(DisplayName = "Georgian"),
	Be UMETA(DisplayName = "Belarusian"),
	Tg UMETA(DisplayName = "Tajik"),
	Sd UMETA(DisplayName = "Sindhi"),
	Gu UMETA(DisplayName = "Gujarati"),
	Am UMETA(DisplayName = "Amharic"),
	Yi UMETA(DisplayName = "Yiddish"),
	Lo UMETA(DisplayName = "Lao"),
	Uz UMETA(DisplayName = "Uzbek"),
	Fo UMETA(DisplayName = "Faroese"),
	Ht UMETA(DisplayName = "Haitian Creole"),
	Ps UMETA(DisplayName = "Pashto"),
	Tk UMETA(DisplayName = "Turkmen"),
	Nn UMETA(DisplayName = "Hynorsk"),
	Mt UMETA(DisplayName = "Maltese"),
	Sa UMETA(DisplayName = "Sanskrit"),
	Lb UMETA(DisplayName = "Luxembourgish"),
	My UMETA(DisplayName = "Myanmar"),
	Bo UMETA(DisplayName = "Tibetan"),
	Tl UMETA(DisplayName = "Tagalog"),
	Mg UMETA(DisplayName = "Malagasy"),
	As UMETA(DisplayName = "Assamese"),
	Tt UMETA(DisplayName = "Tatar"),
	Haw UMETA(DisplayName = "Hawaiian"),
	Ln UMETA(DisplayName = "Lingala"),
	Ha UMETA(DisplayName = "Hausa"),
	Ba UMETA(DisplayName = "Bashkir"),
	Jw UMETA(DisplayName = "Javanese"),
	Su UMETA(DisplayName = "Sundanese")
};

/**
 * Convert ESpeechRecognizerLanguage to string to use when calling the Whisper API
 */
RUNTIMESPEECHRECOGNIZER_API inline const char* EnumToString(ESpeechRecognizerLanguage Enum)
{
	switch (Enum)
	{
	case ESpeechRecognizerLanguage::Auto:
		return "auto";
	case ESpeechRecognizerLanguage::En:
		return "en";
	case ESpeechRecognizerLanguage::Zh:
		return "zh";
	case ESpeechRecognizerLanguage::De:
		return "de";
	case ESpeechRecognizerLanguage::Es:
		return "es";
	case ESpeechRecognizerLanguage::Ru:
		return "ru";
	case ESpeechRecognizerLanguage::Ko:
		return "ko";
	case ESpeechRecognizerLanguage::Fr:
		return "fr";
	case ESpeechRecognizerLanguage::Ja:
		return "ja";
	case ESpeechRecognizerLanguage::Pt:
		return "pt";
	case ESpeechRecognizerLanguage::Tr:
		return "tr";
	case ESpeechRecognizerLanguage::Pl:
		return "pl";
	case ESpeechRecognizerLanguage::Ca:
		return "ca";
	case ESpeechRecognizerLanguage::Nl:
		return "nl";
	case ESpeechRecognizerLanguage::Ar:
		return "ar";
	case ESpeechRecognizerLanguage::Sv:
		return "sv";
	case ESpeechRecognizerLanguage::It:
		return "it";
	case ESpeechRecognizerLanguage::Id:
		return "id";
	case ESpeechRecognizerLanguage::Hi:
		return "hi";
	case ESpeechRecognizerLanguage::Fi:
		return "fi";
	case ESpeechRecognizerLanguage::Vi:
		return "vi";
	case ESpeechRecognizerLanguage::He:
		return "he";
	case ESpeechRecognizerLanguage::Uk:
		return "uk";
	case ESpeechRecognizerLanguage::El:
		return "el";
	case ESpeechRecognizerLanguage::Ms:
		return "ms";
	case ESpeechRecognizerLanguage::Cs:
		return "cs";
	case ESpeechRecognizerLanguage::Ro:
		return "ro";
	case ESpeechRecognizerLanguage::Da:
		return "da";
	case ESpeechRecognizerLanguage::Hu:
		return "hu";
	case ESpeechRecognizerLanguage::Ta:
		return "ta";
	case ESpeechRecognizerLanguage::No:
		return "no";
	case ESpeechRecognizerLanguage::Th:
		return "th";
	case ESpeechRecognizerLanguage::Ur:
		return "ur";
	case ESpeechRecognizerLanguage::Hr:
		return "hr";
	case ESpeechRecognizerLanguage::Bg:
		return "bg";
	case ESpeechRecognizerLanguage::Lt:
		return "lt";
	case ESpeechRecognizerLanguage::La:
		return "la";
	case ESpeechRecognizerLanguage::Mi:
		return "mi";
	case ESpeechRecognizerLanguage::Ml:
		return "ml";
	case ESpeechRecognizerLanguage::Cy:
		return "cy";
	case ESpeechRecognizerLanguage::Sk:
		return "sk";
	case ESpeechRecognizerLanguage::Te:
		return "te";
	case ESpeechRecognizerLanguage::Fa:
		return "fa";
	case ESpeechRecognizerLanguage::Lv:
		return "lv";
	case ESpeechRecognizerLanguage::Bn:
		return "bn";
	case ESpeechRecognizerLanguage::Sr:
		return "sr";
	case ESpeechRecognizerLanguage::Az:
		return "az";
	case ESpeechRecognizerLanguage::Sl:
		return "sl";
	case ESpeechRecognizerLanguage::Kn:
		return "kn";
	case ESpeechRecognizerLanguage::Et:
		return "et";
	case ESpeechRecognizerLanguage::Mk:
		return "mk";
	case ESpeechRecognizerLanguage::Br:
		return "br";
	case ESpeechRecognizerLanguage::Eu:
		return "eu";
	case ESpeechRecognizerLanguage::Is:
		return "is";
	case ESpeechRecognizerLanguage::Hy:
		return "hy";
	case ESpeechRecognizerLanguage::Ne:
		return "ne";
	case ESpeechRecognizerLanguage::Mn:
		return "mn";
	case ESpeechRecognizerLanguage::Bs:
		return "bs";
	case ESpeechRecognizerLanguage::Kk:
		return "kk";
	case ESpeechRecognizerLanguage::Sq:
		return "sq";
	case ESpeechRecognizerLanguage::Sw:
		return "sw";
	case ESpeechRecognizerLanguage::Gl:
		return "gl";
	case ESpeechRecognizerLanguage::Mr:
		return "mr";
	case ESpeechRecognizerLanguage::Pa:
		return "pa";
	case ESpeechRecognizerLanguage::Si:
		return "si";
	case ESpeechRecognizerLanguage::Km:
		return "km";
	case ESpeechRecognizerLanguage::Sn:
		return "sn";
	case ESpeechRecognizerLanguage::Yo:
		return "yo";
	case ESpeechRecognizerLanguage::So:
		return "so";
	case ESpeechRecognizerLanguage::Af:
		return "af";
	case ESpeechRecognizerLanguage::Oc:
		return "oc";
	case ESpeechRecognizerLanguage::Ka:
		return "ka";
	case ESpeechRecognizerLanguage::Be:
		return "be";
	case ESpeechRecognizerLanguage::Tg:
		return "tg";
	case ESpeechRecognizerLanguage::Sd:
		return "sd";
	case ESpeechRecognizerLanguage::Gu:
		return "gu";
	case ESpeechRecognizerLanguage::Am:
		return "am";
	case ESpeechRecognizerLanguage::Yi:
		return "yi";
	case ESpeechRecognizerLanguage::Lo:
		return "lo";
	case ESpeechRecognizerLanguage::Uz:
		return "uz";
	case ESpeechRecognizerLanguage::Fo:
		return "fo";
	case ESpeechRecognizerLanguage::Ht:
		return "ht";
	case ESpeechRecognizerLanguage::Ps:
		return "ps";
	case ESpeechRecognizerLanguage::Tk:
		return "tk";
	case ESpeechRecognizerLanguage::Nn:
		return "nn";
	case ESpeechRecognizerLanguage::Mt:
		return "mt";
	case ESpeechRecognizerLanguage::Sa:
		return "sa";
	case ESpeechRecognizerLanguage::Lb:
		return "lb";
	case ESpeechRecognizerLanguage::My:
		return "my";
	case ESpeechRecognizerLanguage::Bo:
		return "bo";
	case ESpeechRecognizerLanguage::Tl:
		return "tl";
	case ESpeechRecognizerLanguage::Mg:
		return "mg";
	case ESpeechRecognizerLanguage::As:
		return "as";
	case ESpeechRecognizerLanguage::Tt:
		return "tt";
	case ESpeechRecognizerLanguage::Haw:
		return "haw";
	case ESpeechRecognizerLanguage::Ln:
		return "ln";
	case ESpeechRecognizerLanguage::Ha:
		return "ha";
	case ESpeechRecognizerLanguage::Ba:
		return "ba";
	case ESpeechRecognizerLanguage::Jw:
		return "jw";
	case ESpeechRecognizerLanguage::Su:
		return "su";
	default:
		return "en";
	}
}

RUNTIMESPEECHRECOGNIZER_API inline FString GetModelDownloadBaseUrl(ESpeechRecognizerModelSize ModelSize, ESpeechRecognizerModelLanguage ModelLanguage)
{
	switch (ModelSize) {
	case ESpeechRecognizerModelSize::Tiny:
	case ESpeechRecognizerModelSize::Tiny_Q5_1:
	case ESpeechRecognizerModelSize::Tiny_Q8_0:
	case ESpeechRecognizerModelSize::Base:
	case ESpeechRecognizerModelSize::Base_Q5_1:
	case ESpeechRecognizerModelSize::Small:
	case ESpeechRecognizerModelSize::Small_Q5_1:
	case ESpeechRecognizerModelSize::Medium:
	case ESpeechRecognizerModelSize::Medium_Q5_0:
	case ESpeechRecognizerModelSize::Large_V1:
	case ESpeechRecognizerModelSize::Large_V2:
	case ESpeechRecognizerModelSize::Large_V2_Q5_0:
	case ESpeechRecognizerModelSize::Large_V3:
	case ESpeechRecognizerModelSize::Large_V3_Q5_0:
	case ESpeechRecognizerModelSize::Large_V3_Turbo:
	case ESpeechRecognizerModelSize::Large_V3_Turbo_Q5_0:
		return TEXT("https://huggingface.co/ggerganov/whisper.cpp/resolve/main/");
	case ESpeechRecognizerModelSize::Distil_Small:
		return TEXT("https://huggingface.co/distil-whisper/distil-small.en/resolve/main/");
	case ESpeechRecognizerModelSize::Distil_Medium:
		return TEXT("https://huggingface.co/distil-whisper/distil-medium.en/resolve/main/");
	case ESpeechRecognizerModelSize::Distil_Large_V2:
		return TEXT("https://huggingface.co/distil-whisper/distil-large-v2/resolve/main/");
	case ESpeechRecognizerModelSize::Distil_Large_V3:
		return TEXT("https://huggingface.co/distil-whisper/distil-large-v3-ggml/resolve/main/");
	default:
		return TEXT("");
	}
}
