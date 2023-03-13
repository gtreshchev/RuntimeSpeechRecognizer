// Georgy Treshchev 2023.

#pragma once

#include "Engine/EngineBaseTypes.h"
#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 25
#include "DSP/BufferVectorOperations.h"
#endif

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 25
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
	Base,
	Small,
	Medium,
	Large
};

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
	Iw UMETA(DisplayName = "Hebrew"),
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
inline const char* EnumToString(ESpeechRecognizerLanguage Enum)
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
	case ESpeechRecognizerLanguage::Iw:
		return "iw";
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
