#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridDataInterface.h"
#include "GridISM.generated.h"

UCLASS()
class PORTFOLIO2GAME_API AGridISM : public AActor, public IGridDataInterface
{
    GENERATED_BODY()

public:
    AGridISM();

    // BP에서 값 세팅
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid") int32  GridWidth;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid") int32  GridHeight;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid") double GridSizeX;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid") double GridSizeY;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid") FVector GridLocationOffset;

    // 인터페이스 구현 (_Implementation) — 시그니처 ‘그대로’, override OK
    virtual int32   GetGridWidth_Implementation() const override { return GridWidth; }
    virtual int32   GetGridHeight_Implementation() const override { return GridHeight; }
    virtual double  GetGridSizeX_Implementation() const override { return GridSizeX; }
    virtual double  GetGridSizeY_Implementation() const override { return GridSizeY; }
    virtual FVector GetGridLocationOffset_Implementation() const override { return GridLocationOffset; }

    // 🔴 여기서는 const 제거 (헤더와 동일해야 함)
    virtual FIntPoint GetGridCoordFromIndex_Implementation(int32 Index) override;
    virtual int32     GetGridIndexFromCoord_Implementation(FIntPoint Coord) override;
};
