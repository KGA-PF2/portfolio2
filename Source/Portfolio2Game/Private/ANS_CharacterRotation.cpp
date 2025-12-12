#include "ANS_CharacterRotation.h"
#include "CharacterBase.h"

void UANS_CharacterRotation::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		if (ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner()))
		{
			Character->BeginRotationWindow(TotalDuration);
		}
	}
}

void UANS_CharacterRotation::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		if (ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner()))
		{
			Character->TickRotationWindow(FrameDeltaTime);
		}
	}
}

void UANS_CharacterRotation::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		if (ACharacterBase* Character = Cast<ACharacterBase>(MeshComp->GetOwner()))
		{
			Character->EndRotationWindow();
		}
	}
}