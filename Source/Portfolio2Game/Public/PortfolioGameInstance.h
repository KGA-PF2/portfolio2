#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PlayerSkillData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "SkillBase.h"
#include "PortfolioGameInstance.generated.h"

UCLASS()
class PORTFOLIO2GAME_API UPortfolioGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	// ───────── 저장 데이터 (맵 이동 시 유지) ─────────

	// 플레이어 체력 (-1.0f면 새 게임으로 간주하여 풀피 시작)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	float SavedCurrentHP = -1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	float SavedMaxHP = 10.0f;

	// 플레이어 스킬 및 강화 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	TArray<FPlayerSkillData> SavedSkills;

	// 레벨 이동 연출 플래그 (모래바람 유지용)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveData")
	bool bIsLevelTransitioning = false;


	// ───────── 게임 통계 (신규) ─────────

	// 총 플레이 시간 (초 단위, 누적)
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float TotalPlayTime = 0.0f;

	// 게임 전체 누적 처치 수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 TotalKillCount = 0;

	// 시간 측정을 위한 티커 핸들
	FTSTicker::FDelegateHandle TickDelegateHandle;


	// ───────── 스테이지 흐름 ─────────

	// 전체 스테이지 순서 (에디터에서 설정: AA, Enforce, AB...)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameFlow")
	TArray<FName> StageList;

	// 현재 스테이지 인덱스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameFlow")
	int32 CurrentStageIndex = 0;

	// 난이도 (루프 돌 때마다 증가)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameFlow")
	int32 DifficultyLevel = 0;


	// ───────── 함수 ─────────

	// 데이터 저장 (BattleManager -> GameInstance)
	void SavePlayerData(float CurrentHP, float MaxHP, const TArray<FPlayerSkillData>& Skills);

	// 데이터 존재 여부 확인 (PlayerCharacter가 호출)
	bool HasSavedData() const { return SavedCurrentHP > 0.0f; }

	// 다음 스테이지 이름 반환 및 인덱스 증가
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	FName GetNextStageName();

	// ★ [신규] 게임 완전 초기화 (새 게임 시작 시 호출)
	UFUNCTION(BlueprintCallable, Category = "GameFlow")
	void ResetGameData();

	// ★ [신규] 현재 맵 이름 반환 (UI 표시용)
	UFUNCTION(BlueprintCallable, Category = "UI")
	FString GetCurrentStageDisplayName();

	// (내부용) 매 프레임 시간 측정 함수
	bool TickPlayTime(float DeltaTime);


	// 특정 경로에 있는 모든 스킬 데이터를 로드하고 정렬해서 반환
	UFUNCTION(BlueprintCallable, Category = "Game Data")
	TArray<USkillBase*> LoadAllSkillsFromPath(FName Path = "/Game/TeamShare/TeamShare_JSH/Data/SkillData/DA");
};