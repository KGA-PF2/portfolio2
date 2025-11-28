#include "PortfolioGameInstance.h"

void UPortfolioGameInstance::SavePlayerData(float CurrentHP, float MaxHP, const TArray<FPlayerSkillData>& Skills)
{
	SavedCurrentHP = CurrentHP;
	SavedMaxHP = MaxHP;
	SavedSkills = Skills;

	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Data Saved. HP: %.1f / %.1f, Skills: %d"),
		SavedCurrentHP, SavedMaxHP, SavedSkills.Num());
}

// [신규] 다음 스테이지 계산
FName UPortfolioGameInstance::GetNextStageName()
{
	if (StageList.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[GameInstance] StageList is Empty! Please set it in BP_PortfolioGameInstance."));
		return NAME_None;
	}

	// 1. 인덱스 증가
	CurrentStageIndex++;

	// 2. 루프 체크 (마지막 스테이지 다음은?)
	if (CurrentStageIndex >= StageList.Num())
	{
		// 다시 처음(AA)으로
		CurrentStageIndex = 0;

		// ★ 난이도 상승
		DifficultyLevel++;
		UE_LOG(LogTemp, Warning, TEXT("=== Loop Completed! Difficulty Increased to %d ==="), DifficultyLevel);
	}

	// 3. 해당 이름 반환
	FName NextMapName = StageList[CurrentStageIndex];
	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Moving to Stage Index: %d (%s)"), CurrentStageIndex, *NextMapName.ToString());

	return NextMapName;
}