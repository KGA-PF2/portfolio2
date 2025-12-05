#include "PortfolioGameInstance.h"

void UPortfolioGameInstance::Init()
{
	Super::Init();

	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UPortfolioGameInstance::TickPlayTime)
	);
}
bool UPortfolioGameInstance::TickPlayTime(float DeltaTime)
{
	TotalPlayTime += DeltaTime;
	return true; // true를 반환해야 타이머가 계속 반복됨
}

FString UPortfolioGameInstance::GetCurrentStageDisplayName()
{
	// 현재 인덱스를 기반으로 이름 반환
	if (StageList.IsValidIndex(CurrentStageIndex))
	{
		return StageList[CurrentStageIndex].ToString();
	}
	return TEXT("Unknown Area");
}

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

void UPortfolioGameInstance::ResetGameData()
{
	// 1. 저장 데이터 초기화
	SavedCurrentHP = -1.0f; // 초기화 신호 (PlayerCharacter가 이를 보고 InitAttributes 실행)
	SavedMaxHP = 10.0f;
	SavedSkills.Empty();
	bIsLevelTransitioning = false;

	// 2. 스테이지 흐름 초기화
	CurrentStageIndex = 0;
	DifficultyLevel = 0;

	// 3. 통계 초기화
	TotalPlayTime = 0.0f;
	TotalKillCount = 0;

	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Game Data Reset! Ready for New Game."));
}