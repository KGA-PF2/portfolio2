// BattleManager.h (최종 수정본)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridDataInterface.h"
#include "EnemyOrderManager.h"
#include "Camera/CameraActor.h"
#include "StageData.h"
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

	// 내부적으로 스폰과 턴 시작 처리할 함수
	void StartActualBattle();

	// ───────── [FOV 자연스러운 연결용] ─────────
	FTimerHandle FOVBlendTimerHandle;
	float CurrentBlendTimeCount = 0.0f;
	float StartFOV = 90.0f;
	float TargetFOV = 90.0f;

	// 0.01초마다 호출되어 렌즈를 조절할 함수
	void UpdateCameraFOV();

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Managers")
	TObjectPtr<AEnemyOrderManager> OrderManagerRef;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle")
	EBattleState CurrentState = EBattleState::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	int32 CurrentRound = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	int32 TurnCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	int32 MaxRoundCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle")
	int32 M_TurnsToNextRound = 3;

	int32 TurnsSinceSingleEnemy = 0;

	UUserWidget* CurrentTransitionWidget; // 임시 저장용

	// 이 맵에서 잡아야 할 총 적의 수 (기본 4)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Rules")
	int32 MaxKillCount = 4;

	// 클리어 후 이동할 다음 레벨 이름 (예: Stage_Enforce)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Rules")
	FName NextLevelName;

	// 이동 직전에 보여줄 연출 위젯 (검은 화면 페이드 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|UI")
	TSubclassOf<UUserWidget> TransitionWidgetClass;

	// 현재까지 잡은 적 수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle|State")
	int32 CurrentKillCount = 0;

	// 현재까지 스폰된 적 총합 (4마리 넘으면 스폰 중단용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle|State")
	int32 TotalSpawnedCount = 0;

	// 적이 죽었을 때 호출 (Kill Count 증가)
	void OnEnemyKilled(AEnemyCharacter* DeadEnemy);

	// 강제 스테이지 클리어 (F12 디버그용 & 승리 시 호출)
	UFUNCTION(BlueprintCallable)
	void ForceStageClear();

	// [연출] 레벨에 배치된 시네마틱 카메라 액터 (에디터에서 할당)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic")
	TObjectPtr<AActor> IntroCameraActor;

	// [연출] 카메라 전환 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic")
	float CameraBlendTime = 2.0f;

	// [연출] '전투 시작' 텍스트 위젯 클래스 (WBP_BattleStart)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic")
	TSubclassOf<UUserWidget> BattleStartWidgetClass;

	// [연출] 위젯 애니메이션이 끝나면 호출될 함수 (이때 스폰함)
	UFUNCTION(BlueprintCallable)
	void OnBattleStartAnimFinished();


	// ──────────────────────────────
	// 액터 참조
	// ──────────────────────────────

	/** 맵에 배치된 BP_GridISM 액터를 연결해야 합니다. */
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

public:

	// ──────────────────────────────
	// 전투 흐름
	// ──────────────────────────────

	UFUNCTION(BlueprintCallable)
	void BeginBattle();

	UFUNCTION(BlueprintCallable)
	void EndBattle(bool bPlayerVictory);

	UFUNCTION(BlueprintCallable)
	void StartPlayerTurn();

	UFUNCTION(BlueprintCallable)
	void StartEnemyTurn();

	UFUNCTION(BlueprintCallable)
	void EndCharacterTurn(ACharacterBase* Character);

	UFUNCTION(BlueprintCallable)
	void OnPlayerDeathFinished();

	// ───────── 스테이지 설정 (에디터 할당) ─────────
	// 이 맵에서 나올 수 있는 스테이지 후보들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Setup")
	TArray<UStageData*> PossibleStages;

	// ───────── 런타임 상태 ─────────
	// 현재 결정된 스테이지 (PossibleStages 중 하나가 랜덤 선택됨)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Status")
	UStageData* CurrentStageData;

	// 현재 라운드 인덱스 (0부터 시작)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage Status")
	int32 CurrentRoundIndex = 0;

protected:
	// 현재 행동 중인 적의 인덱스 (0, 1, 2...)
	int32 CurrentEnemyActionIndex = 0;

	// 다음 순번 적에게 행동 명령 내리기
	void ProcessNextEnemyAction();

	void MoveToNextLevel();

	void ExecuteUncover();
	// ──────────────────────────────
	// 스폰 관련
	// ──────────────────────────────
protected:
	/** 플레이어가 스폰될 타일의 인덱스 (세로 우선) */
	UPROPERTY(EditAnywhere, Category = "Spawn")
	int32 PlayerSpawnIndex = 10;

	/** 적들이 스폰될 타일 인덱스 목록 (세로 우선) */
	UPROPERTY(EditAnywhere, Category = "Spawn")
	TArray<int32> EnemySpawnIndices;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SpawnPlayer();


	// 현재 라운드 데이터에 맞춰 적 소환
	void SpawnCurrentRoundEnemies();

	// 다음 라운드 시작 (적이 다 죽거나 2턴 지났을 때 호출)
	void StartNextRound();

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> TopBarWidgetClass;

protected:
	void CheckSingleEnemyTimer();
	void CheckBattleResult();

	FTimerHandle TurnDelayHandle;
};