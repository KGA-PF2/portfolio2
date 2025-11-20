// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "NewFunctionLibrary.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EGridMoveDirection : uint8
{
    Up      UMETA(DisplayName = "Up"),
    Down    UMETA(DisplayName = "Down"),
    Left    UMETA(DisplayName = "Left"),
    Right   UMETA(DisplayName = "Right")
};

UCLASS()
class PORTFOLIO2GAME_API UNewFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    /*Calculates the target index for movement in the Input Direction.*/
    UFUNCTION(BlueprintCallable, Category = "Grid Movement", meta = (ExpandEnumAsExecs = "Direction"/*, OutputExecs = "bSuccessOut"*/))
    static void CalcTargetGridNum(
        int32 CurrentGridNum,
        EGridMoveDirection Direction, // ¿Ã Enum¿Ã Ω««‡ «…¿∏∑Œ ∫Ø»Øµ 
        int32 Width,
        int32 Height,
        int32& CalcGridNum,
        bool& bSuccessOut
    );	
};
