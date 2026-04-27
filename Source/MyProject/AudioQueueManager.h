#pragma once

#include "CoreMinimal.h"
#include "RuntimeAudioPlayer.h"
#include "SoundSource.h"
#include "AudioQueueManager.generated.h"

/**
 * Discovers all SoundSource actors in the level, loads a folder of WAVs,
 * and distributes them across the sources sequentially.
 *
 * When a source finishes its clip, it broadcasts OnSourceFinished and
 * this manager assigns it the next clip in the queue.
 *
 * Inherits from ARuntimeAudioPlayer to reuse WAV loading, parsing,
 * bit-depth conversion, and CSV recording.
 */
UCLASS(Blueprintable)
class MYPROJECT_API AAudioQueueManager : public ARuntimeAudioPlayer
{
    GENERATED_BODY()

public:
    AAudioQueueManager();

    // -----------------------------------------------------------------
    // Configuration (set in Details panel)
    // -----------------------------------------------------------------

    /** Folder containing WAV files to load and distribute */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Queue")
    FString AudioFolderPath;

    /** If true, restart from the beginning when all sounds have been played */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Queue")
    bool bLoopQueue = false;

    /** If true, shuffle the play order instead of going sequentially */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Queue")
    bool bRandomize = false;

    /** If true, load WAVs recursively from subfolders */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Queue")
    bool bRecursive = true;

    /** If true, automatically start CSV recording on BeginPlay */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Queue")
    bool bAutoStartCsvRecording = true;

    // -----------------------------------------------------------------
    // State (readable from Blueprint)
    // -----------------------------------------------------------------

    /** All SoundSource actors found in the level */
    UPROPERTY(BlueprintReadOnly, Category = "Queue")
    TArray<ASoundSource*> Sources;

    /** The next index in LoadedSounds to assign */
    UPROPERTY(BlueprintReadOnly, Category = "Queue")
    int32 NextSoundIndex = 0;

    /** Total number of sounds assigned so far (useful for stats/logging) */
    UPROPERTY(BlueprintReadOnly, Category = "Queue")
    int32 TotalAssignments = 0;

    // -----------------------------------------------------------------
    // Controls
    // -----------------------------------------------------------------

    /** Manually trigger the queue to start (if you don't want BeginPlay auto-start) */
    UFUNCTION(BlueprintCallable, Category = "Queue")
    void StartQueue();

    /** Stop all sources and reset the queue */
    UFUNCTION(BlueprintCallable, Category = "Queue")
    void StopQueue();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    /** Called when any SoundSource finishes its current clip */
    UFUNCTION()
    void OnSourceFinished(ASoundSource* Source);

    /** Assign the next available sound to a given source */
    void AssignNextSound(ASoundSource* Source);

    /** Build or rebuild the play order (sequential or shuffled) */
    void BuildPlayOrder();

    // --- Multi-source CSV recording ---
    // Replaces the parent's single-source CSV with one row per source per tick.
    // Format: Timestamp, GameTime, SourceName, SourceX, SourceY, SourceZ, AudioFile

    void StartMultiSourceCsvRecording();
    void StopMultiSourceCsvRecording();
    void WriteMultiSourceCsvRow();

    FString MultiSourceCsvFilePath;
    FTimerHandle MultiSourceCsvTimerHandle;
    bool bMultiSourceCsvActive = false;

    /** The order in which sounds will be assigned (indices into LoadedSounds) */
    TArray<int32> PlayOrder;

    /** Current position within PlayOrder */
    int32 PlayOrderPosition = 0;

    /** Whether the queue is actively running */
    bool bQueueActive = false;
};
