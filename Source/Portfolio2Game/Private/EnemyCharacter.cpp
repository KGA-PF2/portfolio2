#include "EnemyCharacter.h"
#include "PlayerCharacter.h"
#include "BattleManager.h"
#include "Components/CapsuleComponent.h"
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

	// 체력이 변하면 HandleHealthChanged 함수가 자동 실행
	if (HasAuthority())
	{
		OnHealthChanged.AddDynamic(this, &AEnemyCharacter::HandleHealthChanged);
	}
}

// 1단계: 생각하기
void AEnemyCharacter::DecideNextAction()
{
	if (bDead || !PlayerRef || !BrainData)
	{
		PendingAction = EAIActionType::Wait;
		return;
	}

	EAIActionType BestAction = EAIActionType::Wait;

	// DB 규칙 순차 검사 (우선순위 방식)
	for (const FAIActionRule& Rule : BrainData->ActionRules)
	{
		bool bAllConditionsMet = true;
		for (EAIConditionType Cond : Rule.RequiredConditions)
		{
			if (!CheckCondition(Cond))
			{
				bAllConditionsMet = false;
				break;
			}
		}

		if (bAllConditionsMet)
		{
			BestAction = Rule.ActionToExecute;
			break; // 선착순 채택
		}
	}

	PendingAction = BestAction;
}

// 2단계: 행동하기 (저장된 값으로 실행)
void AEnemyCharacter::ExecutePlannedAction()
{
	if (bDead)
	{
		EndAction();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("%s Executing Plan: %d"), *GetName(), (int32)PendingAction);

	// 저장해둔 행동 실행 (기존 PerformAction 호출)
	PerformAction(PendingAction);
}

// ───────── 조건 판독기 ─────────
bool AEnemyCharacter::CheckCondition(EAIConditionType Condition)
{
	if (!PlayerRef) return false;

	switch (Condition)
	{
	case EAIConditionType::None: return true;
	case EAIConditionType::HasReservedSkill: return (ReservedSkill != nullptr);
	case EAIConditionType::NoReservedSkill: return (ReservedSkill == nullptr);
	case EAIConditionType::JustAttacked: return bJustAttacked;

		// 사거리 내 체크
	case EAIConditionType::PlayerInSkillRange_Reserved: return IsPlayerInSkillRange(ReservedSkill);
	case EAIConditionType::PlayerInSkillRange_A: return IsPlayerInSkillRange(Skill_A);
	case EAIConditionType::PlayerInSkillRange_B: return IsPlayerInSkillRange(Skill_B);


		// [신규] 예약된 스킬 사거리 밖인가? (예약은 했는데 적이 튀었나?)
	case EAIConditionType::PlayerOutOfSkillRange_Reserved:
		return (ReservedSkill != nullptr && !IsPlayerInSkillRange(ReservedSkill));

	case EAIConditionType::PlayerOutOfSkillRange_A: return !IsPlayerInSkillRange(Skill_A);
	case EAIConditionType::PlayerOutOfSkillRange_B: return !IsPlayerInSkillRange(Skill_B);


	case EAIConditionType::PlayerInLine_Far:
	{
		int32 XDiff = PlayerRef->GridCoord.X - GridCoord.X;
		int32 YDiff = PlayerRef->GridCoord.Y - GridCoord.Y;
		USkillBase* CurrentSkill = ReservedSkill ? ReservedSkill : Skill_A;
		int32 MaxReach = GetMaxAttackRange(CurrentSkill);

		if (FacingDirection == EGridDirection::Right) return (YDiff == 0 && XDiff > MaxReach);
		if (FacingDirection == EGridDirection::Left)  return (YDiff == 0 && XDiff < -MaxReach);
		if (FacingDirection == EGridDirection::Down)  return (XDiff == 0 && YDiff > MaxReach);
		if (FacingDirection == EGridDirection::Up)    return (XDiff == 0 && YDiff < -MaxReach);
		return false;
	}

	case EAIConditionType::PlayerDifferentLine:
	{
		int32 XDiff = PlayerRef->GridCoord.X - GridCoord.X;
		int32 YDiff = PlayerRef->GridCoord.Y - GridCoord.Y;
		if (FacingDirection == EGridDirection::Right || FacingDirection == EGridDirection::Left) return YDiff != 0;
		else return XDiff != 0;
	}

	case EAIConditionType::PlayerNotInFront:
		return !IsPlayerInFrontCone();

	case EAIConditionType::CanMoveToAttackPos:
	{
		EGridDirection DummyDir;
		// 예약된 스킬이 있으면 그걸 위해, 없으면 A 스킬을 위해 이동 가능한지 체크
		USkillBase* TargetSkill = ReservedSkill ? ReservedSkill : Skill_A;
		return GetBestMovementToAttack(TargetSkill, DummyDir);
	}
	}
	return false;
}

bool AEnemyCharacter::IsPlayerInFrontCone()
{
	if (!PlayerRef) return false;
	int32 XDiff = PlayerRef->GridCoord.X - GridCoord.X;
	int32 YDiff = PlayerRef->GridCoord.Y - GridCoord.Y;

	if (FacingDirection == EGridDirection::Right) return XDiff > 0;
	if (FacingDirection == EGridDirection::Left) return XDiff < 0;
	if (FacingDirection == EGridDirection::Down) return YDiff > 0;
	if (FacingDirection == EGridDirection::Up) return YDiff < 0;
	return false;
}

// ───────── 행동 실행기 (Action Performer) ─────────
void AEnemyCharacter::PerformAction(EAIActionType ActionType)
{
	switch (ActionType)
	{
	case EAIActionType::FireReserved:   Action_FireReserved(); break;
	case EAIActionType::ReserveSkill_A: Action_ReserveSkill(Skill_A); break;
	case EAIActionType::ReserveSkill_B: Action_ReserveSkill(Skill_B); break;

	case EAIActionType::Move_Front: Action_Move(EGridDirection::Right); break;
	case EAIActionType::Move_Back:  Action_Move(EGridDirection::Left);  break;
	case EAIActionType::Move_Left:  Action_Move(EGridDirection::Up);    break;
	case EAIActionType::Move_Right: Action_Move(EGridDirection::Down);  break;

	case EAIActionType::RotateToPlayer: Action_RotateToPlayer(); break;
	case EAIActionType::MoveToBestAttackPos: Action_MoveToBestAttackPos(); break;

	case EAIActionType::Wait:
	default:
		bJustAttacked = false;
		EndAction();
		break;
	}
}

// ───────── [핵심] 이동 로직 통합 (좌표 변환) ─────────
// RelativeDir: 내 기준 상대 방향 (Right=전진, Left=후진, Up=왼쪽, Down=오른쪽)
void AEnemyCharacter::Action_Move(EGridDirection RelativeDir)
{
	if (!AbilitySystem)
	{
		EndAction();
		return;
	}

	FString MoveTag = "Ability.Move.Right"; // Default

	// 예: 내가 Right(X+)를 보고 있다.
	// 전진(Rel.Right) -> World Right
	// 후진(Rel.Left)  -> World Left
	// 왼쪽(Rel.Up)    -> World Up
	// 오른쪽(Rel.Down)-> World Down

	// 예: 내가 Up(Y-)를 보고 있다.
	// 전진(Rel.Right) -> World Up
	// 후진(Rel.Left)  -> World Down
	// 왼쪽(Rel.Up)    -> World Left (Y-에서 왼쪽은 X-)

	EGridDirection FinalWorldDir = EGridDirection::Right;

	if (FacingDirection == EGridDirection::Right) {
		if (RelativeDir == EGridDirection::Right) FinalWorldDir = EGridDirection::Right; // 전
		if (RelativeDir == EGridDirection::Left)  FinalWorldDir = EGridDirection::Left;  // 후
		if (RelativeDir == EGridDirection::Up)    FinalWorldDir = EGridDirection::Up;    // 좌
		if (RelativeDir == EGridDirection::Down)  FinalWorldDir = EGridDirection::Down;  // 우
	}
	else if (FacingDirection == EGridDirection::Left) {
		if (RelativeDir == EGridDirection::Right) FinalWorldDir = EGridDirection::Left;  // 전
		if (RelativeDir == EGridDirection::Left)  FinalWorldDir = EGridDirection::Right; // 후
		if (RelativeDir == EGridDirection::Up)    FinalWorldDir = EGridDirection::Down;  // 좌(Y+)
		if (RelativeDir == EGridDirection::Down)  FinalWorldDir = EGridDirection::Up;    // 우(Y-)
	}
	else if (FacingDirection == EGridDirection::Up) { // Y-
		if (RelativeDir == EGridDirection::Right) FinalWorldDir = EGridDirection::Up;    // 전
		if (RelativeDir == EGridDirection::Left)  FinalWorldDir = EGridDirection::Down;  // 후
		if (RelativeDir == EGridDirection::Up)    FinalWorldDir = EGridDirection::Left;  // 좌(X-)
		if (RelativeDir == EGridDirection::Down)  FinalWorldDir = EGridDirection::Right; // 우(X+)
	}
	else if (FacingDirection == EGridDirection::Down) { // Y+
		if (RelativeDir == EGridDirection::Right) FinalWorldDir = EGridDirection::Down;
		if (RelativeDir == EGridDirection::Left)  FinalWorldDir = EGridDirection::Up;
		if (RelativeDir == EGridDirection::Up)    FinalWorldDir = EGridDirection::Right;
		if (RelativeDir == EGridDirection::Down)  FinalWorldDir = EGridDirection::Left;
	}

	// 태그 문자열 변환
	switch (FinalWorldDir)
	{
	case EGridDirection::Right: MoveTag = "Ability.Move.Right"; break;
	case EGridDirection::Left:  MoveTag = "Ability.Move.Left"; break;
	case EGridDirection::Up:    MoveTag = "Ability.Move.Up"; break;
	case EGridDirection::Down:  MoveTag = "Ability.Move.Down"; break;
	}

	// 실행
	FGameplayTagContainer MoveTags;
	MoveTags.AddTag(FGameplayTag::RequestGameplayTag(*MoveTag));
	if (!AbilitySystem->TryActivateAbilitiesByTag(MoveTags))
	{
		EndAction(); // 실패 시 턴 넘김
	}

	bJustAttacked = false;
}

void AEnemyCharacter::Action_FireReserved()
{
	if (ReservedSkill) ExecuteSkill(ReservedSkill);
	ReservedSkill = nullptr;
	bJustAttacked = true;
	EndAction();
}

void AEnemyCharacter::Action_ReserveSkill(USkillBase* Skill)
{
	ReservedSkill = Skill;
	UE_LOG(LogTemp, Warning, TEXT("Skill Reserved: %s"), *Skill->SkillName.ToString());
	bJustAttacked = false;
	EndAction();
}

void AEnemyCharacter::Action_RotateToPlayer()
{
	int32 XDiff = PlayerRef->GridCoord.X - GridCoord.X;
	int32 YDiff = PlayerRef->GridCoord.Y - GridCoord.Y;
	EGridDirection TargetDir = FacingDirection;

	if (FMath::Abs(XDiff) >= FMath::Abs(YDiff)) TargetDir = (XDiff > 0) ? EGridDirection::Right : EGridDirection::Left;
	else TargetDir = (YDiff > 0) ? EGridDirection::Down : EGridDirection::Up;

	RequestRotation(TargetDir, nullptr);
	bJustAttacked = false;
}

void AEnemyCharacter::Action_MoveToBestAttackPos()
{
	USkillBase* Skill = ReservedSkill ? ReservedSkill : Skill_A;
	EGridDirection WorldMoveDir;

	if (GetBestMovementToAttack(Skill, WorldMoveDir))
	{
		FString MoveTag = "Ability.Move.Right";
		switch (WorldMoveDir) {
		case EGridDirection::Right: MoveTag = "Ability.Move.Right"; break;
		case EGridDirection::Left: MoveTag = "Ability.Move.Left"; break;
		case EGridDirection::Up: MoveTag = "Ability.Move.Up"; break;
		case EGridDirection::Down: MoveTag = "Ability.Move.Down"; break;
		}
		FGameplayTagContainer MoveTags;
		MoveTags.AddTag(FGameplayTag::RequestGameplayTag(*MoveTag));

		if (!AbilitySystem || !AbilitySystem->TryActivateAbilitiesByTag(MoveTags)) EndAction();
	}
	else
	{
		EndAction(); // 갈 곳 없음
	}
	bJustAttacked = false;
}

// 스킬 사용
bool AEnemyCharacter::ExecuteSkill(USkillBase* SkillToUse)
{
	// 1. 필수 데이터 확인
	if (!SkillToUse || !AbilitySystem || !GenericAttackAbilityClass)
	{
		return false;
	}

	// 2. GAS 어빌리티 찾기
	FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromClass(GenericAttackAbilityClass);
	if (Spec)
	{
		// 3. 데이터(Payload) 포장
		FGameplayEventData Payload;
		Payload.OptionalObject = SkillToUse; // 스킬 데이터(A 또는 B)를 넣음
		Payload.Instigator = this;
		Payload.Target = this;

		// 4. 발동 신호 전송
		FGameplayTag TriggerTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Skill.Attack"));

		AbilitySystem->TriggerAbilityFromGameplayEvent(
			Spec->Handle,
			AbilitySystem->AbilityActorInfo.Get(),
			TriggerTag,
			&Payload,
			*AbilitySystem
		);

		UE_LOG(LogTemp, Warning, TEXT("%s Used Skill: %s"), *GetName(), *SkillToUse->SkillName.ToString());
		return true;
	}

	return false;
}


// ───────── 유틸리티 ─────────

bool AEnemyCharacter::GetBestMovementToAttack(USkillBase* Skill, EGridDirection& OutWorldDir)
{
	if (!Skill || !PlayerRef) return false;
	FIntPoint MyPos = GridCoord;
	FIntPoint PlayerPos = PlayerRef->GridCoord;

	TArray<FIntPoint> SweetSpots;
	for (const FIntPoint& Point : Skill->AttackPattern)
	{
		// Y축 반전(-Point.Y)하여 4방향 역산 후보지 생성
		int32 PX = Point.X; int32 PY = -Point.Y;
		SweetSpots.Add(PlayerPos - FIntPoint(PX, PY));   // Right
		SweetSpots.Add(PlayerPos - FIntPoint(-PX, -PY)); // Left
		SweetSpots.Add(PlayerPos - FIntPoint(-PY, PX));  // Down
		SweetSpots.Add(PlayerPos - FIntPoint(PY, -PX));  // Up
	}

	int32 MinDist = 9999;
	bool bFound = false;
	const FIntPoint Dirs[] = { FIntPoint(1,0), FIntPoint(-1,0), FIntPoint(0,1), FIntPoint(0,-1) };
	const EGridDirection Enums[] = { EGridDirection::Right, EGridDirection::Left, EGridDirection::Down, EGridDirection::Up };

	for (int i = 0; i < 4; ++i)
	{
		FIntPoint NextPos = MyPos + Dirs[i];
		if (BattleManagerRef && BattleManagerRef->GetCharacterAt(NextPos) != nullptr) continue;

		for (const FIntPoint& Spot : SweetSpots)
		{
			int32 Dist = FMath::Abs(Spot.X - NextPos.X) + FMath::Abs(Spot.Y - NextPos.Y);
			if (Dist < MinDist)
			{
				MinDist = Dist;
				OutWorldDir = Enums[i];
				bFound = true;
			}
		}
	}
	return bFound;
}

// 스킬의 최대 사거리(전방 X축) 계산
int32 AEnemyCharacter::GetMaxAttackRange(USkillBase* Skill)
{
	if (!Skill) return 1; // 기본값

	int32 MaxX = 0;
	for (const FIntPoint& Pt : Skill->AttackPattern)
	{
		// X값이 전방 거리라고 가정
		if (Pt.X > MaxX) MaxX = Pt.X;
	}
	return (MaxX > 0) ? MaxX : 1;
}

bool AEnemyCharacter::IsPlayerInSkillRange(USkillBase* Skill)
{
	if (!Skill || !PlayerRef) return false;
	FIntPoint Origin = GridCoord;

	for (const FIntPoint& Point : Skill->AttackPattern)
	{
		int32 P_X = Point.X;
		int32 P_Y = -Point.Y; // [수정] Y 반전 (GA_SkillAttack과 통일)

		int32 RX = 0, RY = 0;
		if (FacingDirection == EGridDirection::Right) { RX = P_X; RY = P_Y; }
		else if (FacingDirection == EGridDirection::Left) { RX = -P_X; RY = -P_Y; }
		else if (FacingDirection == EGridDirection::Down) { RX = -P_Y; RY = P_X; }
		else if (FacingDirection == EGridDirection::Up) { RX = P_Y; RY = -P_X; }

		if (Origin + FIntPoint(RX, RY) == PlayerRef->GridCoord) return true;
	}
	return false;
}

// 피격 및 사망
void AEnemyCharacter::HandleHealthChanged(int32 NewHP, int32 NewMaxHP)
{
	if (bDead) return;

	// 1. 사망 판정
	if (NewHP <= 0)
	{
		Die();
	}
	// 2. 피격 판정 (살아있음)
	else
	{
		// 피격 몽타주 재생 (G키 기능 자동화)
		if (HitReactionMontage && GetMesh()->GetAnimInstance())
		{
			// 현재 다른 몽타주(공격 등)가 재생 중이 아닐 때만, 혹은 피격이 최우선이라면 강제 재생
			if (!GetMesh()->GetAnimInstance()->Montage_IsPlaying(nullptr))
			{
				PlayAnimMontage(HitReactionMontage);
			}
		}
	}
}

void AEnemyCharacter::Die()
{
	if (bDead) return;
	bDead = true;

	UE_LOG(LogTemp, Warning, TEXT("%s Died!"), *GetName());

	// 1. 충돌 제거 (시체가 길 막지 않게)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 2. 사망 이펙트 (SpawnVFX 대체)
	if (DeathParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			DeathParticle,
			GetActorLocation(),
			GetActorRotation(),
			true
		);
	}

	// 3. 사망 애니메이션 (T키 대체)
	float Duration = 0.f;
	if (DeathMontage)
	{
		Duration = PlayAnimMontage(DeathMontage);
	}

	// 4. 애니메이션 끝난 뒤 사라짐 처리
	float DestroyDelay = (Duration > 0.f) ? Duration : 0.1f;

	FTimerHandle DeathTimer;
	GetWorld()->GetTimerManager().SetTimer(DeathTimer, [this]()
		{
			this->SetActorHiddenInGame(true);
			this->Destroy();
		}, DestroyDelay, false);
}