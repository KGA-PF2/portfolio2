#include "EnemyCharacter.h"
#include "PlayerCharacter.h"
#include "BattleManager.h"
#include "Kismet/GameplayStatics.h"

AEnemyCharacter::AEnemyCharacter()
{
	RotateToDirection(EGridDirection::Left, false);
}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    PlayerRef = Cast<APlayerCharacter>(
        UGameplayStatics::GetActorOfClass(GetWorld(), APlayerCharacter::StaticClass()));

    BattleManagerRef = Cast<ABattleManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), ABattleManager::StaticClass()));
}

// ──────────────────────────────
// 적 턴 행동 (BattleManager에서 호출됨)
// ──────────────────────────────
void AEnemyCharacter::ExecuteAIAction()
{
	if (bDead || !PlayerRef)
	{
		EndAction();
		return;
	}

	// 1. 플레이어 방향 계산
	int32 XDiff = PlayerRef->GridCoord.X - GridCoord.X;
	int32 YDiff = PlayerRef->GridCoord.Y - GridCoord.Y;

	EGridDirection TargetDir = FacingDirection;

	// 더 멀리 떨어진 축을 기준으로 바라볼 방향 결정
	if (FMath::Abs(XDiff) >= FMath::Abs(YDiff))
	{
		TargetDir = (XDiff > 0) ? EGridDirection::Right : EGridDirection::Left;
	}
	else
	{
		TargetDir = (YDiff > 0) ? EGridDirection::Down : EGridDirection::Up;
	}

	// 2. 바라보는 방향이 틀리면 -> "회전" (턴 소모)
	if (FacingDirection != TargetDir)
	{
		// 180도 회전인지 체크 (단순화된 로직)
		bool bIs180 = (FacingDirection == EGridDirection::Right && TargetDir == EGridDirection::Left) ||
			(FacingDirection == EGridDirection::Left && TargetDir == EGridDirection::Right) ||
			(FacingDirection == EGridDirection::Up && TargetDir == EGridDirection::Down) ||
			(FacingDirection == EGridDirection::Down && TargetDir == EGridDirection::Up);

		UAnimMontage* MontageToUse = bIs180 ? Montage_Rotate180 : Montage_RotateCW;

		RequestRotation(TargetDir, MontageToUse);
		return; // 회전하고 턴 종료
	}

	// 3. 방향이 맞으면 -> 공격 가능 거리인지 체크
	bool bCanAttack = false;

	// 현재 바라보는 방향 기준 직선상에 있는지 확인
	switch (FacingDirection)
	{
	case EGridDirection::Right: bCanAttack = (YDiff == 0 && XDiff > 0 && XDiff <= 2); break;
	case EGridDirection::Left:  bCanAttack = (YDiff == 0 && XDiff < 0 && XDiff >= -2); break;
	case EGridDirection::Down:  bCanAttack = (XDiff == 0 && YDiff > 0 && YDiff <= 2); break;
	case EGridDirection::Up:    bCanAttack = (XDiff == 0 && YDiff < 0 && YDiff >= -2); break;
	}

	if (bCanAttack)
	{
		AttackNearestPlayer();
	}
	else
	{
		MoveForward();
	}
}

// ──────────────────────────────
// 플레이어 공격 (같은 행에서 가장 가까운 전방)
// ──────────────────────────────
void AEnemyCharacter::AttackNearestPlayer()
{
	// 예외 처리
	if (!PlayerRef || bDead)
	{
		EndAction();
		return;
	}

	// 1. 좌표 차이 계산
	int32 XDiff = PlayerRef->GridCoord.X - GridCoord.X;
	int32 YDiff = PlayerRef->GridCoord.Y - GridCoord.Y;

	bool bCanAttack = false;

	// 2. 현재 바라보는 방향(FacingDirection)에 플레이어가 있는지 + 사거리(2칸) 체크
	switch (FacingDirection)
	{
	case EGridDirection::Right: // X+ 방향 (오른쪽)
		// 같은 행(Y)이어야 하고, 플레이어가 내 오른쪽 1~2칸 내에 있어야 함
		bCanAttack = (YDiff == 0 && XDiff > 0 && XDiff <= 2);
		break;

	case EGridDirection::Left:  // X- 방향 (왼쪽)
		// 같은 행(Y), 플레이어가 내 왼쪽 1~2칸 내
		bCanAttack = (YDiff == 0 && XDiff < 0 && XDiff >= -2);
		break;

	case EGridDirection::Down:  // Y+ 방향 (아래쪽)
		// 같은 열(X), 플레이어가 내 아래쪽 1~2칸 내
		bCanAttack = (XDiff == 0 && YDiff > 0 && YDiff <= 2);
		break;

	case EGridDirection::Up:    // Y- 방향 (위쪽)
		// 같은 열(X), 플레이어가 내 위쪽 1~2칸 내
		bCanAttack = (XDiff == 0 && YDiff < 0 && YDiff >= -2);
		break;
	}

	// 3. 공격 실행 혹은 실패 시 이동
	if (bCanAttack)
	{
		// [수정됨] GAS 어빌리티 실행
		if (AbilitySystem && GenericAttackAbilityClass && NormalAttackSkill)
		{
			FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromClass(GenericAttackAbilityClass);
			if (Spec)
			{
				FGameplayEventData Payload;
				Payload.OptionalObject = NormalAttackSkill;
				Payload.Instigator = this;
				Payload.Target = this;

				FGameplayTag TriggerTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Skill.Attack"));

				AbilitySystem->TriggerAbilityFromGameplayEvent(
					Spec->Handle,
					AbilitySystem->AbilityActorInfo.Get(),
					TriggerTag,
					&Payload,
					*AbilitySystem
				);
			}
		}

		EndAction(); // 턴 종료
	}
	else
	{
		// 공격 범위에 없으면 전진 시도
		MoveForward();
	}
}
void AEnemyCharacter::MoveForward()
{
	if (!AbilitySystem)
	{
		EndAction();
		return;
	}

	FString TagName = "Ability.Move.Right"; // Default

	switch (FacingDirection)
	{
	case EGridDirection::Right: TagName = "Ability.Move.Right"; break;
	case EGridDirection::Left:  TagName = "Ability.Move.Left"; break;
	case EGridDirection::Up:    TagName = "Ability.Move.Up"; break;
	case EGridDirection::Down:  TagName = "Ability.Move.Down"; break;
	}

	FGameplayTag ContainerTag = FGameplayTag::RequestGameplayTag(*TagName);
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(ContainerTag);

	if (!AbilitySystem->TryActivateAbilitiesByTag(TagContainer))
	{
		EndAction(); // 실패 시 턴 넘김
	}
}