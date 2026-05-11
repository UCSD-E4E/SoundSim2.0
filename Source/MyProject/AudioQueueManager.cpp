#include "AudioQueueManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFileManager.h"
#include "TimerManager.h"

AAudioQueueManager::AAudioQueueManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

// =============================================================================
// Lifecycle
// =============================================================================

void AAudioQueueManager::BeginPlay()
{
    // Skip ARuntimeAudioPlayer::BeginPlay's auto-play behavior so queue control
    // is responsible for starting playback.
    AActor::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("AudioQueueManager: BeginPlay"));

    // Auto-start if Wwise events are configured
    if (WwiseEvents.Num() > 0)
    {
        StartQueue();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AudioQueueManager: No Wwise events configured, waiting for manual StartQueue()"));
    }
}

void AAudioQueueManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopQueue();
    Super::EndPlay(EndPlayReason);
}

// =============================================================================
// Queue control
// =============================================================================

void AAudioQueueManager::StartQueue()
{
    if (bQueueActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("AudioQueueManager: Queue is already running"));
        return;
    }

    // --- Step 1: Find all SoundSource actors in the level ---
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASoundSource::StaticClass(), FoundActors);

    Sources.Empty();
    for (AActor* Actor : FoundActors)
    {
        ASoundSource* Source = Cast<ASoundSource>(Actor);
        if (Source)
        {
            Sources.Add(Source);
        }
    }

    if (Sources.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("AudioQueueManager: No SoundSource actors found in the level"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("AudioQueueManager: Found %d SoundSource actors"), Sources.Num());

    // --- Step 2: Bind to each source's OnSourceFinished delegate ---
    for (ASoundSource* Source : Sources)
    {
        Source->OnSourceFinished.AddDynamic(this, &AAudioQueueManager::OnSourceFinished);
    }

    if (WwiseEvents.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("AudioQueueManager: No WwiseEvents are assigned for queue playback"));
        return;
    }

    // --- Step 4: Build the play order (sequential or shuffled) ---
    BuildPlayOrder();

    // --- Step 5: Start CSV recording if enabled ---
    if (bAutoStartCsvRecording)
    {
        StartMultiSourceCsvRecording();
    }

    // --- Step 6: Assign initial sounds to each source ---
    bQueueActive = true;

    for (ASoundSource* Source : Sources)
    {
        AssignNextSound(Source);
    }

    UE_LOG(LogTemp, Warning, TEXT("AudioQueueManager: Queue started — %d sounds across %d sources"),
           WwiseEvents.Num(), Sources.Num());
}

void AAudioQueueManager::StopQueue()
{
    if (!bQueueActive)
    {
        return;
    }

    bQueueActive = false;

    // Stop all sources
    for (ASoundSource* Source : Sources)
    {
        if (!Source)
        {
            continue;
        }

        if (Source->AkComponent)
        {
            Source->AkComponent->Stop();
        }
    }

    // Stop CSV recording if active
    if (bMultiSourceCsvActive)
    {
        StopMultiSourceCsvRecording();
    }

    UE_LOG(LogTemp, Warning, TEXT("AudioQueueManager: Queue stopped after %d total assignments"), TotalAssignments);
}

// =============================================================================
// Sound assignment
// =============================================================================

void AAudioQueueManager::OnSourceFinished(ASoundSource* Source)
{
    if (!bQueueActive || !Source)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("AudioQueueManager: Source [%s] finished, assigning next sound"),
           *Source->GetName());

    AssignNextSound(Source);
}

void AAudioQueueManager::AssignNextSound(ASoundSource* Source)
{
    if (!Source || PlayOrder.Num() == 0)
    {
        return;
    }

    // Check if we've gone through the entire play order
    if (PlayOrderPosition >= PlayOrder.Num())
    {
        if (bLoopQueue)
        {
            // Rebuild play order (re-shuffles if randomized) and start over
            BuildPlayOrder();
            UE_LOG(LogTemp, Warning, TEXT("AudioQueueManager: Looping — restarting queue"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AudioQueueManager: All sounds played, queue finished"));
            return;
        }
    }

    // Get the next sound index from the play order
    int32 SoundIndex = PlayOrder[PlayOrderPosition];
    PlayOrderPosition++;

    UAkAudioEvent* Event = WwiseEvents.IsValidIndex(SoundIndex) ? WwiseEvents[SoundIndex] : nullptr;
    Source->AssignWwiseEvent(Event, SoundIndex);

    TotalAssignments++;

    FString AssignedName = TEXT("Unknown");
    if (WwiseEvents.IsValidIndex(SoundIndex) && WwiseEvents[SoundIndex])
    {
        AssignedName = WwiseEvents[SoundIndex]->GetName();
    }

    UE_LOG(LogTemp, Log, TEXT("AudioQueueManager: Assigned sound %d (%s) to source [%s] — assignment #%d"),
           SoundIndex,
           *AssignedName,
           *Source->GetName(),
           TotalAssignments);
}

// =============================================================================
// Play order
// =============================================================================

void AAudioQueueManager::BuildPlayOrder()
{
    PlayOrder.Empty();
    PlayOrderPosition = 0;

    // Fill with sequential indices: 0, 1, 2, ...
    int32 NumItems = WwiseEvents.Num();
    for (int32 i = 0; i < NumItems; i++)
    {
        PlayOrder.Add(i);
    }

    // Shuffle if randomization is enabled (Fisher-Yates shuffle)
    if (bRandomize && PlayOrder.Num() > 1)
    {
        for (int32 i = PlayOrder.Num() - 1; i > 0; i--)
        {
            int32 j = FMath::RandRange(0, i);
            PlayOrder.Swap(i, j);
        }

        UE_LOG(LogTemp, Log, TEXT("AudioQueueManager: Shuffled play order"));
    }
}

// =============================================================================
// Multi-source CSV recording
// =============================================================================

void AAudioQueueManager::StartMultiSourceCsvRecording()
{
    if (bMultiSourceCsvActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("AudioQueueManager: CSV recording is already active"));
        return;
    }

    // Generate file path (same logic as parent, using CsvOutputFolder if set)
    FString OutputFolder = CsvOutputFolder;
    if (OutputFolder.IsEmpty())
    {
        OutputFolder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SoundSimCSV"));
    }
    IFileManager::Get().MakeDirectory(*OutputFolder, true);

    FString Timestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H-%M-%S"));
    FString FileName = FString::Printf(TEXT("soundsim_multi_%s.csv"), *Timestamp);
    MultiSourceCsvFilePath = FPaths::Combine(OutputFolder, FileName);

    // Write header row — one row per source per tick
    FString Header = TEXT("Timestamp,GameTime,SourceName,SourceX,SourceY,SourceZ,AudioFile\n");
    FFileHelper::SaveStringToFile(Header, *MultiSourceCsvFilePath,
        FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_None);

    bMultiSourceCsvActive = true;

    // Start timer using the parent's RecordingInterval
    GetWorldTimerManager().SetTimer(
        MultiSourceCsvTimerHandle,
        this,
        &AAudioQueueManager::WriteMultiSourceCsvRow,
        RecordingInterval,
        true  // looping
    );

    UE_LOG(LogTemp, Warning, TEXT("AudioQueueManager: Multi-source CSV recording started: %s"),
           *MultiSourceCsvFilePath);
}

void AAudioQueueManager::StopMultiSourceCsvRecording()
{
    if (!bMultiSourceCsvActive)
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(MultiSourceCsvTimerHandle);
    bMultiSourceCsvActive = false;

    UE_LOG(LogTemp, Warning, TEXT("AudioQueueManager: CSV recording stopped. File: %s"),
           *MultiSourceCsvFilePath);
}

void AAudioQueueManager::WriteMultiSourceCsvRow()
{
    if (!bMultiSourceCsvActive || MultiSourceCsvFilePath.IsEmpty())
    {
        return;
    }

    FString WallTimestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S"));
    float GameTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    FString AllRows;

    // Write one row for each source
    for (ASoundSource* Source : Sources)
    {
        if (!Source)
        {
            continue;
        }

        // Source name (actor label in the level)
        FString SourceName = Source->GetName();

        // Source world position
        FVector Pos = Source->GetActorLocation();

        // What audio file or event this source is currently playing
        FString AudioFile = TEXT("None");
        int32 SoundIdx = Source->GetCurrentSoundIndex();
        if (SoundIdx >= 0 && WwiseEvents.IsValidIndex(SoundIdx) && WwiseEvents[SoundIdx])
        {
            AudioFile = WwiseEvents[SoundIdx]->GetName();
        }

        AllRows += FString::Printf(TEXT("%s,%.2f,%s,%.2f,%.2f,%.2f,%s\n"),
            *WallTimestamp, GameTime, *SourceName,
            Pos.X, Pos.Y, Pos.Z, *AudioFile);
    }

    // Append all rows in one write
    FFileHelper::SaveStringToFile(AllRows, *MultiSourceCsvFilePath,
        FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
}
