#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PlayerSkillData.h" 
#include "PortfolioGameInstance.generated.h"

UCLASS()
class PORTFOLIO2GAME_API UPortfolioGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// ───────── 저장될 데이터 (맵 넘어가도 유지됨) ─────────

	// 1. 플레이어 체력 (다음 맵에서 이 체력으로 시작)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	float SavedCurrentHP = -1.0f; // -1이면 풀피 시작(첫판)

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	float SavedMaxHP = 100.0f;

	// 2. 플레이어 스킬 및 강화 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	TArray<FPlayerSkillData> SavedSkills;

	// 레벨 이동 연출 플래그
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	bool bIsLevelTransitioning = false;

	// ───────── 함수 ─────────

	// 데이터를 저장하는 함수 (BattleManager가 호출)
	void SavePlayerData(float CurrentHP, float MaxHP, const TArray<FPlayerSkillData>& Skills);

	// 데이터를 불러오는 헬퍼 함수 (PlayerCharacter가 호출)
	bool HasSavedData() const { return SavedCurrentHP > 0.0f; }

	// ───────── 스테이지 네비게이션 ─────────

	// 전체 스테이지 순서 (에디터에서 설정: AA, Enforce, AB, Enforce, ABoss)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFlow")
	TArray<FName> StageList;

	// 현재 스테이지 인덱스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameFlow")
	int32 CurrentStageIndex = 0;

	// ★ 난이도 (루프 돌 때마다 1씩 증가)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameFlow")
	int32 DifficultyLevel = 0;

	// 다음 스테이지 이름 반환 및 인덱스 증가
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	FName GetNextStageName();

	// ───────── 플레이 정보 ─────────

	UPROPERTY(BlueprintReadOnly, Category = "GameFlow")
	float TotalPlayTime = 0.0f;

	// [신규] 초기화 (타이머 시작)
	virtual void Init() override;

	// [신규] 매 초마다 시간 증가
	void UpdatePlayTime();

	// [신규] 현재 맵 이름 반환 (UI용)
	UFUNCTION(BlueprintCallable, Category = "UI")
	FString GetCurrentStageDisplayName();
};