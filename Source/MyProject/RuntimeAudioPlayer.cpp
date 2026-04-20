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
 
    AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
    AudioComponent->SetupAttachment(RootComponent);
    AudioComponent->bAutoActivate = false;
}
 
void ARuntimeAudioPlayer::BeginPlay()
{
    Super::BeginPlay();
 
    UE_LOG(LogTemp, Warning, TEXT("RuntimeAudioPlayer::BeginPlay fired"));
 
    if (!AudioFilePath.IsEmpty())
    {
        if (PlayWavFromFile(AudioFilePath))
        {
            UE_LOG(LogTemp, Warning, TEXT("RuntimeAudioPlayer: Audio playback started successfully"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RuntimeAudioPlayer: Failed to play audio"));
        }
    }
}
 
void ARuntimeAudioPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Auto-stop CSV recording when the level ends to ensure file is finalized
    if (bIsRecording)
    {
        StopCsvRecording();
    }
 
    Super::EndPlay(EndPlayReason);
}
 
// =============================================================================
// Single file: Load (without playing)
// =============================================================================
USoundWaveProcedural* ARuntimeAudioPlayer::LoadWavFromFile(const FString& FilePath)
{
    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("File not found: %s"), *FilePath);
        return nullptr;
    }
 
    UE_LOG(LogTemp, Log, TEXT("Loading WAV file: %s"), *FilePath);
 
    TArray<uint8> RawData;
    if (!FFileHelper::LoadFileToArray(RawData, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FilePath);
        return nullptr;
    }
 
    TArray<uint8> PCMData;
    int32 SampleRate = 0;
    int32 NumChannels = 0;
    int32 BitsPerSample = 0;
 
    if (!ParseWavFile(RawData, PCMData, SampleRate, NumChannels, BitsPerSample))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse WAV: %s"), *FilePath);
        return nullptr;
    }
 
    UE_LOG(LogTemp, Log, TEXT("WAV parsed: %d Hz, %d ch, %d-bit, %d bytes PCM"),
           SampleRate, NumChannels, BitsPerSample, PCMData.Num());
 
    TArray<uint8> FinalPCM;
    if (!ConvertTo16Bit(PCMData, BitsPerSample, FinalPCM))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to convert audio to 16-bit PCM"));
        return nullptr;
    }
 
    USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>(this);
    if (!SoundWave)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create USoundWaveProcedural"));
        return nullptr;
    }
 
    SoundWave->SetSampleRate(SampleRate);
    SoundWave->NumChannels = NumChannels;
    SoundWave->Duration = (float)FinalPCM.Num() / (SampleRate * NumChannels * sizeof(int16));
    SoundWave->SoundGroup = SOUNDGROUP_Default;
    SoundWave->bLooping = false;
 
    SoundWave->QueueAudio(FinalPCM.GetData(), FinalPCM.Num());
 
    UE_LOG(LogTemp, Log, TEXT("Loaded: %s (%.2fs)"), *FPaths::GetCleanFilename(FilePath), SoundWave->Duration);
 
    return SoundWave;
}
 
// =============================================================================
// Single file: Load and play immediately
// =============================================================================
bool ARuntimeAudioPlayer::PlayWavFromFile(const FString& FilePath)
{
    ProceduralSoundWave = LoadWavFromFile(FilePath);
    if (!ProceduralSoundWave)
    {
        return false;
    }
 
    AudioComponent->SetSound(ProceduralSoundWave);
    AudioComponent->Play();
 
    UE_LOG(LogTemp, Log, TEXT("AudioComponent->Play() called"));
    return true;
}
 
// =============================================================================
// Batch folder loading
// =============================================================================
TArray<USoundWaveProcedural*> ARuntimeAudioPlayer::LoadWavsFromFolder(const FString& AudioFolderPath, bool bRecursive)
{
    LoadedSounds.Empty();
    LoadedFilePaths.Empty();
 
    if (!FPaths::DirectoryExists(AudioFolderPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Folder not found: %s"), *AudioFolderPath);
        return LoadedSounds;
    }
 
    UE_LOG(LogTemp, Warning, TEXT("Scanning folder for WAVs: %s (recursive: %s)"),
           *AudioFolderPath, bRecursive ? TEXT("yes") : TEXT("no"));
 
    TArray<FString> FoundFiles;
    IFileManager& FileManager = IFileManager::Get();
 
    if (bRecursive)
    {
        FileManager.FindFilesRecursive(FoundFiles, *AudioFolderPath, TEXT("*.wav"), true, false);
    }
    else
    {
        FString SearchPattern = FPaths::Combine(AudioFolderPath, TEXT("*.wav"));
        FileManager.FindFiles(FoundFiles, *SearchPattern, true, false);
 
        for (FString& FileName : FoundFiles)
        {
            FileName = FPaths::Combine(AudioFolderPath, FileName);
        }
    }
 
    FoundFiles.Sort();
 
    UE_LOG(LogTemp, Warning, TEXT("Found %d WAV files"), FoundFiles.Num());
 
    int32 SuccessCount = 0;
    int32 FailCount = 0;
 
    for (const FString& WavPath : FoundFiles)
    {
        USoundWaveProcedural* Sound = LoadWavFromFile(WavPath);
        if (Sound)
        {
            LoadedSounds.Add(Sound);
            LoadedFilePaths.Add(WavPath);
            SuccessCount++;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Skipped (failed to load): %s"), *WavPath);
            FailCount++;
        }
    }
 
    UE_LOG(LogTemp, Warning, TEXT("Batch load complete: %d loaded, %d failed, %d total"),
           SuccessCount, FailCount, FoundFiles.Num());
 
    return LoadedSounds;
}
 
// =============================================================================
// CSV Recording
// =============================================================================
void ARuntimeAudioPlayer::SetCurrentPlayingIndex(int32 Index)
{
    CurrentPlayingIndex = Index;
}
 
FString ARuntimeAudioPlayer::GetCurrentAudioFileName() const
{
    if (CurrentPlayingIndex >= 0 && CurrentPlayingIndex < LoadedFilePaths.Num())
    {
        return FPaths::GetCleanFilename(LoadedFilePaths[CurrentPlayingIndex]);
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
 
// =============================================================================
// 24-bit / 32-bit → 16-bit conversion
// =============================================================================
bool ARuntimeAudioPlayer::ConvertTo16Bit(const TArray<uint8>& InPCMData, int32 BitsPerSample, TArray<uint8>& Out16BitPCM)
{
    if (BitsPerSample == 16)
    {
        Out16BitPCM = InPCMData;
        return true;
    }
    else if (BitsPerSample == 24)
    {
        int32 NumSamples = InPCMData.Num() / 3;
        Out16BitPCM.SetNum(NumSamples * 2);
 
        for (int32 i = 0; i < NumSamples; i++)
        {
            Out16BitPCM[i * 2 + 0] = InPCMData[i * 3 + 1];
            Out16BitPCM[i * 2 + 1] = InPCMData[i * 3 + 2];
        }
 
        UE_LOG(LogTemp, Log, TEXT("Converted 24-bit -> 16-bit PCM: %d -> %d bytes"),
               InPCMData.Num(), Out16BitPCM.Num());
        return true;
    }
    else if (BitsPerSample == 32)
    {
        int32 NumSamples = InPCMData.Num() / 4;
        Out16BitPCM.SetNum(NumSamples * 2);
 
        for (int32 i = 0; i < NumSamples; i++)
        {
            Out16BitPCM[i * 2 + 0] = InPCMData[i * 4 + 2];
            Out16BitPCM[i * 2 + 1] = InPCMData[i * 4 + 3];
        }
 
        UE_LOG(LogTemp, Log, TEXT("Converted 32-bit -> 16-bit PCM: %d -> %d bytes"),
               InPCMData.Num(), Out16BitPCM.Num());
        return true;
    }
 
    UE_LOG(LogTemp, Error, TEXT("Unsupported bit depth: %d"), BitsPerSample);
    return false;
}
 
// =============================================================================
// Chunk-scanning WAV parser
// =============================================================================
bool ARuntimeAudioPlayer::ParseWavFile(const TArray<uint8>& RawFileData, TArray<uint8>& OutPCMData,
                                        int32& OutSampleRate, int32& OutNumChannels, int32& OutBitsPerSample)
{
    if (RawFileData.Num() < 44)
    {
        UE_LOG(LogTemp, Error, TEXT("WAV file too small: %d bytes"), RawFileData.Num());
        return false;
    }
 
    if (RawFileData[0] != 'R' || RawFileData[1] != 'I' ||
        RawFileData[2] != 'F' || RawFileData[3] != 'F')
    {
        UE_LOG(LogTemp, Error, TEXT("Not a RIFF file"));
        return false;
    }
 
    if (RawFileData[8] != 'W' || RawFileData[9] != 'A' ||
        RawFileData[10] != 'V' || RawFileData[11] != 'E')
    {
        UE_LOG(LogTemp, Error, TEXT("Not a WAVE file"));
        return false;
    }
 
    // --- Scan for 'fmt ' chunk ---
    int32 FmtOffset = -1;
    {
        int32 Offset = 12;
        while (Offset < RawFileData.Num() - 8)
        {
            if (RawFileData[Offset] == 'f' && RawFileData[Offset + 1] == 'm' &&
                RawFileData[Offset + 2] == 't' && RawFileData[Offset + 3] == ' ')
            {
                FmtOffset = Offset;
                break;
            }
            Offset++;
        }
    }
 
    if (FmtOffset < 0 || FmtOffset + 24 > RawFileData.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find 'fmt ' chunk"));
        return false;
    }
 
    int32 FmtDataOffset = FmtOffset + 8;
 
    uint16 AudioFormat  = *reinterpret_cast<const uint16*>(&RawFileData[FmtDataOffset + 0]);
    OutNumChannels      = *reinterpret_cast<const uint16*>(&RawFileData[FmtDataOffset + 2]);
    OutSampleRate       = *reinterpret_cast<const uint32*>(&RawFileData[FmtDataOffset + 4]);
    OutBitsPerSample    = *reinterpret_cast<const uint16*>(&RawFileData[FmtDataOffset + 14]);
 
    if (AudioFormat != 1)
    {
        UE_LOG(LogTemp, Error, TEXT("WAV is not PCM format (format tag: %d). Only PCM is supported."), AudioFormat);
        return false;
    }
 
    UE_LOG(LogTemp, Log, TEXT("fmt chunk: format=%d, channels=%d, sampleRate=%d, bitsPerSample=%d"),
           AudioFormat, OutNumChannels, OutSampleRate, OutBitsPerSample);
 
    // --- Scan for 'data' chunk ---
    int32 DataOffset = -1;
    uint32 DataSize = 0;
    {
        int32 Offset = 12;
        while (Offset < RawFileData.Num() - 8)
        {
            if (RawFileData[Offset] == 'd' && RawFileData[Offset + 1] == 'a' &&
                RawFileData[Offset + 2] == 't' && RawFileData[Offset + 3] == 'a')
            {
                DataSize = *reinterpret_cast<const uint32*>(&RawFileData[Offset + 4]);
                DataOffset = Offset + 8;
                break;
            }
            Offset++;
        }
    }
 
    if (DataOffset < 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find 'data' chunk"));
        return false;
    }
 
    if (DataOffset + (int32)DataSize > RawFileData.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("data chunk size (%u) exceeds file bounds, clamping"), DataSize);
        DataSize = RawFileData.Num() - DataOffset;
    }
 
    UE_LOG(LogTemp, Log, TEXT("data chunk at offset %d, size %u bytes"), DataOffset, DataSize);
 
    OutPCMData.SetNum(DataSize);
    FMemory::Memcpy(OutPCMData.GetData(), &RawFileData[DataOffset], DataSize);
 
    return true;
}
