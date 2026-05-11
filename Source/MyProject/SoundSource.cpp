#include "SoundSource.h"

ASoundSource::ASoundSource()
{
	PrimaryActorTick.bCanEverTick = false;

	// --- Sphere component (root) ---
	SphereVisual = CreateDefaultSubobject<USphereComponent>(TEXT("SphereVisual"));
	SphereVisual->SetSphereRadius(50.0f);
	SphereVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(SphereVisual);

	// --- Wwise AkComponent ---
	AkComponent = CreateDefaultSubobject<UAkComponent>(TEXT("AkComponent"));
	AkComponent->SetupAttachment(SphereVisual);
	AkComponent->bAutoActivate = false;
}

void ASoundSource::AssignWwiseEvent(UAkAudioEvent* Event, int32 SoundIndex)
{
	if (!Event)
	{
		UE_LOG(LogTemp, Warning, TEXT("SoundSource [%s]: AssignWwiseEvent called with null event"), *GetName());
		return;
	}

	CurrentSoundIndex = SoundIndex;

	if (!AkComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("SoundSource [%s]: Missing AkComponent"), *GetName());
		return;
	}

	AkComponent->Stop();
	AkComponent->PostAkEvent(Event, 1 << (int32)EAkCallbackType::EndOfEvent,
		FOnAkPostEventCallback::CreateUObject(this, &ASoundSource::OnWwiseEventCallback));

	UE_LOG(LogTemp, Log, TEXT("SoundSource [%s]: Playing Wwise event index %d"), *GetName(), SoundIndex);
}

void ASoundSource::OnWwiseEventCallback(EAkCallbackType CallbackType, UAkCallbackInfo* CallbackInfo)
{
	if (CallbackType == EAkCallbackType::EndOfEvent)
	{
		UE_LOG(LogTemp, Log, TEXT("SoundSource [%s]: Wwise event finished index %d"), *GetName(), CurrentSoundIndex);
		OnSourceFinished.Broadcast(this);
	}
}

bool ASoundSource::IsPlaying() const
{
	return AkComponent && AkComponent->IsPlaying();
}
