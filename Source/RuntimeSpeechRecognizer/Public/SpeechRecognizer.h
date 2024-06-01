// Georgy Treshchev 2024.

#pragma once

#include "CoreMinimal.h"
#include "SpeechRecognizerThread.h"

#include "SpeechRecognizer.generated.h"

/** Dynamic delegate for speech recognition started */
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSpeechRecognitionStartedDynamic, bool, bSucceeded);

/** Static delegate for speech recognition started */
DECLARE_DELEGATE_OneParam(FOnSpeechRecognitionStartedStatic, bool);


/** Dynamic delegate for speech recognition finished */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpeechRecognitionFinishedDynamic);

/** Static delegate for speech recognition finished */
DECLARE_MULTICAST_DELEGATE(FOnSpeechRecognitionFinishedStatic);


/** Dynamic delegate for recognized words */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpeechRecognizedTextSegmentDynamic, const FString&, RecognizedWords);

/** Static delegate for recognized words */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSpeechRecognizedTextSegmentStatic, const FString&);


/** Dynamic delegate for speech recognition errors */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpeechRecognitionErrorDynamic, const FString&, ShortErrorMessage, const FString&, LongErrorMessage);

/** Static delegate for speech recognition errors */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSpeechRecognitionErrorStatic, const FString&, const FString&);


/** Dynamic delegate for speech recognition progress */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpeechRecognitionProgressDynamic, int32, Progress);

/** Static delegate for speech recognition progress */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSpeechRecognitionProgressStatic, int32);

/** Dynamic delegate for speech recognition thread fully stopped */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpeechRecognitionStoppedDynamic);

/** Static delegate for speech recognition thread fully stopped */
DECLARE_MULTICAST_DELEGATE(FOnSpeechRecognitionStoppedStatic);


/**
 * Represents a speech recognizer that can recognize spoken words
 */
UCLASS(BlueprintType, Category = "Runtime Speech Recognizer")
class RUNTIMESPEECHRECOGNIZER_API USpeechRecognizer : public UObject
{
	GENERATED_BODY()

public:
	USpeechRecognizer();

	/**
	 * Creates an instance of the speech recognizer
	 *
	 * @return Created speech recognizer
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Create")
	static USpeechRecognizer* CreateSpeechRecognizer();

	/**
	 * Starts the speech recognition. Ensure that all the needed parameters are set before calling this function
	 *
	 * @return True if the speech recognition was started successfully, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Main")
	void StartSpeechRecognition(const FOnSpeechRecognitionStartedDynamic& OnStarted);

	/**
	 * Starts the speech recognition. Ensure that all the needed parameters are set before calling this function. Suitable for use in C++
	 *
	 * @return True if the speech recognition was started successfully, false otherwise
	 */
	void StartSpeechRecognition(const FOnSpeechRecognitionStartedStatic& OnStarted);

	/**
	 * Stops the speech recognition. The speech recognition can be started again after calling this function
	 *
	 * @return True if the speech recognition was stopped successfully, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Main")
	void StopSpeechRecognition();

	/**
	 * Processes the audio data and recognizes the words
	 *
	 * @param PCMData PCM audio data in 32-bit floating point interleaved format
	 * @param SampleRate The sample rate of the audio data
	 * @param NumOfChannels The number of channels in the audio data
	 * @param bLast Whether this is the last audio data to process. If true, the audio data will be queued for processing even if the enabled step size is not reached
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Process")
	void ProcessAudioData(TArray<float> PCMData, float SampleRate, int32 NumOfChannels, bool bLast);

	/**
	 * Processes the audio data and recognizes the words. Suitable for use in C++
	 *
	 * @param PCMData PCM audio data in 32-bit floating point interleaved format
	 * @param SampleRate The sample rate of the audio data
	 * @param NumOfChannels The number of channels in the audio data
	 * @param bLast Whether this is the last audio data to process. If true, the audio data will be queued for processing even if the enabled step size is not reached
	 */
	void ProcessAudioData(Audio::FAlignedFloatBuffer PCMData, float SampleRate, int32 NumOfChannels, bool bLast);

	/**
	 * Processes audio data that was queued before but not yet processed, especially useful when using step size functionality
	 * This function ensures all audio data is processed, even if it did not fit into the step size yet
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Process")
	void ForceProcessPendingAudioData();

	/**
	 * Clears the audio data that was queued before but not yet processed
	 *
	 * @param bClearPendingAudioData Whether to clear the pending audio data or not
	 * @param bClearAudioQueue Whether to clear the audio queue or not
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Process")
	void ClearAudioData(bool bClearPendingAudioData, bool bClearAudioQueue);

	/**
	 * Returns whether the thread worker is stopped or not
	 *
	 * @return True if the thread worker is stopped, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Info")
	bool GetIsStopped() const;

	/**
	 * Returns whether the speech recognition is currently stopping (but not yet stopped) or not
	 * It is set to true when the StopSpeechRecognition function is called, and set to false when the thread worker is fully stopped
	 * 
	 * @return True if the speech recognition is currently stopping, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Info")
	bool GetIsStopping() const;

	/**
	 * Returns whether all the audio data has been processed or not
	 *
	 * @return True if all the audio data has been processed, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Info")
	bool GetIsFinished() const;

	/** Dynamic delegate broadcast when all the audio data has been processed */
	UPROPERTY(BlueprintAssignable, Category = "Runtime Speech Recognizer|Delegates")
	FOnSpeechRecognitionFinishedDynamic OnRecognitionFinished;

	/** Static delegate broadcast when all the audio data has been processed */
	FOnSpeechRecognitionFinishedStatic OnRecognitionFinishedNative;

	/** Dynamic delegate broadcast when recognized words are received */
	UPROPERTY(BlueprintAssignable, Category = "Runtime Speech Recognizer|Delegates")
	FOnSpeechRecognizedTextSegmentDynamic OnRecognizedTextSegment;

	/** Static delegate broadcast when recognized words are received */
	FOnSpeechRecognizedTextSegmentStatic OnRecognizedTextSegmentNative;

	/** Dynamic delegate broadcast when an error occurs during speech recognition */
	UPROPERTY(BlueprintAssignable, Category = "Runtime Speech Recognizer|Delegates")
	FOnSpeechRecognitionErrorDynamic OnRecognitionError;

	/** Static delegate broadcast when an error occurs during speech recognition */
	FOnSpeechRecognitionErrorStatic OnRecognitionErrorNative;

	/** Dynamic delegate broadcast when the speech recognition progress obtained */
	UPROPERTY(BlueprintAssignable, Category = "Runtime Speech Recognizer|Delegates")
	FOnSpeechRecognitionProgressDynamic OnRecognitionProgress;

	/** Static delegate broadcast when the speech recognition progress obtained */
	FOnSpeechRecognitionProgressStatic OnRecognitionProgressNative;

	/** Dynamic delegate broadcast when the speech recognition thread is fully stopped */
	UPROPERTY(BlueprintAssignable, Category = "Runtime Speech Recognizer|Delegates")
	FOnSpeechRecognitionStoppedDynamic OnRecognitionStopped;

	/** Static delegate broadcast when the speech recognition thread is fully stopped */
	FOnSpeechRecognitionStoppedStatic OnRecognitionStoppedNative;

	/**
	 * Sets the parameters for speech recognition. If you want to change only specific parameters, consider using the individual setter functions
	 *
	 * @param Parameters The parameters to use for speech recognition
	 * @return True if the parameters were set successfully, false otherwise
	 * @note Can only be called when the thread worker is stopped
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|All")
	bool SetRecognitionParameters(const FSpeechRecognitionParameters& Parameters);

	/**
	 * Returns the default parameters suitable for non-streaming speech recognition
	 * @return The default parameters suitable for non-streaming speech recognition
	 */
	UFUNCTION(BlueprintPure, Category = "Runtime Speech Recognizer|Getters|All")
	static FSpeechRecognitionParameters GetNonStreamingDefaults();

	/**
	 * Returns the default parameters suitable for streaming speech recognition
	 * @return The default parameters suitable for streaming speech recognition
	 */
	UFUNCTION(BlueprintPure, Category = "Runtime Speech Recognizer|Getters|All")
	static FSpeechRecognitionParameters GetStreamingDefaults();

	/**
	 * Returns the current recognition parameters
	 * @return The current recognition parameters
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Getters|All")
	FSpeechRecognitionParameters GetRecognitionParameters() const;

	/**
	 * Sets the default parameters suitable for non-streaming speech recognition
	 *
	 * @return True if the parameters were set successfully, false otherwise
	 * @note Can only be called when the thread worker is stopped
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|All")
	bool SetNonStreamingDefaults();

	/**
	 * Sets the default parameters suitable for streaming speech recognition
	 *
	 * @return True if the parameters were set successfully, false otherwise
	 * @note Can only be called when the thread worker is stopped
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|All")
	bool SetStreamingDefaults();

	/**
	 * Sets the number of threads to use for speech recognition
	 *
	 * @param Value The number of threads to use
	 * @return True if the number of threads was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 * @note Set this value to 0 to use the number of cores
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetNumOfThreads(int32 Value);

	/**
	 * Sets the language to use for speech recognition
	 *
	 * @param Language The language to use. Must be supported by the selected language model in the Editor settings
	 * @return True if the language was successfully set, false otherwise
	 * @note Can only be called when the thread worker is stopped
	 * @note Setting the language to Auto will decrease the recognition accuracy and performance
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetLanguage(ESpeechRecognizerLanguage Language);

	/**
	 * Sets whether to translate the recognized words to English
	 *
	 * @param bTranslate Whether to translate the recognized words to English or not. If true, the language model must be multilingual
	 * @return True if the translation was successfully set, false otherwise
	 * @note Can only be called when the thread worker is stopped
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetTranslateToEnglish(bool bTranslate);

	/**
	 * Sets the step size in milliseconds. Determines how often to send audio data for recognition.
	 * 5000 ms (5 seconds) is used by default
	 *
	 * @param Value The step size in milliseconds
	 * @return True if the step size was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 * @note Set this value to 0 to disable step size
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetStepSize(int32 Value);

	/**
	 * Sets whether to use past transcription (if any) as initial prompt for the decoder
	 *
	 * @param bNoContext Whether to use past transcription (if any) as initial prompt for the decoder
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetNoContext(bool bNoContext);

	/**
	 * Sets whether to force single segment output (useful for streaming)
	 *
	 * @param bSingleSegment Whether to force single segment output (useful for streaming)
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetSingleSegment(bool bSingleSegment);

	/**
	 * Sets the maximum number of tokens per text segment (0 = no limit)
	 *
	 * @param Value The maximum number of tokens per text segment (0 = no limit)
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetMaxTokens(int32 Value);

	/**
	 * Sets whether to speed up the recognition by 2x using Phase Vocoder
	 *
	 * @param bSpeedUp Whether to speed up the recognition by using a smaller model
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetSpeedUp(bool bSpeedUp);

	/**
	 * Sets the size of the audio context
	 *
	 * @param Value The size of the audio context
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetAudioContextSize(int32 Value);

	/**
	 * Sets the temperature to increase when falling back when the decoding fails to meet either of the thresholds below
	 *
	 * @param Value The temperature to increase when falling back when the decoding fails to meet either of the thresholds below
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetTemperatureToIncrease(float Value);

	/**
	 * Sets the entropy threshold
	 * If the compression ratio is higher than this value, treat the decoding as failed. Similar to OpenAI's "compression_ratio_threshold"
	 *
	 * @param Value The entropy threshold
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetEntropyThreshold(float Value);

	/**
	 * Sets whether to suppress blanks showing up in outputs
	 *
	 * @param Value Whether to suppress blanks showing up in outputs
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 * @author https://github.com/amartinz
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetSuppressBlank(bool Value);

	/**
	 * Sets whether to suppress non speech tokens showing up in outputs
	 *
	 * @param Value Whether to suppress non speech tokens showing up in outputs
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 * @author https://github.com/amartinz
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetSuppressNonSpeechTokens(bool Value);

	/**
	 * Sets the number of beams in beam search. Only applicable when temperature is zero
	 * 
	 * @param Value The number of beams in beam search
	 * @return True if the setting was set successfully, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetBeamSize(int32 Value);

	/**
	 * Sets the initial prompt for the first window
	 * This can be used to provide context for the recognition to make it more likely to predict the words correctly
	 * 
	 * @param Value The initial prompt for the first window
	 * @return True if the setting was set successfully, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "Runtime Speech Recognizer|Setters|Individual")
	bool SetInitialPrompt(const FString& Value);

private:
	/** The thread that handles speech recognition */
	TSharedPtr<FSpeechRecognizerThread> Thread;
};
