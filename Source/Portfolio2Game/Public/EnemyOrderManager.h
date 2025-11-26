#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyAIStructs.h"
#include "CharacterBase.h"
#include "EnemyOrderManager.generated.h"

class AEnemyCharacter;
class USkillBase;

// UI로 보낼 데이터 구조체 (이쪽으로 이동)
USTRUCT(BlueprintType)
struct FEnemyActionIconInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	UTexture2D* Icon = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FString ActionName = "";

	UPROPERTY(BlueprintReadOnly)
	USkillBase* SkillRef = nullptr;
};

// UI 업데이트 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpdateActionQueue, const TArray<FEnemyActionIconInfo>&, ActionList);

UCLASS()
class PORTFOLIO2GAME_API AEnemyOrderManager : public AActor
{
	GENERATED_BODY()

public:
	AEnemyOrderManager();

public:
	// ───────── 설정 (에디터에서 아이콘 지정) ─────────
	UPROPERTY(EditAnywhere, Category = "UI|Icons|Move")
	UTexture2D* Icon_Move_Up;
	UPROPERTY(EditAnywhere, Category = "UI|Icons|Move")
	UTexture2D* Icon_Move_Down;
	UPROPERTY(EditAnywhere, Category = "UI|Icons|Move")
	UTexture2D* Icon_Move_Left;
	UPROPERTY(EditAnywhere, Category = "UI|Icons|Move")
	UTexture2D* Icon_Move_Right;

	UPROPERTY(EditAnywhere, Category = "UI|Icons|Rotate")
	UTexture2D* Icon_Rotate_CW;
	UPROPERTY(EditAnywhere, Category = "UI|Icons|Rotate")
	UTexture2D* Icon_Rotate_CCW;
	UPROPERTY(EditAnywhere, Category = "UI|Icons|Rotate")
	UTexture2D* Icon_Rotate_180;

	UPROPERTY(EditAnywhere, Category = "UI|Icons|State")
	UTexture2D* Icon_Charge;
	UPROPERTY(EditAnywhere, Category = "UI|Icons|State")
	UTexture2D* Icon_Wait;
	UPROPERTY(EditAnywhere, Category = "UI|Icons|State")
	UTexture2D* Icon_Attack;

	// ───────── 기능 ─────────

	/** BattleManager가 적 리스트를 던져주면, 분석해서 UI로 방송함 */
	UFUNCTION(BlueprintCallable, Category = "Order")
	void UpdateActionQueue(const TArray<AEnemyCharacter*>& Enemies);

	/** 특정 적의 행동에 맞는 '이동/회전/대기 아이콘'을 계산해서 반환 (BattleManager가 호출함) */
	UFUNCTION(BlueprintCallable, Category = "Order")
	UTexture2D* GetIconForAction(AEnemyCharacter* Enemy);

	/** UI가 구독할 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Order")
	FOnUpdateActionQueue OnUpdateActionQueue;

private:
	// 내부 계산용: 상대 방향 -> 월드 방향 변환
	EGridDirection CalculateWorldDirection(EGridDirection Facing, EGridDirection RelativeMove);
};