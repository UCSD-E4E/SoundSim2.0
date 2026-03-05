// MicRecorder.cpp

#include "AMicRecorder.h"

#include "AudioMixerBlueprintLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundWave.h"
#include "HAL/FileManager.h"
#include "TimerManager.h"

// Sets default values
AMicRecorder::AMicRecorder()
{
    PrimaryActorTick.bCanEverTick = false; // we use a timer, not Tick
}

// Called when the game starts or when spawned
void AMicRecorder::BeginPlay()
{
    Super::BeginPlay();
}

// Called every frame (unused because bCanEverTick=false)
void AMicRecorder::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AMicRecorder::StartMicCapture()
{
    if (!MicSubmix)
    {
        UE_LOG(LogTemp, Warning, TEXT("MicSubmix not set. Assign SM_MicCapture to MicSubmix in the Details panel."));
        return;
    }

    // Start recording the chosen submix
    UAudioMixerBlueprintLibrary::StartRecordingOutput(
        this,
        RecordSeconds,   // can be 0 for indefinite; we stop with a timer
        MicSubmix
    );

    // Stop after N seconds
    GetWorldTimerManager().SetTimer(
        StopTimer,
        this,
        &AMicRecorder::StopMicCapture,
        RecordSeconds,
        false
    );

    UE_LOG(LogTemp, Log, TEXT("Mic capture started for %.2f seconds."), RecordSeconds);
}

void AMicRecorder::StopMicCapture()
{
    if (!MicSubmix)
    {
        UE_LOG(LogTemp, Warning, TEXT("MicSubmix not set on StopMicCapture."));
        return;
    }

    // Stop recording, return a SoundWave in memory
    USoundWave* Recorded = UAudioMixerBlueprintLibrary::StopRecordingOutput(
        this,
        EAudioRecordingExportType::SoundWave,
        TEXT(""),
        TEXT(""),
        MicSubmix
    );

    if (!Recorded)
    {
        UE_LOG(LogTemp, Warning, TEXT("StopRecordingOutput returned null."));
        return;
    }

    // Output folder
    const FString OutDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MicCaptures"));
    IFileManager::Get().MakeDirectory(*OutDir, true);

    // NOTE:
    // Depending on platform/settings, Recorded->RawPCMData may not be populated.
    // If this happens, the most reliable next step is exporting WAV instead.
    if (!Recorded->RawPCMData || Recorded->RawPCMDataSize <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Recorded SoundWave has no RawPCMData. Consider exporting WAV instead of CSV."));
        return;
    }

    // Interpret RawPCMData as 16-bit PCM
    const int32 NumSamples = Recorded->RawPCMDataSize / sizeof(int16);
    const int16* Samples16 = reinterpret_cast<const int16*>(Recorded->RawPCMData);

    FString Csv;
    Csv.Reserve(NumSamples * 16);
    Csv += TEXT("sample_index,time_s,amplitude_norm\n");

    for (int32 i = 0; i < NumSamples; i++)
    {
        const float Amp = FMath::Clamp(static_cast<float>(Samples16[i]) / 32768.0f, -1.0f, 1.0f);
        const float TimeS = (SampleRateHint > 0) ? (static_cast<float>(i) / static_cast<float>(SampleRateHint)) : 0.0f;

        Csv += FString::Printf(TEXT("%d,%.6f,%.6f\n"), i, TimeS, Amp);
    }

    const FString FileName = FString::Printf(TEXT("MicCapture_%s.csv"), *FDateTime::Now().ToString());
    const FString CsvPath = FPaths::Combine(OutDir, FileName);

    if (FFileHelper::SaveStringToFile(Csv, *CsvPath))
    {
        UE_LOG(LogTemp, Log, TEXT("Saved CSV to: %s"), *CsvPath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to save CSV to: %s"), *CsvPath);
    }
}
