// Georgy Treshchev 2024.

#pragma once

#include "CoreMinimal.h"
#include "SpeechRecognizerTypes.h"
#include "Containers/Queue.h"
#include "SampleBuffer.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "Templates/UniquePtr.h"
#include "Async/Future.h"
#include <atomic>

#include "SpeechRecognizerThread.generated.h"

// Forward declarations
class FSpeechRecognizerThread;
class USpeechRecognizerSettings;
struct whisper_context;
struct whisper_full_params;

/** Static delegate for speech recognition finished recognizing all the queued audio data */
DECLARE_MULTICAST_DELEGATE(FOnSpeechRecognitionFinished);

/** Static delegate for recognized words. The recognized text segment is passed as a parameter */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSpeechRecognizedTextSegment, const FString&);

/** Static delegate for speech recognition errors. The error message and long error message are passed as parameters */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSpeechRecognitionError, const FString& /* ShortErrorMessage */, const FString& /* LongErrorMessage */);

/** Static delegate for speech recognition progress. The progress value is passed as a parameter */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSpeechRecognitionProgress, int32 /* Progress */);

/** Dynamic delegate for speech recognition thread fully stopped */
DECLARE_MULTICAST_DELEGATE(FOnSpeechRecognitionStopped);

/**
 * User data for Whisper speech recognizer
 * Used to identify the thread worker responsible for recognized words
 */
struct RUNTIMESPEECHRECOGNIZER_API FWhisperSpeechRecognizerUserData
{
	/** Weak pointer to the speech recognizer thread */
	TWeakPtr<FSpeechRecognizerThread> SpeechRecognizerWeakPtr;
};

/**
 * The state of the Whisper speech recognizer, which includes the context, parameters, and user data
 */
struct RUNTIMESPEECHRECOGNIZER_API FWhisperSpeechRecognizerState
{
	FWhisperSpeechRecognizerState();

	/** The Whisper context used for speech recognition */
	whisper_context* WhisperContext;

	/** The parameters used for configuring the Whisper speech recognizer */
	whisper_full_params* WhisperParameters;

	/** The user data associated with the Whisper speech recognizer */
	FWhisperSpeechRecognizerUserData WhisperUserData;

	/**
	 * Initializes the Whisper speech recognizer state. This also allocates memory for the context, parameters, and user data
	 *
	 * @param BulkDataPtr Bulk data pointer to the language model
	 * @param BulkDataSize The size of the bulk data
	 * @param SpeechRecognizerPtr Pointer to the speech recognizer thread
	 * @return True if the initialization was successful, false otherwise
	 */
	bool Init(uint8* BulkDataPtr, int64 BulkDataSize, TSharedPtr<FSpeechRecognizerThread> SpeechRecognizerPtr);

	/**
	 * Releases the resources associated with the Whisper speech recognizer state
	 */
	void Release();

	/**
	 * Clears the initial prompt for the first window
	 */
	void ClearInitialPrompt();

private:
	/** Release guard (mutex) for thread safety */
	mutable FCriticalSection ReleaseGuard;
};

/**
 * Parameters for speech recognition
 * These parameters are intended to be immutable once the speech recognition thread is running
 * This is not an exhaustive list of parameters available in Whisper. Only the most important ones are exposed here
 * When adding more parameters, make sure to update the FillWhisperStateParameters() function
 */
USTRUCT(BlueprintType, Category = "Runtime Speech Recognizer")
struct RUNTIMESPEECHRECOGNIZER_API FSpeechRecognitionParameters
{
	GENERATED_BODY()

	/** The number of threads to use for speech recognition. Uses the number of cores if 0 */
	UPROPERTY(BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"), Category = "Runtime Speech Recognizer")
	int32 NumOfThreads = 0;

	/** The language to use for speech recognition */
	UPROPERTY(BlueprintReadWrite, Category = "Runtime Speech Recognizer")
	ESpeechRecognizerLanguage Language = ESpeechRecognizerLanguage::En;

	/** Whether to translate the recognized words to English or not */
	UPROPERTY(BlueprintReadWrite, Category = "Runtime Speech Recognizer")
	bool bTranslateToEnglish = false;

	/** The step size in milliseconds used to accumulate audio in the pending audio buffer to be queued (e.g. 5000 ms = 5 seconds) */
	UPROPERTY(BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"), Category = "Runtime Speech Recognizer")
	int32 StepSizeMs = 5000;

	/** Whether to use past transcription (if any) as initial prompt for the decoder */
	UPROPERTY(BlueprintReadWrite, Category = "Runtime Speech Recognizer")
	bool bNoContext = false;

	/** Whether to force single segment output (useful for streaming) */
	UPROPERTY(BlueprintReadWrite, Category = "Runtime Speech Recognizer")
	bool bSingleSegment = false;

	/** The maximum number of tokens per text segment (0 = no limit) */
	UPROPERTY(BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"), Category = "Runtime Speech Recognizer")
	int32 MaxTokens = 0;

	/** Whether to speed up the audio by 2x using Phase Vocoder */
	UPROPERTY(BlueprintReadWrite, Category = "Runtime Speech Recognizer")
	bool bSpeedUp = false;

	/** The size of the audio context (0 = use default) */
	UPROPERTY(BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"), Category = "Runtime Speech Recognizer")
	int32 AudioContextSize = 0;

	/** The temperature to increase when falling back when the decoding fails to meet either of the thresholds below */
	UPROPERTY(BlueprintReadWrite, Category = "Runtime Speech Recognizer")
	float TemperatureToIncrease = 0.4f;

	/** If the compression ratio is higher than this value, treat the decoding as failed. Similar to OpenAI's "compression_ratio_threshold" */
	UPROPERTY(BlueprintReadWrite, Category = "Runtime Speech Recognizer")
	float EntropyThreshold = 2.4f;

	/** Whether to suppress blanks showing up in outputs */
	UPROPERTY(BlueprintReadWrite, Category = "Runtime Speech Recognizer")
	bool bSuppressBlank = true;

	/** Whether to suppress non speech tokens in outputs */
	UPROPERTY(BlueprintReadWrite, Category = "Runtime Speech Recognizer")
	bool bSuppressNonSpeechTokens = false;
	
	/** Number of beams in beam search, only applicable when temperature is zero */
	UPROPERTY(BlueprintReadWrite, Category = "Runtime Speech Recognizer")
	int32 BeamSize = -1.f;

	/**
	 * Optional text to provide as a prompt for the first window
	 * This can be used to provide context for the recognition to make it more likely to predict the words correctly
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Runtime Speech Recognizer")
	FString InitialPrompt;

	/**
	 * Returns the default parameters suitable for non-streaming speech recognition
	 * @return The default parameters suitable for non-streaming speech recognition
	 */
	static FSpeechRecognitionParameters GetNonStreamingDefaults();

	/**
	 * Returns the default parameters suitable for streaming speech recognition
	 * @return The default parameters suitable for streaming speech recognition
	 */
	static FSpeechRecognitionParameters GetStreamingDefaults();

	/**
	 * Sets the default parameters suitable for non-streaming speech recognition
	 */
	void SetNonStreamingDefaults();

	/**
	 * Sets the default parameters suitable for streaming speech recognition
	 */
	void SetStreamingDefaults();

	/**
	 * Fills the Whisper state parameters with the current parameters
	 * @param WhisperState The Whisper state to fill the parameters for
	 */
	void FillWhisperStateParameters(FWhisperSpeechRecognizerState& WhisperState) const;
};

/**
 * Thread worker for speech recognition. Manages a worker thread that performs speech recognition on audio input data
 */
class RUNTIMESPEECHRECOGNIZER_API FSpeechRecognizerThread : public FRunnable, public TSharedFromThis<FSpeechRecognizerThread>
{
public:
	FSpeechRecognizerThread();
	virtual ~FSpeechRecognizerThread() override;

	/**
	 * Starts the thread worker
	 *
	 * @return True if the thread was successfully started, false otherwise
	 */
	TFuture<bool> StartThread();

private:
	/** Promise for starting the thread. Invalidated once the thread is fully started */
	TUniquePtr<TPromise<bool>> StartThreadPromise;

public:
	/**
	 * Stops the thread worker
	 */
	void StopThread();

	/**
	 * Processes the audio data and recognizes the words
	 *
	 * @param PCMData PCM audio data in 32-bit floating point interleaved format
	 * @param SampleRate The sample rate of the audio data
	 * @param NumOfChannels The number of channels in the audio data
	 * @param bLast Whether this is the last audio data to process. If true, the audio data will be queued for processing even if the enabled step size is not reached
	 */
	void ProcessPCMData(Audio::FAlignedFloatBuffer PCMData, float SampleRate, uint32 NumOfChannels, bool bLast);

	/**
	 * Processes audio data that was queued before but not yet processed, especially useful when using step size functionality
	 * This function ensures all audio data is processed, even if it did not fit into the step size yet
	 */
	void ForceProcessPendingAudioData();

	/**
	 * Clears the audio data that was queued before but not yet processed
	 *
	 * @param bClearPendingAudioData Whether to clear the pending audio data or not
	 * @param bClearAudioQueue Whether to clear the audio queue or not
	 */
	void ClearAudioData(bool bClearPendingAudioData, bool bClearAudioQueue);

	//~ Begin FRunnable Interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;
	//~ End FRunnable Interface

	/**
	 * Returns whether the thread worker is stopped or not
	 *
	 * @return True if the thread worker is stopped, false otherwise
	 */
	bool GetIsStopped() const;

	/**
	 * Returns whether the thread worker is currently stopping (but not yet stopped) or not
	 * It is set to true when the StopThread function is called, and set to false when the thread worker is fully stopped
	 * 
	 * @return True if the thread worker is currently stopping, false otherwise 
	 */
	bool GetIsStopping() const;

	/**
	 * Returns whether all the audio data has been processed or not
	 *
	 * @return True if all the audio data has been processed, false otherwise
	 */
	bool GetIsFinished() const;

	/** Delegate broadcast when all the audio data has been processed */
	FOnSpeechRecognitionFinished OnRecognitionFinished;

	/** Delegate broadcast when recognized words are received */
	FOnSpeechRecognizedTextSegment OnRecognizedTextSegment;

	/** Delegate broadcast when the speech recognition progress changes */
	FOnSpeechRecognitionProgress OnRecognitionProgress;

	/** Delegate broadcast when an error occurs during speech recognition */
	FOnSpeechRecognitionError OnRecognitionError;

	/** Delegate broadcast when the speech recognition thread fully stopped */
	FOnSpeechRecognitionStopped OnRecognitionStopped;

	/**
	 * Sets the parameters for speech recognition. If you want to change only specific parameters, consider using the individual setter functions
	 *
	 * @param Parameters The parameters to use for speech recognition
	 * @return True if the parameters were set successfully, false otherwise
	 * @note Can only be called when the thread worker is stopped
	 */
	bool SetRecognitionParameters(const FSpeechRecognitionParameters& Parameters);

	/**
	 * Returns the default parameters suitable for non-streaming speech recognition
	 * @return The default parameters suitable for non-streaming speech recognition
	 */
	static FSpeechRecognitionParameters GetNonStreamingDefaults();

	/**
	 * Returns the default parameters suitable for streaming speech recognition
	 * @return The default parameters suitable for streaming speech recognition
	 */
	static FSpeechRecognitionParameters GetStreamingDefaults();

	/**
	 * Returns the current recognition parameters
	 * @return The current recognition parameters
	 */
	FSpeechRecognitionParameters GetRecognitionParameters() const;

	/**
	 * Sets the default parameters suitable for non-streaming speech recognition
	 *
	 * @return True if the parameters were set successfully, false otherwise
	 * @note Can only be called when the thread worker is stopped
	 */
	bool SetNonStreamingDefaults();

	/**
	 * Sets the default parameters suitable for streaming speech recognition
	 *
	 * @return True if the parameters were set successfully, false otherwise
	 * @note Can only be called when the thread worker is stopped
	 */
	bool SetStreamingDefaults();

	/**
	 * Sets the number of threads to use for speech recognition
	 *
	 * @param Value The number of threads to use
	 * @return True if the number of threads was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 * @note Set this value to 0 to use the number of cores
	 */
	bool SetNumOfThreads(int32 Value);

	/**
	 * Sets the language to use for speech recognition
	 *
	 * @param Language The language to use. Must be supported by the selected language model in the Editor settings
	 * @return True if the language was successfully set, false otherwise
	 * @note Can only be called when the thread worker is stopped
	 * @note Setting the language to Auto will decrease the recognition accuracy and performance
	 */
	bool SetLanguage(ESpeechRecognizerLanguage Language);

	/**
	 * Sets whether to translate the recognized words to English
	 *
	 * @param bTranslate Whether to translate the recognized words to English or not. If true, the language model must be multilingual
	 * @return True if the translation was successfully set, false otherwise
	 * @note Can only be called when the thread worker is stopped
	 */
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
	bool SetStepSize(int32 Value);

	/**
	 * Sets whether to use past transcription (if any) as initial prompt for the decoder
	 *
	 * @param bNoContext Whether to use past transcription (if any) as initial prompt for the decoder
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	bool SetNoContext(bool bNoContext);

	/**
	 * Sets whether to force single segment output (useful for streaming)
	 *
	 * @param bSingleSegment Whether to force single segment output (useful for streaming)
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	bool SetSingleSegment(bool bSingleSegment);

	/**
	 * Sets the maximum number of tokens per text segment (0 = no limit)
	 *
	 * @param Value The maximum number of tokens per text segment (0 = no limit)
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	bool SetMaxTokens(int32 Value);

	/**
	 * Sets whether to speed up the recognition by 2x using Phase Vocoder
	 *
	 * @param bSpeedUp Whether to speed up the recognition by using a smaller model
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	bool SetSpeedUp(bool bSpeedUp);

	/**
	 * Sets the size of the audio context
	 *
	 * @param Value The size of the audio context
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	bool SetAudioContextSize(int32 Value);

	/**
	 * Sets the temperature to increase when falling back when the decoding fails to meet either of the thresholds below
	 *
	 * @param Value The temperature to increase when falling back when the decoding fails to meet either of the thresholds below
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	bool SetTemperatureToIncrease(float Value);

	/**
	 * Sets the entropy threshold
	 * If the compression ratio is higher than this value, treat the decoding as failed. Similar to OpenAI's "compression_ratio_threshold"
	 *
	 * @param Value The entropy threshold
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 */
	bool SetEntropyThreshold(float Value);

	/**
	 * Sets whether to suppress blanks showing up in outputs
	 *
	 * @param Value Whether to suppress blanks showing up in outputs
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 * @author https://github.com/amartinz
	 */
	bool SetSuppressBlank(bool Value);

	/**
	 * Sets whether to suppress non speech tokens showing up in outputs
	 *
	 * @param Value Whether to suppress non speech tokens showing up in outputs
	 * @return True if the setting was set successfully, false otherwise
	 * @note Can only be called when the thread is stopped
	 * @author https://github.com/amartinz
	 */
	bool SetSuppressNonSpeechTokens(bool Value);

	/**
	 * Sets the number of beams in beam search. Only applicable when temperature is zero
	 * 
	 * @param Value The number of beams in beam search
	 * @return True if the setting was set successfully, false otherwise
	 */
	bool SetBeamSize(int32 Value);

	/**
	 * Sets the initial prompt for the first window
	 * This can be used to provide context for the recognition to make it more likely to predict the words correctly
	 * 
	 * @param Value The initial prompt for the first window
	 * @return True if the setting was set successfully, false otherwise
	 */
	bool SetInitialPrompt(const FString& Value);

private:
	/**
	 * Callback type for loading the language model data
	 * The first argument indicates whether the load succeeded or not
	 * The second argument is a pointer to the language model bulk data
	 * The third argument is the size of the language model bulk data in memory
	 */
	using FOnLanguageModelLoaded = TFunction<void(bool, uint8*, int64)>;

	/**
	 * Asynchronously load the language model data and pass it to the provided callback
	 *
	 * @param OnLoadLanguageModel The callback function to pass the loaded language model data to
	 */
	void LoadLanguageModel(FOnLanguageModelLoaded&& OnLoadLanguageModel);

	/**
	 * Releases the memory used by the language model. Intended to be called when the thread is stopped
	 */
	void ReleaseMemory();

	/**
	 * Broadcasts an error message
	 *
	 * @param ShortErrorMessage Short error message to broadcast externally
	 * @param LongErrorMessage Long error message to display in the log
	 */
	void ReportError(const FString& ShortErrorMessage, const FString& LongErrorMessage);

private:
	/** Whether the thread worker is stopped or not */
	FThreadSafeBool bIsStopped;

	/**	Whether all the audio data has been processed or not */
	FThreadSafeBool bIsFinished;

	/** Whether the thread worker is currently stopping (but not yet stopped) */
	FThreadSafeBool bIsStopping;

	/** Thread instance */
	TUniquePtr<FRunnableThread> Thread;

	/** Queue of audio data waiting to be processed */
	TQueue<Audio::FAlignedFloatBuffer> AudioQueue;

	/**
	 * Pending audio data that automatically mixes and resamples audio data based on the whisper recognition requirements
	 */
	struct FPendingAudioData
	{
		FPendingAudioData() = default;
		FPendingAudioData(FPendingAudioData&& Other) noexcept
		{
			*this = MoveTemp(Other);
		}
		FPendingAudioData(const FPendingAudioData& Other) noexcept
		{
			*this = Other;
		}

		FPendingAudioData& operator=(FPendingAudioData&& Other) noexcept
		{
			if (this != &Other)
			{
				FScopeLock Lock(&DataGuard);
				AudioDataMap = MoveTemp(Other.AudioDataMap);
				TotalMixedAndResampledSize = Other.TotalMixedAndResampledSize;
			}
			return *this;
		}

		FPendingAudioData& operator=(const FPendingAudioData& Other) noexcept
		{
			if (this != &Other)
			{
				FScopeLock Lock(&DataGuard);
				AudioDataMap = Other.AudioDataMap;
				TotalMixedAndResampledSize = Other.TotalMixedAndResampledSize;
			}
			return *this;
		}

		/**
		 * Adds audio data to the pending audio data
		 * 
		 * @param AudioData Audio data to add
		 * @param SampleRate Sample rate of the audio data
		 * @param NumOfChannels Number of channels of the audio data
		 * @note This function is thread safe
		 */
		bool AddAudio(Audio::FAlignedFloatBuffer&& AudioData, float SampleRate, uint32 NumOfChannels);

		/**
		 * Gets the total size of the mixed and resampled audio data
		 * @return The total size of the mixed and resampled audio data
		 */
		int64 GetTotalMixedAndResampledSize() const;

		/**
		 * Gets the mixed and resampled audio data
		 * @param OutPCMData The mixed and resampled audio data
		 * @return True if the mixed and resampled audio data was successfully retrieved, false otherwise
		 */
		bool GetMixedAndResampledAudio(Audio::FAlignedFloatBuffer& OutPCMData);

	protected:
		/**
		 * Recalculates the estimated total size of the mixed and resampled audio data
		 */
		void RecalculateTotalMixedAndResampledSize();

	private:
		/** Map of audio data keyed by sample rate and number of channels */
		TMap<TPair<float /*SampleRate*/, uint32 /*NumOfChannels*/>, Audio::FAlignedFloatBuffer> AudioDataMap;

		/** Estimated total size of the mixed and resampled audio data */
		int64 TotalMixedAndResampledSize = 0;

		/** Data guard (mutex) for thread safety of the audio data map */
		mutable FCriticalSection DataGuard;
	};

	/** Audio data accumulated but not yet added to the queue */
	FPendingAudioData PendingAudio;

	/** Whisper state */
	FWhisperSpeechRecognizerState WhisperState;

	/** Recognition parameters */
	FSpeechRecognitionParameters RecognitionParameters;

public:
	/** The last progress made in the speech recognition process */
	std::atomic<int32> LastProgress { 0 };
};
