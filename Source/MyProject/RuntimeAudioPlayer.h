#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AkComponent.h"
#include "AkAudioEvent.h"
#include "RuntimeAudioPlayer.generated.h"

/**
 * A runtime audio player that uses Wwise events and AkComponent playback.
 *
 * Also provides CSV recording support for queue managers and audio analysis.
 */
UCLASS(Blueprintable)
class MYPROJECT_API ARuntimeAudioPlayer : public AActor
{
    GENERATED_BODY()
 
public:
    ARuntimeAudioPlayer();
 
    /** Wwise AkComponent used for event-based playback */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio|Wwise")
    UAkComponent* AkComponent;
 
    /** Default Wwise event to play from this actor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Wwise")
    UAkAudioEvent* WwiseEvent;
 
    /** Post a Wwise event on the attached AkComponent. If Event is null, uses the default WwiseEvent. */
    UFUNCTION(BlueprintCallable, Category = "Audio|Wwise")
    bool PostWwiseEvent(UAkAudioEvent* Event = nullptr);
 
    /** Stop any currently playing Wwise event on the attached AkComponent. */
    UFUNCTION(BlueprintCallable, Category = "Audio|Wwise")
    void StopWwise();
 
    // -----------------------------------------------------------------
    // CSV Recording
    // -----------------------------------------------------------------
 
    /**
     * Start recording CSV data at a regular interval.
     * Each row logs: Timestamp, CurrentAudioFile
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
 
    /** Manually set which index is currently playing. */
    UFUNCTION(BlueprintCallable, Category = "Audio|CSV")
    void SetCurrentPlayingIndex(int32 Index);
 
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
 
private:
    // --- CSV internals ---
    void WriteCsvRow();
    FString GenerateCsvFilePath() const;
 
    bool bIsRecording = false;
    int32 CurrentPlayingIndex = -1;
    FString CsvFilePath;
    FTimerHandle CsvTimerHandle;
    bool bCsvHeaderWritten = false;
};
