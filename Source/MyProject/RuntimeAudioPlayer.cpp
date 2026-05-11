#include "RuntimeAudioPlayer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFileManager.h"
#include "TimerManager.h"
 
ARuntimeAudioPlayer::ARuntimeAudioPlayer()
{
    PrimaryActorTick.bCanEverTick = false;
 
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
 
    AkComponent = CreateDefaultSubobject<UAkComponent>(TEXT("AkComponent"));
    AkComponent->SetupAttachment(RootComponent);
    AkComponent->bAutoActivate = false;
}
 
void ARuntimeAudioPlayer::BeginPlay()
{
    Super::BeginPlay();
 
    UE_LOG(LogTemp, Warning, TEXT("RuntimeAudioPlayer::BeginPlay fired"));
 
    if (WwiseEvent)
    {
        if (PostWwiseEvent())
        {
            UE_LOG(LogTemp, Warning, TEXT("RuntimeAudioPlayer: Wwise playback started successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RuntimeAudioPlayer: Failed to post Wwise event"));
        }
    }
}
 
void ARuntimeAudioPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bIsRecording)
    {
        StopCsvRecording();
    }
 
    Super::EndPlay(EndPlayReason);
}

bool ARuntimeAudioPlayer::PostWwiseEvent(UAkAudioEvent* Event)
{
    if (!Event)
    {
        Event = WwiseEvent;
    }

    if (!Event)
    {
        UE_LOG(LogTemp, Error, TEXT("RuntimeAudioPlayer::PostWwiseEvent called with no valid event"));
        return false;
    }

    if (!AkComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("RuntimeAudioPlayer::PostWwiseEvent missing AkComponent"));
        return false;
    }

    AkComponent->PostAkEvent(Event, 0, nullptr);
    UE_LOG(LogTemp, Log, TEXT("RuntimeAudioPlayer::PostWwiseEvent posted event %s"), *Event->GetName());
    return true;
}

void ARuntimeAudioPlayer::StopWwise()
{
    if (AkComponent)
    {
        AkComponent->Stop();
        UE_LOG(LogTemp, Log, TEXT("RuntimeAudioPlayer::StopWwise stopped AkComponent"));
    }
}

// =============================================================================
// CSV Recording
// =============================================================================
 
// =============================================================================
// CSV Recording
// =============================================================================
void ARuntimeAudioPlayer::SetCurrentPlayingIndex(int32 Index)
{
    CurrentPlayingIndex = Index;
}
 
FString ARuntimeAudioPlayer::GetCurrentAudioFileName() const
{
    if (WwiseEvent)
    {
        return WwiseEvent->GetName();
    }
    return TEXT("None");
}
 
FString ARuntimeAudioPlayer::GenerateCsvFilePath() const
{
    // Use custom folder if set, otherwise default to Saved/SoundSimCSV/
    FString OutputFolder = CsvOutputFolder;
    if (OutputFolder.IsEmpty())
    {
        OutputFolder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SoundSimCSV"));
    }
 
    // Create folder if it doesn't exist
    IFileManager::Get().MakeDirectory(*OutputFolder, true);
 
    // Generate filename with timestamp so each session gets its own file
    FString Timestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H-%M-%S"));
    FString FileName = FString::Printf(TEXT("soundsim_%s.csv"), *Timestamp);
 
    return FPaths::Combine(OutputFolder, FileName);
}
 
void ARuntimeAudioPlayer::StartCsvRecording()
{
    if (bIsRecording)
    {
        UE_LOG(LogTemp, Warning, TEXT("CSV recording is already active"));
        return;
    }
 
    // Generate the CSV file path
    CsvFilePath = GenerateCsvFilePath();
    bCsvHeaderWritten = false;
    bIsRecording = true;
 
    // Write header row
    FString Header = TEXT("Timestamp,GameTime,CurrentAudioFile\n");
    FFileHelper::SaveStringToFile(Header, *CsvFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_None);
    bCsvHeaderWritten = true;
 
    UE_LOG(LogTemp, Warning, TEXT("CSV recording started: %s (interval: %.1fs)"), *CsvFilePath, RecordingInterval);
 
    // Start timer that calls WriteCsvRow at the configured interval
    GetWorldTimerManager().SetTimer(
        CsvTimerHandle,
        this,
        &ARuntimeAudioPlayer::WriteCsvRow,
        RecordingInterval,
        true  // looping
    );
}
 
void ARuntimeAudioPlayer::StopCsvRecording()
{
    if (!bIsRecording)
    {
        UE_LOG(LogTemp, Warning, TEXT("CSV recording is not active"));
        return;
    }
 
    // Stop the timer
    GetWorldTimerManager().ClearTimer(CsvTimerHandle);
    bIsRecording = false;
 
    UE_LOG(LogTemp, Warning, TEXT("CSV recording stopped. File saved to: %s"), *CsvFilePath);
}
 
void ARuntimeAudioPlayer::WriteCsvRow()
{
    if (!bIsRecording || CsvFilePath.IsEmpty())
        return;
 
    // Get current wall clock timestamp
    FString WallTimestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S"));
 
    // Get game time in seconds
    float GameTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
 
    // Get current audio file name
    FString AudioFile = GetCurrentAudioFileName();
 
    // Format row
    FString Row = FString::Printf(TEXT("%s,%.2f,%s\n"), *WallTimestamp, GameTime, *AudioFile);
 
    // Append to file
    FFileHelper::SaveStringToFile(Row, *CsvFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
}
 
