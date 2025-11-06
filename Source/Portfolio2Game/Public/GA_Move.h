#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CharacterBase.h" // (신규) EGridDirection Enum을 사용하기 위해 포함
#include "GA_Move.generated.h"

class UAbilityTask_MoveToLocation;

/**
 * (수정됨) WASD 입력에 바인딩되어 실제 그리드 이동을 처리하는 Gameplay Ability입니다.
 * 1. 목표 좌표 계산
 * 2. 애니메이션 재생
 * 3. Latent Task(MoveToLocation)를 사용한 부드러운 이동
 * 4. 이동 완료 시 턴 종료(EndAction) 호출
 */
UCLASS()
class PORTFOLIO2GAME_API UGA_Move : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Move();

	/**
	 * (신규) 이 어빌리티가 담당할 이동 방향입니다.
	 * (에디터에서 GA_Move_Up, GA_Move_Down 등 4개의 BP를 만들고 각각 설정)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grid Movement")
	EGridDirection MoveDirection;

	/**
	 * (신규) 캐릭터가 이동할 속도입니다.
	 * (CharacterBase의 MoveSpeed와 일치시키거나 여기서 개별 설정)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grid Movement")
	float MoveSpeed = 600.f;


	/** 어빌리티 활성화 시 호출됩니다. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	/** (신규) 이동이 완료되었을 때 AbilityTask에 의해 호출될 콜백 함수입니다. */
	UFUNCTION()
	void OnMoveFinished();
};