// BattleManager.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridDataInterface.h" // (필수) 인터페이스 포함
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
protected: // (보호됨)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle")
	EBattleState CurrentState = EBattleState::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	int32 CurrentRound = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	int32 TurnCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	int32 MaxRoundCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	int32 M_TurnsToNextRound = 3;

	int32 TurnsSinceSingleEnemy = 0;

	// ──────────────────────────────
	// 액터 참조 (수정됨)
	// ──────────────────────────────
protected: // (보호됨)
	/** (필수) 맵에 배치된 BP_GridISM 액터를 연결해야 합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	TObjectPtr<AActor> GridActorRef;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Actors")
	TObjectPtr<APlayerCharacter> PlayerRef;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Actors")
	TArray<TObjectPtr<AEnemyCharacter>> Enemies;

	UPROPERTY(EditDefaultsOnly, Category = "Actors")
	TSubclassOf<APlayerCharacter> PlayerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Actors")
	TSubclassOf<AEnemyCharacter> EnemyClass;

	// ──────────────────────────────
	// 전투 흐름
	// ──────────────────────────────
public: // (공개됨)
	UFUNCTION(BlueprintCallable)
	void BeginBattle();

	UFUNCTION(BlueprintCallable)
	void EndBattle(bool bPlayerVictory);

	UFUNCTION(BlueprintCallable)
	void StartRound();

	UFUNCTION(BlueprintCallable)
	void EndRound();

	UFUNCTION(BlueprintCallable)
	void StartPlayerTurn();

	UFUNCTION(BlueprintCallable)
	void StartEnemyTurn();

	UFUNCTION(BlueprintCallable)
	void EndCharacterTurn(ACharacterBase* Character);

protected: // (보호됨)
	void ExecuteEnemyActions();

	// ──────────────────────────────
	// 스폰 관련 (수정됨)
	// ──────────────────────────────
protected: // (보호됨)
	/** (수정됨) 플레이어가 스폰될 타일의 인덱스 (세로 우선) */
	UPROPERTY(EditAnywhere, Category = "Spawn")
	int32 PlayerSpawnIndex = 10; // 7x5(WxH) 기준 (X=2, Y=0) -> 2*5+0 = 10

	/** (수정됨) 적들이 스폰될 타일 인덱스 목록 (세로 우선) */
	UPROPERTY(EditAnywhere, Category = "Spawn")
	TArray<int32> EnemySpawnIndices; // 예: { 30, 31, 32 } (X=6)

	UFUNCTION(BlueprintCallable)
	void SpawnPlayer();

	UFUNCTION(BlueprintCallable)
	void SpawnEnemiesForRound();

	// ──────────────────────────────
	// 유틸리티 (오류 수정을 위해 public: 으로 이동)
	// ──────────────────────────────
public:
	/** (오류 수정) GA_Move가 접근할 수 있도록 public으로 이동 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Grid")
	TScriptInterface<IGridDataInterface> GridInterface;

	/** (기존) 셀 중심 월드 좌표를 반환 (GridActorRef의 정보를 사용) */
	FVector GridToWorld(FIntPoint GridPos) const;

	/** (오류 수정) GA_Move가 접근할 수 있도록 public으로 이동 */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FVector GetWorldLocation(FIntPoint GridPos) const;

	/** (기존) 캐릭터의 현재 그리드 좌표를 기반으로 중앙 월드 위치를 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FVector GetWorldLocationForCharacter(ACharacterBase* Character) const;

	/** (오류 수정) GA_Move가 접근할 수 있도록 public으로 이동 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	int32 GetGridIndexFromCoord(FIntPoint Coord) const;

	/** (오류 수정) GA_Move가 접근할 수 있도록 public으로 이동 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	FIntPoint GetGridCoordFromIndex(int32 Index) const;

	/** (오류 수정) GA_Move가 접근할 수 있도록 public으로 이동 */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	ACharacterBase* GetCharacterAt(FIntPoint Coord) const;

	// ──────────────────────────────
	// 전투 종료 조건
	// ──────────────────────────────
protected: // (보호됨)
	void CheckSingleEnemyTimer();
	void CheckBattleResult();
};