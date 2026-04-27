#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundWaveProcedural.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "SoundSource.generated.h"

/**
 * A placeable sound emitter in the world.
 *
 * The queue manager discovers all SoundSource actors in the level,
 * assigns each one a sound clip, and listens for OnSourceFinished
 * to know when to feed it the next clip.
 *
 * The sphere component is just for editor visibility / collision —
 * it makes it easy to see and select sources in the viewport.
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

	/** Audio component that actually plays spatialized sound */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SoundSource")
	UAudioComponent* AudioComponent;

	// -----------------------------------------------------------------
	// Interface for the queue
	// -----------------------------------------------------------------

	/**
	 * Assign a sound to this source and start playing it.
	 * @param Sound       The procedural sound wave to play.
	 * @param SoundIndex  Index into the queue's LoadedSounds array (for CSV tracking).
	 */
	UFUNCTION(BlueprintCallable, Category = "SoundSource")
	void AssignSound(USoundWaveProcedural* Sound, int32 SoundIndex);

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

	/** Timer handle for the duration-based completion workaround */
	FTimerHandle DurationTimerHandle;

	/** Called when the duration timer fires — broadcasts OnSourceFinished */
	void OnDurationElapsed();
};
