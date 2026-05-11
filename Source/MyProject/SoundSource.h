#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "AkComponent.h"
#include "AkAudioEvent.h"
#include "SoundSource.generated.h"

/**
 * A placeable Wwise sound emitter in the world.
 *
 * The queue manager discovers all SoundSource actors in the level,
 * assigns each one a Wwise event, and listens for OnSourceFinished
 * to know when to feed it the next event.
 */
UCLASS(Blueprintable)
class MYPROJECT_API ASoundSource : public AActor
{
	GENERATED_BODY()

public:
	ASoundSource();

	// -----------------------------------------------------------------
	// Components
	// -----------------------------------------------------------------

	/** Visible sphere so you can see/select this source in the editor */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SoundSource")
	USphereComponent* SphereVisual;

	/** Wwise AkComponent that plays spatialized Wwise events */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SoundSource")
	UAkComponent* AkComponent;

	/** Assign a Wwise event to this source and start playback */
	UFUNCTION(BlueprintCallable, Category = "SoundSource")
	void AssignWwiseEvent(UAkAudioEvent* Event, int32 SoundIndex);

	/** Returns true if this source is currently playing audio */
	UFUNCTION(BlueprintPure, Category = "SoundSource")
	bool IsPlaying() const;

	/** Returns the queue index of the sound currently assigned to this source */
	UFUNCTION(BlueprintPure, Category = "SoundSource")
	int32 GetCurrentSoundIndex() const { return CurrentSoundIndex; }

	// -----------------------------------------------------------------
	// Delegate — the queue binds to this to know when we're done
	// -----------------------------------------------------------------

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSourceFinished, ASoundSource*, Source);

	/** Broadcast when this source finishes playing its current sound */
	UPROPERTY(BlueprintAssignable, Category = "SoundSource")
	FOnSourceFinished OnSourceFinished;

private:
	/** Index of the currently assigned sound in the queue's array */
	int32 CurrentSoundIndex = -1;

	/** Called when a Wwise event posts an EndOfEvent callback */
	void OnWwiseEventCallback(EAkCallbackType CallbackType, UAkCallbackInfo* CallbackInfo);
};
