// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Portfolio2GameGameMode.generated.h"

UCLASS(minimalapi)
class APortfolio2GameGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APortfolio2GameGameMode();

	/**
	 * (신규) 맵의 모든 액터가 BeginPlay를 마친 후 호출되는 함수입니다.
	 * BattleManager의 시작 시점으로 사용합니다.
	 */
	virtual void StartPlay() override;
};



