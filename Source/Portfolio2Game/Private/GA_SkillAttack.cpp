// GA_SkillAttack.cpp
#include "GA_SkillAttack.h"
#include "CharacterBase.h"
#include "BattleManager.h"
#include "PlayerCharacter.h" // 아군/적군 판별용
#include "Kismet/GameplayStatics.h" // Cascade 파티클 스폰용
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
	// TriggerEventData의 OptionalObject에 담겨온 스킬 정보를 꺼냅니다.
	USkillBase* SkillInfo = Cast<USkillBase>(TriggerEventData->OptionalObject);

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
	PlayMontageTask->OnCompleted.AddDynamic(this, &UGA_SkillAttack::K2_EndAbility);
	PlayMontageTask->OnInterrupted.AddDynamic(this, &UGA_SkillAttack::K2_EndAbility);
	PlayMontageTask->OnCancelled.AddDynamic(this, &UGA_SkillAttack::K2_EndAbility);

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
		int32 RotatedX = 0;
		int32 RotatedY = 0;

		switch (Facing)
		{
		case EGridDirection::Right: // 기본 (X+)
			RotatedX = Point.X;
			RotatedY = Point.Y;
			break;
		case EGridDirection::Left:  // 180도 (X-)
			RotatedX = -Point.X;
			RotatedY = -Point.Y;
			break;
		case EGridDirection::Down:  // 90도 시계 (Y+) -> 전방이 Y+, 우측이 X-
			RotatedX = -Point.Y;
			RotatedY = Point.X;
			break;
		case EGridDirection::Up:    // 90도 반시계 (Y-) -> 전방이 Y-, 우측이 X+
			RotatedX = Point.Y;
			RotatedY = -Point.X;
			break;
		}

		FIntPoint TargetCoord = Origin + FIntPoint(RotatedX, RotatedY);
		// ────────────────────────────────

		// 1. [시각 효과] Cascade 파티클 스폰
		// 인덱스가 유효하지 않아도(맵 밖이라도) 좌표만 구해서 스폰함
		FVector TargetPos = BM->GetWorldLocation(TargetCoord);
		TargetPos += SkillInfo->EffectOffset;

		if (SkillInfo->TileEffect)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				SkillInfo->TileEffect,
				TargetPos,
				FRotator::ZeroRotator,
				true, // Auto Destroy
				EPSCPoolMethod::AutoRelease
			);
		}

		// 2. [데미지 처리] 유효한 캐릭터가 있는지 확인
		ACharacterBase* TargetChar = BM->GetCharacterAt(TargetCoord);

		if (TargetChar && TargetChar != Caster)
		{
			// 아군 오인 사격 방지
			bool bIsCasterPlayer = Caster->IsA(APlayerCharacter::StaticClass());
			bool bIsTargetPlayer = TargetChar->IsA(APlayerCharacter::StaticClass());

			if (bIsCasterPlayer != bIsTargetPlayer)
			{
				TargetChar->ApplyDamage(FinalDamage);
			}
		}
	}
}