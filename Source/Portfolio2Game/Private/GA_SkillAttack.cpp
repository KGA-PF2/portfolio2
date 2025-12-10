// GA_SkillAttack.cpp
#include "GA_SkillAttack.h"
#include "CharacterBase.h"
#include "BattleManager.h"
#include "PlayerCharacter.h"
#include "EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/Character.h"
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
	// 1. 기본값 (플레이어용 DA 몽타주)
	UAnimMontage* MontageToPlay = SkillInfo->SkillMontage;
	FName StartSectionName = NAME_None; // 기본값: 처음부터 재생

	// 2. 적 캐릭터라면? -> 적 전용 몽타주로 교체
	if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Caster))
	{
		UAnimMontage* EnemyMontage = Enemy->GetAttackMontageForSkill(SkillInfo);
		if (EnemyMontage)
		{
			MontageToPlay = EnemyMontage;

			// ★ [핵심 수정 1] 이미 이 몽타주를 재생 중인가? (준비 자세 루프 중인가?)
			if (Caster->GetMesh() && Caster->GetMesh()->GetAnimInstance())
			{
				if (Caster->GetMesh()->GetAnimInstance()->Montage_IsPlaying(MontageToPlay))
				{
					// 재생 중이라면 루프를 끊고 바로 "Attack_Start" 섹션부터 시작!
					// 이렇게 하면 Ready -> Ready_Stay(Loop) -> Attack_Start 로 매끄럽게 이어집니다.
					StartSectionName = FName("Attack_Start");
				}
			}
		}
	}

	// 3. 몽타주 검사
	if (!MontageToPlay)
	{
		ApplySkillEffects(Caster, SkillInfo);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}
	if (Caster)
	{
		Caster->SetAnimRootMotionTranslationScale(0.0f); // 제자리 고정
	}

	// 4. 재생 (섹션 이름 없이 처음부터 재생)
	UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		MontageToPlay,
		1.0f,
		StartSectionName,
		false
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
	if (Caster)
	{
		Caster->SetAnimRootMotionTranslationScale(1.0f); // 이동 가능 복구
	}
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
	float FinalDamage = (CachedDamage > 0.0f) ? CachedDamage : (float)SkillInfo->BaseDamage;

	// 추가 [회전 계산] 캐릭터의 GridDirection에 맞춰 이펙트 회전값(Yaw) 설정
	FRotator EffectRotation = FRotator::ZeroRotator;
	switch (Facing)
	{
	case EGridDirection::Right: // X+ (Unreal 기준 Forward가 0도)
		EffectRotation = FRotator(0.0f, 0.0f, 0.0f);
		break;
	case EGridDirection::Down:  // Y+ (Right가 90도)
		EffectRotation = FRotator(0.0f, 90.0f, 0.0f);
		break;
	case EGridDirection::Left:  // X- (Back이 180도)
		EffectRotation = FRotator(0.0f, 180.0f, 0.0f);
		break;
	case EGridDirection::Up:    // Y- (Left가 270도 혹은 -90도)
		EffectRotation = FRotator(0.0f, -90.0f, 0.0f);
		break;
	}

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
		// 추가 [위치 계산] 이펙트 오프셋도 회전 적용
		FVector TargetPos = BM->GetWorldLocation(TargetCoord);
		
		// 추가 [변경점] 단순히 더하는 것이 아니라, 회전값을 적용하여 더함
		TargetPos += EffectRotation.RotateVector(SkillInfo->EffectOffset);

		// 1순위: 나이아가라가 있으면 나이아가라 재생
		if (SkillInfo->NiagaraEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				SkillInfo->NiagaraEffect,
				TargetPos,
				EffectRotation,
				FVector(0.5f), 
				true,          
				true,          
				ENCPoolMethod::None,
				true
			);
		}
		// 2순위: 나이아가라가 없고 Cascade만 있으면 Cascade 재생
		else if (SkillInfo->TileEffect)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				SkillInfo->TileEffect,
				TargetPos,
				EffectRotation, // ZeroRotator -> EffectRotation 변경
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