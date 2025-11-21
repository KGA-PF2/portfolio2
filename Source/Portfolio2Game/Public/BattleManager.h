// BattleManager.h (최종 수정본)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridDataInterface.h"
#include "Camera/CameraActor.h"
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

public:

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

	/** (필수) 맵에 배치된 BP_GridISM 액터를 연결해야 합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	TObjectPtr<AActor> GridActorRef;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Actors")
	TObjectPtr<APlayerCharacter> PlayerRef; // 에디터에서 None이 정상

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Actors")
	TArray<TObjectPtr<AEnemyCharacter>> Enemies;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actors")
	TSubclassOf<APlayerCharacter> PlayerClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actors")
	TSubclassOf<AEnemyCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<ACameraActor> FieldCamera;

	// ──────────────────────────────
	// 전투 흐름
	// ──────────────────────────────
public:
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

protected:
	// 현재 행동 중인 적의 인덱스 (0, 1, 2...)
	int32 CurrentEnemyActionIndex = 0;

	// 다음 순번 적에게 행동 명령 내리기
	void ProcessNextEnemyAction();

	// ──────────────────────────────
	// 스폰 관련 (수정됨)
	// ──────────────────────────────
protected:
	/** (수정됨) 플레이어가 스폰될 타일의 인덱스 (세로 우선) */
	UPROPERTY(EditAnywhere, Category = "Spawn")
	int32 PlayerSpawnIndex = 10;

	/** (수정됨) 적들이 스폰될 타일 인덱스 목록 (세로 우선) */
	UPROPERTY(EditAnywhere, Category = "Spawn")
	TArray<int32> EnemySpawnIndices;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SpawnPlayer();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SpawnEnemiesForRound();

	// ──────────────────────────────
	// 유틸리티
	// ──────────────────────────────
public:
	/** (오류 수정) GA_Move가 접근할 수 있도록 public으로 이동 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Grid")
	TScriptInterface<IGridDataInterface> GridInterface;

	FVector GridToWorld(FIntPoint GridPos) const;

	/** (오류 수정) GA_Move가 접근할 수 있도록 public으로 이동 */
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FVector GetWorldLocation(FIntPoint GridPos) const;

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
protected:
	void CheckSingleEnemyTimer();
	void CheckBattleResult();

	FTimerHandle TurnDelayHandle;
};