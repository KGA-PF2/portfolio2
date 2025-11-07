#include "GridISM.h"

AGridISM::AGridISM()
{
    PrimaryActorTick.bCanEverTick = false;
}

FIntPoint AGridISM::GetGridCoordFromIndex_Implementation(int32 Index)
{
    if (GridHeight <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("GridISM: GridHeight <= 0"));
        return FIntPoint(-1, -1);
    }
    const int32 X = Index / GridHeight;  // Column-Major
    const int32 Y = Index % GridHeight;
    return FIntPoint(X, Y);
}

int32 AGridISM::GetGridIndexFromCoord_Implementation(FIntPoint Coord)
{
    if (GridHeight <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("GridISM: GridHeight <= 0"));
        return -1;
    }
    return (Coord.X * GridHeight) + Coord.Y; // Column-Major
}
