#include "EnemyCharacter.h"
#include "PlayerCharacter.h"
#include "BattleManager.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

AEnemyCharacter::AEnemyCharacter()
{
	// 위젯 컴포넌트 생성
	OrderWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OrderWidgetComponent"));
	OrderWidgetComponent->SetupAttachment(RootComponent);

	// 설정: 월드 공간(캐릭터 옆에 둥둥), 원하는 크기로 그리기
	OrderWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	OrderWidgetComponent->SetDrawAtDesiredSize(true);

	// 위치: 캐릭터 오른쪽 위 (Y+, Z+)
	OrderWidgetComponent->SetRelativeLocation(FVector(0.0f, 60.0f, 100.0f));
}

void AEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

	//순서 숨김
	HideActionOrder();

    PlayerRef = Cast<APlayerCharacter>(
        UGameplayStatics::GetActorOfClass(GetWorld(), APlayerCharacter::StaticClass()));

	// 체력이 변하면 HandleHealthChanged 함수가 자동 실행
	if (HasAuthority())
	{
		OnHealthChanged.AddDynamic(this, &AEnemyCharacter::HandleHealthChanged);
	}
}

UAnimMontage* AEnemyCharacter::GetAttackMontageForSkill(USkillBase* SkillDef)
{
	if (SkillDef == Skill_A) return Montage_Atk_A;
	if (SkillDef == Skill_B) return Montage_Atk_B;
	return nullptr;
}

UAnimMontage* AEnemyCharacter::GetReadyMontageForSkill(USkillBase* SkillDef)
{
	if (SkillDef == Skill_A) return Montage_AtkReady_A;
	if (SkillDef == Skill_B) return Montage_AtkReady_B;
	return nullptr;
}

// ───────── 공격 준비 모션 재생 ─────────
void AEnemyCharacter::PlayChargeMontageIfReady()
{
	// 이번 턴에 발사(FireReserved) 예정이라면 -> 준비 동작 재생
	if (PendingAction == EAIActionType::FireReserved && ReservedSkill)
	{
		UAnimMontage* ReadyMontage = GetReadyMontageForSkill(ReservedSkill);

		if (ReadyMontage && GetMesh()->GetAnimInstance())
		{
			if (!GetMesh()->GetAnimInstance()->Montage_IsPlaying(ReadyMontage))
			{
				PlayAnimMontage(ReadyMontage);
			}
		}
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

			// 이동/회전 저장
			if (BestAction == EAIActionType::MoveToBestAttackPos)
			{
				USkillBase* TargetSkill = ReservedSkill ? ReservedSkill : Skill_A;

				// 방향을 계산해서 PendingMoveDir에 저장 (결과가 false면 대기로 변경)
				if (!GetBestMovementToAttack(TargetSkill, PendingMoveDir))
				{
					BestAction = EAIActionType::Wait;
				}
			}
			else if (BestAction == EAIActionType::RotateToPlayer)
			{
				// 플레이어 위치와 비교해서 어느 쪽을 볼지 미리 계산
				int32 XDiff = PlayerRef->GridCoord.X - GridCoord.X;
				int32 YDiff = PlayerRef->GridCoord.Y - GridCoord.Y;

				// X축 차이가 더 크면 앞/뒤, Y축 차이가 더 크면 좌/우
				if (FMath::Abs(XDiff) >= FMath::Abs(YDiff))
					PendingFaceDir = (XDiff > 0) ? EGridDirection::Right : EGridDirection::Left;
				else
					PendingFaceDir = (YDiff > 0) ? EGridDirection::Down : EGridDirection::Up;
			}
			break;
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

void AEnemyCharacter::Action_MoveDirectly(EGridDirection WorldDir)
{
	if (!AbilitySystem)
	{
		EndAction();
		return;
	}

	CurrentStopMontage = WalkStopMontage;

	// ───────── 1. 몽타주 결정 (상대 방향 역산) ─────────
	// 내가 보고 있는 방향(Facing)과 가야 할 월드 방향(WorldDir)을 비교해서
	// 어떤 몽타주(앞/뒤/좌/우)를 틀어야 할지 결정합니다.

	if (FacingDirection == EGridDirection::Right) // 나는 오른쪽(X+)을 보는 중
	{
		if (WorldDir == EGridDirection::Right) RunMontage = WalkFrontMontage; // 전진
		else if (WorldDir == EGridDirection::Left)  RunMontage = WalkBackMontage;  // 후퇴
		else if (WorldDir == EGridDirection::Up)    RunMontage = WalkLeftMontage;  // 왼쪽(Y-)으로 게걸음
		else if (WorldDir == EGridDirection::Down)  RunMontage = WalkRightMontage; // 오른쪽(Y+)으로 게걸음
	}
	else if (FacingDirection == EGridDirection::Left) // 나는 왼쪽(X-)을 보는 중
	{
		if (WorldDir == EGridDirection::Left)  RunMontage = WalkFrontMontage; // 전진
		else if (WorldDir == EGridDirection::Right) RunMontage = WalkBackMontage;  // 후퇴
		else if (WorldDir == EGridDirection::Down)  RunMontage = WalkLeftMontage;  // 왼쪽(Y+)으로 게걸음
		else if (WorldDir == EGridDirection::Up)    RunMontage = WalkRightMontage; // 오른쪽(Y-)으로 게걸음
	}
	else if (FacingDirection == EGridDirection::Up) // 나는 위쪽(Y-)을 보는 중
	{
		if (WorldDir == EGridDirection::Up)    RunMontage = WalkFrontMontage;
		else if (WorldDir == EGridDirection::Down)  RunMontage = WalkBackMontage;
		else if (WorldDir == EGridDirection::Left)  RunMontage = WalkLeftMontage;  // 왼쪽(X-)
		else if (WorldDir == EGridDirection::Right) RunMontage = WalkRightMontage; // 오른쪽(X+)
	}
	else if (FacingDirection == EGridDirection::Down) // 나는 아래쪽(Y+)을 보는 중
	{
		if (WorldDir == EGridDirection::Down)  RunMontage = WalkFrontMontage;
		else if (WorldDir == EGridDirection::Up)    RunMontage = WalkBackMontage;
		else if (WorldDir == EGridDirection::Right) RunMontage = WalkLeftMontage;  // 왼쪽(X+)
		else if (WorldDir == EGridDirection::Left)  RunMontage = WalkRightMontage; // 오른쪽(X-)
	}

	// ───────── 2. GAS 태그 결정 (월드 방향 그대로 사용) ─────────
	FString MoveTag = "Ability.Move.Right";

	switch (WorldDir)
	{
	case EGridDirection::Right: MoveTag = "Ability.Move.Right"; break;
	case EGridDirection::Left:  MoveTag = "Ability.Move.Left"; break;
	case EGridDirection::Up:    MoveTag = "Ability.Move.Up"; break;
	case EGridDirection::Down:  MoveTag = "Ability.Move.Down"; break;
	}

	// ───────── 3. 실행 ─────────
	FGameplayTagContainer MoveTags;
	MoveTags.AddTag(FGameplayTag::RequestGameplayTag(*MoveTag));

	// GAS 실행 시도 (여기서 내부적으로 StartVisualMove -> PlayAnimMontage(RunMontage)가 호출됨)
	if (!AbilitySystem->TryActivateAbilitiesByTag(MoveTags))
	{
		EndAction(); // 실패 시 턴 넘김
	}

	bJustAttacked = false;
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

	CurrentStopMontage = WalkStopMontage;

	// 1. 방향에 따라 RunMontage 교체 (CharacterBase의 변수를 덮어씌움)
	switch (RelativeDir)
	{
	case EGridDirection::Right: // 전진
		RunMontage = WalkFrontMontage;
		break;
	case EGridDirection::Left:  // 후퇴
		RunMontage = WalkBackMontage;
		break;
	case EGridDirection::Up:    // 왼쪽 게걸음
		RunMontage = WalkLeftMontage;
		break;
	case EGridDirection::Down:  // 오른쪽 게걸음
		RunMontage = WalkRightMontage;
		break;
	}

	FString MoveTag = "Ability.Move.Right";

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
	RequestRotation(PendingFaceDir, nullptr);
	bJustAttacked = false;
}

void AEnemyCharacter::Action_MoveToBestAttackPos()
{
	Action_MoveDirectly(PendingMoveDir);
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

	// [디버그 1] 필수 데이터 체크
	if (!Skill)
	{
		UE_LOG(LogTemp, Error, TEXT("[AI Fail] Skill is NULL! BP_EnemyCharacter에서 Skill_A를 할당했는지 확인하세요."));
		return false;
	}
	if (!PlayerRef)
	{
		UE_LOG(LogTemp, Error, TEXT("[AI Fail] PlayerRef is NULL!"));
		return false;
	}
	if (!BattleManagerRef || !BattleManagerRef->GridInterface)
	{
		UE_LOG(LogTemp, Error, TEXT("[AI Fail] GridInterface is NULL!"));
		return false;
	}

	FIntPoint MyPos = GridCoord;
	FIntPoint PlayerPos = PlayerRef->GridCoord;

	// 1. 맵 크기 가져오기 (경계 검사용)
	if (!BattleManagerRef->GridInterface) return false;
	int32 MapW = IGridDataInterface::Execute_GetGridWidth(BattleManagerRef->GridActorRef);
	int32 MapH = IGridDataInterface::Execute_GetGridHeight(BattleManagerRef->GridActorRef);

	// 2. "명당(Sweet Spots)" 리스트 확보
	// 명당이란? -> 내가 거기 서 있으면 플레이어를 때릴 수 있는 모든 좌표
	// (적은 회전이 자유로우므로, 4방향 회전을 모두 가정한 공격 위치를 다 찾습니다)
	TArray<FIntPoint> SweetSpots;

	for (const FIntPoint& Point : Skill->AttackPattern)
	{
		// Y축 반전(-Point.Y) 적용 (좌표계 통일)
		int32 PX = Point.X;
		int32 PY = -Point.Y;

		// 플레이어 위치에서 역산 -> "내가 서야 할 위치"
		SweetSpots.Add(PlayerPos - FIntPoint(PX, PY));   // 내가 Right 볼 때
		SweetSpots.Add(PlayerPos - FIntPoint(-PX, -PY)); // 내가 Left 볼 때
		SweetSpots.Add(PlayerPos - FIntPoint(-PY, PX));  // 내가 Down 볼 때
		SweetSpots.Add(PlayerPos - FIntPoint(PY, -PX));  // 내가 Up 볼 때
	}


	// [디버그 2] 명당 계산 결과
	if (SweetSpots.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[AI Fail] SweetSpots is Empty! 스킬 데이터(%s)의 AttackPattern에 좌표를 추가하세요."), *Skill->SkillName.ToString());
		return false;
	}

	// 명당이 하나도 없으면(불가능한 스킬 등) 이동 불가
	if (SweetSpots.Num() == 0) return false;


	// 3. 내 상하좌우 4칸을 검사해서 "가장 좋은 칸" 찾기
	int32 BestMinDist = 99999; // 가장 짧은 거리 기록용
	bool bFoundValidMove = false;

	// 상하좌우 탐색 순서 (Right, Left, Down, Up)
	const FIntPoint Dirs[] = { FIntPoint(1,0), FIntPoint(-1,0), FIntPoint(0,1), FIntPoint(0,-1) };
	const EGridDirection Enums[] = { EGridDirection::Right, EGridDirection::Left, EGridDirection::Down, EGridDirection::Up };

	for (int i = 0; i < 4; ++i)
	{
		FIntPoint NextPos = MyPos + Dirs[i];

		// ───────── [유효성 검사] 갈 수 없는 칸은 거름 ─────────

		// A. 맵 밖인가?
		if (NextPos.X < 0 || NextPos.X >= MapW || NextPos.Y < 0 || NextPos.Y >= MapH)
			continue;

		// B. 장애물(다른 캐릭터)이 있는가?
		if (BattleManagerRef->GetCharacterAt(NextPos) != nullptr)
			continue;

		// ──────────────────────────────────────────────────

		// C. 이 칸(NextPos)에서 "가장 가까운 명당"까지의 거리 측정
		// (즉, 이쪽으로 가면 공격 위치랑 얼마나 가까워지는가?)
		int32 LocalMinDist = 99999;

		for (const FIntPoint& Spot : SweetSpots)
		{
			// 맨해튼 거리 (가로+세로 칸수)
			int32 Dist = FMath::Abs(Spot.X - NextPos.X) + FMath::Abs(Spot.Y - NextPos.Y);

			if (Dist < LocalMinDist)
			{
				LocalMinDist = Dist;
			}
		}

		// D. 지금까지 찾은 칸들보다 이 칸이 더 명당에 가깝다면 선택
		if (LocalMinDist < BestMinDist)
		{
			BestMinDist = LocalMinDist;
			OutWorldDir = Enums[i]; // 이 방향이 정답이다
			bFoundValidMove = true;
		}
	}

	// [디버그 3] 최종 결과
	if (!bFoundValidMove)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AI Fail] 갈 수 있는 칸이 없거나 더 가까워질 수 없음 (Current: %d,%d)"), MyPos.X, MyPos.Y);
	}

	return bFoundValidMove;
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

	if (BattleManagerRef)
	{
		BattleManagerRef->OnEnemyKilled(this);
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
	GetWorld()->GetTimerManager().SetTimer(
        DeathTimer, 
        this, 
        &AEnemyCharacter::FinishDying, // 호출할 함수
        DestroyDelay, 
        false
    );
}

// 타이머가 끝나면 호출됨
void AEnemyCharacter::FinishDying()
{
	// 혹시라도 이미 파괴된 상태면 무시
	if (!IsValid(this)) return;

	SetActorHiddenInGame(true);
	Destroy();
}

// 맵이 바뀌거나 파괴될 때 호출됨
void AEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 이 액터에 걸린 모든 타이머 강제 종료 (FinishDying이 호출되지 않게 막음)
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AEnemyCharacter::SetActionOrder(int32 OrderIndex, UTexture2D* SubActionIcon, bool bIsDangerous)
{
	// 1. 배열 인덱스로 변환 (1st -> 0, 2nd -> 1 ...)
	int32 ArrayIdx = OrderIndex - 1;

	// 2. 메인 아이콘(숫자) 결정
	UTexture2D* MainIconToUse = nullptr;

	if (bIsDangerous)
	{
		// 위험(공격 장전/발사) 상태면 '빨간색' 배열 우선 사용
		if (OrderIcons_Red.IsValidIndex(ArrayIdx))
		{
			MainIconToUse = OrderIcons_Red[ArrayIdx];
		}
		// 혹시 빨간 아이콘이 비어있다면 흰색이라도 사용 (안전장치)
		else if (OrderIcons.IsValidIndex(ArrayIdx))
		{
			MainIconToUse = OrderIcons[ArrayIdx];
		}
	}
	else
	{
		// 평상시는 '흰색' 배열 사용
		if (OrderIcons.IsValidIndex(ArrayIdx))
		{
			MainIconToUse = OrderIcons[ArrayIdx];
		}
	}

	// 3. 아이콘이 결정되었으면 UI 업데이트 요청
	if (MainIconToUse)
	{
		BP_UpdateOrderIcon(MainIconToUse, SubActionIcon);
		if (OrderWidgetComponent)
		{
			OrderWidgetComponent->SetVisibility(true);
		}		
	}
}
void AEnemyCharacter::HideActionOrder()
{
	if (OrderWidgetComponent)
	{
		OrderWidgetComponent->SetVisibility(false);
	}
}

void AEnemyCharacter::EndAction()
{
	// 행동이 끝나면 내 순서표 끄기
	HideActionOrder();

	Super::EndAction(); // 부모(CharacterBase)의 턴 종료 로직 실행
}