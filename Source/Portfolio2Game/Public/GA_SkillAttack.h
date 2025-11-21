#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SkillBase.h"
#include "GA_SkillAttack.generated.h"

UCLASS()
class PORTFOLIO2GAME_API UGA_SkillAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_SkillAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 몽타주 재생 및 노티파이 대기
	void ExecuteAttackSequence(class ACharacterBase* Caster, USkillBase* SkillInfo);

	// 몽타주에서 보낸 노티파이(GameplayEvent) 수신 시 실행
	UFUNCTION()
	void OnMontageNotify(FGameplayEventData EventData);

	// 실제 이펙트 및 데미지 적용 (좌표 회전 포함)
	void ApplySkillEffects(ACharacterBase* Caster, USkillBase* SkillInfo);

private:
	// 실행 중인 스킬 정보 임시 저장
	UPROPERTY()
	TObjectPtr<USkillBase> CachedSkillInfo;

	// [신규] 전달받은 데미지 저장용
	float CachedDamage = 0.0f;
};