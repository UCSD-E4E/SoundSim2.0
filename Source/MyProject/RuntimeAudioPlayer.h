#pragma once
 
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundWaveProcedural.h"
#include "Components/AudioComponent.h"
#include "RuntimeAudioPlayer.generated.h"
 
/**
 * A runtime audio player that loads WAV files from disk and plays them
 * using USoundWaveProcedural (no precaching, no asset import needed).
 *
 * Also provides batch loading from folders for integration with
 * queue/playlist systems, and CSV recording for ML data export.
 */
UCLASS(Blueprintable)
class MYPROJECT_API ARuntimeAudioPlayer : public AActor
{
    GENERATED_BODY()
 
public:
    ARuntimeAudioPlayer();
 
    /** Audio component used to play runtime-loaded audio */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
    UAudioComponent* AudioComponent;
 
    /** Editable path so you can set it per-instance in the Details panel */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Runtime")
    FString AudioFilePath;
 
    // -----------------------------------------------------------------
    // Single file operations
    // -----------------------------------------------------------------
 
    UFUNCTION(BlueprintCallable, Category = "Audio|Runtime")
    USoundWaveProcedural* LoadWavFromFile(const FString& FilePath);
 
    UFUNCTION(BlueprintCallable, Category = "Audio|Runtime")
    bool PlayWavFromFile(const FString& FilePath);
 
    // -----------------------------------------------------------------
    // Batch folder loading
    // -----------------------------------------------------------------
 
    UFUNCTION(BlueprintCallable, Category = "Audio|Runtime")
    TArray<USoundWaveProcedural*> LoadWavsFromFolder(const FString& AudioFolderPath, bool bRecursive = true);
 
    /** All sounds loaded by the most recent LoadWavsFromFolder call */
    UPROPERTY(BlueprintReadOnly, Category = "Audio|Runtime")
    TArray<USoundWaveProcedural*> LoadedSounds;
 
    /** File paths corresponding to each entry in LoadedSounds (same order) */
    UPROPERTY(BlueprintReadOnly, Category = "Audio|Runtime")
    TArray<FString> LoadedFilePaths;
 
    // -----------------------------------------------------------------
    // CSV Recording
    // -----------------------------------------------------------------
 
    /**
     * Start recording CSV data at a regular interval.
     * Each row logs: Timestamp, CurrentAudioFile
     * Full version will add: ListenerLocation, SourceLocation
     */
    UFUNCTION(BlueprintCallable, Category = "Audio|CSV")
    void StartCsvRecording();
 
    /** Stop recording and finalize the CSV file */
    UFUNCTION(BlueprintCallable, Category = "Audio|CSV")
    void StopCsvRecording();
 
    /** Check if CSV recording is currently active */
    UFUNCTION(BlueprintPure, Category = "Audio|CSV")
    bool IsRecording() const { return bIsRecording; }
 
    /** How often to write a CSV row (in seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|CSV")
    float RecordingInterval = 1.0f;
 
    /** Folder where CSV files are saved. Defaults to project Saved/SoundSimCSV/ */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|CSV")
    FString CsvOutputFolder;
 
    /** Manually set which index in LoadedFilePaths is currently playing.
     *  Call this from BP_AudioQueuePlayer whenever CurrentIndex changes. */
    UFUNCTION(BlueprintCallable, Category = "Audio|CSV")
    void SetCurrentPlayingIndex(int32 Index);
 
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
 
    UPROPERTY()
    USoundWaveProcedural* ProceduralSoundWave;
 
private:
    // --- WAV parsing ---
    bool ParseWavFile(const TArray<uint8>& RawFileData, TArray<uint8>& OutPCMData,
                      int32& OutSampleRate, int32& OutNumChannels, int32& OutBitsPerSample);
    bool ConvertTo16Bit(const TArray<uint8>& InPCMData, int32 BitsPerSample, TArray<uint8>& Out16BitPCM);
 
    // --- CSV internals ---
    void WriteCsvRow();
    FString GetCurrentAudioFileName() const;
    FString GenerateCsvFilePath() const;
 
    bool bIsRecording = false;
    int32 CurrentPlayingIndex = -1;
    FString CsvFilePath;
    FTimerHandle CsvTimerHandle;
    bool bCsvHeaderWritten = false;
};
