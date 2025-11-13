// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Portfolio2GameCharacter.generated.h"

UCLASS(Blueprintable)
class APortfolio2GameCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APortfolio2GameCharacter();

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

};

