// GA_SkillAttack.cpp
#include "GA_SkillAttack.h"
#include "CharacterBase.h"
#include "BattleManager.h"
#include "PlayerCharacter.h"
#include "EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

UGA_SkillAttack::UGA_SkillAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 이 태그로 스킬을 트리거합니다.
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Skill.Attack")));
}

void UGA_SkillAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacterBase* Caster = Cast<ACharacterBase>(ActorInfo->AvatarActor.Get());
	USkillBase* SkillInfo = nullptr;

	if (TriggerEventData)
	{
		SkillInfo = Cast<USkillBase>(TriggerEventData->OptionalObject);

		if (TriggerEventData->EventMagnitude > 0.0f)
		{
			CachedDamage = TriggerEventData->EventMagnitude;
		}
		else if (SkillInfo)
		{
			CachedDamage = (float)SkillInfo->BaseDamage;
		}
	}

	if (!Caster || !SkillInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedSkillInfo = SkillInfo;
	ExecuteAttackSequence(Caster, SkillInfo);
}

void UGA_SkillAttack::ExecuteAttackSequence(ACharacterBase* Caster, USkillBase* SkillInfo)
{
	// 1. 몽타주가 없으면 즉시 발동 후 종료
	if (!SkillInfo->SkillMontage)
	{
		ApplySkillEffects(Caster, SkillInfo);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 2. 몽타주 재생 태스크
	UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, SkillInfo->SkillMontage, 1.0f, NAME_None, false
	);

	// 종료/취소 시 어빌리티 종료 연결
	PlayMontageTask->OnCompleted.AddDynamic(this, &UGA_SkillAttack::OnMontageEnded);
	PlayMontageTask->OnInterrupted.AddDynamic(this, &UGA_SkillAttack::OnMontageEnded);
	PlayMontageTask->OnCancelled.AddDynamic(this, &UGA_SkillAttack::OnMontageEnded);

	// 3. "Event.Skill.Hit" 노티파이 대기 태스크
	FGameplayTag HitTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Skill.Hit"));
	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, HitTag, nullptr, false, false
	);

	WaitEventTask->EventReceived.AddDynamic(this, &UGA_SkillAttack::OnMontageNotify);

	// 태스크 활성화
	WaitEventTask->ReadyForActivation();
	PlayMontageTask->ReadyForActivation();
}

// 애니메이션 종료 시 호출
void UGA_SkillAttack::OnMontageEnded()
{
	ACharacterBase* Caster = Cast<ACharacterBase>(GetAvatarActorFromActorInfo());

	// 어빌리티 종료
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

	if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Caster))
	{
		Enemy->EndAction();
	}
}

void UGA_SkillAttack::OnMontageNotify(FGameplayEventData EventData)
{
	ACharacterBase* Caster = Cast<ACharacterBase>(GetAvatarActorFromActorInfo());
	if (Caster && CachedSkillInfo)
	{
		ApplySkillEffects(Caster, CachedSkillInfo);
	}
}

void UGA_SkillAttack::ApplySkillEffects(ACharacterBase* Caster, USkillBase* SkillInfo)
{
	ABattleManager* BM = Caster->BattleManagerRef;
	if (!BM) return;

	FIntPoint Origin = Caster->GridCoord;
	EGridDirection Facing = Caster->FacingDirection;
	float FinalDamage = (float)SkillInfo->BaseDamage; // 플레이어라면 강화 수치 합산 로직 추가 가능

	for (const FIntPoint& Point : SkillInfo->AttackPattern)
	{
		// ───────── [좌표 회전 계산] ─────────
		// Point.X = 전방 거리, Point.Y = 우측 거리
		int32 P_X = Point.X;
		int32 P_Y = -Point.Y;

		int32 RotatedX = 0;
		int32 RotatedY = 0;

		switch (Facing)
		{
		case EGridDirection::Right: // X+
			RotatedX = P_X; RotatedY = P_Y; break;
		case EGridDirection::Left:  // X-
			RotatedX = -P_X; RotatedY = -P_Y; break;
		case EGridDirection::Down:  // Y+
			RotatedX = -P_Y; RotatedY = P_X; break;
		case EGridDirection::Up:    // Y-
			RotatedX = P_Y; RotatedY = -P_X; break;
		}
		FIntPoint TargetCoord = Origin + FIntPoint(RotatedX, RotatedY);
		// ────────────────────────────────

		// 1. [시각 효과] Cascade 파티클 스폰
		// 인덱스가 유효하지 않아도(맵 밖이라도) 좌표만 구해서 스폰함
		FVector TargetPos = BM->GetWorldLocation(TargetCoord);
		TargetPos += SkillInfo->EffectOffset;

		// 1순위: 나이아가라가 있으면 나이아가라 재생
		if (SkillInfo->NiagaraEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				SkillInfo->NiagaraEffect,
				TargetPos,
				FRotator::ZeroRotator
			);
		}
		// 2순위: 나이아가라가 없고 Cascade만 있으면 Cascade 재생
		else if (SkillInfo->TileEffect)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				SkillInfo->TileEffect,
				TargetPos,
				FRotator::ZeroRotator,
				true,
				EPSCPoolMethod::AutoRelease
			);
		}
		// 둘 다 없으면 아무것도 안 나옴

		// 2. [데미지 처리] 유효한 캐릭터가 있는지 확인
		ACharacterBase* TargetChar = BM->GetCharacterAt(TargetCoord);

		// "대상이 존재하고" && "나 자신이 아닐 때"만 공격
		if (TargetChar && TargetChar != Caster)
		{
			TargetChar->ApplyDamage(FinalDamage);
		}
	}
}