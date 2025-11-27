#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnforceCardData.generated.h"


// ★ [핵심] 데이터 에셋 클래스
UCLASS(BlueprintType)
class PORTFOLIO2GAME_API UEnforceCardData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FString CardName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card", meta = (MultiLine = true))
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	UTexture2D* Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	int32 AtkEnforceValue; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	int32 CoolEnforceValue; 
};