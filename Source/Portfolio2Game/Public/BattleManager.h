#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridDataInterface.h" // (중요!) 방금 만든 인터페이스 헤더 포함
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
	int32 M_TurnsToNextRound = 3; // 적 1명 남은 뒤 m턴 경과 시 다음 라운드

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	int32 MaxRoundCount = 3; // 마지막 라운드 기준

	// ──────────────────────────────
	// 전장 정보
	// ──────────────────────────────

	// ❌ 임시 그리드 변수 제거
	// UPROPERTY(EditAnywhere, Category = "Grid")
	// int32 GridWidth = 14;
	// ... (GridHeight, TileSize 제거)

	UPROPERTY(EditAnywhere, Category = "Grid")
	TArray<int32> EnemySpawnColumns;

	

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

	/**
	 * (수정됨) 맵에 배치된 실제 그리드 액터 ('BP_GroundGrid')입니다.
	 * C++ 컴파일 후, 에디터의 디테일 패널에서 이 변수에 BP_GroundGrid 인스턴스를 연결해야 합니다.
	 * C++에서는 AActor로 받은 뒤, IGridDataInterface로 통신합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	TObjectPtr<AActor> GridActorRef;

	// ──────────────────────────────
	// 전투 루프 (기존과 동일)
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
	// 스폰 관련 (기존과 동일)
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

	/** (신규) 플레이어/적 오프셋이 적용된 월드 위치를 반환 (요구사항 1) */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FVector GetWorldLocation(FIntPoint GridPos, bool bIsPlayer) const;

	/** (신규) 캐릭터의 현재 그리드 좌표를 기반으로 오프셋이 적용된 월드 위치를 반환 */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FVector GetWorldLocationForCharacter(ACharacterBase* Character) const;

	/** (신규) 2D 그리드 좌표(Coord)를 1D 인덱스(칸 번호)로 변환 (GridActorRef에게 요청) (요구사항 3) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	int32 GetGridIndexFromCoord(FIntPoint Coord) const;

	/** (신규) 1D 인덱스(칸 번호)를 2D 그리드 좌표(Coord)로 변환 (GridActorRef에게 요청) (요구사항 3) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	FIntPoint GetGridCoordFromIndex(int32 Index) const;

protected:
	void CheckSingleEnemyTimer();
	void CheckBattleResult();
};