#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyAIStructs.generated.h"

// 1. 행동 종류 (적이 할 수 있는 일)
UENUM(BlueprintType)
enum class EAIActionType : uint8
{
	Wait,               // 대기
	FireReserved,       // 장전된 스킬 발사
	ReserveSkill_A,     // 스킬 A 예약
	ReserveSkill_B,     // 스킬 B 예약
	ReserveSkill_Random, // 랜덤 스킬 예약

	// [이동] 상대적 방향 기준 (내 시점)
	Move_Front,         // 전진
	Move_Back,          // 후퇴
	Move_Left,          // 왼쪽으로 이동 (게걸음)
	Move_Right,         // 오른쪽으로 이동

	RotateToPlayer,      // 플레이어 쪽으로 회전

	MoveToBestAttackPos
};

// 2. 조건 종류 (적이 체크할 상황 변수)
UENUM(BlueprintType)
enum class EAIConditionType : uint8
{
	None,                   // 무조건 Pass
	HasReservedSkill,       // 스킬 장전 중인가?
	NoReservedSkill,        // 예약된 스킬이 없는가?
	JustAttacked,           // 방금 턴에 공격했는가?


	PlayerInSkillRange_Reserved, // 예약된 스킬 사거리 내인가?
	PlayerInSkillRange_A,        // 스킬 A 사거리 내인가?
	PlayerInSkillRange_B,        // 스킬 B 사거리 내인가?
	PlayerOutOfSkillRange_Reserved, // 예약된 스킬 사거리 밖인가?
	PlayerOutOfSkillRange_A,     // 스킬 A 사거리 밖인가?
	PlayerOutOfSkillRange_B,     // 스킬 B 사거리 밖인가?

	PlayerInLine_Far,       // 같은 줄인데 '내 사거리보다' 먼가?
	PlayerDifferentLine,    // 아예 다른 줄인가?
	PlayerNotInFront,        // 내 시야각(전방)에 없는가?

	CanMoveToAttackPos		// 이동하면 공격각이 나오는가?
};

// 3. 행동 규칙 (하나의 판단 단위)
USTRUCT(BlueprintType)
struct FAIActionRule
{
	GENERATED_BODY()

	// 이 행동을 하려면 만족해야 하는 조건들 (AND 조건)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EAIConditionType> RequiredConditions;

	// 조건 만족 시 실행할 행동
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAIActionType ActionToExecute;
};

// 4. 뇌 데이터 에셋 (이걸 여러 개 만들어서 적마다 갈아끼움)
UCLASS(BlueprintType)
class PORTFOLIO2GAME_API UEnemyBrainData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	TArray<FAIActionRule> ActionRules;
};