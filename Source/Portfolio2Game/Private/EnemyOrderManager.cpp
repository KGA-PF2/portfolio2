#include "EnemyOrderManager.h"
#include "EnemyCharacter.h"
#include "SkillBase.h"

AEnemyOrderManager::AEnemyOrderManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEnemyOrderManager::UpdateActionQueue(const TArray<AEnemyCharacter*>& Enemies)
{
	TArray<FEnemyActionIconInfo> QueueData;

	for (AEnemyCharacter* Enemy : Enemies)
	{
		if (!Enemy || Enemy->bDead) continue;

		// 공격(발사)하는 턴에만 큐 UI에 띄움
		if (Enemy->PendingAction == EAIActionType::FireReserved)
		{
			USkillBase* Skill = Enemy->ReservedSkill;
			if (Skill)
			{
				FEnemyActionIconInfo Info;
				Info.Icon = Skill->SkillIcon;
				Info.ActionName = Skill->SkillName.ToString();
				Info.SkillRef = Skill;
				QueueData.Add(Info);
			}
		}
	}
	OnUpdateActionQueue.Broadcast(QueueData);
}


UTexture2D* AEnemyOrderManager::GetIconForAction(AEnemyCharacter* Enemy)
{
	if (!Enemy) return nullptr;

	EAIActionType Action = Enemy->PendingAction;

	if (Action == EAIActionType::ReserveSkill_A || Action == EAIActionType::ReserveSkill_B)
	{
		// 만약 아이콘을 할당 안 했으면 Wait 아이콘이라도 리턴 (디버깅용)
		return Icon_Charge ? Icon_Charge : Icon_Wait;
	}

	// 2. 공격(발사) 단계인가
	if (Action == EAIActionType::FireReserved)
	{
		return Icon_Attack ? Icon_Attack : Icon_Wait;
	}

	// 이동 관련 (방향 계산 필요)
	if (Action >= EAIActionType::Move_Front && Action <= EAIActionType::Move_Right)
	{
		EGridDirection RelDir = EGridDirection::Right; // Front
		if (Action == EAIActionType::Move_Back) RelDir = EGridDirection::Left;
		else if (Action == EAIActionType::Move_Left) RelDir = EGridDirection::Up;
		else if (Action == EAIActionType::Move_Right) RelDir = EGridDirection::Down;

		// 현재 적이 보는 방향을 고려해 실제 화면상 방향(월드 방향) 계산
		EGridDirection WorldDir = CalculateWorldDirection(Enemy->FacingDirection, RelDir);

		switch (WorldDir)
		{
		case EGridDirection::Up:    return Icon_Move_Up;
		case EGridDirection::Down:  return Icon_Move_Down;
		case EGridDirection::Left:  return Icon_Move_Left;
		case EGridDirection::Right: return Icon_Move_Right;
		}
	}

	else if (Action == EAIActionType::MoveToBestAttackPos)
	{
		// ★ 재계산하지 않고, 적이 저장해둔 '확정된 방향'을 가져옴
		EGridDirection FixedDir = Enemy->PendingMoveDir;

		switch (FixedDir)
		{
		case EGridDirection::Up:    return Icon_Move_Up;
		case EGridDirection::Down:  return Icon_Move_Down;
		case EGridDirection::Left:  return Icon_Move_Left;
		case EGridDirection::Right: return Icon_Move_Right;
		}
		return Icon_Wait;
	}

	// 5. ★ [신규] 회전 아이콘 세분화 (CW / CCW / 180)
	else if (Action == EAIActionType::RotateToPlayer)
	{
		EGridDirection Current = Enemy->FacingDirection;
		EGridDirection Target = Enemy->PendingFaceDir; // 아까 저장한 값

		// 같은 방향이면(이미 보고 있으면) 대기 아이콘
		if (Current == Target) return Icon_Wait;

		// 방향 정의 (Up=0, Down=1, Left=2, Right=3 이라고 가정 - Enum 정의 확인 필요)
		// 하지만 Enum 순서가 꼬여있을 수 있으므로 명시적 케이스로 처리하는 게 가장 안전합니다.

		// ── 시계 방향 (CW) 케이스 ──
		// Right(X+) -> Down(Y+) -> Left(X-) -> Up(Y-) -> Right
		bool bIsCW = (Current == EGridDirection::Right && Target == EGridDirection::Down) ||
			(Current == EGridDirection::Down && Target == EGridDirection::Left) ||
			(Current == EGridDirection::Left && Target == EGridDirection::Up) ||
			(Current == EGridDirection::Up && Target == EGridDirection::Right);

		// ── 반시계 방향 (CCW) 케이스 ──
		// Right -> Up -> Left -> Down
		bool bIsCCW = (Current == EGridDirection::Right && Target == EGridDirection::Up) ||
			(Current == EGridDirection::Up && Target == EGridDirection::Left) ||
			(Current == EGridDirection::Left && Target == EGridDirection::Down) ||
			(Current == EGridDirection::Down && Target == EGridDirection::Right);

		if (bIsCW) return Icon_Rotate_CW;
		if (bIsCCW) return Icon_Rotate_CCW;

		// 둘 다 아니면 180도 회전임
		return Icon_Rotate_180;
	}

	// 대기
	else if (Action == EAIActionType::Wait)
	{
		return Icon_Wait;
	}

	return nullptr;
}

EGridDirection AEnemyOrderManager::CalculateWorldDirection(EGridDirection Facing, EGridDirection RelativeMove)
{
	// EnemyCharacter.cpp의 Action_Move 로직을 그대로 가져옴
	// (코드가 길어지니 핵심 로직만 요약)
	if (Facing == EGridDirection::Right) return RelativeMove;

	if (Facing == EGridDirection::Left) {
		if (RelativeMove == EGridDirection::Right) return EGridDirection::Left;
		if (RelativeMove == EGridDirection::Left) return EGridDirection::Right;
		if (RelativeMove == EGridDirection::Up) return EGridDirection::Down;
		if (RelativeMove == EGridDirection::Down) return EGridDirection::Up;
	}
	if (Facing == EGridDirection::Up) {
		if (RelativeMove == EGridDirection::Right) return EGridDirection::Up;
		if (RelativeMove == EGridDirection::Left) return EGridDirection::Down;
		if (RelativeMove == EGridDirection::Up) return EGridDirection::Left;
		if (RelativeMove == EGridDirection::Down) return EGridDirection::Right;
	}if (Facing == EGridDirection::Down) {
		if (RelativeMove == EGridDirection::Right) return EGridDirection::Down;
		if (RelativeMove == EGridDirection::Left) return EGridDirection::Up;
		if (RelativeMove == EGridDirection::Up) return EGridDirection::Right;
		if (RelativeMove == EGridDirection::Down) return EGridDirection::Left;
	}

	return EGridDirection::Right; // Fallback
}