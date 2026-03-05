
// AMicRecorder.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundSubmix.h"
#include "AMicRecorder.generated.h"

UCLASS()
class MYPROJECT_API AMicRecorder : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mic")
    USoundSubmix* MicSubmix = nullptr; // assign SM_MicCapture in editor

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mic")
    float RecordSeconds = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mic")
    int32 SampleRateHint = 48000; // used for timestamps in CSV

    UFUNCTION(BlueprintCallable, Category="Mic")
    void StartMicCapture();

private:
    void StopMicCapture();

    FTimerHandle StopTimer;
};

