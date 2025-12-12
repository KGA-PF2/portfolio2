#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyCharacter.h"
#include "StageData.generated.h"

// 하나의 라운드 정보 (이번 라운드에 나올 적들)
USTRUCT(BlueprintType)
struct FRoundDef
{
	GENERATED_BODY()

	// 이번 라운드에 소환할 적 종류 리스트
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<AEnemyCharacter>> EnemiesToSpawn;
};

// 스테이지 정보 (라운드들의 모음)
UCLASS(BlueprintType)
class PORTFOLIO2GAME_API UStageData : public UDataAsset
{
	GENERATED_BODY()

public:
	// 이 스테이지의 라운드 구성 (인덱스 0 = 1라운드, 1 = 2라운드...)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	TArray<FRoundDef> Rounds;
};