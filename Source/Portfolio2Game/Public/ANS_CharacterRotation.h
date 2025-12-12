#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_CharacterRotation.generated.h"

UCLASS()
class PORTFOLIO2GAME_API UANS_CharacterRotation : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// 노티파이 시작 (구간 진입)
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	// 노티파이 진행 중 (매 프레임 호출)
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	// 노티파이 끝 (구간 종료)
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};