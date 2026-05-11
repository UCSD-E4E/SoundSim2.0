#include "SoundSource.h"
#include "TimerManager.h"

ASoundSource::ASoundSource()
{
	PrimaryActorTick.bCanEverTick = false;

	// --- Sphere component (root) ---
	// This is just a visual/collision marker so you can see the source
	// in the editor viewport and select it. It doesn't affect audio.
	SphereVisual = CreateDefaultSubobject<USphereComponent>(TEXT("SphereVisual"));
	SphereVisual->SetSphereRadius(50.0f);
	SphereVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(SphereVisual);

	// --- Audio component ---
	// Attached to the sphere, so wherever you place this actor in the
	// world is where the sound plays from (spatialized).
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(SphereVisual);
	AudioComponent->bAutoActivate = false;
}

void ASoundSource::AssignSound(USoundWaveProcedural* Sound, int32 SoundIndex)
{
	if (!Sound)
	{
		UE_LOG(LogTemp, Warning, TEXT("SoundSource [%s]: AssignSound called with null sound"),
			   *GetName());
		return;
	}

	// Clear any existing duration timer from a previous sound
	GetWorldTimerManager().ClearTimer(DurationTimerHandle);

	// Store which index this is (so the queue/CSV knows what's playing here)
	CurrentSoundIndex = SoundIndex;

	// Hand the sound to the audio component and play it
	AudioComponent->SetSound(Sound);
	AudioComponent->Play();

	// --- Duration-based completion workaround ---
	// USoundWaveProcedural doesn't fire OnAudioFinished, so we set a timer
	// that fires after the sound's duration. When it fires, we broadcast
	// OnSourceFinished so the queue knows to assign us a new clip.
	float Duration = Sound->Duration;

	if (Duration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			DurationTimerHandle,
			this,
			&ASoundSource::OnDurationElapsed,
			Duration,
			false  // not looping — fire once
		);

		UE_LOG(LogTemp, Log, TEXT("SoundSource [%s]: Playing index %d (%.2fs)"),
			   *GetName(), SoundIndex, Duration);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SoundSource [%s]: Sound has zero duration, firing finished immediately"),
			   *GetName());
		OnDurationElapsed();
	}
}

bool ASoundSource::IsPlaying() const
{
	return AudioComponent && AudioComponent->IsPlaying();
}

void ASoundSource::OnDurationElapsed()
{
	UE_LOG(LogTemp, Log, TEXT("SoundSource [%s]: Finished playing index %d"),
		   *GetName(), CurrentSoundIndex);

	// Stop the audio component (it may still be "playing" due to timing drift)
	if (AudioComponent)
	{
		AudioComponent->Stop();
	}

	// Tell the queue "I'm done, give me another one"
	OnSourceFinished.Broadcast(this);
}
