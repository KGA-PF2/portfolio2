#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridDataInterface.h" // 1/2단계에서 만든 인터페이스 포함
#include "BattleManager.generated.h"

// 전방 선언
class APlayerCharacter;
class AEnemyCharacter;
class ACharacterBase;

UENUM(BlueprintType)
enum class EBattleState : uint8
{
	None,
	PlayerTurn,
	EnemyTurn,
	Victory,
	Defeat
};

/**
 * (오류 수정) UCLASS() 바로 아래에 PORTFOLIO2GAME_API 매크로를 추가해야 합니다.
 */
UCLASS()
class PORTFOLIO2GAME_API ABattleManager : public AActor
{
	GENERATED_BODY()

public:
	ABattleManager();

protected:
	virtual void BeginPlay() override;

	// ──────────────────────────────
	// 전투 상태
	// ──────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle")
	EBattleState CurrentState = EBattleState::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	int32 CurrentRound = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	int32 TurnCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	int32 TurnsSinceSingleEnemy = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	int32 M_TurnsToNextRound = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	int32 MaxRoundCount = 3;

	// ──────────────────────────────
	// 전장 정보
	// ──────────────────────────────
	UPROPERTY(EditAnywhere, Category = "Grid")
	TArray<int32> EnemySpawnColumns;

	// ❌ 임시 그리드 변수(GridWidth 등) 제거

public: // ⬅️ (오류 수정) GA_Move가 접근할 수 있도록 Public으로 변경
	/**
	 * (수정됨) 맵에 배치된 실제 그리드 액터 ('BP_GridISM')입니다.
	 * 에디터에서 이 변수에 BP_GridISM 인스턴스를 연결해야 합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	TObjectPtr<AActor> GridActorRef;

	// ──────────────────────────────
	// 캐릭터 참조
	// ──────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	TSubclassOf<APlayerCharacter> PlayerClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	TSubclassOf<AEnemyCharacter> TestEnemyClass;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	APlayerCharacter* PlayerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	TArray<AEnemyCharacter*> Enemies;

public:
	// ──────────────────────────────
	// 전투 루프
	// ──────────────────────────────
	UFUNCTION(BlueprintCallable)
	void BeginBattle();

	UFUNCTION(BlueprintCallable)
	void StartRound();

	UFUNCTION(BlueprintCallable)
	void EndRound();

	UFUNCTION(BlueprintCallable)
	void StartPlayerTurn();

	UFUNCTION(BlueprintCallable)
	void EndPlayerTurn();

	UFUNCTION(BlueprintCallable)
	void StartEnemyTurn();

	UFUNCTION(BlueprintCallable)
	void EndEnemyTurn();

	// ──────────────────────────────
	// 스폰 관련
	// ──────────────────────────────
	UFUNCTION(BlueprintCallable)
	void SpawnPlayer();

	UFUNCTION(BlueprintCallable)
	void SpawnEnemiesForRound();

	// ──────────────────────────────
	// 유틸리티 (수정 및 추가)
	// ──────────────────────────────

	/** (수정됨) 셀 중심 월드 좌표를 반환 (GridActorRef의 정보를 사용) */
	FVector GridToWorld(FIntPoint GridPos) const;

	/** (수정됨) 겹치기/오프셋이 없는 중앙 월드 위치를 반환합니다. (중앙 정렬) */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FVector GetWorldLocation(FIntPoint GridPos) const;

	/** (수정됨) 캐릭터의 현재 그리드 좌표를 기반으로 중앙 월드 위치를 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FVector GetWorldLocationForCharacter(ACharacterBase* Character) const;

	/** (신규) 2D 그리드 좌표(Coord)를 1D 인덱스(칸 번호)로 변환 (GridActorRef에게 요청) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	int32 GetGridIndexFromCoord(FIntPoint Coord) const;

	/** (신규) 1D 인덱스(칸 번호)를 2D 그리드 좌표(Coord)로 변환 (GridActorRef에게 요청) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	FIntPoint GetGridCoordFromIndex(int32 Index) const;

	/** (신규) 해당 좌표가 그리드 범위 내인지 검사합니다. (경계 검사) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	bool IsValidGridCoord(FIntPoint Coord) const;

	/** (신규) 해당 좌표에 캐릭터가 있는지 검사합니다. (겹치기 검사) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	ACharacterBase* GetCharacterAt(FIntPoint Coord) const;

protected:
	void CheckSingleEnemyTimer();
	void CheckBattleResult();
};